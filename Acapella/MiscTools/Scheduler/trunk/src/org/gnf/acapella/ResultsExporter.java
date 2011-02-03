package org.gnf.acapella;

import java.io.File;
import java.io.IOException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.Map;
import java.util.Set;

import org.gnf.IO.FileReader;
import org.gnf.IO.FileWritter;

public class ResultsExporter extends Thread implements Runnable {

	private Map<String, Map<String, String>> results;
	private File resultFile;
	private File headerFile;
	private boolean terminate = false;
	private String[] commonHeader = { "PlateID", "MeasID", "WellID", "Row",
			"Col", "filename" };

	/**
	 * @param results
	 * @param resultFile
	 */
	public ResultsExporter(Map<String, Map<String, String>> results,
			File resultFile, File headerFile) {
		super();
		this.results = results;
		this.resultFile = resultFile;
		this.headerFile = headerFile;
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
		boolean newFeaturesAdded = false;
		Set<String> featuresNames = new LinkedHashSet<String>();
		while (results.size() > 0 || !terminate) {
			StringBuffer data = new StringBuffer();
			if (results.size() >= 12 || terminate) {
				Set<String> wells = new HashSet<String>(results.keySet());
				// initialize the most common data set
				if (!initialized) {
					featuresNames.addAll(Arrays.asList(commonHeader));
					try {
						if (headerFile.exists()
								&& FileReader.readFile(headerFile).length() > 1) {
							String[] currentHeader = FileReader.readHeader(
									headerFile).toString().split(",");
							featuresNames.addAll(Arrays.asList(currentHeader));

						}
					} catch (IOException e) {
						// TODO Auto-generated catch block
						e.printStackTrace();
					}
					for (String wellInfo : wells) {
						featuresNames.addAll(results.get(wellInfo).keySet());
					}
					newFeaturesAdded = true;
					initialized = true;
				}

				// int index = 0;
				for (String wellID : wells) {
					newFeaturesAdded |= featuresNames.addAll(results
							.get(wellID).keySet());
					// Remove data for each well overtime
					Map<String, String> wellData = results.remove(wellID);
					// Appends to the CSV file the different features.
					for (String feature : featuresNames) {
						// collects the data according to the list of available
						// features constructed from the initialization

						String value = wellData.get(feature);
						value = value == null ? "" : value;
						data.append(value + ",");
					}
					data.append(System.getProperty("line.separator"));
					// index++;

					// if (index > 12 && !terminate)
					// break;
				}
				try {
					if (newFeaturesAdded) {
						newFeaturesAdded = false;
						String header = featuresNames.toString();
						header = header.replaceAll("(?m)^\\[", "");
						header = header.replaceAll("(?m)\\]$", "");
						header = header.replaceAll(",\\s*", ",");
						header = header + System.getProperty("line.separator");
						FileWritter.writeFile(headerFile, header, false);
					}
					FileWritter.writeFile(resultFile, data, true);

				} catch (IOException e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
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
