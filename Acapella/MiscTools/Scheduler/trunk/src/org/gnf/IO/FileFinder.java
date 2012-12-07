package org.gnf.IO;

import java.io.IOException;
import java.nio.file.FileSystems;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.nio.file.Paths;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.LinkedList;
import java.util.List;

public class FileFinder extends SimpleFileVisitor<Path> {

	private final PathMatcher matcher;
	private List<Path> files = new LinkedList<>();

	public FileFinder(String type, String pattern) {
		matcher = FileSystems.getDefault().getPathMatcher(type + ":" + pattern);
	}

	// Compares the glob pattern against
	// the file or directory name.
	void find(Path file) {
		Path path = file.getFileName();
		if (path != null && matcher.matches(path)) {
			files.add(file);
		}
	}

	public List<Path> getFiles() {
		return files;
	}

	// Invoke the pattern matching
	// method on each file.
	@Override
	public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) {
		find(file);
		return FileVisitResult.CONTINUE;
	}

	// Invoke the pattern matching
	// method on each directory.
	@Override
	public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) {
		find(dir);
		return FileVisitResult.CONTINUE;
	}

	@Override
	public FileVisitResult visitFileFailed(Path file, IOException exc) {
		System.err.println(exc);
		return FileVisitResult.CONTINUE;
	}

	/**
	 * This main class is for testing purposes
	 *
	 * @param args
	 * @throws IOException
	 */
	public static void main(String[] args) throws IOException {
		FileFinder fileFinder = new FileFinder("regex", ".+?\\.xml");
		Files.walkFileTree(Paths.get("c:\\windows\\"), fileFinder);
		System.out.print(fileFinder.getFiles());
	}
}
