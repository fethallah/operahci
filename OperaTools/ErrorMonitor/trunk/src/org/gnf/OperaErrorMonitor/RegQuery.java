/**
 * 
 */
package org.gnf.OperaErrorMonitor;

/**
 * @author ghislain
 * 
 */
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.StringWriter;
import java.util.HashMap;
import java.util.Vector;

public class RegQuery {

	public static final String REG_SZ_TOKEN = "REG_SZ";
	public static final String REG_BINARY_TOKEN = "REG_BINARY";
	public static final String REG_DWORD_TOKEN = "REG_DWORD";
	public static final String REG_HEX_TOKEN = "REG_HEX";
	public static final String REG_MULTI_SZ_TOKEN = "REG_MULTI_SZ";
	public static final String REG_EXPAND_SZ_TOKEN = "REG_EXPAND_SZ";
	private static final File REG_EXE = new File(
			System.getProperty("user.dir"), "Resources/reg.exe");

	RegQuery() throws IOException {
		if (!REG_EXE.exists())
			throw new IOException("Could not find reg.exe in the resources");
		if (!REG_EXE.canExecute())
			throw new IOException("Could not execute reg.exe in the resources");
	}

	public static Object getKey(String keyPath, String keyName, String regToken)
			throws IOException {
		new RegQuery();
		String data = null;
		try {
			if (regToken == null)
				return null;
			Process process = Runtime.getRuntime().exec(
					REG_EXE.getPath() + " query \"" + keyPath + "\" /v "
							+ keyName);
			StreamReader reader = new StreamReader(process.getInputStream());

			reader.start();
			process.waitFor();
			reader.join();

			data = reader.getResult();
			int p = data.indexOf(regToken);

			if (p == -1)
				return null;
			if (regToken.equals(REG_DWORD_TOKEN)) {
				String temp = data.substring(p + regToken.length()).trim();
				return (Integer.parseInt(temp.substring("0x".length()).trim()
						.toUpperCase(), 16));
			}
			if (regToken.equals(REG_MULTI_SZ_TOKEN))
				return data.substring(p + regToken.length()).trim().replace(
						"\\0", "\r\n");

			return data.substring(p + regToken.length()).trim();
		} catch (Exception e) {
			return data;
		}
	}

	/**
	 * Returns a list of all the keys in the keyPath provided. Returns null if
	 * the keyPath does not exist.
	 * 
	 * @param keyPath
	 * @return
	 */
	public static Vector<String> getKeySet(String keyPath) throws IOException {
		new RegQuery();
		Vector<String> keySet = null;

		try {
			keySet = new Vector<String>();
			Process process = Runtime.getRuntime().exec(
					REG_EXE.getPath() + " query \"" + keyPath);
			StreamReader reader = new StreamReader(process.getInputStream());

			reader.start();
			process.waitFor();
			reader.join();

			String[] data = reader.getResult().split("\\r\\n");
			for (int i = 0; i < data.length; i++) {
				if (!data[i].endsWith(keyPath.replaceFirst("H.*\\\\", ""))
						&& data[i]
								.contains(keyPath.replaceFirst("H.*\\\\", "")))
					keySet.add(data[i]);

			}

			return keySet;
		} catch (Exception e) {
			return keySet;
		}
	}

	/**
	 * Returns a list of all the values in the keyPath provided (not including
	 * remaining keys). The list contains a hasmap with the following keys:
	 * "name", "type", "value". Returns "null" if the keyPath does not exist.
	 * 
	 * @param keyPath
	 * @return
	 */
	public static Vector<HashMap<String, String>> getValueSet(String keyPath)
			throws IOException {
		new RegQuery();
		Vector<HashMap<String, String>> valueSet = null;
		Process process = null;
		try {
			valueSet = new Vector<HashMap<String, String>>();
			process = Runtime.getRuntime().exec(
					REG_EXE.getPath() + " query \"" + keyPath);
			StreamReader reader = new StreamReader(process.getInputStream());

			reader.start();
			process.waitFor();
			reader.join();

			String[] data = reader.getResult().split("\\r\\n");
			boolean start = false;
			for (int i = 0; i < data.length; i++) {
				HashMap<String, String> value = new HashMap<String, String>();
				if (data[i].startsWith("H") && !start)
					start = true;
				else {
					if (data[i].startsWith(" ")) {
						String[] v = data[i].split("  +");
						value.put("name", v[1]);
						value.put("type", v[2]);
						if (v.length > 3)
							value.put("value", v[3]);
						else
							value.put("value", "");
						valueSet.add(value);
					}
				}
			}

			return valueSet;
		} catch (Exception e) {
			return valueSet;
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
	}

	static class StreamReader extends Thread {
		private InputStream is;
		private StringWriter sw;

		StreamReader(InputStream is) {
			this.is = is;
			sw = new StringWriter();
		}

		public void run() {
			try {
				int c;
				while ((c = is.read()) != -1)
					sw.write(c);
			} catch (IOException e) {
				;
			}
		}

		String getResult() {
			return sw.toString();
		}
	}

	public static void main(String s[]) {

		try {
			System.out
					.println(getKeySet("HKEY_LOCAL_MACHINE\\SOFTWARE\\Evotec"));
			System.out
					.println(getValueSet("HKEY_LOCAL_MACHINE\\SOFTWARE\\Evotec"));
			System.out.println(getKey("HKEY_LOCAL_MACHINE\\SOFTWARE\\Evotec",
					"EVOHOMEDIR", REG_SZ_TOKEN));
		} catch (IOException e) {
			e.printStackTrace();
		}
	}

}
