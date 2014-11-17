/**
 *
 */
package org.nitd.tools;

import java.io.File;
import java.io.IOException;
import java.io.StringWriter;
import java.nio.charset.Charset;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.Date;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;
import org.xml.sax.SAXException;

import com.sanityinc.jargs.CmdLineParser;
import com.sanityinc.jargs.CmdLineParser.Option;

/**
 * @author BONAMGH1
 *
 */
public class MSRWriter {

	Date date = new Date();

	public MSRWriter() {
	}

	/**
	 * @param args
	 * @throws IOException
	 */
	public static void main(String[] args) throws IOException {

		CmdLineParser parser = new CmdLineParser();

		Option<Boolean> helpArg = parser.addBooleanOption('h', "help");

		Option<String> versionArg = parser.addStringOption('v', "version");

		Option<String> commentArg = parser.addStringOption('c', "comment");

		Option<String> outputPathArg = parser.addStringOption('o',
				"output-path");

		try {
			parser.parse(args);
		} catch (CmdLineParser.OptionException e) {
			error((Exception) e, true);
		}

		boolean help = parser.getOptionValue(helpArg, Boolean.FALSE);

		if (help) {
			printUsage();
			System.exit(0);
		}

		String version = parser.getOptionValue(versionArg, "1");
		String comment = parser.getOptionValue(commentArg, "");
		String outputPathName = parser.getOptionValue(outputPathArg, "");

		File outputPath = null;
		if (!outputPathName.isEmpty()) {
			outputPath = new File(outputPathName);
			if (!outputPath.exists() || !outputPath.canWrite()
					|| !outputPath.isDirectory())
				error(new Exception(
						String.format(
								"The output dir select: '%s' does not exist or is not writable or is not a directory.",
								outputPath)), true);
		}
		comment = !comment.isEmpty() ? "" : String.format("%n%n%s", comment);
		String[] values = parser.getRemainingArgs();
		if (values.length == 0)
			error(new Exception(
					"You must provide at least one paths to the script files you wish to convert."),
					true);

		Finder finder = new Finder("glob", "*.script");
		for (String pathStr : values)
			Files.walkFileTree(Paths.get(pathStr), finder);

		for (Path path : finder.getFiles()) {

			File descriptionFile = new File(path.getParent().toString(),
					"Description.fdt");
			String comment2write = "";
			if (descriptionFile.exists() && descriptionFile.canRead())
				comment2write = getCommentFromFile(descriptionFile);
			comment2write = comment.isEmpty() ? comment2write : String.format(
					"%s%s", comment2write, comment);
			MSRWriter msrWriter = new MSRWriter();
			List<String> test = Files.readAllLines(path,
					Charset.defaultCharset());
			String fileContent = "";
			for (String line : test)
				fileContent += line + "\n";
			String msrContent = msrWriter.xmlData(version, comment2write,
					fileContent);
			String msrFilePath = path.toString();
			if (outputPath != null)
				msrFilePath = (new File(outputPath.getPath(), path.toFile()
						.getName())).toString();
			msrFilePath = msrFilePath.replaceAll("script$", "msr");
			Files.write(Paths.get(msrFilePath), msrContent.getBytes());
			System.err.printf("Done converting: '%s' => '%s'%n",
					path.toString(), msrFilePath);
		}

	}

	private static String getCommentFromFile(File descriptionFile) {
		DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
		String message = "";
		try {
			DocumentBuilder db = dbf.newDocumentBuilder();
			Document doc = db.parse(descriptionFile);
			Element docEle = doc.getDocumentElement();
			NodeList descriptionElements = docEle
					.getElementsByTagName("Description");

			Element element = (Element) descriptionElements.item(0);
			message = element.getTextContent();

		} catch (ParserConfigurationException e1) {
			// TODO Auto-generated catch block
			e1.printStackTrace();
		} catch (SAXException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		return message;
	}

	private String xmlData(String version, String comment, String Script) {
		String xmlString = null;
		String timestamp = String.format("%tF %tT", date, date);
		try {
			// ///////////////////////////
			// Creating an empty XML Document
			// We need a Document
			DocumentBuilderFactory dbfac = DocumentBuilderFactory.newInstance();
			DocumentBuilder docBuilder = dbfac.newDocumentBuilder();
			Document doc = docBuilder.newDocument();

			// //////////////////////
			// Creating the XML tree

			// create the root element and add it to the document
			Element root = doc.createElement("Script");
			root.setAttribute("code_version", version);
			root.setAttribute("date", timestamp);
			doc.appendChild(root);

			// create a comment and put it in the root element
			// Comment comment = doc.createComment("Just a thought");
			// root.appendChild(comment);

			Element child = null;
			Text text = null;
			// create child element with text
			if (!comment.isEmpty()) {
				child = doc.createElement("Description");
				root.appendChild(child);
				// add a text element to the child
				text = doc.createTextNode(comment);
				child.appendChild(text);
			}

			// add another child element with text
			child = doc.createElement("MacroScript");
			root.appendChild(child);
			text = doc.createTextNode(Script);
			child.appendChild(text);

			// ///////////////
			// Output the XML

			// set up a transformer
			TransformerFactory transfac = TransformerFactory.newInstance();
			Transformer trans = transfac.newTransformer();
			trans.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
			trans.setOutputProperty(OutputKeys.INDENT, "yes");
			trans.setOutputProperty(OutputKeys.ENCODING, "UTF-8");
			trans.setOutputProperty(OutputKeys.STANDALONE, "yes");
			trans.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "no");
			trans.setOutputProperty(OutputKeys.METHOD, "xml");
			trans.setOutputProperty(OutputKeys.ENCODING, "ISO-8859-1");
			doc.setXmlStandalone(true);

			// create string from xml tree
			StringWriter sw = new StringWriter();
			StreamResult result = new StreamResult(sw);
			DOMSource source = new DOMSource(doc);

			trans.transform(source, result);
			xmlString = sw.toString();

		} catch (Exception e) {
			System.out.println(e);
		}
		return xmlString;

	}

	private static void error(Exception e, boolean printUsage) {
		System.err.println(e.getMessage());
		if (printUsage)
			printUsage();
		System.exit(2);
	}

	private static void printUsage() {

		System.err
				.println("Usage:"
						+ "\r\nAcapellaScheduler [{-v,--version}] [{-c,--comment}] [Path(s) to script files]\r\n"

						+ "\r\n\t[{-h,--help}]: Print this message"
						+ "\r\n\t[{-v,--version}]: Version of the script file (default= 1)."
						+ "\r\n\t[{-c,--comment}]: Comment that should be added with the file and displayed in the info"
						+ "\r\n\t\tbubble (Optional, default= '')."
						+ "\r\n\t[{-o,--output-path}]: Path where the scripts should be output (default: in-place along .script files)"
						+ "\r\n\t[Path(s) to script file(s)]: Final argument(s) should be the path(s) to the script file(s)"
						+ "\r\n\t\t(*.script), directory(s) containing script file(s), or a global pattern(s).");
	}

}
