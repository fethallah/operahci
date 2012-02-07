package org.gnf.acapella;

import java.io.File;
import java.io.IOException;
import java.net.URISyntaxException;
import java.util.LinkedHashMap;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.gnf.IO.FileReader;
import org.gnf.IO.FileWritter;
import org.w3c.dom.Document;
import org.xml.sax.SAXException;

public class ScriptLoading {

	private final String scriptXMLTags = "MacroScript";
	private final String parametersXMLTags = "Parameter";
	private File scriptFile;
	private File parametersFile;

	private StringBuffer script;
	private StringBuffer parameters;

	public ScriptLoading(File scriptFile) throws IOException {
		this(scriptFile, null);
	}

	public ScriptLoading(File scriptFile, File parametersFile)
			throws IOException {
		super();
		this.scriptFile = scriptFile;
		this.parametersFile = parametersFile;
		getScript();
		if (parametersFile != null)
			getParameters();
		else
			parameters = new StringBuffer();
	}

	public StringBuffer getParameters() throws IOException {
		if (parameters == null) {
			if (parametersFile.getName().endsWith(".mpr"))
				parameters = extractXMLdata(parametersFile);
			else
				parameters = FileReader.readFile(parametersFile);
			parameters.append("\r\n");
		}
		return parameters;
	}

	public void writeScriptInfo(File file) throws IOException {
		if (file.exists() && !file.canWrite())
			throw new IOException("The file: " + file.getPath()
					+ " cannot be written to.");
		String parameters = getParametersValues().toString();
		parameters = parameters.replaceAll("(?m)^\\{", "");
		parameters = parameters.replaceAll("(?m)\\}$", "");
		parameters = parameters.replace(",", "");
		parameters = parameters.replace("=", ",");
		StringBuffer scriptInfo = new StringBuffer();
		scriptInfo.append("Script File," + scriptFile.getPath());
		scriptInfo.append(System.getProperty("line.separator"));
		scriptInfo.append("Script Parameters File," + parametersFile.getPath());
		scriptInfo.append(System.getProperty("line.separator"));
		scriptInfo.append(parameters);
		FileWritter.writeFile(file, scriptInfo);
	}

	public LinkedHashMap<String, Object> getParametersValues() {
		LinkedHashMap<String, Object> parameterList = new LinkedHashMap<String, Object>();
		String text = parameters.toString();
		text = text.replaceAll("(?m)(?s)if\\s*\\(.*?\\)\\s*(.*)else\\s*\\(.*",
				"$1");
		text = text.replaceAll("(?m).*provideinput\\((.*)\\).*", "$1<>");
		text = text.replaceAll("(?m)^\\s*$", "");
		text = text.replaceAll("\\s*,\\s*", ",");
		text = text.replaceAll("\"", "");
		String[] parameters = text.split("<>");
		for (String param : parameters) {
			String[] data = param.split(",");
			if (data.length < 2)
				continue;
			parameterList.put(data[0], data[1]);
		}
		return parameterList;
	}

	public StringBuffer getScript() throws IOException {
		if (script == null) {
			if (scriptFile.getName().endsWith(".msr"))
				script = extractXMLdata(scriptFile);
			else
				script = FileReader.readFile(scriptFile);
			script.append("\r\n");
		}
		return script;
	}

	private StringBuffer extractXMLdata(File file) throws IOException {
		String nodeName = file.getName().endsWith(".msr") ? scriptXMLTags
				: parametersXMLTags;

		StringBuffer result = new StringBuffer();

		try {
			DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
			DocumentBuilder db = dbf.newDocumentBuilder();
			Document doc = db.parse(file);
			doc.getDocumentElement().normalize();

			result.append(doc.getElementsByTagName(nodeName).item(0)
					.getChildNodes().item(0).getNodeValue());

		} catch (ParserConfigurationException e) {
			e.printStackTrace();
		} catch (SAXException e) {
			e.printStackTrace();
		}
		return result;

	}

	public File makeJobScript() throws IOException, URISyntaxException {
		File resourcesLocs = new File(getClass().getProtectionDomain()
				.getCodeSource().getLocation().toURI()).getParentFile();
		resourcesLocs = new File(resourcesLocs, "Resources");
		File prefixFile = new File(resourcesLocs, "ScriptPrefix");
		File sufixFile = new File(resourcesLocs, "ScriptSuffix");
		if (!prefixFile.canRead() || !sufixFile.canRead()) {
			prefixFile = new File(getClass().getResource(
					"/Resources/ScriptPrefix").getPath());
			sufixFile = new File(getClass().getResource(
					"/Resources/ScriptSuffix").getPath());
		}
		StringBuffer prefix = FileReader.readFile(prefixFile);
		StringBuffer suffix = FileReader.readFile(sufixFile);
		script.insert(0, prefix);
		script.insert(0, parameters);
		script.append(suffix);

		File jobScriptFile = File.createTempFile("AcapellaJob", ".script");
		removeComment();

		FileWritter.writeFile(jobScriptFile, script);
		jobScriptFile.deleteOnExit();
		return jobScriptFile;

	}

	private void removeComment() {
		String text = script.toString();
		text = text.replaceAll("(?m)^\\s*", "");
		text = text.replaceAll("(?m)//.*$", "");
		text = text.replaceAll("(?m)\\s*$", "");
		text = text.replaceAll("[\r\n]+", "\r\n");
		script = new StringBuffer(text);
	}

	public static void main(String argv[]) throws IOException,
			URISyntaxException {
		ScriptLoading script = new ScriptLoading(new File(argv[0]), new File(
				argv[1]));
		script.writeScriptInfo(new File("T:\\temp\\parameters"));
		System.out.println(FileReader.readFile(script.makeJobScript()));
	}

}