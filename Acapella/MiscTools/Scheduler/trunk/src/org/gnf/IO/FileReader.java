/**
 * 
 */
package org.gnf.IO;

import java.io.File;
import java.io.IOException;
import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.nio.channels.FileLock;

/**
 * @author gbonamy
 * 
 */
public class FileReader {

	public static StringBuffer readFile(File file) throws IOException {

		StringBuffer text = new StringBuffer();
		if (file == null || !file.exists())
			return text;

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

		// BufferedReader reader = null;

		try {
			// reader = new BufferedReader(new java.io.FileReader(file));
			String line = null;

			// repeat until all lines are read
			while ((line = raf.readLine()) != null) {
				text.append(line).append(System.getProperty("line.separator"));
			}
		} finally {
			if (lock != null && lock.isValid())
				lock.release();
			if (channel != null)
				channel.close();
			if (raf != null)
				raf.close();
			// if (reader != null)
			// reader.close();
		}

		return text;
	}

	public static StringBuffer readHeader(File file) throws IOException {

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

		StringBuffer text = new StringBuffer();
		if (file == null || !file.exists())
			return text;

		try {
			text.append(raf.readLine());

		} finally {
			if (lock != null && lock.isValid())
				lock.release();
			if (channel != null)
				channel.close();
			if (raf != null)
				raf.close();
			// if (reader != null)
			// reader.close();
		}

		return text;
	}
}
