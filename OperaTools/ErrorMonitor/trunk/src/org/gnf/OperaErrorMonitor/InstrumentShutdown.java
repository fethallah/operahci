/**
 * 
 */
package org.gnf.OperaErrorMonitor;

import java.io.IOException;

/**
 * @author gbonamy
 * 
 */
public class InstrumentShutdown {

	/**
	 * 
	 */
	public static void InstrumentShutdown() {

		Process process = null;

		try {
			process = Runtime.getRuntime()
					.exec("./Resources/OperaShutdown.exe");
			process.waitFor();
			System.exit(0);
		} catch (final InterruptedException e) {
			System.exit(0);
			e.printStackTrace();
		} catch (final IOException e) {
			System.exit(0);
			e.printStackTrace();
		} finally {
			process.destroy();
		}
	}

}
