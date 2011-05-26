package org.gnf.OperaErrorMonitor;

import java.io.BufferedWriter;
import java.io.File;
import java.io.FileWriter;
import java.io.IOException;

import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JTextArea;
import javax.swing.UIManager;

import org.gnf.OperaErrorMonitor.OperaLogsParsers.DateUtils;
import org.ini4j.Ini;
import org.ini4j.InvalidFileFormatException;

public class OperaErrorMonitor {
	private static final String MONITOR_PARAMETER = "monitorParameters";
	private static final String MONITOR_PARAMETER_NOTIFICATION_DELAY = "notificationDelay";
	private static final String MONITOR_PARAMETER_DELAY = "monitorDelay";
	private static final String MONITOR_PARAMETER_RESEND_DELAY = "resendDelay";
	private static final String MONITOR_PARAMETER_MAX_NUM_NOTIFCATION = "maxNumberNotifications";
	private static final String MONITOR_PARAMETER_EMAIL_BASE = "baseEmail";
	private static final String LOG_FILE_PATH = "logFilePath";
	private static String errorNotificationDelay;
	private static String delay;
	private static String resendDelay;
	private static String maxNumNotif;
	static String loggedUserEmail;
	static File MONITOR_LOG_FILE;
	static String INSTRUMENT;
	static final File RESOURCES = new File(System.getProperty("java.io.tmpdir")
			+ "/OperaErrorMonitor");

	/**
	 * @param args
	 * @return
	 * @throws Exception
	 */
	OperaErrorMonitor() throws Exception {

		try {
			INSTRUMENT = (String) RegQuery
					.getKey(
							"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
							"ComputerName", RegQuery.REG_SZ_TOKEN);
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		Ini ini = null;
		try {
			ini = new Ini(ErrorManagment.INI_FILE);
		} catch (InvalidFileFormatException e) {
			e.printStackTrace();
			throw new Exception(
					"The ini file does not have a correct format and returned this error: "
							+ e.getMessage());
		} catch (IOException e) {
			e.printStackTrace();
			throw new Exception(
					"The ini file could not be read and returned this error: "
							+ e.getMessage());

		}
		String path = ini.get(MONITOR_PARAMETER).get(LOG_FILE_PATH);
		path = (path == null || path.isEmpty() || !(new File(path)).exists()) ? "./"
				: path;
		MONITOR_LOG_FILE = new File(path + INSTRUMENT + "-"
				+ "OperaErrorMonitor.log");
		// Make a copy of the Monitoring file if it is more than 5Mb
		if (MONITOR_LOG_FILE.length() / 1024 / 1024 > 5) {
			MONITOR_LOG_FILE.renameTo(new File(MONITOR_LOG_FILE.getPath()
					.replace(".log", ".bak")));
			MONITOR_LOG_FILE.delete();
		}
		writeLog(DateUtils.now(OperaLogsParsers.timeFormat)
				+ " - MONITORING TOOL LAUNCHED", null);
	}

	public static void main(String[] args) throws Exception {
		RESOURCES.mkdirs();
		RESOURCES.deleteOnExit();
		try {
			Directory.copyDirectory(new File("./Resources"), new File(
					RESOURCES, "Resources"));
			System.getProperty("user.dir");
		} catch (IOException x) {
			// Print the error but continue
			System.err.println(x);
		}
		new OperaErrorMonitor();

		GUI window;

		try {
			Ini ini = null;
			try {
				ini = new Ini(ErrorManagment.INI_FILE);
			} catch (InvalidFileFormatException e) {
				e.printStackTrace();
				throw new Exception(
						"The ini file does not have a correct format and returned this error: "
								+ e.getMessage());
			} catch (IOException e) {
				e.printStackTrace();
				throw new Exception(
						"The ini file could not be read and returned this error: "
								+ e.getMessage());

			}
			window = new GUI();
			errorNotificationDelay = ini.get(MONITOR_PARAMETER).get(
					MONITOR_PARAMETER_NOTIFICATION_DELAY);
			if (errorNotificationDelay == null)
				throw new Exception("The ini file does not contain"
						+ MONITOR_PARAMETER_NOTIFICATION_DELAY);
			else
				window.setErrorNotificationDelay(Double
						.valueOf(errorNotificationDelay));
			
			delay = ini.get(MONITOR_PARAMETER).get(MONITOR_PARAMETER_DELAY);
			if (delay == null)
				throw new Exception("The ini file does not contain"
						+ MONITOR_PARAMETER_DELAY);
			else
				window.setDelay(Long.valueOf(delay));

			resendDelay = ini.get(MONITOR_PARAMETER).get(
					MONITOR_PARAMETER_RESEND_DELAY);
			if (resendDelay == null)
				throw new Exception("The ini file does not contain"
						+ MONITOR_PARAMETER_RESEND_DELAY);
			else
				window.setResendDelay(Long.valueOf(resendDelay) * 60);

			maxNumNotif = ini.get(MONITOR_PARAMETER).get(
					MONITOR_PARAMETER_MAX_NUM_NOTIFCATION);
			if (maxNumNotif == null)
				throw new Exception("The ini file does not contain"
						+ MONITOR_PARAMETER_MAX_NUM_NOTIFCATION);
			else
				window.setMaxNumNotifications(Integer.valueOf(maxNumNotif));
			loggedUserEmail=ini.get(MONITOR_PARAMETER).get(
					MONITOR_PARAMETER_EMAIL_BASE);
			loggedUserEmail = (loggedUserEmail == null || !loggedUserEmail
					.matches("^@\\w*\\.\\w*$")) ? "" : loggedUserEmail;
			loggedUserEmail = System.getProperty("user.name") + loggedUserEmail;
			window.setLoggedUserEmail(loggedUserEmail);
			if (args.length > 0) {
				for (int i = 0; i < args.length; i++) {
					if (args[i].equals("-startMonitoring")) {
						window.startStop();
						break;
					}
				}
			}

		} catch (Exception e) {
			error(e.getMessage(), null);
			e.printStackTrace();
			exit(e.getMessage());
		}
	}

	static void error(String error, JFrame jFrame) {

		writeLog(DateUtils.now(OperaLogsParsers.timeFormat)
				+ "- MONITORING TOOL ERROR - Message= "
				+ error.replaceAll("\r\n", " "), null);
		if (jFrame == null) {
			try {
				UIManager.setLookAndFeel(UIManager
						.getSystemLookAndFeelClassName());
			} catch (Exception exception) {
			}
			jFrame = new JFrame();
		}
		JOptionPane
				.showMessageDialog(
						jFrame,
						"The ini file containing the initialization paramters is missing or invalid!\r\nThe following error message was returned: "
								+ error, "Error", JOptionPane.ERROR_MESSAGE);
	}

	public static void writeLog(String log, JTextArea logTextArea) {
		log += "\r\n";
		log = log.replaceAll("\r\n\r\n", "\r\n");
		if (logTextArea != null) {
			logTextArea.append(log);
			logTextArea.setCaretPosition(logTextArea.getText().length());
			logTextArea.repaint();
		}
		log = log.replaceFirst("^\r\n", "");
		BufferedWriter fileOut = null;
		try {

			fileOut = new BufferedWriter(new FileWriter(MONITOR_LOG_FILE, true));
			fileOut.write(log);
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				if (fileOut != null) {
					fileOut.close();
				}
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
	}

	static void exit(String log) {
		writeLog(DateUtils.now(OperaLogsParsers.timeFormat)
				+ " - MONITORING TOOL EXITED. Message=" + log, null);
		new Directory();
		boolean deleteDirectory = Directory.deleteDirectory(RESOURCES);
		System.exit(0);

	}

}
