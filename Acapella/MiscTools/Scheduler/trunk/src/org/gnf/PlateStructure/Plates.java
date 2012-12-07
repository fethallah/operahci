/**
 *
 */
package org.gnf.PlateStructure;

import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Collections;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.Vector;

import org.gnf.IO.FileFinder;
import org.gnf.acapella.WellMask;

/**
 * @author bonamgh1
 *
 */
abstract public class Plates {

	private final Path path;
	private final FileFinder fileFinder;
	private List<Path> files = new LinkedList<>();

	Plates(File dir) throws Exception {

		if (!dir.isDirectory())
			throw new Exception(
					String.format(
							"The directory containing the images to analyze: '%1$s' is not directory.",
							dir.toString()));
		if (!dir.exists())
			throw new Exception(
					String.format(
							"The directory containing the images to analyze: '%1$s' does not exit.",
							dir.toString()));
		if (!dir.canRead())
			throw new Exception(
					String.format(
							"The directory containing the images to analyze: '%1$s' cannot be read.",
							dir.toString()));

		this.path = Paths.get(dir.toURI());

		fileFinder = new FileFinder("regex", getPattern());
		Files.walkFileTree(path, fileFinder);
		files = fileFinder.getFiles();

	}

	public List<Path> getFiles() {
		return files;
	}

	public Map<File, Vector<String>> getWells() throws Exception {
		return getWells(new WellMask("*"));
	}

	public Map<File, Vector<String>> getWells(WellMask wellMask) {
		Map<File, Vector<String>> plateSet = new TreeMap<>();
		Vector<String> wellSet;
		for (Path file : files) {
			File filePath = file.getParent().toFile();
			String wellID = getWellIndex(file.toFile());
			wellSet = plateSet.get(filePath);
			if (wellID == null)
				continue;
			if (wellSet != null) {
				if (!wellSet.contains(wellID))
					wellSet.add(wellID);
			} else {
				wellSet = new Vector<>();
				wellSet.add(wellID);
				plateSet.put(filePath, wellSet);
			}

		}
		for (File platePath : plateSet.keySet()) {
			wellSet = plateSet.get(platePath);
			if (!wellSet.isEmpty()) {
				Collections.sort(wellSet);
				wellMask.filterWellSet(wellSet);
			}
		}
		return plateSet;
	}

	protected abstract String getPattern();

	protected abstract String getWellIndex(File file);

}
