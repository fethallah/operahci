/**
 * 
 */
package org.gnf.acapella;

import java.io.BufferedInputStream;
import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.gnf.IO.RelativePath;
import org.gnf.baseconverter.BaseConverterUtil;

/**
 * @author gbonamy
 * 
 */
public class AcapellaJob extends Thread implements Runnable {

	private File root;
	private File imagePath;
	private String plateID = "";
	private String measID = "";
	private String wellIDs;
	private Map<String, Map<String, String>> results;
	private Map<String, String> errors;
	private File scriptPath;

	public AcapellaJob(File scriptPath, File imagePath, String wellIDs,
			Map<String, Map<String, String>> results, Map<String, String> errors) {
		// super(new ThreadGroup("test"),this);
		new Thread(new ThreadGroup("test"), this);
		this.scriptPath = scriptPath;
		this.imagePath = imagePath;
		this.wellIDs = wellIDs;
		this.results = results;
		this.errors = errors;
	}

	public void setPlateID(String plateID) {
		this.plateID = plateID;
	}

	public void setMeasID(String measID) {
		this.measID = measID;
	}

	public void setImageRootPath(File root) {
		this.root = root;
	}

	@Override
	public void run() {
		super.run();
		Process process = null;
		ErrorExtractor errorReader;
		DataExtractor dataReader;
		try {
			process = Runtime.getRuntime().exec(
					"Acapella -dataset \"" + imagePath.getPath() + "\";"
							+ wellIDs + " " + scriptPath);
			errorReader = new ErrorExtractor(new BufferedInputStream(process
					.getErrorStream()), errors);
			errorReader.setName(this.getName() + "-errorReader");
			dataReader = new DataExtractor(new BufferedInputStream(process
					.getInputStream()), results);
			dataReader.setName(this.getName() + "-dataReader");
			dataReader.setPlateID(plateID);
			dataReader.setMeasID(measID);
			dataReader.setFilePath(RelativePath
					.getRelativePath(root, imagePath));
			dataReader.start();
			errorReader.setPlateID(plateID);
			errorReader.setMeasID(measID);
			errorReader.setWellIDs(wellIDs);
			errorReader.start();
			process.waitFor();
			dataReader.join();
			errorReader.join();
		} catch (InterruptedException e) {
			e.printStackTrace();
		} catch (IOException e) {
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
	}
}

class DataExtractor extends Thread implements Runnable {
	private BufferedReader br;
	private LinkedHashMap<String, String> wellResults = new LinkedHashMap<String, String>();
	private Map<String, Map<String, String>> results;
	private String imagePath;
	private String plateID = "";
	private String measID = "";

	public DataExtractor(BufferedInputStream is,
			Map<String, Map<String, String>> results) {
		super();
		this.br = new BufferedReader(new InputStreamReader(is));
		this.results = results;
	}

	public void setFilePath(String imagePath) {
		this.imagePath = imagePath;
	}

	public void setPlateID(String plateID) {
		this.plateID = plateID;
	}

	public void setMeasID(String measID) {
		this.measID = measID;
	}

	public void run() {
		super.run();
		String lineIndexIni = plateID + "/" + measID + "/";
		try {
			String line;
			String[] output;
			while ((line = br.readLine()) != null) {
				if (line.startsWith("OUTPUT:")) {
					output = line.replaceFirst("OUTPUT:\\s", "").split("\t=");
					if (!output[0].equals("WellEnd")) {
						// Makes sure that for each output a separate column is
						// created
						while (wellResults.get(output[0]) != null) {
							String[] value = output[0].split("|");
							output[0] = output[0].replaceFirst("|\\d+$", "");
							try {
								output[0] += "|"
										+ (Integer
												.parseInt(value[value.length - 1]) + 1);
							} catch (NumberFormatException e) {
								output[0] += "|" + 1;
							}

						}
						wellResults.put(output[0], output[1]);
					} else {
						String wellID = output[1].replaceAll("\"", "");
						wellResults.put("PlateID", plateID);
						wellResults.put("MeasID", measID);
						Pattern pattern = Pattern
								.compile("(?m)^(\\d{3})(\\d{3})");
						Matcher matcher = pattern.matcher(wellID);
						matcher.find();
						int row = Integer.valueOf(matcher.group(1));
						int col = Integer.valueOf(matcher.group(2));
						String WellIDStr = "";
						WellIDStr += BaseConverterUtil.toBase26(row - 1);
						WellIDStr += BaseConverterUtil.pad("" + col, "0", 2);

						wellResults.put("WellID", WellIDStr);
						wellResults.put("Row", "" + row);
						wellResults.put("Col", "" + col);

						wellResults.put("filename", imagePath + File.separator
								+ wellID);
						if (!wellResults.isEmpty()) {
							results.put(lineIndexIni + wellID, wellResults);
							wellResults = new LinkedHashMap<String, String>();
						}
					}
				}

			}

		} catch (IOException e) {
			e.printStackTrace();
		}
	}
}

class ErrorExtractor extends Thread implements Runnable {
	private BufferedReader br;
	private Map<String, String> errors;
	private String plateID;
	private String measID = "";
	private String wellIDs = "";

	public ErrorExtractor(BufferedInputStream is, Map<String, String> errors) {
		super();
		this.br = new BufferedReader(new InputStreamReader(is));
		this.errors = errors;
	}

	public void setPlateID(String plateID) {
		this.plateID = plateID;
	}

	public void setMeasID(String measID) {
		this.measID = measID;
	}

	public void setWellIDs(String wellIDs) {
		this.wellIDs = wellIDs;

	}

	public void run() {
		super.run();
		try {
			String lineIndexIni = plateID + "/" + measID + "/";
			String line;
			String[] output;
			while ((line = br.readLine()) != null) {
				if (line.startsWith("ERROR:")) {
					output = line.replaceFirst("ERROR:\\s", "").split("\t=");
					errors.put(lineIndexIni + output[0].replaceAll("\"", ""),
							output[1]);

				} else if (line.startsWith("Error in obtaining")) {
					errors.put(lineIndexIni + wellIDs.replaceAll(",", ";")
							+ "/Licensing", line);
				}

			}

		} catch (IOException e) {
			e.printStackTrace();
		}
	}

	public Map<String, String> getErrors() {
		return errors;
	}
}
