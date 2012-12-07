package org.gnf.PlateStructure;

import java.io.File;
import java.io.IOException;
import java.util.Map;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.commons.io.FileUtils;
import org.gnf.IO.ResourcesResolver;
import org.gnf.Tools.BaseConverterUtil;
import org.gnf.acapella.WellMask;

public class PlatesInCell extends Plates {
	private static final String PATTERN = "(?:([^/_]+)_([^/_]+)_(\\d+)[/])?(([A-Z]+) - (\\d+)\\((.+?)\\))\\.(?:tiff?)$";

	public PlatesInCell(File path) throws Exception {
		super(path);

	}

	@Override
	public Map<File, Vector<String>> getWells(WellMask wellMask) {
		Map<File, Vector<String>> plateSet = super.getWells(wellMask);

		for (File platePath : plateSet.keySet()) {
			try {
				copyScriptFiles(platePath);
			} catch (IOException e) {
				System.err
						.print(String
								.format("An error occured copying the files Acapella files required to analyze the InCell images: '%1$s'%nPlate '%2$s' will be ignored.",
										e.getMessage(), platePath));
				plateSet.remove(platePath);
				continue;
			}
		}

		return plateSet;

	}

	private static void copyScriptFiles(File platePath) throws IOException {

		File resourcesLocs = new File("InCell");

		File srcFile = ResourcesResolver.getResourceFile(new File(
				resourcesLocs, "index.script"));
		File destFile = new File(platePath, "index.script");
		FileUtils.copyFile(srcFile, destFile);
		destFile.deleteOnExit();

		srcFile = ResourcesResolver.getResourceFile(new File(resourcesLocs,
				"readimage.script"));
		destFile = new File(platePath, "readimage.script");
		FileUtils.copyFile(srcFile, destFile);
		destFile.deleteOnExit();

	}

	@Override
	protected String getPattern() {
		return PATTERN;
	}

	@Override
	protected String getWellIndex(File file) {
		Pattern pattern = Pattern.compile(PATTERN);
		Matcher matcher = pattern.matcher(file.getName());
		boolean matchFound = matcher.find();
		if (matchFound) {
			String row = matcher.group(5);
			int row_n = row.length() > 1 ? 26 : 0;
			row_n += BaseConverterUtil.fromBase26(""
					+ row.charAt(row.length() - 1)) + 1;
			int col = Integer.valueOf(matcher.group(6));
			String wellID = String.format("%03d%03d000", row_n, col);
			return wellID;

		} else
			return null;
	}

	/**
	 * This main class is for testing purposes
	 *
	 * @param args
	 * @throws Exception
	 */
	public static void main(String[] args) throws Exception {
		new PlatesInCell(
				new File(
						"\\\\nibr.novartis.net\\chbs-dfs\\LABDATA\\Inbox\\PHCHBS-I21583\\Projects\\BRD4\\3val\\analysed\\YI14_FX00238646_1"))
				.getWells();
	}

}
