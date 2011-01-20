/**
 * 
 */

package org.gnf.OperaErrorMonitor;

import java.awt.BorderLayout;
import java.awt.ComponentOrientation;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Image;
import java.awt.Insets;
import java.awt.Panel;
import java.awt.Toolkit;
import java.awt.event.KeyEvent;
import java.io.File;
import java.io.IOException;
import java.net.URL;
import java.text.ParseException;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.TreeSet;
import java.util.Vector;
import java.util.regex.Pattern;

import javax.swing.BorderFactory;
import javax.swing.JButton;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JFormattedTextField;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.JProgressBar;
import javax.swing.JScrollPane;
import javax.swing.JTextArea;
import javax.swing.SwingConstants;
import javax.swing.UIManager;
import javax.swing.WindowConstants;
import javax.swing.text.MaskFormatter;

import org.gnf.OperaErrorMonitor.OperaLogsParsers.DateUtils;
import org.gnf.OperaErrorMonitor.OperaLogsParsers.Sleep;

/**
 * @author gbonamy
 */
public class GUI {

	/**
	 * 
	 */
	private static final long serialVersionUID = 1L;
	// @jve:decl-index=0:
	private static JFrame jFrame = null; // @jve:decl-index=0:visual-constraint="218,62"
	private JPanel mainPannel = null;
	private static JLabel emailListLabel = null;
	private static JFormattedTextField emailListField = null;
	private JPanel jPanel = null;
	private static JLabel statusBar = null;
	private static JButton executeButton = null;
	private Panel statusPane = null;
	private static JProgressBar progressBar = null;
	public static String homeDir = System.getProperty("user.dir"); // @jve:decl-index=0:
	public static String separtor = System.getProperty("file.separator"); // @jve:decl-index=0:
	private JPanel emailPannel = null;
	private JPanel cellPhonePannel = null;
	private JLabel cellPhoneLabel = null;
	private static JFormattedTextField cellPhoneTextField = null;
	private static JTextArea logTextArea = null;

	Thread monitor = null;
	private JLabel cellProviderLabel = null;
	protected static boolean done = true;
	private static JComboBox providerComboBox = null;
	private JScrollPane logScrollPane = null;
	protected static int maxNumNotifications = 5;
	protected static double errorNotificationDelay = 2.5;
	protected static long delay = 120;
	protected static long resendDelay = 30 * 60;
	private JLabel shuttdownLabel = null;
	private static JCheckBox shuttdownCheckBox = null;
	private JPanel operaShuttdownPannel = null;

	public GUI() {
		try {
			UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
			getJFrame();
		} catch (Exception exception) {
			exception.printStackTrace();
			OperaErrorMonitor.error(exception.getMessage(), jFrame);

		}

	}

	/**
	 * This method initializes jFrame
	 * 
	 * @return javax.swing.JFrame
	 * @throws Exception
	 */
	private JFrame getJFrame() throws Exception {

		if (jFrame == null) {
			try {
				jFrame = new JFrame();
				jFrame
						.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
				jFrame.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				jFrame.setResizable(true);
				jFrame.setSize(new Dimension(600, 250));
				jFrame.setMinimumSize(new Dimension(550, 200));
				jFrame.setPreferredSize(new Dimension(600, 250));
				centerFrame(jFrame);
				jFrame.setContentPane(getJPanel());
				jFrame.setTitle("Opera Error Monitor");
				URL imageURL = getClass().getResource("/resources/icon16.png");
				if (imageURL != null) {
					Image img = Toolkit.getDefaultToolkit().getImage(imageURL);
					jFrame.setIconImage(img);
				}
				new About(jFrame);
				jFrame.pack();
				jFrame.setVisible(true);
				jFrame.addWindowListener(new java.awt.event.WindowAdapter() {
					public void windowClosing(java.awt.event.WindowEvent e) {

						int exit = JOptionPane
								.showConfirmDialog(
										jFrame,
										"Are you sure you wish to exit the Opera Monitoring tool?",
										"Quit?", JOptionPane.YES_NO_OPTION);
						if (exit == JOptionPane.YES_OPTION) {
							OperaErrorMonitor.exit("Exit Requested by User.");
						}
					}
				});
			} catch (java.lang.Throwable e) {
				e.printStackTrace();
				throw new Exception("Unable to initialize the window");
			}
		}
		return jFrame;
	}

	/**
	 * This method initializes hitPickPannel
	 * 
	 * @return javax.swing.JPanel
	 */
	private JPanel getMainPannel() {

		if (mainPannel == null) {
			try {
				GridBagConstraints gridBagConstraints41 = new GridBagConstraints();
				gridBagConstraints41.anchor = GridBagConstraints.CENTER;
				gridBagConstraints41.insets = new Insets(5, 5, 5, 5);
				gridBagConstraints41.gridheight = 1;
				gridBagConstraints41.gridwidth = 1;
				gridBagConstraints41.gridx = 1;
				gridBagConstraints41.gridy = 4;
				gridBagConstraints41.weightx = 1.0;
				gridBagConstraints41.fill = GridBagConstraints.HORIZONTAL;
				GridBagConstraints gridBagConstraints21 = new GridBagConstraints();
				gridBagConstraints21.gridx = -1;
				gridBagConstraints21.anchor = GridBagConstraints.CENTER;
				gridBagConstraints21.fill = GridBagConstraints.BOTH;
				gridBagConstraints21.gridy = 4;
				GridBagConstraints gridBagConstraints31 = new GridBagConstraints();
				gridBagConstraints31.fill = GridBagConstraints.BOTH;
				gridBagConstraints31.weighty = 1.0;
				gridBagConstraints31.gridx = 0;
				gridBagConstraints31.gridy = 3;
				gridBagConstraints31.gridwidth = 2;
				gridBagConstraints31.insets = new Insets(5, 5, 5, 5);
				gridBagConstraints31.weightx = 1.0;
				GridBagConstraints gridBagConstraints10 = new GridBagConstraints();
				gridBagConstraints10.fill = GridBagConstraints.BOTH;
				gridBagConstraints10.weighty = 0.0;
				gridBagConstraints10.gridheight = 1;
				gridBagConstraints10.anchor = GridBagConstraints.EAST;
				gridBagConstraints10.weightx = 1.0;
				GridBagConstraints gridBagConstraints91 = new GridBagConstraints();
				gridBagConstraints91.fill = GridBagConstraints.BOTH;
				gridBagConstraints91.weighty = 0.0;
				gridBagConstraints91.weightx = 1.0;
				GridBagConstraints gridBagConstraints51 = new GridBagConstraints();
				gridBagConstraints51.gridx = 2;
				GridBagConstraints gridBagConstraints5 = new GridBagConstraints();
				gridBagConstraints5.gridx = 0; // Generated
				gridBagConstraints5.insets = new Insets(2, 10, 2, 0); // Generated
				gridBagConstraints5.anchor = GridBagConstraints.EAST; // Generated
				gridBagConstraints5.gridy = 1; // Generated
				GridBagConstraints gridBagConstraints1 = new GridBagConstraints();
				gridBagConstraints1.gridx = 0; // Generated
				gridBagConstraints1.gridy = 0; // Generated
				emailListLabel = new JLabel();
				emailListLabel.setText("Email Address:"); // Generated
				emailListLabel.setHorizontalAlignment(SwingConstants.TRAILING);
				emailListLabel
						.setToolTipText("Enter your email address or your GNF user name.");
				emailListLabel.setPreferredSize(new Dimension(84, 20));
				emailListLabel.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				GridBagConstraints gridBagConstraints = new GridBagConstraints();
				gridBagConstraints.fill = GridBagConstraints.VERTICAL; // Generated
				gridBagConstraints.gridy = 0; // Generated
				gridBagConstraints.weightx = 1.0; // Generated
				gridBagConstraints.gridx = 0; // Generated
				mainPannel = new JPanel();
				mainPannel.setBorder(BorderFactory.createLoweredBevelBorder());
				mainPannel.setLayout(new GridBagLayout()); // Generated
				mainPannel.setFont(new Font("Arial", Font.PLAIN, 11)); // Generated
				mainPannel.setPreferredSize(new Dimension(300, 200));
				mainPannel.setMinimumSize(new Dimension(300, 150));
				mainPannel.add(getEmailPannel(), gridBagConstraints91);
				mainPannel.add(getCellPhonePannel(), gridBagConstraints10);
				mainPannel.add(getLogScrollPane(), gridBagConstraints31);
				mainPannel.add(getOperaShuttdownPannel(), gridBagConstraints21);
				mainPannel.add(getExecuteButton(), gridBagConstraints41);
			} catch (java.lang.Throwable e) {
				// TODO: Something
			}
		}
		return mainPannel;
	}

	/**
	 * This method initializes hitListField
	 * 
	 * @return javax.swing.JTextField
	 */
	private JFormattedTextField getEmailListField() {

		if (emailListField == null) {
			try {
				Pattern emailPattern = Pattern.compile("\\w+@\\w+\\.\\w{2,4}");
				emailListField = new JFormattedTextField(
						new RegexPatternFormatter(emailPattern));
				emailListField.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				emailListField.setPreferredSize(new Dimension(80, 20));
				emailListField.setToolTipText("Enter your email address.");
			} catch (java.lang.Throwable e) {
				// TODO: Something
			}
		}
		return emailListField;
	}

	/**
	 * This method initializes jPanel
	 * 
	 * @return javax.swing.JPanel
	 */
	private JPanel getJPanel() {

		if (jPanel == null) {
			try {
				statusBar = new JLabel();
				statusBar.setText("");
				statusBar.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				statusBar.setPreferredSize(new Dimension(9, 25));
				statusBar.setDisplayedMnemonic(KeyEvent.VK_UNDEFINED);
				statusBar.setBorder(BorderFactory.createLoweredBevelBorder());
				jPanel = new JPanel();
				jPanel.setLayout(new BorderLayout()); // Generated
				jPanel.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				jPanel.add(getMainPannel(), java.awt.BorderLayout.CENTER);
				jPanel.add(getStatusPane(), BorderLayout.SOUTH);
			} catch (java.lang.Throwable e) {
				// TODO: Something
			}
		}
		return jPanel;
	}

	/**
	 * This method initializes executeButton
	 * 
	 * @return javax.swing.JButton
	 */
	private JButton getExecuteButton() {

		if (executeButton == null) {
			try {
				executeButton = new JButton();
				executeButton.setEnabled(true);
				executeButton.setSelected(true);
				executeButton.setDefaultCapable(true);
				executeButton.setMnemonic(KeyEvent.VK_ENTER);
				executeButton.setFont(new Font("Arial", Font.PLAIN, 12)); // Generated
				executeButton.setToolTipText("Start/Stop the Monitoring");
				executeButton.setText("Start"); // Generated
				executeButton
						.addActionListener(new java.awt.event.ActionListener() {

							public void actionPerformed(
									java.awt.event.ActionEvent e) {
								if (monitor == null || !monitor.isAlive()) {
									MonitorOpera opMon = new MonitorOpera();
									monitor = new Thread(opMon);
									executeButton.setText("Stop");
									done = false;
									statusBar
											.setText("Monitoring is started. Click stop to terminate monitoring.");
									statusBar.repaint();
									OperaErrorMonitor
											.writeLog(
													"\r\n"
															+ DateUtils
																	.now(OperaLogsParsers.timeFormat)
															+ " - MONITORING IS STARTED",
													logTextArea);
									monitor.start();
									executeButton.setSelected(true);

								} else {
									executeButton.setText("Start");
									monitor.interrupt();
									monitor = null;
									done = true;
									statusBar
											.setText("Monitoring is completed. Click start to restart monitoring.");
									statusBar.repaint();
									OperaErrorMonitor
											.writeLog(
													"\r\n"
															+ DateUtils
																	.now(OperaLogsParsers.timeFormat)
															+ " - MONITORING IS STOPPED",
													logTextArea);
									executeButton.setSelected(true);
								}
							}

						});
			} catch (java.lang.Throwable e) {
				// TODO: Something
			}
		}
		return executeButton;
	}

	/**
	 * This method initializes statusPane
	 * 
	 * @return java.awt.Panel
	 */
	private Panel getStatusPane() {

		if (statusPane == null) {
			try {
				GridBagConstraints gridBagConstraints9 = new GridBagConstraints();
				gridBagConstraints9.gridx = 4;
				gridBagConstraints9.weightx = 1.0D;
				gridBagConstraints9.weighty = 1.0D;
				gridBagConstraints9.gridwidth = 1;
				gridBagConstraints9.ipadx = 0;
				gridBagConstraints9.fill = GridBagConstraints.HORIZONTAL;
				gridBagConstraints9.insets = new Insets(0, 5, 0, 6);
				gridBagConstraints9.gridy = 0;
				GridBagConstraints gridBagConstraints8 = new GridBagConstraints();
				gridBagConstraints8.anchor = GridBagConstraints.CENTER;
				gridBagConstraints8.gridx = 0;
				gridBagConstraints8.gridy = 0;
				gridBagConstraints8.gridwidth = 1;
				gridBagConstraints8.fill = GridBagConstraints.BOTH;
				gridBagConstraints8.weightx = 4.0D;
				gridBagConstraints8.weighty = 1.0D;
				gridBagConstraints8.ipadx = 1;
				gridBagConstraints8.insets = new Insets(0, 0, 0, 0);
				gridBagConstraints8.gridheight = 1;
				GridBagConstraints gridBagConstraints6 = new GridBagConstraints();
				gridBagConstraints6.fill = GridBagConstraints.BOTH; // Generated
				gridBagConstraints6.gridy = -1; // Generated
				gridBagConstraints6.weightx = 1.0; // Generated
				gridBagConstraints6.gridx = -1; // Generated
				GridBagConstraints gridBagConstraints3 = new GridBagConstraints();
				gridBagConstraints3.fill = GridBagConstraints.BOTH; // Generated
				gridBagConstraints3.weightx = 1.0; // Generated
				statusPane = new Panel();
				statusPane.setLayout(new GridBagLayout()); // Generated
				statusPane.setFont(new Font("Arial", Font.PLAIN, 11)); // Generated
				statusPane.setName("StatusPane");
				statusPane.add(statusBar, gridBagConstraints8);
				statusPane.add(getProgressBar(), gridBagConstraints9);
			} catch (java.lang.Throwable e) {
				// TODO: Something
			}
		}
		return statusPane;
	}

	/**
	 * This method initializes progressBar
	 * 
	 * @return javax.swing.JProgressBar
	 */
	private static JProgressBar getProgressBar() {
		if (progressBar == null) {
			progressBar = new JProgressBar();
			progressBar.setPreferredSize(new Dimension(1, 21));
			progressBar.setToolTipText("Percent Progress");
			progressBar.setMinimum(0);
			progressBar.setMaximum(100);
		}
		return progressBar;
	}

	/** Put frame at center of screen. **/
	void centerFrame(JFrame f) {
		// Need the toolkit to get info on system.
		Toolkit tk = Toolkit.getDefaultToolkit();

		// Get the screen dimensions.
		Dimension screen = tk.getScreenSize();

		// Make the frame 1/4th size of screen.
		// int fw = (int) (screen.getWidth ()/4);
		// int fh = (int) (screen.getWidth ()/4);
		// f.setSize (fw,fh);

		// And place it in center of screen.
		int lx = (int) ((screen.getWidth() / 2.0) - (f.getWidth() / 2.0));
		int ly = (int) ((screen.getHeight() / 2.0) - (f.getHeight() / 2.0));
		f.setLocation(lx, ly);
	}

	/**
	 * This method initializes emailPannel
	 * 
	 * @return java.awt.Panel
	 */
	private JPanel getEmailPannel() {
		if (emailPannel == null) {
			GridBagConstraints gridBagConstraints2 = new GridBagConstraints();
			gridBagConstraints2.fill = GridBagConstraints.HORIZONTAL;
			gridBagConstraints2.gridheight = 0;
			gridBagConstraints2.gridwidth = 1;
			gridBagConstraints2.gridx = -1;
			gridBagConstraints2.gridy = -1;
			gridBagConstraints2.ipadx = 0;
			gridBagConstraints2.weightx = 1.0;
			gridBagConstraints2.weighty = 0.0;
			gridBagConstraints2.anchor = GridBagConstraints.WEST;
			gridBagConstraints2.insets = new Insets(5, 0, 5, 5);
			GridBagConstraints gridBagConstraints4 = new GridBagConstraints();
			gridBagConstraints4.anchor = GridBagConstraints.WEST;
			gridBagConstraints4.gridx = -1;
			gridBagConstraints4.gridy = -1;
			gridBagConstraints4.fill = GridBagConstraints.NONE;
			gridBagConstraints4.gridwidth = 1;
			gridBagConstraints4.ipadx = 0;
			gridBagConstraints4.gridheight = 0;
			gridBagConstraints4.insets = new Insets(5, 5, 5, 5);
			emailPannel = new JPanel();
			emailPannel.setLayout(new GridBagLayout());
			emailPannel.add(emailListLabel, gridBagConstraints4);
			emailPannel.add(getEmailListField(), gridBagConstraints2);
		}
		return emailPannel;
	}

	/**
	 * This method initializes cellPhonePannel
	 * 
	 * @return java.awt.Panel
	 */
	private JPanel getCellPhonePannel() {
		if (cellPhonePannel == null) {
			GridBagConstraints gridBagConstraints14 = new GridBagConstraints();
			gridBagConstraints14.fill = GridBagConstraints.HORIZONTAL;
			gridBagConstraints14.gridy = 0;
			gridBagConstraints14.weightx = 1.0;
			gridBagConstraints14.insets = new Insets(5, 0, 5, 5);
			gridBagConstraints14.anchor = GridBagConstraints.WEST;
			gridBagConstraints14.gridheight = 0;
			gridBagConstraints14.gridx = 3;
			GridBagConstraints gridBagConstraints13 = new GridBagConstraints();
			gridBagConstraints13.gridx = 2;
			gridBagConstraints13.anchor = GridBagConstraints.EAST;
			gridBagConstraints13.insets = new Insets(5, 0, 5, 5);
			gridBagConstraints13.gridheight = 0;
			gridBagConstraints13.fill = GridBagConstraints.NONE;
			gridBagConstraints13.gridy = 0;
			cellProviderLabel = new JLabel();
			cellProviderLabel.setText("Provider:");
			cellProviderLabel.setPreferredSize(new Dimension(57, 20));
			cellProviderLabel.setFont(new Font("Arial", Font.PLAIN, 12));
			cellProviderLabel
					.setToolTipText("Select your Cell phone carrier from the list");
			cellProviderLabel.setHorizontalAlignment(SwingConstants.TRAILING);
			GridBagConstraints gridBagConstraints12 = new GridBagConstraints();
			gridBagConstraints12.insets = new Insets(5, 5, 5, 5);
			gridBagConstraints12.fill = GridBagConstraints.HORIZONTAL;
			gridBagConstraints12.gridx = 0;
			gridBagConstraints12.gridy = 0;
			gridBagConstraints12.weightx = 0.0;
			gridBagConstraints12.gridheight = 0;
			gridBagConstraints12.anchor = GridBagConstraints.EAST;
			GridBagConstraints gridBagConstraints7 = new GridBagConstraints();
			gridBagConstraints7.fill = GridBagConstraints.HORIZONTAL;
			gridBagConstraints7.insets = new Insets(5, 0, 5, 5);
			gridBagConstraints7.anchor = GridBagConstraints.WEST;
			gridBagConstraints7.gridheight = 0;
			gridBagConstraints7.weightx = 1.0;
			cellPhoneLabel = new JLabel();
			cellPhoneLabel.setText("Cell Phone#:");
			cellPhoneLabel
					.setToolTipText("Enter your cell phone number ex. (XXX) XXX-XXXX.\nNote: this is optional and you may be charged by your company for SMS.");
			cellPhoneLabel.setHorizontalAlignment(SwingConstants.TRAILING);
			cellPhoneLabel.setHorizontalTextPosition(SwingConstants.TRAILING);
			cellPhoneLabel.setPreferredSize(new Dimension(75, 20));
			cellPhoneLabel.setFont(new Font("Arial", Font.PLAIN, 12));
			cellPhonePannel = new JPanel();
			cellPhonePannel.setLayout(new GridBagLayout());
			cellPhonePannel.add(cellPhoneLabel, gridBagConstraints12);
			cellPhonePannel.add(getCellPhoneTextField(), gridBagConstraints7);
			cellPhonePannel.add(cellProviderLabel, gridBagConstraints13);
			cellPhonePannel.add(getJComboBox(), gridBagConstraints14);
		}
		return cellPhonePannel;
	}

	/**
	 * This method initializes cellPhoneTextField
	 * 
	 * @return java.awt.TextField
	 */
	private JFormattedTextField getCellPhoneTextField() {
		if (cellPhoneTextField == null) {
			try {
				cellPhoneTextField = new JFormattedTextField(new MaskFormatter(
						"(###) ###-####"));

			} catch (ParseException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
			;
			cellPhoneTextField.setPreferredSize(new Dimension(80, 20));
			cellPhoneTextField
					.setToolTipText("Enter your cell phone number ex. (XXX) XXX-XXXX.\r\nNote: this is optional and you may be charged by your company for SMS.");
			cellPhoneTextField.setFont(new Font("Arial", Font.PLAIN, 12));
		}
		return cellPhoneTextField;
	}

	/**
	 * This method initializes logTextArea
	 * 
	 * @return java.awt.TextArea
	 */
	private JTextArea getLogTextArea() {
		if (logTextArea == null) {

			logTextArea = new JTextArea();
			logTextArea.setLineWrap(true);
			logTextArea.setEnabled(true);
			logTextArea
					.setComponentOrientation(ComponentOrientation.LEFT_TO_RIGHT);
			logTextArea.setToolTipText("Application log.");
			logTextArea.setEditable(false);
			logTextArea.setFont(new Font("Courier New", Font.PLAIN, 10));
		}
		return logTextArea;
	}

	/**
	 * This method initializes jComboBox
	 * 
	 * @return javax.swing.JComboBox
	 */
	private JComboBox getJComboBox() {
		if (providerComboBox == null) {
			providerComboBox = new JComboBox((new TreeSet<String>(
					ErrorManagment.getCellProvider().keySet())).toArray());
			providerComboBox.setFont(new Font("Arial", Font.PLAIN, 12));
			providerComboBox
					.setToolTipText("Select your Cell phone carrier from the list");
			providerComboBox.setPreferredSize(new Dimension(80, 20));

		}
		return providerComboBox;
	}

	/**
	 * This method initializes logScrollPane
	 * 
	 * @return javax.swing.JScrollPane
	 */
	private JScrollPane getLogScrollPane() {
		if (logScrollPane == null) {
			logScrollPane = new JScrollPane();
			logScrollPane.setBorder(null);
			logScrollPane
					.setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
			logScrollPane
					.setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
			logScrollPane.setViewportView(getLogTextArea());
		}
		return logScrollPane;
	}

	/**
	 * This method initializes shuttdownCheckBox
	 * 
	 * @return javax.swing.JCheckBox
	 */
	private static JCheckBox getShuttdownCheckBox() {
		if (shuttdownCheckBox == null) {
			shuttdownCheckBox = new JCheckBox();
			shuttdownCheckBox.setSelected(true);
			shuttdownCheckBox
					.setToolTipText("If selected the lasers will be turned off (on the Opera QEHS), the Evoshell will be closed and the user will be logged off. Do not select if you wish to resume a different run afterwards.");
		}
		return shuttdownCheckBox;
	}

	/**
	 * This method initializes operaShuttdownPannel
	 * 
	 * @return javax.swing.JPanel
	 */
	private JPanel getOperaShuttdownPannel() {
		if (operaShuttdownPannel == null) {
			GridBagConstraints gridBagConstraints18 = new GridBagConstraints();
			gridBagConstraints18.gridx = -1;
			gridBagConstraints18.gridy = -1;
			GridBagConstraints gridBagConstraints17 = new GridBagConstraints();
			gridBagConstraints17.gridx = -1;
			gridBagConstraints17.gridy = -1;
			operaShuttdownPannel = new JPanel();
			operaShuttdownPannel.setLayout(new GridBagLayout());
			shuttdownLabel = new JLabel();
			shuttdownLabel.setText("Shutdown Opera when finished?");
			shuttdownLabel
					.setToolTipText("If selected the lasers will be turned off (on the Opera QEHS), the Evoshell will be closed and the user will be logged off. Do not select if you wish to resume a different run afterwards.");
			shuttdownLabel.setFont(new Font("Arial", Font.PLAIN, 12));
			operaShuttdownPannel.add(shuttdownLabel, gridBagConstraints17);
			operaShuttdownPannel.add(getShuttdownCheckBox(),
					gridBagConstraints18);
		}
		return operaShuttdownPannel;
	}

	/**
	 * Application entry point.
	 * 
	 * @param args
	 *            String[]
	 */
	public static void main(String[] args) {

		new GUI();
	}

	public static class MonitorOpera implements Runnable {
		OperaLogsParsers logs = null;
		private Vector<String[]> bernsteinStatus = new Vector<String[]>();
		private Vector<String[]> bernsteinLog = new Vector<String[]>();
		String lastLogEntry = "";
		String lastLogEntryDate = "";
		int lastActivationIndex = 0;
		int lastDeActivationIndex = 0;
		Date firstPlateDate = new Date();
		Date lastPlateDate = new Date();
		Date beforeLastPlateDate = new Date();
		long averagePlateReadTime = 0;
		long currentPlateReadTime = 0;
		int notificationNumber = 0;

		// private long delay = 120;
		// private int maxNumNotifications = 5;

		public MonitorOpera() {
			// TODO Auto-generated constructor stub
		}

		public void run() {

			getProgressBar().setValue(0);
			progressBar.repaint();
			boolean bernsteinReady = false;

			while (!done) {
				long currentDelay = delay;
				try {
					logs = new OperaLogsParsers();
				} catch (IOException e) {
					e.printStackTrace();
					logs = null;
					OperaErrorMonitor.error(e.getMessage(), jFrame);
					done = true;
					statusBar
							.setText("Unable To Start Monitoring. Click start to try monitoring again.");
					statusBar.repaint();
					executeButton.setText("Start");
					executeButton.setSelected(true);
					OperaErrorMonitor.writeLog("\r\n"
							+ DateUtils.now(OperaLogsParsers.timeFormat)
							+ " - " + e.getMessage(), logTextArea);
					OperaErrorMonitor.writeLog("\r\n"
							+ DateUtils.now(OperaLogsParsers.timeFormat)
							+ " - MONITORING IS STOPPED", logTextArea);
					return;
				}
				bernsteinLog = logs.getBernsteinLog();
				bernsteinStatus = logs.getBernsteinStatus();

				if (bernsteinStatus.size() == 0) {
					if (!bernsteinReady) {
						Sleep.delay(currentDelay);
						continue;
					}
					bernsteinReady = false;
					statusBar.setText("Berstein Status: "
							+ DateUtils.now(OperaLogsParsers.timeFormat)
							+ " No Plates to read.");
					statusBar.repaint();
					OperaErrorMonitor
							.writeLog(
									DateUtils.now(OperaLogsParsers.timeFormat)
											+ " - Bernstein not yet initialized. No plates to read.",
									logTextArea);
					Sleep.delay(currentDelay);
					continue;
				}
				if (logs.countDonePlates(bernsteinStatus) == bernsteinStatus
						.size()) {
					if (!bernsteinReady) {
						Sleep.delay(currentDelay);
						continue;
					}
					bernsteinReady = false;
					statusBar.setText("Berstein Status: "
							+ DateUtils.now(OperaLogsParsers.timeFormat)
							+ " Last plate done reading.");
					statusBar.repaint();
					OperaErrorMonitor.writeLog(DateUtils
							.now(OperaLogsParsers.timeFormat)
							+ " - The last plate was successfully read.",
							logTextArea);
					ErrorManagment em = new ErrorManagment();
					String email = emailListField.getText();
					em.setContactInfo(email, cellPhoneTextField.getText()
							.replaceAll("[^0-9]", ""),
							(String) providerComboBox.getSelectedItem());
					if (!email.equals(System.getProperty("user.name")
							+ "@gnf.org"))
						em.setContactInfo(System.getProperty("user.name")
								+ "@gnf.org", null, null);
					String subject = "OPERA SUCCESS";
					String message = "The Opera successfully read the last plate. Remember to shutdown the instrument.";
					String notificationResult = em.sendNotification(subject,
							message, null);

					OperaErrorMonitor.writeLog(notificationResult, logTextArea);

					getProgressBar().setValue(0);
					getProgressBar().repaint();
					if (getShuttdownCheckBox().isSelected())
						InstrumentShutdown.InstrumentShutdown();
					continue;
				}
				if (bernsteinStatus.size() > 0) {
					getProgressBar().setValue(
							100 * logs.countDonePlates(bernsteinStatus)
									/ bernsteinStatus.size());
					// getProgressBar().repaint();
				}
				if (!(bernsteinLog.get(bernsteinLog.size() - 1)[1]
						.equals(lastLogEntry) && bernsteinLog.get(bernsteinLog
						.size() - 1)[0].equals(lastLogEntryDate))
						|| lastLogEntry.equals("")) {
					lastLogEntry = bernsteinLog.get(bernsteinLog.size() - 1)[1];
					lastLogEntryDate = bernsteinLog
							.get(bernsteinLog.size() - 1)[0];
					statusBar.setText("Berstein Status: " + lastLogEntryDate
							+ " " + lastLogEntry);
					statusBar.repaint();
					if (averagePlateReadTime != 0 && currentPlateReadTime != 0) {
						OperaErrorMonitor.writeLog(DateUtils
								.now(OperaLogsParsers.timeFormat)
												+ " - PlateReadTime="
												+ Math
														.round(currentPlateReadTime / 100.0 / 60.0)
												/ 10
												+ " - AverageReadTime="
												+ Math
														.round(averagePlateReadTime / 100.0 / 60.0)
												/ 10 + " minutes.", logTextArea);
						averagePlateReadTime = 0;
						currentPlateReadTime = 0;
					}
					OperaErrorMonitor.writeLog(DateUtils
							.now(OperaLogsParsers.timeFormat)
							+ " - Last log Entry: "
							+ lastLogEntryDate
							+ " - "
							+ lastLogEntry, logTextArea);

				}
				if (lastLogEntry.equals("BERNSTEIN-TERMINATION")
						|| lastLogEntry.equals("DEACTIVATION")
						|| bernsteinStatus.size() == 0) {
					Sleep.delay(currentDelay);
					continue;
				}
				lastActivationIndex = logs.getLastActivationIndex(bernsteinLog);
				lastDeActivationIndex = logs
						.getLastDeActivationIndex(bernsteinLog);
				if (lastActivationIndex < lastDeActivationIndex) {
					Sleep.delay(currentDelay);
					continue;
				}

				bernsteinReady = true;

				if (lastActivationIndex < bernsteinLog.size() - 2) {
					// Make sure that at least 1 plate went through the system
					statusBar.setText("Berstein Status: " + lastLogEntry);
					statusBar.repaint();
					SimpleDateFormat sdf = new SimpleDateFormat(
							OperaLogsParsers.timeFormat);

					try {
						firstPlateDate = sdf.parse(bernsteinLog
								.get(lastActivationIndex + 1)[0]);
						beforeLastPlateDate = sdf.parse(bernsteinLog
								.get(bernsteinLog.size() - 2)[0]);
						lastPlateDate = sdf.parse(bernsteinLog.get(bernsteinLog
								.size() - 1)[0]);
						averagePlateReadTime = DateUtils.difference(
								lastPlateDate, firstPlateDate)
								/ (bernsteinLog.size()
										- (lastActivationIndex + 1) - 1);
						currentPlateReadTime = DateUtils.difference(
								lastPlateDate, beforeLastPlateDate);
						statusBar.setText(statusBar.getText()
								+ ". Average read time=" + averagePlateReadTime
								/ 1000 / 60 + " minutes.");

						double foldAboveRegularReadTime = (double) Math
								.round(((double) DateUtils.difference(DateUtils
										.now(), lastPlateDate) / (double) averagePlateReadTime) * 10.0) / 10.0;

						if (foldAboveRegularReadTime < errorNotificationDelay) {
							if (notificationNumber <= maxNumNotifications) {
								notificationNumber = 0;
								statusBar.repaint();
								currentDelay = delay;
							}
						} else if (notificationNumber <= maxNumNotifications) {
							OperaErrorMonitor.writeLog(DateUtils
									.now(OperaLogsParsers.timeFormat)
									+ " - Error: plate read time "
									+ foldAboveRegularReadTime
									+ " times above average (Read time="
									+ currentPlateReadTime
									/ 1000
									/ 60
									+ " minutes).", logTextArea);

							ErrorManagment em = new ErrorManagment();

							if (notificationNumber == 2)
								try {
									em.setContactInfo("managers");
								} catch (Exception e) {
									e.printStackTrace();
									OperaErrorMonitor.writeLog(e.getMessage(),
											null);
								}
							if (notificationNumber > 1)
								try {
									em.setContactInfo("operators");
								} catch (Exception e) {
									e.printStackTrace();
									OperaErrorMonitor.writeLog(e.getMessage(),
											null);
								}
							String subject = "OPERA ERROR - INSTRUMENT IDLE SINCE "
									+ lastPlateDate;
							String message = "The instrument has been idle "
									+ foldAboveRegularReadTime
									+ " times longer than average this strongly suggests that the instrument has crashed.\r\nAttached are the log files for the Bernstein software.";
							File[] attachments = { logs.getBernsteinLogFile(),
									logs.getBernsteinStatusFile(),
									logs.getBernsteinHostLinkReaderFile(),
									logs.getBernsteinHostLinkRobFile() };
							String notificationResult = "";
							if (notificationNumber > 0) {

								String email = emailListField.getText();
								em.setContactInfo(email, cellPhoneTextField
										.getText().replaceAll("[^0-9]", ""),
										(String) providerComboBox
												.getSelectedItem());
								if (!email.equals(System
										.getProperty("user.name")
										+ "@gnf.org"))
									em.setContactInfo(System
											.getProperty("user.name")
											+ "@gnf.org", null, null);
								notificationResult = em.sendNotification(
										subject, message, attachments);
								currentDelay = resendDelay; // set wait time
								// before re-sending
								// notification
								// before
								// proceeding.
							} else {
								try {
									em.setContactInfo("managers");
								} catch (Exception e1) {
									e1.printStackTrace();
									OperaErrorMonitor.writeLog(e1.getMessage(),
											null);
								}

								try {
									notificationResult = em
											.rebootAnalyzers("Analysis taking longer than usual. Current read times is "
													+ foldAboveRegularReadTime
													+ " times above average ("
													+ currentPlateReadTime
													/ 1000
													/ 60
													+ " minutes). Possible faulty connection");
								} catch (Exception e) {
									notificationResult += e.getMessage();
									e.printStackTrace();
								}
								currentDelay = 180; // wait 3 minutes before
								// trying again
							}
							OperaErrorMonitor.writeLog(notificationResult,
									logTextArea);
							notificationNumber++;
						}

					} catch (ParseException e) {
						e.printStackTrace();
						OperaErrorMonitor.writeLog(e.getMessage(), null);
					}
				}

				Sleep.delay(currentDelay);
			}

		}
	}

	public void setErrorNotificationDelay(double s) {
		errorNotificationDelay = s;
	}

	public void setDelay(long s) {
		delay = s;
	}

	public void setResendDelay(long fold) {
		resendDelay = fold;
	}

	public void startStop() {
		executeButton.doClick();

	}

	public void setMaxNumNotifications(int num) {
		maxNumNotifications = num;
	}
}
