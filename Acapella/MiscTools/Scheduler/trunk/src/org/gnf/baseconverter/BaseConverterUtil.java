package org.gnf.baseconverter;

public class BaseConverterUtil {

	private static String baseDigits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	public static String toBase62(int decimalNumber) {
		return fromDecimalToOtherBase(62, decimalNumber);
	}

	public static String toBase36(int decimalNumber) {
		return fromDecimalToOtherBase(36, decimalNumber);
	}

	public static String toBase26(int decimalNumber) {
		baseDigits = baseDigits.replaceFirst("[0-9]{10}", "");
		return fromDecimalToOtherBase(26, decimalNumber);
	}

	public static String toBase16(int decimalNumber) {
		return fromDecimalToOtherBase(16, decimalNumber);
	}

	public static String toBase8(int decimalNumber) {
		return fromDecimalToOtherBase(8, decimalNumber);
	}

	public static String toBase2(int decimalNumber) {
		return fromDecimalToOtherBase(2, decimalNumber);
	}

	public static int fromBase62(String base62Number) {
		return fromOtherBaseToDecimal(62, base62Number);
	}

	public static int fromBase36(String base36Number) {
		return fromOtherBaseToDecimal(36, base36Number);
	}

	public static int fromBase26(String base26Number) {
		baseDigits = baseDigits.replaceFirst("[0-9]{10}", "");
		return fromOtherBaseToDecimal(26, base26Number);
	}

	public static int fromBase16(String base16Number) {
		return fromOtherBaseToDecimal(16, base16Number);
	}

	public static int fromBase8(String base8Number) {
		return fromOtherBaseToDecimal(8, base8Number);
	}

	public static int fromBase2(String base2Number) {
		return fromOtherBaseToDecimal(2, base2Number);
	}

	private static String fromDecimalToOtherBase(int base, int decimalNumber) {
		String tempVal = decimalNumber == 0 ? baseDigits.substring(0, 1) : "";
		int mod = 0;

		while (decimalNumber != 0) {
			mod = decimalNumber % base;
			tempVal = baseDigits.substring(mod, mod + 1) + tempVal;
			decimalNumber = decimalNumber / base;
		}

		return tempVal;
	}

	private static int fromOtherBaseToDecimal(int base, String number) {
		int iterator = number.length();
		int returnValue = 0;
		int multiplier = 1;

		while (iterator > 0) {
			returnValue = returnValue
					+ (baseDigits.indexOf(number.substring(iterator - 1,
							iterator)) * multiplier);
			multiplier = multiplier * base;
			--iterator;
		}
		return returnValue;
	}

	public static String pad(String string, String padding, int size) {
		while (string.length() < size) {
			string = padding + string;
		}
		return string;

	}
}
