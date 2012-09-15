package org.gnf.IO;

import java.io.File;
import java.io.FilenameFilter;

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