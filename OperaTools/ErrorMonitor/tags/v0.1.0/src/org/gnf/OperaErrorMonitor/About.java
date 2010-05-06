package org.gnf.OperaErrorMonitor;


import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Dimension;
import java.awt.Frame;
import java.awt.Graphics;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Image;
import java.awt.Insets;
import java.awt.Toolkit;
import java.io.IOException;
import java.io.Serializable;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.HashMap;
import java.util.jar.Attributes;
import java.util.jar.Manifest;

import javax.swing.JDialog;
import javax.swing.JEditorPane;
import javax.swing.JPanel;
import javax.swing.JScrollPane;


public class About extends JDialog {

	HashMap<String, String>   properties       = new HashMap<String, String>(); //  @jve:decl-index=0:
	private String            result           = "";

	private static final long serialVersionUID = 1L;
	private JPanel            jContentPane     = null;
	private JScrollPane       jScrollPane      = null;
	private JEditorPane       notes            = null;
	private ImagePanel        imagePanel       = null;

	/**
	 * @param owner
	 */
	public About(Frame owner) {
		super(owner);
		initialize();
	}

	/**
	 * This method initializes this
	 * 
	 * @return void
	 */
	private void initialize() {
		this.setSize(new Dimension(400, 200));
		this.setPreferredSize(new Dimension(400, 200));
		this.setResizable(false);
		this.setTitle("Program Information.");
		this.setContentPane(getJContentPane());
		this.setIconImage(((Frame) this.getOwner()).getIconImage());
		//this.setDefaultCloseOperation(WindowConstants.DISPOSE_ON_CLOSE);
		center();
		this.setVisible(true);
		this.pack();
		Sleep.delay(5);
		this.dispose();

	}

	/**
	 * This method initializes jContentPane
	 * 
	 * @return javax.swing.JPanel
	 */
	private JPanel getJContentPane() {
		if (jContentPane == null) {
			GridBagConstraints gridBagConstraints = new GridBagConstraints();
			gridBagConstraints.gridx = 0;
			gridBagConstraints.insets = new Insets(5, 5, 5, 0);
			gridBagConstraints.gridy = 0;
			gridBagConstraints.weightx = 1.0;
			gridBagConstraints.anchor = GridBagConstraints.WEST;
	gridBagConstraints.weighty = 1.0;
			GridBagConstraints gridBagConstraints13 = new GridBagConstraints();
			gridBagConstraints13.fill = GridBagConstraints.BOTH;
			gridBagConstraints13.gridy = 0;
			gridBagConstraints13.weightx = 1.0;
			gridBagConstraints13.weighty = 1.0;

			gridBagConstraints13.insets = new Insets(5, 0, 5, 5);
			gridBagConstraints13.gridx = 1;
			jContentPane = new JPanel();
			jContentPane.setLayout(new GridBagLayout());
			jContentPane.add(getImagePanel(), gridBagConstraints);
			jContentPane.add(getNotes(), gridBagConstraints13);
		}
		return jContentPane;
	}

	/**
	 * This method initializes jScrollPane
	 * 
	 * @return javax.swing.JScrollPane
	 */
	private JScrollPane getJScrollPane() {
		if (jScrollPane == null) {
			jScrollPane = new JScrollPane(getNotes());
			jScrollPane
			      .setVerticalScrollBarPolicy(JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
			jScrollPane
			      .setHorizontalScrollBarPolicy(JScrollPane.HORIZONTAL_SCROLLBAR_NEVER);
		}
		return jScrollPane;
	}

	/**
	 * This method initializes notes
	 * 
	 * @return javax.swing.JTextArea
	 */
	private JEditorPane getNotes() {
		if (notes == null) {
			Attributes attr = null;
			try {
				String classContainer = OperaErrorMonitor.class.getProtectionDomain()
				      .getCodeSource().getLocation().toString();
				URL manifestUrl = new URL("jar:" + classContainer
				      + "!/META-INF/MANIFEST.MF");
				Manifest manifest = new Manifest(manifestUrl.openStream());
				attr = manifest.getMainAttributes();
			} catch (MalformedURLException e) {
				// TODO Auto-generated catch block
				//e.printStackTrace();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				//e.printStackTrace();
			}
			// Keep this line in this for to prevent webcrawler issues
			String email = "gb"+"on" +"amy@"+"gnf."+"org";
			
			String programer = attr == null ? "" : attr.getValue("Programed-By");
			String vendor = attr == null ? "" : attr.getValue("Implementation-VendorURL");
			vendor=OperaErrorMonitor.class.getPackage().getImplementationVendor()+" - <a href="+vendor+">"+vendor+"</a>";
			String text = "<H1>"
			      + OperaErrorMonitor.class.getPackage().getImplementationTitle()
			      + "</H1><br />";
			text += "<b>Version: </b>"
			      + OperaErrorMonitor.class.getPackage().getImplementationVersion();
			text += "<br />";
			text += "<b>Written by: </b>"+programer +" - <a href=mailto:" + email + ">"+email+"</a>";
			     
			text += "<br />";
			text += "<b>Developped at: </b>"
			      + vendor;
			;
			notes = new JEditorPane("text/html", text);
			notes.setEditable(false);
			notes.setBackground(jContentPane.getBackground());
		}
		return notes;
	}

	public String getResult() {
		return result;
	}

	/**
	 * This method initializes imagePanel
	 * 
	 * @return javax.swing.JPanel
	 */
	private ImagePanel getImagePanel() {
		if (imagePanel == null) {
			imagePanel = new ImagePanel();
			imagePanel.setLayout(new BorderLayout());
			imagePanel.setSize(new Dimension(80, 80));
			imagePanel.setPreferredSize(new Dimension(80, 80));
			imagePanel.setImage("/resources/icon.png");
			imagePanel.repaint();
		}
		return imagePanel;
	}

	/** Put frame at center of the parent frame. **/
	void center() {
		// And place it in center of screen.
		int lx = (int) ((int) (((Frame) this.getOwner()).getX() + ((Frame) this
		      .getOwner()).getWidth() / 2.0) - (int) (this.getWidth() / 2.0));
		int ly = (int) ((int) (((Frame) this.getOwner()).getY() + ((Frame) this
		      .getOwner()).getHeight() / 2.0) - (this.getHeight() / 2.0));
		this.setLocation(lx, ly);
	}

	// @jve:decl-index=0:visual-constraint="288,110"
	public class ImagePanel extends JPanel implements Serializable {
		/**
	    * 
	    */
		private static final long serialVersionUID = 1644531440473206148L;
		Image                     image            = null;

		public ImagePanel() {
			super();
		}

		public ImagePanel(String imagePath) {
			super();
			this.image = getImage(imagePath);
		}

		private Image getImage(String imagePath) {
			Image img = null;
			URL imageURL = getClass().getResource(imagePath);
			if (imageURL != null) {
				img = Toolkit.getDefaultToolkit().getImage(imageURL);
			}
			return img;
		}

		public ImagePanel(Image image) {
			super();
			this.image = image;
		}

		public void setImage(Image image) {
			this.image = image;
		}

		public void setImage(String imagePath) {
			this.image = getImage(imagePath);
		}

		public Image getImage(Image image) {
			return image;
		}

		public void paintComponent(Graphics g) {
			super.paintComponent(g); //paint background
			if (image != null) { //there is a picture: draw it
				g.drawImage(image, 0, 0, this.getWidth(), this.getHeight(), this);
				//g.drawImage(image, 0, 0, this); //original image size 
			} else {
				Color c = g.getColor();
				g.fillRect(0, 0, this.getWidth(), this.getHeight());
				g.setColor(c);
			}
		} //end paint
	} //end class

	public static class Sleep {

		public static void delay(long s) {
			try {
				Thread.currentThread().sleep(s * 1000);
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
		}
	}
}