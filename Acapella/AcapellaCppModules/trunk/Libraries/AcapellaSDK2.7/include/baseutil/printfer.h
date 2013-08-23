#ifndef x_PKI_CT_BASEUTIL_PRINTFER_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_PRINTFER_H_INCLUDED_

#include "mb_malloc.h"
#include "safestring.h"
#include "heap_dbg_start.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

#if defined(ACAPELLA_PTR64) && defined(ACAPELLA_COMPILER_GCC)
// long and int64 are the same type and cannot be overloaded, ditto for unsigned long and uint64.
#define ACAPELLA_LONG_IS_INT64
#endif

namespace Nbaseutil {

/**
* A class for implemeting type-safe printf()-like functionality.
* Despite the name, it does not actually print anything and resembles thus more the sprintf() family functions.
* The class object is usually constructed as a temporary by using the helper Printf() free function. 
* There are multitude of <tt>operator()</tt> operators
* for filling in the placeholders with data. Because of type-safe interface the caller has not to worry about 
* argument types and sizes, the passed arguments are converted into the type corresponding
* to the placeholder before actual injection.
*
* Argument types supported: int, long, int64, uint64, double, const char*, std::string, void*.
* Supported format fields in format string: cdiouxeEfFgGps, plus all printf modifiers and size specifiers
* except the '*' field (for passing the length or precision by an argument).
* Every printf format field in format string is replaced by the corresponding argument.
* Arguments are converted to format field type. if necessary.
* If conversion is not possible (like numeric<->string), then argument is printed in its "natural" representation.
* If there is not enough format fields, arguments are added to the end of string.
* Numeric arguments are prepended by a space, if the format field is missing or unsuitable.
* 
* This class is mostly used for composing relatively short texts, e.g. for error messages.
* The composed text is stored in an internal buffer. There is a conversion operator to <tt>const char*</tt>,
* which is used for retrieving the formatted text. There is also a <tt>str()</tt> member function for retrieving
* the text as a <tt>safestring</tt>.
*
* Because of the type checks and conversions this class is slower than plain sprintf() call. 
* If the program speed is important, then one should use plain sprintf() or other 
* means instead. This class stresses more the type safety and successful operation in every case, it does not throw
* exceptions by itself, thus the only potential exceptions can come from  memory exhaustion.
*
* Example:
* <pre>
* Nbaseutil::safestring name("example");
* int n=42;
* throw Nbaseutil::Exception(ERR_BADDATA, Nbaseutil::Printf("The item %s has invalid content: %d")(name)(n))
* </pre>
*
* Supported format placeholder fields:
*   - %%c - convert the argument into an integer and output the UTF-8 character corresponding to that code.
*   - %%d %%i %%u - convert the argument into an integer and output in decimal format. 
*   - %%o - convert the argument into an integer and output in octal format.
*   - %%x - convert the argument into an integer and output in hexadecimal format.
*   - %%s - convert the argument into a string and output that string.
*   - %%e - convert the argument into floating-point and output in scientific notation (exponent). The number of fill-up zero digits in the exponent may depend on the compiler version.
*   - %%f - convert the argument into floating-point and output in decimal fractional notation. 
*   - %%g - convert the argument into floating-point and output either as by %%e or %%f format, whichever is shorter.
*   - %%p - the argument should be a pointer; its value is printed out in hexadecimal notation, with "0x" prefix prepended.
*
* Inside the placeholder fields one can insert the length, precision and representation flags as for usual printf() function.
* See e.g. http://msdn2.microsoft.com/en-us/library/56e442dc(VS.71).aspx for more details. The 'n' and '*' format fields are 
* not supported.
*/
class DI_baseutil printfer: public mb_malloced {
public:
	/**
	* Ctor takes a format string s, in usual printf() syntax.
	* @param s The format string containing usual printf style placeholders.
	*/
	printfer(const safestring& s); 
	~printfer();

	/// Fill format fields with data. A NULL pointer is handled as an empty string.
	printfer& operator()(const char* s);
	/// Fill format fields with data. The string may not contain embedded zero bytes.
	printfer& operator()(const std::string& s) { return operator()(s.c_str()); }
	/// Fill format fields with data. The string may not contain embedded zero bytes.
	printfer& operator()(const safestring& s) { return operator()(s.c_str()); }
	/// Fill format fields with data.
	printfer& operator()(int k);
	/// Fill format fields with data.
	printfer& operator()(unsigned int k);

#ifndef ACAPELLA_LONG_IS_INT64
	/// Fill format fields with data.
	printfer& operator()(long k);
	/// Fill format fields with data.
	printfer& operator()(unsigned long k);
#endif

	/// Fill format fields with data.
	printfer& operator()(double x);
	/// Fill format fields with data.
	printfer& operator()(int64 x);
	/// Fill format fields with data.
	printfer& operator()(uint64 x);
	/// Fill format fields with data.
	printfer& operator()(const void* x);

	/// Returns a pointer to the internal result string buffer. Note that the string buffer is destroyed together with the printfer object.
	operator const safestring&() {Finalize(); return buffer_;}		

	/// Returns a reference to the internal result string buffer. Note that the string buffer is destroyed together with the printfer object.
	const safestring& str() {Finalize(); return buffer_;}

	/// Same as <tt>operator const char*()</tt>. Returns a pointer to the internal string buffer. Note that the string buffer is destroyed together with the printfer object.
	const char* c_str() {Finalize(); return buffer_.c_str();}

private: // implementation
	void insert(const safestring& s);
	bool IsTag(const char* Types);
	void FindTag();
	const safestring& Tag();
	void Finalize();
	template<typename T> void Sprintf(T k, const safestring& fmt);
	template<typename T> void PrintInteger(T x);
	template<typename T> void PrintFloating(T x);
private: // data
	safestring buffer_, tag_;
	strlen_t tagpos_;
	char tagtype_;
};

/**
* Helper function for enabling the inline calling syntax when using the printfer class.
* See the printfer class documentation for examples.
*/
inline printfer Printf(const safestring& format) {
	return printfer(format);
}

/**
* Utility function for formatting the time_t value as a local time timestamp.
* This function uses thread-safe SDK functions if possible.
* In case of errors an exception is thrown.
* @param seconds The timestamp in seconds since the epoch. The time_t type is 64-bit in decent implementations.
* @param format The format string to feed into strftime().
* @return The formatted time string.
*/
DI_baseutil safestring FormatLocalTime(time_t seconds, const char* format="%Y-%m-%d %H:%M:%S %Z");

/**
* Utility function for formatting the time_t value as an UTC timestamp.
* This function uses thread-safe SDK functions if possible.
* In case of errors an exception is thrown.
* @param seconds The timestamp in seconds since the epoch. The time_t type is 64-bit in decent implementations.
* @param format The format string to feed into strftime().
* @return The formatted time string.
*/
DI_baseutil safestring FormatUtcTime(time_t seconds, const char* format="%Y-%m-%dT%H:%M:%SZ");

/**
* Extra commands for performing special tasks.
* @return Empty string if command not supported, otherwise the result or "ok".
*/
DI_baseutil safestring BaseUtilCmd(const safestring& command, const safestring& arg1="", int64 arg2=0, void* arg3=NULL);

} // namespace Nbaseutil

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
