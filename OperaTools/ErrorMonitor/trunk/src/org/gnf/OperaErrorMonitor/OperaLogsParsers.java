package org.gnf.OperaErrorMonitor;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Date;
import java.util.Vector;

public class OperaLogsParsers {

	public final static String currentDir = System.getProperty("user.dir");
	public final static String separtor = System.getProperty("file.separator");
	private final static String OperaRegPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Evotec";
	private static File bernsteinLogDir = null;
	private static File bernsteinNotesDir = null;
	private File bernsteinHostLinkDir = null;
	private static File bernsteinLogFile = new File("RunProtocol.wri");
	private static File bernsteinStatusFile = new File(
			"Bernstein_OnlineModulestate0.txt");

	private Vector<String[]> bernsteinStatus = new Vector<String[]>();
	private Vector<String[]> bernsteinLog = new Vector<String[]>();
	public final static String timeFormat = "yyyy-MMM-dd HH:mm:ss";

	OperaLogsParsers() throws IOException {
		try {
			if (bernsteinLogDir == null) {
				bernsteinLogDir = new File((String) RegQuery.getKey(
						OperaRegPath + "\\Bernstein",
						"m_sBernsteinProtocolsPath", RegQuery.REG_SZ_TOKEN));
				bernsteinLogFile = new File(bernsteinLogDir.getPath(),
						bernsteinLogFile.getPath());
			}
			if (bernsteinNotesDir == null) {
				bernsteinNotesDir = new File((String) RegQuery.getKey(
						OperaRegPath + "\\Bernstein",
						"m_sPersistentModuleStatePath", RegQuery.REG_SZ_TOKEN));
				bernsteinStatusFile = new File(bernsteinNotesDir.getPath(),
						bernsteinStatusFile.getPath());
			}
			if (bernsteinHostLinkDir == null) {
				bernsteinHostLinkDir = new File((String) RegQuery.getKey(
						OperaRegPath + "\\Hostlink",
						"sPersistentDeviceStatePath", RegQuery.REG_SZ_TOKEN));
				bernsteinHostLinkRobFile = new File(bernsteinHostLinkDir
						.getPath(), bernsteinHostLinkRobFile.getPath());
				bernsteinHostLinkReaderFile = new File(bernsteinHostLinkDir
						.getPath(), bernsteinHostLinkReaderFile.getPath());
			}
			bernsteinLog = bernsteinLogParser(bernsteinLogFile);
			bernsteinStatus = bernsteinStatusParser(bernsteinStatusFile);
		} catch (NullPointerException e) {
			e.printStackTrace();
			throw new IOException(
					"Your registry does not contain the proper registry keys for Bernstein:\r\n\""
							+ OperaRegPath + "\\Bernstein\\\"");

		}

		if (bernsteinLog.isEmpty())
			throw new IOException(
					"The file containing the info about the bernstein status:\r\n\""
							+ bernsteinLogFile.getPath()
							+ "\" could not be parsed.");

		if (bernsteinStatus == null)
			throw new IOException(
					"The file containing the info about the bernstein status:\r\n\""
							+ bernsteinStatusFile.getPath()
							+ "\" could not be parsed.");
	}

	Vector<String[]> bernsteinStatusParser(File bernsteinStatusFile)
			throws IOException {

		Vector<String[]> bernsteinStatus = new Vector<String[]>();

		int repeats = 0;

		while (!bernsteinStatusFile.exists()) {
			if (repeats > 10)
				throw new IOException(
						"The file containing the info about the bernstein status:\r\n\""
								+ bernsteinStatusFile.getPath()
								+ "\" does not exist.");
			Sleep.delay(100);
		}

		repeats = 0;
		while (!bernsteinStatusFile.canRead()) {
			if (repeats > 10)
				throw new IOException(
						"The file containing the info about the bernstein status:\r\n\""
								+ bernsteinStatusFile.getPath()
								+ "\" is locked and cannot be read.");
			Sleep.delay(100);
		}
		repeats = 0;

		while (bernsteinStatus.isEmpty() && repeats < 10) {
			try {
				BufferedReader filein = getFileStream(bernsteinStatusFile);
				if (filein == null)
					return bernsteinStatus;
				String currentLine = null;

				boolean start = false;
				while ((currentLine = filein.readLine()) != null) {
					if (currentLine.contains("ENDOFPLATES"))
						return bernsteinStatus;
					if (currentLine.startsWith("PLATES")) {
						start = true;
						continue;
					}
					if (start && currentLine.split("\\s+").length > 2)
						bernsteinStatus.add(currentLine.split("\\s+"));
				}

				filein.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				if (repeats < 10) {
					Sleep.delay(100);
					repeats++;
					continue;
				}
			}
		}

		return null;
	}

	Vector<String[]> bernsteinLogParser(File bernsteinLogFile)
			throws IOException {

		Vector<String[]> bernsteinLog = new Vector<String[]>();
		int repeats = 0;
		while (!bernsteinLogFile.exists()) {
			if (repeats > 10)
				throw new IOException(
						"The file containing the info about the bernstein status:\r\n\""
								+ bernsteinLogFile.getPath()
								+ "\" does not exist.");
			Sleep.delay(100);
		}

		repeats = 0;
		while (!bernsteinLogFile.canRead()) {
			if (repeats > 10)
				throw new IOException(
						"The file containing the info about the bernstein status:\r\n\""
								+ bernsteinLogFile.getPath()
								+ "\" is locked and cannot be read.");
			Sleep.delay(100);
		}
		repeats = 0;
		while (bernsteinLog.isEmpty()) {
			try {
				BufferedReader filein = getFileStream(bernsteinLogFile);
				if (filein == null)
					return bernsteinLog;
				String currentLine = null;

				while ((currentLine = filein.readLine()) != null) {
					String[] paresedLine = currentLine.split("\\s+");
					if (currentLine.isEmpty() || paresedLine.length <= 1)
						continue;
					int size = bernsteinLog.size();
					String[] data = {
							paresedLine[4] + "-" + paresedLine[1] + "-"
									+ paresedLine[2] + " " + paresedLine[3],
							paresedLine[5] };
					if (size > 1
							&& bernsteinLog.get(size - 1)[1].equals(data[1]))
						continue; // skip duplicated lines

					bernsteinLog.add(data);
				}

				filein.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				if (repeats < 10) {
					Sleep.delay(5);
					repeats++;
					continue;
				}
			}
		}
		return bernsteinLog;
	}

	public int countDonePlates(Vector<String[]> bernsteinStatus) {
		int done = 0;
		for (int i = 0; i < bernsteinStatus.size(); i++) {
			if (!bernsteinStatus.get(i)[0].isEmpty()
					&& bernsteinStatus.get(i)[2].equals("Done"))
				done++;
		}
		return done;
	}

	public int getLastDeActivationIndex(Vector<String[]> bernsteinLog2) {
		for (int i = bernsteinLog.size() - 1; i >= 0; i--) {
			if (bernsteinLog.get(i)[1].equals("DEACTIVATION"))
				return i;
		}
		return bernsteinLog.size() - 1;
	}

	public int getLastActivationIndex(Vector<String[]> bernsteinLog) {
		for (int i = bernsteinLog.size() - 1; i >= 0; i--) {
			if (bernsteinLog.get(i)[1].equals("ACTIVATION"))
				return i;
		}
		return bernsteinLog.size() - 1;
	}

	public Vector<String[]> getBernsteinStatus() {
		return bernsteinStatus;
	}

	public Vector<String[]> getBernsteinLog() {
		return bernsteinLog;
	}

	/**
	 * @return the bernsteinHostLinkRobFile
	 */
	public File getBernsteinHostLinkRobFile() {
		return bernsteinHostLinkRobFile;
	}

	/**
	 * @return the bernsteinHostLinkReaderFile
	 */
	public File getBernsteinHostLinkReaderFile() {
		return bernsteinHostLinkReaderFile;
	}

	private File bernsteinHostLinkRobFile = new File("DeviceState_Rob.txt");
	private File bernsteinHostLinkReaderFile = new File(
			"DeviceState_Reader.txt");

	private static BufferedReader getFileStream(File data) {

		try {
			FileReader fr = new FileReader(data);
			BufferedReader br = new BufferedReader(fr);
			return br;
		} catch (FileNotFoundException fnfeEx) {
			System.err.println(fnfeEx.toString());
			return null;
		}
	}

	public static class DateUtils {

		public static String now(String dateFormat) {
			Calendar cal = Calendar.getInstance();
			SimpleDateFormat sdf = new SimpleDateFormat(dateFormat);
			return sdf.format(cal.getTime());
		}

		public static Date now() {
			Calendar cal = Calendar.getInstance();
			return cal.getTime();
		}

		public static long difference(Date ti, Date tf) {

			return ti.getTime() - tf.getTime();
		}

		public static String formatedTime(String dateFormat, long ms) {
			SimpleDateFormat sdf = new SimpleDateFormat(dateFormat);
			return sdf.format(ms);
		}
	}

	public static class Sleep {

		public static void delay(long s) {
			try {
				Thread.currentThread().sleep(s * 1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}

	public File getBernsteinLogFile() {
		return bernsteinLogFile;
	}

	public File getBernsteinStatusFile() {
		return bernsteinStatusFile;
	}

}
