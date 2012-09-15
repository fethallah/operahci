/**
 *
 */
package org.gnf.IO;

import java.io.File;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.gnf.acapella.WellMask;

/**
 * @author gbonamy
 *
 */
/**
 * @author gbonamy
 *
 */
public class FileSeekerOpera {
	private final static String nameFilter = "(?:\\d{1,3}_)?(\\d{3})(\\d{3})\\d{3}";
	private final static String[] EXTENSIONS = { "tiff", "tif", "flex" };

	public static Map<File, Vector<String>> getFilesToAnalyze(File path)
			throws Exception {
		return getFilesToAnalyze(path, new WellMask("*"));
	}

	public static Map<File, Vector<String>> getFilesToAnalyze(File path,
			WellMask wellMask) throws Exception {

		if (path == null)
			return new TreeMap<File, Vector<String>>();

		if (!path.isDirectory())
			throw new Exception(
					"The directory containing the images to analyze: "
							+ path.getPath() + " is not directory.");
		if (!path.exists())
			throw new Exception(
					"The directory containing the images to analyze: "
							+ path.getPath() + " does not exist.");
		if (!path.canRead())
			throw new Exception(
					"The directory containing the images  to analyze: "
							+ path.getPath() + " cannot be read.");

		FileListFilter filter = new FileListFilter(nameFilter, EXTENSIONS);
		Map<File, Vector<String>> fileSet = new TreeMap<File, Vector<String>>();
		Vector<String> wellSet = new Vector<String>();
		for (File file : path.listFiles(filter)) {
			if (file.isDirectory() && file.canRead()) {
				fileSet.putAll(getFilesToAnalyze(file, wellMask));
				continue;
			}
			String wellID = getWellIndex(file);
			if (wellID != null && !wellSet.contains(wellID))
				wellSet.add(wellID);
		}
		if (!wellSet.isEmpty()) {
			Collections.sort(wellSet);
			wellMask.filterWellSet(wellSet);
			fileSet.put(path, wellSet);
		}
		return fileSet;

	}

	private static String getWellIndex(File file) {
		Pattern pattern = Pattern.compile(nameFilter);
		Matcher matcher = pattern.matcher(file.getName());
		boolean matchFound = matcher.find();
		if (matchFound) {
			String wellID = String.format("$1%03d$2%03d000", matcher.group(1),
					matcher.group(2));
			return wellID;
		}
		else
			return null;
	}
}

