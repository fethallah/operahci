package org.gnf.acapella;

import java.io.File;
import java.io.IOException;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.gnf.IO.FileReader;
import org.gnf.IO.FileWritter;
import org.gnf.baseconverter.BaseConverterUtil;

public class ErrorExporter extends Thread implements Runnable {

	private Map<String, String> errors;
	private File errorFile;
	private boolean terminate = false;
	private String commonHeader = "PlateID,MeasID,WellID,Path,Errors";

	/**
	 * @param results
	 * @param resultFile
	 */
	public ErrorExporter(Map<String, String> errors, File errorFile) {
		super();
		this.errors = errors;
		this.errorFile = errorFile;
		this.start();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Thread#run()
	 */
	@Override
	public void run() {
		super.run();
		boolean initialized = false;
		while (errors.size() > 0 || !terminate) {
			StringBuffer data = new StringBuffer();
			Set<String> wells = new HashSet<String>(errors.keySet());
			if (!initialized) {
				try {
					if (!errorFile.exists()
							|| FileReader.readFile(errorFile).length() < 1) {
						data.append(commonHeader);
						data.append(System.getProperty("line.separator"));
					}
				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
				initialized = true;
			}

			int index = 0;
			for (String wellID : wells) {
				String wellData = errors.remove(wellID);
				String[] wellInfo = wellID.split("/");
				// if (wellInfo.length == 2) {
					data.append(wellInfo[0] + "," + wellInfo[1] + ",");

					Pattern pattern = Pattern.compile("(?m)^(\\d{3})(\\d{3})");
					Matcher matcher = pattern.matcher(wellInfo[2]);
					matcher.find();
					int row = Integer.valueOf(matcher.group(1));
					int col = Integer.valueOf(matcher.group(2));
					data.append(BaseConverterUtil.toBase26(row - 1));
					data.append(BaseConverterUtil.pad("" + col, "0", 2));
					data.append(",");
					data.append(AcapellaScheduler.getDataFile().getPath()
							+ File.separator + wellInfo[2]);
				data.append(",");
				// } else {
				// data.append(wellInfo[0] + ",,,");
				// }
				data.append(wellData);
				data.append(System.getProperty("line.separator"));
				index++;
			}
			try {

				FileWritter.writeFile(errorFile, data, true);
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			try {
				if (!terminate)
					sleep(30000); // wait 30s
			} catch (InterruptedException e) {
				if (!terminate)
					e.printStackTrace();
			}
		}

	}

	/*
	 * Terminates the thread by setting the terminate flag to true. In this case
	 * the remainders of ExportResults are written to the selected file and the
	 * thread run() function is exited.
	 */

	public void terminate() {
		// TODO Auto-generated method stub
		terminate = true;
		interrupt();
		try {
			join();

		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}

}
