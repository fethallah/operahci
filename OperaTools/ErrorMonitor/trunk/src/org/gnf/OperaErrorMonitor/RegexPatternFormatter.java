package org.gnf.OperaErrorMonitor;

import javax.swing.text.DefaultFormatter;

public class RegexPatternFormatter extends DefaultFormatter {
	/**
    * 
    */
   private static final long serialVersionUID = 1176718698913145323L;
	protected java.util.regex.Matcher matcher;

	public RegexPatternFormatter(final java.util.regex.Pattern regex) {
		setOverwriteMode(false);
		matcher = regex.matcher(""); // create a Matcher for the regular expression
	}

	public Object stringToValue(final String string) throws java.text.ParseException {
		if (string == null) return null;
		matcher.reset(string); // set 'string' as the matcher's input

		if (!matcher.matches()) // Does 'string' match the regular expression?
		   throw new java.text.ParseException("Does not match regex", 0);

		// If we get this far, then it did match.
		return super.stringToValue(string); // will honor the 'valueClass' property
	}
}
