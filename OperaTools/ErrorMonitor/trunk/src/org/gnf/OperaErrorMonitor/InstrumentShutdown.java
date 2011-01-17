/**
 * 
 */
package org.gnf.OperaErrorMonitor;

import java.io.File;
import java.io.IOException;

import org.gnf.OperaErrorMonitor.OperaLogsParsers.Sleep;

/**
 * @author gbonamy
 * 
 */
public class InstrumentShutdown {

	/**
	 * 
	 */
	private static final File SHUTDOWN_SOFT = new File(System
			.getProperty("user.dir"), "Resources/OperaShutdown.exe");

	public static void InstrumentShutdown() {

		Process process = null;

		try {
			Sleep.delay(10);
			process = Runtime.getRuntime().exec(SHUTDOWN_SOFT.getPath());
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
