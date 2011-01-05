/**
 * 
 */
package org.gnf.IO;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.ByteBuffer;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;

/**
 * @author gbonamy
 * 
 */
public class FileWritter {

	public static void addFileHeader(File file, Object header)
			throws IOException {
		changeFileHeader(file, header, false);
	}

	/**
	 * This method will happens a file at the beginning. If replaceHeader is set
	 * to true, the first line of the file is removed.
	 * 
	 * @param file
	 * @param header
	 * @param replaceHeader
	 * @throws IOException
	 */
	public static void changeFileHeader(File file, Object header,
			boolean replaceHeader) throws IOException {
		if (file == null || !file.exists()) {
			FileWritter.writeFile(file, header);
			return;
		}

		File tempFile = new File(file + ".tmp");
		file.renameTo(tempFile);

		writeFile(file, header, false);

		BufferedReader reader = null;

		try {
			reader = new BufferedReader(new FileReader(tempFile));
			boolean initialized = false;
			String text = null;
			StringBuffer data = new StringBuffer();
			int lineIndex = 0;
			// repeat until all lines are read
			while ((text = reader.readLine()) != null) {
				if (!initialized && replaceHeader) {
					initialized = true;
					continue;
				}
				data.append(text);
				data.append(System.getProperty("line.separator"));

				if (lineIndex >= 1535) {
					writeFile(file, data, true);
					data = new StringBuffer();
					lineIndex = 0;
				}
				lineIndex++;
			}
			// Copy the rest of the data.
			writeFile(file, data, true);
		} finally {

			if (reader != null)
				reader.close();

			tempFile.delete();
		}
	}

	public static void writeFile(File file, Object data, boolean append)
			throws IOException {

		if (!file.getParentFile().exists())
			file.getParentFile().mkdirs();

		RandomAccessFile raf = new RandomAccessFile(file, "rw");
		FileChannel channel = raf.getChannel();

		// Use the file channel to create a lock on the file.
		// This methodblocks until it can retrieve the lock.
		FileLock lock;
		int index = 0;
		while ((lock = channel.tryLock()) == null && index < 500) {
			try {
				Thread.sleep(500);
			} catch (InterruptedException e) {
				// TODOAuto-generated catch block
				e.printStackTrace();
			}
		}

		if (!append) {
			raf.setLength(0);
			raf.seek(0);
		} else {
			raf.seek(file.length());
		}

		ByteBuffer out = ByteBuffer.wrap(data.toString().getBytes());

		try {
			channel.write(out);
		} catch (Exception e) {
			e.printStackTrace();
		} finally {

			if (lock != null && lock.isValid())
				lock.release();
			if (channel != null)
				channel.close();
			if (raf != null)
				raf.close();

		}
	}

	public static void writeFile(File file, Object data) throws IOException {
		writeFile(file, data, false);

	}

}
