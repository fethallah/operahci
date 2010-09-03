/**
 * 
 */
package org.gnf.acapella;

import jargs.gnu.CmdLineParser;

import java.io.File;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.Vector;

import org.gnf.IO.FileReader;
import org.gnf.IO.FileSeeker;
import org.gnf.IO.FileWritter;

/**
 * @author gbonamy
 * 
 */
public class AcapellaScheduler {

	static Map<String, Map<String, String>> results = Collections
			.synchronizedMap(new LinkedHashMap<String, Map<String, String>>());
	static Map<String, String> errors = Collections
			.synchronizedMap(new LinkedHashMap<String, String>());

	private static File scriptFile;
	private static File parametersFile = null;

	private static File resultPath;
	private static File dataFile;
	private static File errorFile;
	private static boolean overWrite = true;
	private static boolean separateHeader = false;

	private static File imagesPath;
	private static WellMask wellMask;
	private static int maxBatchSize = 50;

	private static int numProcessor = 1;

	/**
	 * @param args
	 * @throws Exception
	 */
	public static void main(String[] args) throws Exception {

		CmdLineParser parser = new CmdLineParser();

		CmdLineParser.Option parameterFile = parser.addStringOption('p',
				"param");
		CmdLineParser.Option scriptFileName = parser.addStringOption('s',
				"script");

		CmdLineParser.Option resultPath = parser.addStringOption('r',
				"resultPath");
		CmdLineParser.Option dataFileName = parser.addStringOption('d',
				"dataFile");
		CmdLineParser.Option errorFileName = parser.addStringOption('e',
				"errorFile");

		CmdLineParser.Option overWriteOpt = parser.addBooleanOption('O',
				"overWrite");

		CmdLineParser.Option separateHeaderOpt = parser.addBooleanOption('h',
				"separateHeader");

		CmdLineParser.Option wellMask = parser.addStringOption('m', "wellMask");
		CmdLineParser.Option batchSize = parser.addIntegerOption('b',
				"batchSize");

		CmdLineParser.Option numProcessor = parser.addIntegerOption('t',
				"threads");
		try {
			parser.parse(args);
		} catch (CmdLineParser.OptionException e) {
			error((Exception) e);
		}

		String value;
		value = (String) parser.getOptionValue(parameterFile, "");
		setParametersFile(value.isEmpty() ? null : new File(value));

		value = (String) parser.getOptionValue(scriptFileName, "");
		if (value.isEmpty())
			error(new Exception(
					"You must provide a script for the analysis. See usage:\r\n\r\n"));
		setScriptFile(new File(value));
		if (!getScriptFile().exists())
			error(new Exception("The script provided:" + value
					+ " does not exist."));

		value = (String) parser.getOptionValue(resultPath, "");
		value = value.isEmpty() ? System.getProperty("user.dir") : value;
		setResultPath(new File(value));
		if (!getResultPath().isDirectory())
			error(new Exception("The path provided to store the results:"
					+ value + " is not a directory. See usage:\r\n\r\n"));
		if (!getResultPath().exists())
			getResultPath().mkdirs();

		setOverWrite((Boolean) parser.getOptionValue(overWriteOpt,
				Boolean.FALSE));
		setSeparateHeader((Boolean) parser.getOptionValue(separateHeaderOpt,
				Boolean.FALSE));
		value = (String) parser.getOptionValue(dataFileName, "");
		value = value.isEmpty() ? "Results.csv" : value + ".csv";
		setDataFile(new File(getResultPath(), value));

		value = (String) parser.getOptionValue(errorFileName, "");
		value = value.isEmpty() ? "Errors.csv" : value + ".csv";
		setErrorFile(new File(getResultPath(), value));

		if (isOverWrite()) {
			if (getDataFile().exists() && !getDataFile().delete())
				error(new Exception("Could not overwrite the data file."));
			if (getErrorFile().exists() && !getErrorFile().delete())
				error(new Exception("Could not overwrite the error file."));
		}

		value = (String) parser.getOptionValue(wellMask, "*");
		setWellMask(new WellMask(value));

		String[] values = parser.getRemainingArgs();
		setImagesPath(new File(value = values[values.length - 1]));

		setMaxBatchSize((Integer) parser.getOptionValue(batchSize, 50));

		int numProc = ((Integer) parser.getOptionValue(numProcessor, 1));
		if (numProc == 1000) {
			Runtime runtime = Runtime.getRuntime();
			numProc = runtime.availableProcessors();
		}
		setNumProcessor(numProc < 1 ? 1 : numProc);

		new AcapellaScheduler();

	}

	AcapellaScheduler() throws Exception {
		File headerFile = new File(getDataFile().getPath().replace(".csv",
				".head"));

		boolean containedHeader = false;
		if (dataFile.exists() && !separateHeader) {
			String header = FileReader.readHeader(dataFile).toString();
			if (header.startsWith("PlateID")) {
				FileWritter.writeFile(headerFile, FileReader
						.readHeader(dataFile));
				containedHeader = true;
			}
		}

		ScriptLoading script = new ScriptLoading(scriptFile, parametersFile);
		File scriptPath = script.makeJobScript();
		script.writeScriptInfo(new File(imagesPath + File.separator
				+ "ScriptInfo.csv"));
		Map<File, Vector<String>> fileSet = FileSeeker.getFilesToAnalyze(
				getImagesPath(), wellMask);

		ResultsExporter resultExporter = new ResultsExporter(getResults(),
				getDataFile(), headerFile);
		resultExporter.setName("resultExporter");

		ErrorExporter errrorExporter = new ErrorExporter(getErrors(),
				getErrorFile());
		errrorExporter.setName("errrorExporter");

		for (File path : fileSet.keySet()) {
			Vector<String> wellIndexList = fileSet.get(path);
			// Do not create more processor than necessary
			numProcessor = Math.min(numProcessor, wellIndexList.size());
			// create a Thread object for each processor
			AcapellaJob[] multiProcessor = new AcapellaJob[numProcessor];

			int numberOfWells = wellIndexList.size();
			int fromIndex = 0, toIndex = 0, processIndex = 0;
			// a max Batch size of 0 indicate not to restrict the size of the
			// batch
			maxBatchSize = maxBatchSize == 0 ? numberOfWells : maxBatchSize;
			int batchSize = Math.min(maxBatchSize, (int) Math
					.ceil((double) numberOfWells / (double) numProcessor));
			while (toIndex < wellIndexList.size()) {

				fromIndex = toIndex;
				toIndex = Math.min(fromIndex + batchSize, numberOfWells);

				String wellIndex = wellIndexList.subList(fromIndex, toIndex)
						.toString();
				// Make a comma delimited list of wellIndex to use in the
				// acapella WellMask
				wellIndex = wellIndex.replaceAll("(?m)^\\[", "").replaceAll(
						"(?m)\\]$", "").replaceAll(", ", ",");

				// only create a new thread if the thread exist and is not
				// running
				if (multiProcessor[processIndex] != null) {
					int loop = 1;
					while (true) {
						if (!multiProcessor[processIndex].isAlive())
							break;
						// loop around the different threads
						processIndex = (processIndex + 1) % numProcessor;
						// sleep after checking all threads for activity
						if (loop % numProcessor == 0) {
							Thread.sleep(3000);
							loop = 1;
						}
						loop++;
					}
				}

				multiProcessor[processIndex] = new AcapellaJob(scriptPath,
						path, wellIndex, results, errors);
				multiProcessor[processIndex].setName("AcapellaJob-"
						+ processIndex);
				multiProcessor[processIndex].setMeasID(path.getName());
				multiProcessor[processIndex].setPlateID(path.getParentFile()
						.getName());
				multiProcessor[processIndex].setImageRootPath(getImagesPath());
				multiProcessor[processIndex].start();

				// loop around the different threads
				processIndex = (processIndex + 1) % numProcessor;
			}
			for (AcapellaJob job : multiProcessor) {
				job.join();
			}
		}
		resultExporter.terminate();
		errrorExporter.terminate();
		if (!isSeparateHeader()) {
			FileWritter.changeFileHeader(dataFile, FileReader
					.readFile(headerFile), containedHeader);
			headerFile.delete();
		}
	}

	private static void error(Exception e) {
		System.err.println(e.getMessage());
		printUsage();
		System.exit(2);
	}

	private static void printUsage() {

		System.err
				.println("Usage: AcapellaScheduler [{-p,--param}] [-s,--script] [{-r,--resultPath}]\r\n"
						+ "\t\t[{-d,--dataFile}] [{-e,--errorFile}] [{-O,--overWrite}]\r\n"
						+ "\t\t[{-m,--wellMask} a String] [{-b,--batchSize} an Integer] [{-t, --threads} an Integer]\r\n"
						+ "\t\t[Path to Your images]\r\n"

						+ "\r\n\t[-s,--script]]: Path to your script."
						+ "\r\n\t[{-r,--resultPath}]: Path in which results should be stored (Optional, default= working directory)."
						+ "\r\n\t[{-d,--dataFile}]: Name for the file containing the data generated by Acapella the extension will be set to \".csv\" (Optional, default= Results.csv)."
						+ "\r\n\t[{-e,--errorFile}]: Name for the file containing the errors generated by Acapella. The extension will be set to \".csv\" (Optional, default= Errors.csv)."
						+ "\r\n\t[{-O,--overWrite}]: If this flag is set, the errors and data files will be over-written if they already exist."
						+ "\r\n\t[{-m,--wellMask}]: Mask used to define the subset of images to analyze within the directory provided. This value can be a comma delimited list of wells, a range of wells. \"*\" wildcard is allowed. (Optional, default= \"*\")."
						+ "\r\n\t[{-b,--batchSize}]: Maximum number of files analyzed by one Acapella thread. Using \"0\" means that the batch size is not limited (Optional, default= 0)."
						+ "\r\n\t[{-h,--separateHeader}]: If this flag is set, the data files will not contain an header. The header will be stored in a separate file. Note that if the -O (overwrite) flag is not set and that you choose to have the header file embeded in the data file, you this method may not be distributed on a cluster correctly."
						+ "\r\n\t[{-t, --threads}]: Maximum Number of threads used to analyze your data. To let the system automatically detect this use \"-t 1000\" (Optional, default= 1)."
						+ "\r\n\t[Path to Your images]: The final argument should be the path to the images you wish to analyze.");
	}

	/**
	 * @return the results
	 */
	public static Map<String, Map<String, String>> getResults() {
		return results;
	}

	/**
	 * @param results
	 *            the results to set
	 */
	public static void setResults(Map<String, Map<String, String>> results) {
		AcapellaScheduler.results = results;
	}

	/**
	 * @return the errors
	 */
	public static Map<String, String> getErrors() {
		return errors;
	}

	/**
	 * @return the overWrite
	 */
	public static boolean isOverWrite() {
		return overWrite;
	}

	/**
	 * @param overWrite
	 *            the overWrite to set
	 */
	public static void setOverWrite(boolean overWrite) {
		AcapellaScheduler.overWrite = overWrite;
	}

	/**
	 * @param errors
	 *            the errors to set
	 */
	public static void setErrors(Map<String, String> errors) {
		AcapellaScheduler.errors = errors;
	}

	/**
	 * @return the separateHeader
	 */
	public static boolean isSeparateHeader() {
		return separateHeader;
	}

	/**
	 * @param separateHeader
	 *            the separateHeader to set
	 */
	public static void setSeparateHeader(boolean separateHeader) {
		AcapellaScheduler.separateHeader = separateHeader;
	}

	/**
	 * @return the scriptFile
	 */
	public static File getScriptFile() {
		return scriptFile;
	}

	/**
	 * @param scriptFile
	 *            the scriptFile to set
	 */
	public static void setScriptFile(File scriptFile) {
		AcapellaScheduler.scriptFile = scriptFile;
	}

	/**
	 * @return the parametersFile
	 */
	public static File getParametersFile() {
		return parametersFile;
	}

	/**
	 * @param parametersFile
	 *            the parametersFile to set
	 */
	public static void setParametersFile(File parametersFile) {
		AcapellaScheduler.parametersFile = parametersFile;
	}

	/**
	 * @return the resultPath
	 */
	public static File getResultPath() {
		return resultPath;
	}

	/**
	 * @param resultPath
	 *            the resultPath to set
	 */
	public static void setResultPath(File resultPath) {
		AcapellaScheduler.resultPath = resultPath;
	}

	/**
	 * @return the dataFile
	 */
	public static File getDataFile() {
		return dataFile;
	}

	/**
	 * @param dataFile
	 *            the dataFile to set
	 */
	public static void setDataFile(File dataFile) {
		AcapellaScheduler.dataFile = dataFile;
	}

	/**
	 * @return the errorFile
	 */
	public static File getErrorFile() {
		return errorFile;
	}

	/**
	 * @param errorFile
	 *            the errorFile to set
	 */
	public static void setErrorFile(File errorFile) {
		AcapellaScheduler.errorFile = errorFile;
	}

	/**
	 * @return the imagesPath
	 */
	public static File getImagesPath() {
		return imagesPath;
	}

	/**
	 * @param imagesPath
	 *            the imagesPath to set
	 */
	public static void setImagesPath(File imagesPath) {
		AcapellaScheduler.imagesPath = imagesPath;
	}

	/**
	 * @return the wellMask
	 */
	public static WellMask getWellMask() {
		return wellMask;
	}

	/**
	 * @param wellMask
	 *            the wellMask to set
	 */
	public static void setWellMask(WellMask wellMask) {
		AcapellaScheduler.wellMask = wellMask;
	}

	/**
	 * @return the maxBatchSize
	 */
	public static int getMaxBatchSize() {
		return maxBatchSize;
	}

	/**
	 * @param maxBatchSize
	 *            the maxBatchSize to set
	 */
	public static void setMaxBatchSize(int maxBatchSize) {
		AcapellaScheduler.maxBatchSize = maxBatchSize;
	}

	/**
	 * @return the numProcessor
	 */
	public static int getNumProcessor() {
		return numProcessor;
	}

	/**
	 * @param numProcessor
	 *            the numProcessor to set
	 */
	public static void setNumProcessor(int numProcessor) {
		AcapellaScheduler.numProcessor = numProcessor;
	}

}
