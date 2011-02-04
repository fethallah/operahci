package org.gnf.OperaErrorMonitor;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.Vector;

import org.gnf.OperaErrorMonitor.OperaLogsParsers.DateUtils;
import org.gnf.OperaErrorMonitor.RegQuery.StreamReader;
import org.ini4j.Ini;
import org.ini4j.InvalidFileFormatException;

public class ErrorManagment {
	Date lastPlateStartTime = new Date();
	long plateReadTimeAverage = 0;
	static Vector<HashMap<String, String>> contactInfo = new Vector<HashMap<String, String>>();
	public final static File INI_FILE = new File(
			System.getProperty("user.dir"), "Resources/OperaErrorMonitor.ini");
	private final static String AnalyzerRegPath = "HKEY_LOCAL_MACHINE\\SOFTWARE\\Evotec\\OperaPI\\2.0\\Distribution\\Connections";
	OperaLogsParsers log;
	static final String RMCA_PARAMETERS_KEY = "RMCARebootSettings";
	static final String CELL_PROVIDER_KEY = "cellProvider";
	final static String[][] cellProviders = {
			{ "Alltel", "Boost Mobile", "Cingular/AT&T", "Sprint Nextel",
					"Sprint PCS", "T-Mobile", "Verizon Wireless",
					"Virgin Mobile USA" },
			{ "message.alltel.com", "myboostmobile.com", "txt.att.net",
					"messaging.nextel.com", "messaging.sprintpcs.com",
					"tmomail.net", "vtext.com", "vmobl.com" } };
	private static final String INI_EMAIL_TAG = "email";
	private static final String INI_CELLNUMBER_TAG = "cellNumber";
	private static final String INI_PROVIDER_TAG = "cellProvider";
	private static final String[] INI_SUPORTED_TYPES = { "current",
			"operators", "managers" };
	private static String FROM = "OPERA@NoReply.org";
	private static final File RESTART_SOFT = new File(System
			.getProperty("user.dir"), "Resources/RMCARestart.exe");

	public ErrorManagment() {
		contactInfo = new Vector<HashMap<String, String>>();
		FROM = OperaErrorMonitor.INSTRUMENT + "@NoReply.org";
	}

	public ErrorManagment(OperaLogsParsers log) {
		super();
		this.log = log;

	}

	public ErrorManagment(Date lastPlateStartTime, long plateReadTimeAverage) {

		this.lastPlateStartTime = lastPlateStartTime;
		this.plateReadTimeAverage = plateReadTimeAverage;
	}

	public String rebootAnalyzers(String message) throws Exception {
		String userID = "";
		String pwd = "";
		try {
			Ini ini = new Ini(ErrorManagment.INI_FILE);
			userID = ini.get(RMCA_PARAMETERS_KEY).get("userID");
			pwd = ini.get(RMCA_PARAMETERS_KEY).get("PWD");
		} catch (InvalidFileFormatException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}

		Process process = null;
		String result = "";
		Vector<String> keySet = RegQuery.getKeySet(AnalyzerRegPath);
		if (keySet == null)
			throw new Exception("Could not locate the registry key: "
					+ AnalyzerRegPath);
		String status = "";
		Set<String> computers = new HashSet<String>();
		String computerName = "";
		for (String key : keySet) {
			if (Integer.valueOf(RegQuery.getKey(key, "InUse",
					RegQuery.REG_DWORD_TOKEN).toString()) == 1) {
				status = "OK";
				if (computers.add(computerName = (String) RegQuery.getKey(key,
						"Server_Name", RegQuery.REG_SZ_TOKEN))) {
					result += DateUtils.now(OperaLogsParsers.timeFormat)
							+ " - Restaring: \\\\" + computerName + " ... ";
					StreamReader reader;
					String[] command = { RESTART_SOFT.getPath() + " ",
							computerName, userID, pwd };
					try {
						process = Runtime.getRuntime().exec(command);
						reader = new StreamReader(process.getErrorStream());
						reader.start();
						process.waitFor();
						reader.join();
						String data = reader.getResult();
						if (data != null && !data.isEmpty())
							status = "FAILED - Error= "
									+ data.replaceAll("\\n", "").replace("\\r",
											"");
					} catch (InterruptedException e) {
						status = "FAILED - Error= " + e.getMessage();
						e.printStackTrace();
					} catch (IOException e) {
						status = "FAILED - Error= " + e.getMessage();
						e.printStackTrace();
					} finally {
						if (process != null)
							try {
								if (process.getErrorStream() != null)
									process.getErrorStream().close();
								if (process.getInputStream() != null)
									process.getInputStream().close();
								if (process.getOutputStream() != null)
									process.getOutputStream().close();
							} catch (IOException e) {
								e.printStackTrace();
							}
					}
					result += status + "\r\n";
				}

			}

		}
		String to;
		String peopleEmailed = "";
		EMail eMail;
		for (int index = 0; index < contactInfo.size(); index++) {
			to = contactInfo.get(index).get(INI_EMAIL_TAG);
			status = "OK";
			String instrument = OperaErrorMonitor.INSTRUMENT;
			eMail = new EMail(
					to,
					FROM,
					"FAILURE: " + instrument + " is IDLE",
					"The Opera was idle for too long as a first step computers in charge of analysis were restarted due to the following error: \r\n\r\n"
							+ message
							+ "\r\n\r\nThe following restart log was returned:\r\n"
							+ result, null);
			peopleEmailed += DateUtils.now(OperaLogsParsers.timeFormat)
					+ " - Emailing: " + to + "...";
			try {
				eMail.send();
			} catch (Exception e) {
				status = "FAILED - Error= " + e.getMessage();
				e.printStackTrace();
			}
			peopleEmailed += status + "\r\n";
		}

		return result + peopleEmailed;
	}

	public String sendNotification(String subject, String message,
			File[] attachments) {
		String instrument = "";
		try {
			instrument = (String) RegQuery
					.getKey(
							"HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Control\\ComputerName\\ComputerName",
							"ComputerName", RegQuery.REG_SZ_TOKEN);
		} catch (IOException e1) {
			e1.printStackTrace();
		}
		String result = "";
		String status = "";
		String to;
		EMail eMail;
		for (int index = 0; index < contactInfo.size(); index++) {
			status = "OK";
			to = contactInfo.get(index).get(INI_EMAIL_TAG);
			if (to != null && !to.isEmpty()) {
				result += DateUtils.now(OperaLogsParsers.timeFormat)
						+ " - Emailing: " + to + "...";
				eMail = new EMail(to, FROM, instrument + " - " + subject,
						message, attachments);
				try {

					eMail.send();
				} catch (Exception e) {
					status = "FAILED - Error= " + e.getMessage();
					e.printStackTrace();
				}
				result += status + "\r\n";
			}
			String part1 = contactInfo.get(index).get(INI_CELLNUMBER_TAG);
			String part2 = getCellProvider().get(
					contactInfo.get(index).get(INI_PROVIDER_TAG));
			if (part1 != null && !part1.isEmpty() && part2 != null
					&& !part2.isEmpty()) {
				status = "OK";
				to = part1 + part2;
				result += DateUtils.now(OperaLogsParsers.timeFormat)
						+ " - Emailing: " + to + "...";
				eMail = new EMail(
						to,
						instrument,
						"InstrumentFailure",
						"The Opera: "
								+ instrument
								+ " appears idle. A precise description of the message was send to your email",
						null);
				try {
					eMail.send();
				} catch (Exception e) {
					status = "FAILED - Error= " + e.getMessage();
					e.printStackTrace();
				}
				result += status + "\r\n";
			}

		}
		return result;

	}

	public Vector<HashMap<String, String>> getContactInfo() {
		return contactInfo;
	}

	public void setContactInfo(String email, String cellNumber, String provider) {
		// contactInfo = new Vector<HashMap<String, String>>();
		HashMap<String, String> info = new HashMap<String, String>();
		info.put(INI_EMAIL_TAG, email);
		info.put(INI_CELLNUMBER_TAG, cellNumber);
		info.put(INI_PROVIDER_TAG, provider);

		contactInfo.add(info);
	}

	public void setContactInfo(String type) throws Exception {
		// contactInfo = new Vector<HashMap<String, String>>();
		Ini ini = new Ini(INI_FILE);
		if (!Arrays.asList(INI_SUPORTED_TYPES).contains(type))
			throw new Exception(
					"Your current type: \""
							+ type
							+ "\" is not contained in the ini file provide.\r\nCurrently the group types supported are: "
							+ INI_SUPORTED_TYPES.toString());
		Ini.Section operators = ini.get("contacts").getChild(type);
		if (operators == null)
			throw new Exception(
					"Yor current INI file does not contain any contacts.");

		for (String operator : operators.childrenNames()) {
			HashMap<String, String> info = new HashMap<String, String>();
			for (String key : operators.getChild(operator).keySet()) {

				info.put(key, operators.getChild(operator).get(key));

			}
			contactInfo.add(info);
		}
	}

	public static Map<String, String> getCellProvider() {
		Map<String, String> cellProvider = new HashMap<String, String>();
		try {
			Ini ini = new Ini(ErrorManagment.INI_FILE);
			for (String key : ini.get(CELL_PROVIDER_KEY).keySet())
				cellProvider.put(key, ini.get(CELL_PROVIDER_KEY).get(key));
		} catch (InvalidFileFormatException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}
		if (cellProvider == null || cellProvider.isEmpty())
			for (int i = 0; i < cellProviders[0].length; i++)
				cellProvider.put(cellProviders[0][i], cellProviders[1][i]);
		return cellProvider;
	}

	public static void main(String[] args) throws Exception {
		ErrorManagment em = new ErrorManagment();
		// Add some tests ex:em.rebootAnalyzers("OperaC");
	}
}
