/**
 *
 */
package org.gnf.IO;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.util.Collections;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.commons.io.FileUtils;
import org.gnf.acapella.WellMask;
import org.gnf.baseconverter.BaseConverterUtil;

/**
 * @author gbonamy
 *
 */
/**
 * @author gbonamy
 *
 */
public class FileSeekerInCell {
	private final static String nameFilter = "([^/_]+)_([^/_]+)_(\\\\d+)[/])?(([A-Z]+) - (\\\\d+)\\\\((.+?)\\\\)";
	private final static String[] EXTENSIONS = { "tiff", "tif" };

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
		boolean isFirstFile = true;
		for (File file : path.listFiles(filter)) {
			if (file.isDirectory() && file.canRead()) {
				fileSet.putAll(getFilesToAnalyze(file, wellMask));
				continue;
			}
			String wellID = getWellIndex(file);
			if (wellID != null && !wellSet.contains(wellID)) {
				wellSet.add(wellID);
				if(isFirstFile) {
					copyScriptFiles(file);
					isFirstFile=false;
				}
			}
		}
		if (!wellSet.isEmpty()) {
			Collections.sort(wellSet);
			wellMask.filterWellSet(wellSet);
			fileSet.put(path, wellSet);
		}

		return fileSet;

	}

	private static void copyScriptFiles(File imagePath) throws IOException,
			URISyntaxException {
		File resourcesLocs = new File((new FileSeekerInCell()).getClass()
				.getProtectionDomain().getCodeSource().getLocation().toURI())
				.getParentFile();
		resourcesLocs = new File(resourcesLocs, "Resources/InCell");
		File indexScript = new File(resourcesLocs, "index.script");
		File readimageScirpt = new File(resourcesLocs, "readimage.script");
		if (!indexScript.canRead() || !readimageScirpt.canRead()) {
			indexScript = new File((new FileSeekerInCell()).getClass()
					.getResource("/Resources/InCell/index.script").getPath());
			readimageScirpt = new File((new FileSeekerInCell()).getClass()
					.getResource("/Resources/InCell/reader.script").getPath());
		}

		FileUtils.copyFile(imagePath, indexScript);
		FileUtils.copyFile(imagePath, readimageScirpt);

	}

	private static String getWellIndex(File file) {
		Pattern pattern = Pattern.compile(nameFilter);
		Matcher matcher = pattern.matcher(file.getName());
		boolean matchFound = matcher.find();
		if (matchFound) {
			String row = matcher.group(4);
			int row_n = row.length() > 1 ? 26 : 0;
			row_n += BaseConverterUtil.fromBase26(""
					+ row.charAt(row.length() - 1)) + 1;
			int col = Integer.valueOf(matcher.group(5));
			String wellID = String.format("$1%03d$2%03d000", row_n, col);
			return wellID;

		} else
			return null;
	}
}
