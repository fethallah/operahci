/**
 *
 */
package org.gnf.acapella;

import java.util.Arrays;
import java.util.List;
import java.util.Set;
import java.util.TreeSet;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * @author gbonamy
 * 
 */
public class WellMask {

	private String maskStr = "";
	private String[] mask = { "0", "" + Integer.MAX_VALUE };
	private String type = "All";

	/**
	 * @param maskParam
	 * @throws Exception
	 */
	public WellMask(String maskStr) throws Exception {
		super();
		setmaskStr(maskStr);
		setType(getMaskStr());
		setMask(getMaskStr(), getType());
	}

	/**
	 * @return the maskStr
	 */
	public String getMaskStr() {
		return maskStr;
	}

	/**
	 * @param maskStr
	 *            the maskStr to set
	 */
	public void setmaskStr(String maskStr) {
		this.maskStr = maskStr.replaceAll("\\s*", "");
	}

	/**
	 * Initialize the mask.
	 * 
	 * @param maskParam
	 * @param type
	 */
	private void setMask(String maskParam, String type) {

		if (type == null || type.equals("All"))
			return;

		if (type.equals("Bellow") || type.equals("Above")
				|| type.equals("Range")) {

			mask = maskParam.split("-");
			if (mask[0].equals("*"))
				mask[0] = "0";
			if (mask[1].equals("*"))
				mask[1] = "" + Integer.MAX_VALUE;
		} else if (type.equals("RangeList")) {
			mask = maskParam.split("[,]");
			if (mask[0].startsWith("*"))
				mask[0].replaceFirst("\\*", "0");
			if (mask[mask.length - 1].endsWith("*"))
				mask[mask.length - 1].replaceAll("\\*", "" + Integer.MAX_VALUE);
		} else
			mask = maskParam.split(",");

	}

	/**
	 * @return the mask
	 */
	public String[] getMask() {
		return mask;
	}

	/**
	 * @param maskStr
	 */
	public void setType(String maskStr) {
		type = findType(maskStr);

	}

	/**
	 * This method returns the type of WellMask used: <li>"All" indicates that
	 * all well returned should be analyze, <li>"Range" indicates that the
	 * wellMask is a range of values, <li>"Above" indicates that the well should
	 * be above a value, <li>"Bellow" indicates that the well should be bellow a
	 * value, <li>"List" in the case of comma separated values, null is returned
	 * if the well mask is incorrect
	 * 
	 * @param wellMask
	 * @return null, "Above", "Bellow", "Range", "RangeList", "List", "All",
	 */
	private static String findType(String wellMask) {

		if (wellMask.equals("*"))
			return "All";

		Pattern p;
		Matcher m;

		p = Pattern.compile("\\d{4,9}-\\d{4,9}");
		m = p.matcher(wellMask);
		if (m.find())
			return "Range";

		p = Pattern.compile("\\d{4,9}-\\*");
		m = p.matcher(wellMask);
		if (m.find())
			return "Above";

		p = Pattern.compile("\\*-\\d{4,9}");
		m = p.matcher(wellMask);
		if (m.find())
			return "Bellow";

		p = Pattern.compile("(?m)^\\d{4,}$");
		m = p.matcher(wellMask.replaceAll(",", ""));
		if (m.find())
			return "List";

		p = Pattern.compile("(?m)^\\*?\\d{4,}\\*?$");
		m = p.matcher(wellMask.replaceAll("[,-]", ""));
		if (m.find())
			return "RangeList";

		return null;

	}

	/**
	 * @return the type
	 */
	public String getType() {
		return type;
	}

	/**
	 * This method will filter a set of well provided the current filter.
	 * 
	 * @param wellSet
	 * @return
	 */
	public Vector<String> filterWellSet(Vector<String> wellSet) {

		Vector<String> removedWells = new Vector<String>();

		if (wellSet == null || type.equals("All"))
			return removedWells;

		Set<String> currentWellSet = new TreeSet<String>(wellSet);
		if (type.equals("List")) {
			List<String> set = Arrays.asList(mask);
			for (String wellMaskUsed : set) {

				int index = 0;
				for (String wellID : currentWellSet) {

					if (!wellID.equals(wellMaskUsed))
						removedWells.add(wellSet.remove(index));
					else
						index++;
				}
			}
			return removedWells;
		}

		if (type.equals("Bellow")) {
			String boundary = mask[1];
			int index = 0;
			for (String wellID : currentWellSet) {

				if (wellID.compareTo(boundary) > 0)
					removedWells.add(wellSet.remove(index));
				else
					index++;
			}
			return removedWells;
		}

		if (type.equals("Above")) {
			String boundary = mask[0];
			int index = 0;
			for (String wellID : currentWellSet) {

				if (wellID.compareTo(boundary) < 0)
					removedWells.add(wellSet.remove(index));
				else
					break;
			}
			return removedWells;
		}

		if (type.equals("Range")) {
			String lowBoundary = mask[0], highBoundary = mask[1];
			int index = 0;

			for (String wellID : currentWellSet) {

				if (wellID.compareTo(highBoundary) > 0
						|| wellID.compareTo(lowBoundary) < 0)
					removedWells.add(wellSet.remove(index));
				else
					index++;
			}
			return removedWells;
		}

		// if (type.equals("RangeList")) {
		// List<String> set = Arrays.asList(mask);
		// // need to do recursion over each of values
		// System.err
		// .println("List of ranges is not designed yet! All Wells will be analyzed");
		//
		// return removedWells;
		// }
		System.err
				.println("This type of well mask is not supported! All Wells will be analyzed");

		return removedWells;
	}
}
