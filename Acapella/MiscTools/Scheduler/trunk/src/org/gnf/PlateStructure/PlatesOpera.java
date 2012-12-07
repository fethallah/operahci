package org.gnf.PlateStructure;

import java.io.File;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

public class PlatesOpera extends Plates {
	private static final String PATTERN = "(?:\\d{1,3}_)?(\\d{3})(\\d{3})\\d{3}\\.(?:tiff?|flex)$";

	public PlatesOpera(File path) throws Exception {
		super(path);

	}

	@Override
	protected String getPattern() {
		return PATTERN;
	}

	@Override
	protected String getWellIndex(File file) {
		Pattern pattern = Pattern.compile(PATTERN);
		Matcher matcher = pattern.matcher(file.getName());
		if (matcher.find()) {
			String wellID = String.format("%03d%03d000", Integer.valueOf(matcher.group(1)),
					Integer.valueOf(matcher.group(2)));
			return wellID;
		} else
			return null;
	}

}
