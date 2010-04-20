/**
 * 
 */
package org.gnf.IO;

import java.io.File;
import java.io.FilenameFilter;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Vector;

import org.gnf.acapella.WellMask;

/**
 * @author gbonamy
 *
 */
/**
 * @author gbonamy
 * 
 */
public class FileSeeker {

	private final static String[] EXTENSIONS = { "tiff", "tif", "flex" };

	public static Map<File, Vector<String>> getFilesToAnalyze(File path)
			throws Exception {
		return getFilesToAnalyze(path, new WellMask("*"));
	}

	public static Map<File, Vector<String>> getFilesToAnalyze(File path,
			WellMask wellMask) throws Exception {

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

		FileListFilter filter = new FileListFilter("\\d{1,3}_\\d{4,9}",
				EXTENSIONS);
		Map<File, Vector<String>> fileSet = new LinkedHashMap<File, Vector<String>>();
		Vector<String> wellSet = new Vector<String>();
		for (File file : path.listFiles(filter)) {
			if (file.isDirectory() && file.canRead()) {
				fileSet.putAll(getFilesToAnalyze(file, wellMask));
				continue;
			}
			String wellID = file.getName().replaceAll(".*_(\\d{4,9}).*", "$1");
			if (!wellSet.contains(wellID))
				wellSet.add(wellID);
		}
		if (!wellSet.isEmpty()) {
			wellMask.filterWellSet(wellSet);
			fileSet.put(path, wellSet);
}
		return fileSet;

	}
}

class FileListFilter implements FilenameFilter {
	private String regex;
	private String[] extensions;

	public FileListFilter(String regex, String[] extensions) {
		this.regex = regex;
		this.extensions = extensions;
	}

	public boolean accept(File directory, String filename) {
		return accept(directory, filename, true);
	}

	public boolean accept(File directory, String filename,
			boolean acceptDirectory) {
		boolean fileOK = true;
		if (acceptDirectory) {
			boolean isDirectory = new File(directory, filename).isDirectory();
			if (isDirectory)
				return fileOK;
		}
		regex = regex == null ? ".*" : regex;
		extensions = extensions == null ? new String[] { ".*" } : extensions;
		boolean matchExtension = false;
		for (String extension : extensions) {
			if (matchExtension = filename.matches(regex + "\\." + extension))
				break;
		}
		fileOK &= matchExtension;

		return fileOK;
	}
}