package org.gnf.OperaErrorMonitor;

import java.util.*;
import java.io.*;

import javax.mail.Authenticator;
import javax.mail.MessagingException;
import javax.mail.Multipart;
import javax.mail.PasswordAuthentication;
import javax.mail.Session;
import javax.mail.Transport;
import javax.mail.internet.InternetAddress;
import javax.mail.internet.MimeBodyPart;
import javax.mail.internet.MimeMessage;
import javax.mail.internet.MimeMultipart;

import org.ini4j.Ini;
import org.ini4j.InvalidFileFormatException;

public class EMail {

	private static File         iniFile        = null;
	private final String        iniTag         = "emailParameters";
	private static final String SSL_FACTORY    = "javax.net.ssl.SSLSocketFactory";
	private boolean             debug          = true;
	private String              to;
	private String              from;
	private String              subject        = "";
	private String              message        = "";
	private File[]              attachments    = null;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		if (args.length < 4 || args.length > 6) {
			System.out
			      .println("usage: java sendfile <to> <from> <subject> <message> <debug:[true|false]> <iniFile> ");
			System.exit(1);
		}
		EMail mail = new EMail(args[0], args[1], args[2], args[3], null);
		if (args.length > 4 && args[4] != null)
		   mail.setDebug(Boolean.valueOf(args[4]));
		if (args.length > 5 && args[5] != null)
		   EMail.setIniFile(new File(args[5]));
		try {
			mail.send();
		} catch (MessagingException mex) {
			mex.printStackTrace();
			Exception ex = null;
			if ((ex = mex.getNextException()) != null) {
				ex.printStackTrace();
			}
		} catch (IOException ioex) {
			ioex.printStackTrace();
		} catch (Exception e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

	}

	private Properties getConnectionProperties() throws Exception {
		Ini ini;
		Properties props = new Properties();
		try {
			ini = new Ini(iniFile);
			if (ini.get(iniTag).get("senderEmail") != null
			      && !ini.get(iniTag).get("senderEmail").isEmpty())
			   props.put("from", ini.get(iniTag).get("senderEmail"));
			if (ini.get(iniTag).get("smtp") == null
			      || ini.get(iniTag).get("smtp").isEmpty())
			   throw new Exception("Smtp host server adress is not defined");
			props.put("mail.smtp.host", ini.get(iniTag).get("smtp"));

			if (ini.get(iniTag).get("port") == null
			      || ini.get(iniTag).get("port").isEmpty())
				props.put("mail.smtp.port", 25);
			else props.put("mail.smtp.port", Integer.valueOf(ini.get(iniTag).get(
			      "port")));
			props.put("mail.smtp.socketFactory.port", props.get("mail.smtp.port"));
			props.put("user", ini.get(iniTag).get("user"));
			if (props.get("user") == null) props.put("user", "");
			props.put("pwd", ini.get(iniTag).get("pwd"));
			if (props.get("pwd") == null) props.put("pwd", "");
			if (ini.get(iniTag).get("useSSL") == null)
				props.put("useSSL", false);
			else props.put("useSSL", Boolean
			      .valueOf(ini.get(iniTag).get("useSSL")));
			if (ini.get(iniTag).get("debug") == null)
				props.put("debug", false);
			else props.put("mail.debug", Boolean.valueOf(ini.get(iniTag).get(
			      "debug")));
			if (ini.get(iniTag).get("useSSL") != null
			      && ini.get(iniTag).get("useSSL").equals("true")) {
				props.put("mail.smtp.socketFactory.fallback", false);
				props.put("mail.smtp.ssl.enable", true);
				props.put("mail.smtp.socketFactory.class", SSL_FACTORY);
			}

		} catch (InvalidFileFormatException e) {
			e.printStackTrace();
			throw new Exception(
			      "The parameter file OperaResourceMonitor.ini is invalid make sure that a group [parameters] exists and that it contains the key \"smtp\" pointing to a valid smtp server.");
		} catch (IOException e) {
			e.printStackTrace();

			throw new Exception(
			      "The parameter file you provided does not exists. Make sure that OperaResourceMonitor.ini is present in the same directory as this application");

		}
		return props;
	}

	public EMail(String to, String from, String subject, String message,
	      File[] attachments) {
		super();
		this.to = to;
		this.from = from;
		this.subject = subject;
		this.message = message;
		this.attachments = attachments;
	}

	public void send() throws Exception {
		final Properties props = System.getProperties();
		Authenticator aut = null;
		iniFile = ErrorManagment.INI_FILE;
		props.putAll(getConnectionProperties());
		if (!((String) props.get("user")).isEmpty()) {
			props.put("mail.smtp.auth", "true");
			aut = new Authenticator() {
				String user = props.getProperty("user");
				String pwd  = props.getProperty("pwd");

				@Override
				protected PasswordAuthentication getPasswordAuthentication() {
					return new PasswordAuthentication(user, pwd);
				}
			};
		}

		Session session = Session.getDefaultInstance(props, aut);
		session.setDebug((Boolean) props.get("mail.debug"));

		// create a message
		MimeMessage msg = new MimeMessage(session);
		from = props.get("from") != null ? (String) props.get("from") : from;
		msg.setFrom(new InternetAddress(from));
		InternetAddress[] address = {new InternetAddress(to)};
		msg.setRecipients(javax.mail.Message.RecipientType.TO, address);
		msg.setSubject(subject);

		// create and fill the first message part
		MimeBodyPart mbp1 = new MimeBodyPart();
		mbp1.setText(message);
		// create the Multipart and add its parts to it
		Multipart mp = new MimeMultipart();
		mp.addBodyPart(mbp1);

		// attach the file to the message
		if (attachments != null) {
			for (File attachment : attachments) {
				// create the second message part
				MimeBodyPart mbp2 = new MimeBodyPart();
				if (attachment.canRead()) {

					mbp2.attachFile(attachment);
					/*
					 * Use the following approach instead of the above line if you
					 * want to control the MIME type of the attached file. Normally
					 * you should never need to do this.
					 * 
					 * FileDataSource fds = new FileDataSource(filename) { public
					 * String getContentType() { return "application/octet-stream"; }
					 * }; mbp2.setDataHandler(new DataHandler(fds));
					 * mbp2.setFileName(fds.getName());
					 */

					mp.addBodyPart(mbp2);
				} else {
					System.err.print("The file you wish to attach: \""
					      + attachment.getPath()
					      + "\" does not exist or cannot be read.");
				}
			}
		}
		// add the Multipart to the message
		msg.setContent(mp);

		// set the Date: header
		msg.setSentDate(new Date());

		/*
		 * If you want to control the Content-Transfer-Encoding of the attached
		 * file, do the following. Normally you should never need to do this.
		 * 
		 * msg.saveChanges(); mbp2.setHeader("Content-Transfer-Encoding",
		 * "base64");
		 */

		// send the message
		Transport.send(msg);

	}

	public void setDebug(boolean debug) {
		this.debug = debug;
	}

	public static void setIniFile(File iniFile) {
		EMail.iniFile = iniFile;
	}

}
