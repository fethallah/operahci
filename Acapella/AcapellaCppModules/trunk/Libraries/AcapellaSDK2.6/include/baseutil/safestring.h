#ifndef x_PKI_CT_BASEUTIL_SAFESTRING_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_SAFESTRING_H_INCLUDED_

#ifdef _MSC_VER
// #pragma warning(disable:4996)
#endif

#include <string.h> // for strchr()
#include <string>
#include <sstream>
#include <limits>
#include <memory>
#include <ctype.h>	// for tolower()
#ifdef _MSC_VER
#	include <float.h> // for _finite() and _isnan()
#else
#include <stdint.h> // for int64_t
#endif

#include "config.h"
#include "gcc.h"
#include "mb_malloc.h"

// Windows evil min/max macro guards
#ifdef min
#define ACAPELLA_EVIL_MIN_WAS_DEFINED
#undef min
#endif
#ifdef max
#define ACAPELLA_EVIL_MAX_WAS_DEFINED
#undef max
#endif

// Define a string class (safestring) whose exemplars can be safely interchanged inbetween 
// different DLLs.

// Also define a number of utility functions for this string class.

// Also define the safestringlist class, corresponding iterators and functions.

namespace Nbaseutil {


#ifdef _MSC_VER
#	pragma warning (disable:4097) // disable MSVC strange warning at warning level 4.
#endif

#ifdef ACAPELLA_COMPILER_MSVC
	/// Unsigned 64-bit integer type
	typedef unsigned __int64 uint64;
	/// Signed 64-bit integer type
	typedef signed __int64 int64;
#endif

#ifdef ACAPELLA_COMPILER_GCC
	/// Unsigned 64-bit integer type
	typedef uint64_t uint64;
	/// Signed 64-bit integer type
	typedef int64_t int64;
#endif

#if defined(_WIN64) || defined(_LP64)
// ACAPELLA_PTR64 is defined if pointers are 64-bit.
#define ACAPELLA_PTR64
#else
#define ACAPELLA_PTR32
#endif

#ifdef ACAPELLA_PTR64
	/// An integral type capable of holding a data pointer.
	typedef uint64 iptr_t;
#else
	/// An integral type capable of holding a data pointer.
	typedef unsigned int iptr_t;
#endif


	class safestringlist;

////////////////////////////////////////////////////
////////////////  safestring class /////////////////
////////////////////////////////////////////////////

/// Special allocator for the safestring class; always use the first loaded baseutil DLL heap.
#ifndef _MSC_VER
	// gcc branch
	// no need for specific allocator in unix
	// Change 07.11.2005: use "template typedef" trick in order to get the thing compile under gcc4.
	typedef std::allocator<char> mb_allocator;
	template <class T> class mb_allocator_typedef {
	public:
		typedef std::allocator<T> allocator;
	};
#else
#	if _MSC_VER>1200
	// .NET branch

	/**
	* Memory allocator for overcoming the multiple-heap problem of Windows DLL-s. 
	* This should be used as the allocator for all STL containers that are passed over DLL borders.
	*
	* Do not use this class directly, use mb_allocator_typedef<T>::allocator instead. This ensures that the code
	* can be compiled under Linux gcc4 as well.
	* Example:
	* <pre>
	* 	std::vector<Nbaseutil::safestring, Nbaseutil::mb_allocator_typedef<Nbaseutil::safestring>::allocator> my_vector;
	* </pre>
	*/
	template<class T>
	class mb_allocator_tmpl: public std::allocator<T> {
	public:
		// Provide allocator typedefs
		typedef typename std::allocator<T>::pointer pointer;
		typedef typename std::allocator<T>::size_type size_type;
		typedef typename std::allocator<T>::value_type value_type;
		typedef typename std::allocator<T>::difference_type difference_type;
		typedef typename std::allocator<T>::const_pointer const_pointer;
		typedef typename std::allocator<T>::reference reference;
		typedef typename std::allocator<T>::const_reference const_reference;

		// 06.04.2004: provide proper rebind mechanism, .NET uses default std::allocator otherwise.
		template <class U>
		struct rebind {typedef typename mb_allocator_tmpl<U> other; };

		pointer allocate(size_type n) {
			// allocate array of _Count elements
			return (pointer) mb_malloc(n*sizeof(T));
		}
		pointer allocate(size_type n, const void _FARQ *) {
			// allocate array of _Count elements, ignore hint
			return (pointer) mb_malloc(n*sizeof(T));
		}
		void deallocate(pointer p, size_type n) { mb_free(p); }

		// required ctors and assignment ops
		mb_allocator_tmpl() {}
		mb_allocator_tmpl(const mb_allocator_tmpl& b): std::allocator<T>(b) {}
		template <class U> mb_allocator_tmpl(const  mb_allocator_tmpl<U>& b): std::allocator<T>(b) {}
		mb_allocator_tmpl<T>& operator=(const mb_allocator_tmpl& b) { std::allocator<T>::operator =(b); return *this; }
		template <class U> mb_allocator_tmpl& operator=(const  mb_allocator_tmpl<U>& b) { std::allocator<T>::operator =(b); return *this; }
	};

	/**
	* Use "template typedef" trick in order to get the allocator compile under gcc4.
	* The 'allocator' member is just a typedef for mb_allocator_tmpl<T>.
	* Example:
	* <pre>
	* 	std::vector<Nbaseutil::safestring, Nbaseutil::mb_allocator_typedef<Nbaseutil::safestring>::allocator> my_vector;
	* </pre>
	*/
	template <class T> class mb_allocator_typedef {
	public:
		typedef mb_allocator_tmpl<T> allocator;
	};

#	else
	// MSVC++ 6.0 branch
	class mb_allocator: public std::allocator<char> {
	public:
		pointer allocate(size_type n, const void *hint=NULL) {return (char*) mb_malloc(n);}
		void deallocate(pointer p, size_type n) { mb_free(p); }
	};
	template<class T>
	class mb_allocator_tmpl: public std::allocator<T> {
	public:
		pointer allocate(size_type n, const void *hint=NULL) {return (T*) mb_malloc(n*sizeof(T));}
		void deallocate(void* p, size_type n) { mb_free(p); }
		char *_Charalloc(size_type n) {return (char*) mb_malloc(n);}
	};
#	endif
#endif

/// Portable typedef for standard char_traits
#if defined(__GNUC__) && __GNUC__ < 3
	typedef std::string_char_traits<char> std_char_traits;
#else
	typedef std::char_traits<char> std_char_traits;
#endif

/// Case-insensitive (C-locale only!) char traits for safestring:
struct safestring_traits: public std_char_traits {
    static bool eq(const char_type& x, const char_type& y) {
		return x==y || (x>='A' && x<='Z' && x+32==y) || (y>='A' && y<='Z' && y+32==x);
	}
    static bool lt(const char_type& x, const char_type& y) {
		char_type x1 = x>='a' && x<='z' ? x-32: x;
		char_type y1 = y>='a' && y<='z' ? y-32: y;
		return x1<y1;
	}
    static int compare(const char_type *x, const char_type *y, size_t n) {
		for (const char_type* xend=x+n; x!=xend; ++x, ++y) {
			char_type x1 = *x>='a' && *x<='z' ? *x-32: *x;
			char_type y1 = *y>='a' && *y<='z' ? *y-32: *y;
			if (x1<y1) return -1; 
			else if (x1>y1) return 1;
		}
		return 0;
	}
    static const char_type *find(const char_type *x, size_t n, const char_type& y) {
		for (; 0 < n; --n, ++x) {
			if (eq(*x, y)) return x;
		}
		return NULL;
	}
    static bool eq_int_type(const int_type& ch1, const int_type& ch2) {
		if (ch1==eof() && ch2!=eof()) return false;
		if (ch2==eof() && ch1!=eof()) return false;
		return eq( to_char_type(ch1), to_char_type(ch2));
	}
};

/// safestring string class: comparison is case-insensitive (C-locale always), memory allocation always in heap of baseutil dll.
typedef std::basic_string<char, safestring_traits, mb_allocator_typedef<char>::allocator> safestring;

/// A shorter typedef for safestring::size_type
// Note: if you get VC++ warning C4996 related to the next line, then you should 
// #define _SCL_SECURE_NO_WARNINGS 
// before including STL headers in your code!
typedef safestring::size_type strlen_t;


/** 
* Compare safestring with an ASCIIZ string.
* This is several times faster than the default implementation via safestring_traits,
* because of better inlining, early returns and avoiding strlen() call.
* You will need 'using namespace Nbaseutil;' for this operator to be visible without Koenig lookup.
*/
inline bool operator==(const safestring& a, const char* b) {
	safestring::size_type n = a.length();
	// Do not compare with the strlen(b) here; strlen will scan the whole b string
	// unconditionally, whereas the scan below stops on first nonmatching char.
	// If most comparisons return false, this is a win. Tested by timings in MSVC++ 6.0 release mode.
	const char* s = a.data();
	// Do not use pointers as loop variables; it appears MSVC++ 6.0 can optimize the int loop better.
	for (safestring::size_type i=0; i<n; ++i) {
		char x = s[i]; 
		if (x==0) return false; // safestring with embedded 0 cannot equal ASCIIZ string.
		char y = b[i];
		if (x==y) continue;
		// Check for case-insensitive match:
		if (x>='A' && x<='Z') x+=32;
		if (y>='A' && y<='Z') y+=32;
		if (x!=y) return false; 
	}
	return b[n]==0;
}

/// Exact opposite of operator==(safestring, const char*).
inline bool operator!=(const safestring& a, const char* b) {
	return !(a==b);
}


////////////////////////////////////////////////////////////
//////////////  Utility functions  /////////////////////////
////////////////////////////////////////////////////////////

/////////////// Simple predicates

/// Check for a whitespace character (space, tab, CR, LF).
inline bool is_whitespace(char c) {
	return c==' ' || c=='\t' || c=='\r' || c=='\n';
}

/// Check if the character is an ASCII letter, digit, underscore or '@'.
inline bool is_alphanumeric(char c) {
	// Enh 28.01.2004: allow @ sign as a unit separator in variable names.
	return ((c>='a' && c<='z') || (c>='@' && c<='Z') || (c>='0' && c<='9') || c=='_');
}

/// Check if the string contains only ASCII letters, digits, underscores and '@' and does not begin with a digit.
DI_baseutil bool is_alphanumeric(const safestring& s);

/**
*  Returns true if the string conforms to the syntax of data item name in Acapella syntax. 
*  @param s The checked string.
*  @param extras List of extra characters considered to be allowed in the string s.
*  @param SkipBrackets Ignore substrings in brackets [].
*/
DI_baseutil bool is_alphanumeric(const safestring& s, const char* extras, bool SkipBrackets);

/// Return true if string is an Acapella predefined positive boolean constant; otherwise return false.
DI_baseutil bool is_positive(const safestring& s);

/// Return true if string is an Acapella negative predefined boolean constant.
DI_baseutil bool is_negative(const safestring& s);

/// Return true if string is an Acapella predefined boolean constant.
inline bool is_boolean(const safestring& s) {
	return is_positive(s) || is_negative(s);
}

/// Return true is string is a decimal or hexadecimal integer number or a floating-point number. In floating-point numbers, accepts both a dot and current locale decimal point character.
DI_baseutil bool is_numeric(const safestring& s);

/// Return true is string is a decimal or hexadecimal integer number or a floating-point number, or an Acapella predefined numeric constant. In floating-point numbers, accepts both a dot and current locale decimal point character.
DI_baseutil bool is_numeric_acapella(const safestring& s);

/// Convert two-digit hexadecimal value kl into corresponding number.
DI_baseutil int Hexa2Byte(unsigned char k, unsigned char l);


////////////// Conversion from/to numbers

/// Convert integer into a safestring, in decimal.
DI_baseutil safestring str(int k);


/// Convert unsigned integer into a safestring, in decimal.
DI_baseutil safestring str(unsigned int k);

/// Convert long into a safestring, in decimal.
DI_baseutil safestring str(long k);

/// Convert unsigned long into a safestring, in decimal.
DI_baseutil safestring str(unsigned long k);

/// Convert unsigned long into a safestring, in decimal.
DI_baseutil safestring str(int64 k);

/// Convert unsigned long into a safestring, in decimal.
DI_baseutil safestring str(uint64 k);


/** Format double number by \%g format. The decimal separator is output according to the current locale.
* @param f The number to format. NaN and INF are formated as "NaN" and "inf".
* @param precision The number of significant digits to output. Default is 6 as in C printf() facility.
*/
DI_baseutil safestring str_locale(double f, int precision=6);

/** Format double number by %g format, independent of locale, using always dot as decimal separator. See also: str_locale().
* @param f The number to format. NaN and INF are formated as "NaN" and "inf".
* @param precision The number of significant digits to output. Default is 6 as in C printf() facility.
*/
DI_baseutil safestring str(double f, int precision=6);

/** 
* Return the decimal point as currently set in the current thread by setlocale(), for C functions at least.
*/
DI_baseutil char CurrentDecimalPoint();

/// @cond implementation_details
namespace detail {
	DI_baseutil void Throw(const char* msg1, const char* msg2);

	// Stores a positive integer or floating-point number.
	struct numeric_t {
		union {
			double d;
			uint64 k;
		} x_;
		enum type_t {
			numeric_error,
			numeric_double,
			numeric_uint64,
		} type_;
		numeric_t(): type_(numeric_error) {x_.k = 0;}
		void operator=(double d) {type_=numeric_double; x_.d=d;}
		void operator=(uint64 k) {type_=numeric_uint64; x_.k=k;}
		void operator=(int k) {type_=numeric_uint64; x_.k=k;}
	};
	DI_baseutil const char* extract_acapella_constant(const char* p, numeric_t& result);
	DI_baseutil const char* extract_numeric_lowlevel(const char* str, numeric_t& result, char decpoint1, char decpoint2);

	template<bool is_signed> struct Negate;
	template<> struct Negate<true> {
		template<typename T> void operator()(T& x) {x=-x;}
	};
	template<> struct Negate<false> {
		template<typename T> void operator()(T& x) {throw "Never happens: SSH415";}
	};

} // namespace detail
/// @endcond

namespace NStnPol {

	/// A policy for StringToNumber<> RoundingPolicy template parameter. Allows conversion of floating-point values into integers via rounding.
	struct Rounding_Supported {
		// x shall be positive here
		static uint64 Round(double x) {return static_cast<uint64>(x+0.5);}
	};

	/// A policy for StringToNumber<> RoundingPolicy template parameter. Throws in case of attempt to convert a floating-point value to an integer.
	struct Rounding_Prohibited {
		static uint64 Round(double x) {detail::Throw("Floating-point number not accepted as an integer.", str(x).c_str()); return 0;}
	};
 
	/// A policy for StringToNumber<> DecimalPointPolicy template parameter.
	struct DecimalPoint_Dot_And_Comma {
		static char DecPoint1() {return '.';}
		static char DecPoint2() {return ',';}
	};
		
	/// A policy for StringToNumber<> DecimalPointPolicy template parameter.
	struct DecimalPoint_Dot {
		static char DecPoint1() {return '.';}
		static char DecPoint2() {return 0;}
	};

	/// A policy for StringToNumber<> DecimalPointPolicy template parameter.
	struct DecimalPoint_Current_Locale {
		static char DecPoint1() {return CurrentDecimalPoint();}
		static char DecPoint2() {return 0;}
	};

	/// A policy for StringToNumber<> DecimalPointPolicy template parameter.
	struct DecimalPoint_Current_Locale_And_Dot {
		static char DecPoint1() {return CurrentDecimalPoint();}
		static char DecPoint2() {return '.';}
	};

	/// A policy for StringToNumber<> AcapellaKeywordPolicy template parameter. Accepts Acapella numeric constant keywords.
	struct Acapella_Keywords_Supported {
		static void RequestClearance(const char* p) {}
	};

	/// A policy for StringToNumber<> AcapellaKeywordPolicy template parameter. Throws in case of an Acapella numeric constant keyword.
	struct Acapella_Keywords_Prohibited {
		static void RequestClearance(const char* p) {detail::Throw("Invalid numeric", p);}
	};

	/// A policy for StringToNumber<> EmptyStringPolicy template parameter. Converts an empty string to a zero.
	struct Empty_String_Is_Zero {
		static int Value() {return 0;}
	};

	/// A policy for StringToNumber<> EmptyStringPolicy template parameter. Throws in case of an empty string.
	struct Empty_String_Is_Error {
		static int Value() {detail::Throw("Empty string is not accepted as a number.",""); return 0;}
	};

	/// A policy for StringToNumber<> PaddingPolicy template parameter. Allows whitespace padding in the beginning and end of the string.
	struct Padding_Allow_Whitespace {
		static const char* SkipPadding(const char* p) {while(is_whitespace(*p)) ++p; return p;}
	};

	/// A policy for StringToNumber<> PaddingPolicy template parameter. Does not support any padding in the beginning and end of the string.
	struct Padding_Prohibited {
		static const char* SkipPadding(const char* p) {return p;}
	};

} // namespace NStnPol



// Change 04.03.2008: by default, do not convert empty string to a zero any more, by Kaupo's request.
/**
* Generic string-to-number conversion function template. The policy template parameters specify the 
* exact supported format of the input string. If the input string does not conform to the format, an exception will be thrown.
* @param T Template parameter, specifies the numeric type of the resulting number. Must possess std::numeric_limits specializations.
* @param RoundingPolicy Template parameter, specifies the behaviour in case of converting a floating-point numeric string into an integer type. 
*			Can be: NStnPol::Rounding_Supported, NStnPol::Rounding_Prohibited.
* @param DecimalPointPolicy Template parameter, specifies the allowed decimal point separators in floating-point numeric strings.
*			Can be: NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::DecimalPoint_Dot, NStnPol::DecimalPoint_Current_Locale, NStnPol::DecimalPoint_Current_Locale_And_Dot
* @param AcapellaKeywordPolicy Template parameter, specifies if the Acapella numeric keyword constants are recognized or not. 
*			Can be: NStnPol::Acapella_Keywords_Supported, NStnPol::Acapella_Keywords_Prohibited.
* @param EmptyStringPolicy Template parameter, specifies the behaviour in case of empty string (possibly containing padding characters).
*			Can be: NStnPol::Empty_String_Is_Zero, NStnPol::Empty_String_Is_Error.
* @param PaddingPolicy Template parameter, specifies which padding characters can occur in the beginning and end of the string.
*			Can be: NStnPol::Padding_Allow_Whitespace, NStnPol::Padding_Prohibited.
*/
template<
	typename T,
	class RoundingPolicy=NStnPol::Rounding_Supported,
	class DecimalPointPolicy=NStnPol::DecimalPoint_Dot_And_Comma,
	class AcapellaKeywordPolicy=NStnPol::Acapella_Keywords_Supported,
	class EmptyStringPolicy=NStnPol::Empty_String_Is_Error, 
	class PaddingPolicy=NStnPol::Padding_Allow_Whitespace
	>
struct StringToNumber {
	/// Performs the conversion. Throws if the input string does not satisfy the policy requirements, or if the result does not fit in the destination type.
	T operator()(const safestring& s) const {
		const char* p = s.c_str();

		// Skip initial padding
		p = PaddingPolicy::SkipPadding(p);

		if (!*p) {
			return EmptyStringPolicy::Value();
		}

		// Check the negation sign, strtoul() et. al. perform a silent conversion into unsigned.
		bool is_negative=false;
		if (*p=='-') {
			if (!std::numeric_limits<T>::is_signed) {
				detail::Throw("Cannot convert negative number into an unsigned type: ", p);
			} else {
				is_negative=true;
				++p;
				if (*p=='-') {
					detail::Throw("Invalid numeric: ", s.c_str());
				}
			}
		} else if (*p=='+') {
			// Enh 27.11.2008: allow '+' sign in the beginning of the number, ignore it.
			++p;
		}
		// Convert the string
		detail::numeric_t result;
		if (isdigit(*p) || *p=='.' || *p==',' || *p=='e' || *p=='E') {
			p = detail::extract_numeric_lowlevel(p, result, DecimalPointPolicy::DecPoint1(), DecimalPointPolicy::DecPoint2());
		} else {
			AcapellaKeywordPolicy::RequestClearance(p);
			p = detail::extract_acapella_constant(p, result);
		}

		// Skip trailing padding
		p = PaddingPolicy::SkipPadding(p);
		if (*p) {
			detail::Throw("Invalid numeric: ", p);
		}
		
		// Perform conversions
		T z;
		switch(result.type_) {
			case detail::numeric_t::numeric_error:
				detail::Throw("Invalid numeric: ", p);
				z=0; // to silence warnings
				break;
			case detail::numeric_t::numeric_double:
				if (!std::numeric_limits<T>::is_integer) {
					z = static_cast<T>(result.x_.d);
					if (is_negative) {
						detail::Negate<std::numeric_limits<T>::is_signed>()(z);
					}
					break;
				} else {
					result = RoundingPolicy::Round(result.x_.d);
				}
				// FALL THROUGH
			case detail::numeric_t::numeric_uint64:
				if (std::numeric_limits<T>::is_integer) {
					if (result.x_.k > static_cast<uint64>(std::numeric_limits<T>::max())) {
						if (is_negative && result.x_.k == static_cast<uint64>(std::numeric_limits<T>::max())+1) {
							z = std::numeric_limits<T>::min(); 
							break;
						}
						detail::Throw("Number does not fit in the destination data type range: ", s.c_str());
					}
				}
				z = static_cast<T>(result.x_.k);
				if (is_negative) {
					detail::Negate<std::numeric_limits<T>::is_signed>()(z);
				}
				break;
			default: // should never happen
				z=0; // to silence warnings
		}
		return z;
	}
};


/**	
* Converts the string to double, allowing both period '.'
* and the current locale decimal point.
* Parameter meaning is the same as for strtod().
* s must point to the modifiable memory.
* the pointed memory will appear unchanged after return.
* See also: StringToNumber template.
*/
DI_baseutil double strtod1(const char* s, char** p);

/**	
* Converts the string to double, allowing only period '.'
* as the decimal point symbol.
* Parameter meaning is the same as for strtod().
* See also: StringToNumber template.
*/
DI_baseutil double strtod_dot(const char* nptr, char** endptr);

/**	
* Converts the string to double, allowing only comma ','
* as the decimal point symbol.
* Parameter meaning is the same as for strtod().
* See also: StringToNumber template.
*/
DI_baseutil double strtod_comma(const char* nptr, char** endptr);

/**	
* Converts the string to double, allowing only global
* user-set locale decimal point character, as determined
* by evironment variables (LC_ALL, LC_NUMERIC, LANG) on Linux
* or by Control Panel settings in Windows.
* Parameter meaning is the same as for strtod().
* See also: StringToNumber template.
*/
DI_baseutil double strtod_environ(const char* nptr, char** endptr);


/** Convert string to integer or double number.
*   String can be: decimal integer or floating-point number, hexadecimal integer number starting with 0x, 
*	or an Acapella predefined numeric or boolean constant.
*   Decimal delimiter for floating-point number can be either dot or the current locale delimiter.
*   Returns true, if output is an integer. Returns false, if output is a double. 
*   Throws an Exception if the string is empty or cannot be correctly converted to a number.
* See also: StringToNumber template.
* @param str Input string, can contain starting whitespace.
* @param k Output variable for integer output.
* @param f Output variable for double output.
*/
DI_baseutil bool StrToNumber(const safestring& str, int64& k, double& f);


/// Same as StrToNumber, but allows only period '.' as a decimal separator.
DI_baseutil bool StrToNumber_dot(const safestring& str, int64& k, double& f);

/// Check if an integer number fits into a 32-bit signed int.
inline bool IsInt32(int64 x) {return x>=-2147483648LL && x<=2147483647;}

/// Same as the other overload, except that converts large 64-bit integers to doubles, losing precision.
DI_baseutil bool StrToNumber(const safestring& str, int& k, double& f);

/// Same as the other overload, except that converts large 64-bit integers to doubles, losing precision.
DI_baseutil bool StrToNumber_dot(const safestring& str, int& k, double& f);

/** If s is a positive integer numeric string, then return it's value.
* If s is a positive floating-point number in range of int, then the rounded value is returned.
* Decimal delimiter for floating-point number can be either dot or the current locale delimiter.
* Otherwise, an Exception will be thrown.
* May call strtod1 internally, meaning that s cannot be a static C string.
* See also: StringToNumber template.
*/
inline unsigned int StrToUnsignedInt(const safestring& s) {
	return StringToNumber<unsigned int, NStnPol::Rounding_Supported, NStnPol::DecimalPoint_Current_Locale_And_Dot>()(s);
}

/// Returns string converted to a number, allowing for Acapella keywords, rounding from both dot and comma floating-point strings, whitespace padding and empty strings.  See also: StringToNumber template.
inline int StrToIntTolerant(const safestring& s) {
	return StringToNumber<int, NStnPol::Rounding_Supported,	NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::Acapella_Keywords_Supported, NStnPol::Empty_String_Is_Zero>()(s);
}

/// Returns string converted to a number, allowing for Acapella keywords, rounding from both dot and comma floating-point strings, whitespace padding and empty strings. See also: StringToNumber template.
inline unsigned int StrToUIntTolerant(const safestring& s) {
	return StringToNumber<unsigned int, NStnPol::Rounding_Supported,	NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::Acapella_Keywords_Supported, NStnPol::Empty_String_Is_Zero>()(s);
}

/// Returns string converted to a number, allowing for Acapella keywords, rounding from both dot and comma floating-point strings, whitespace padding and empty strings. See also: StringToNumber template.
inline int64 StrToInt64Tolerant(const safestring& s) {
	return StringToNumber<int64, NStnPol::Rounding_Supported,	NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::Acapella_Keywords_Supported, NStnPol::Empty_String_Is_Zero>()(s);
}

/// Returns string converted to a number, allowing for Acapella keywords, rounding from both dot and comma floating-point strings, whitespace padding and empty strings. See also: StringToNumber template.
inline uint64 StrToUInt64Tolerant(const safestring& s) {
	return StringToNumber<uint64, NStnPol::Rounding_Supported,	NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::Acapella_Keywords_Supported, NStnPol::Empty_String_Is_Zero>()(s);
}

/// Returns string converted to a number, allowing for Acapella keywords, rounding from both dot and comma floating-point strings, whitespace padding and empty strings. See also: StringToNumber template.
inline double StrToDoubleTolerant(const safestring& s) {
	return StringToNumber<double, NStnPol::Rounding_Supported,	NStnPol::DecimalPoint_Dot_And_Comma, NStnPol::Acapella_Keywords_Supported, NStnPol::Empty_String_Is_Zero>()(s);
}

///////////////////// Trimming, case-changing etc

/// Return a trimmed string. Trims space, tab, cr, lf. Can be applied to any std::basic_string specialization.
template<class T> 
T trim(const T& s) {
	strlen_t n = s.length();
	if (n==0) return s;
	strlen_t k=0, l=n-1;
	while (k<n && (s[k]==' ' || s[k]=='\t' || s[k]=='\r' || s[k]=='\n')) ++k;
	if (k==n) return T();
	while (l>k && (s[l]==' ' || s[l]=='\t' || s[l]=='\r' || s[l]=='\n')) --l;
	return s.substr(k,l-k+1);
}

/// Same as trim(), but applies trimming in-place.
template<typename str_t>
void apply_trim(str_t& s) {
	strlen_t k = s.find_first_not_of(" \t\r\n");
	if (k==s.npos) {
		s.erase();
		return;
	}
	if (k>0) {
		s.erase(0,k);
	}
	safestring::size_type l = s.find_last_not_of(" \t\r\n");
	if (l<s.length()-1) {
		s.erase(l+1);
	}
}

/** Delete whitespace characters from s.
* @param s The processed string (in-out).
* @param skip_quotes Do not touch included string constants in double quotes.
*/
DI_baseutil void DelSpace(safestring& s, bool skip_quotes=false);


/// Return a string in lowercase in current locale. Can be applied to any std::basic_string specialization.
template<class T> 
T lower(const T& s) {
	T t=s;
	typename T::size_type n = t.length();
	for (typename T::size_type i=0; i<n; ++i) {
		t[i] = static_cast<typename T::value_type>(tolower(t[i]));
	}
	return t;
}


///////////////////// Substring operations

/// Determine a triple-quote string literal end marker by the start marker. MatchMarker('<') returns '>', for example.
DI_baseutil unsigned char MatchMarker(unsigned char c);

/// The namespace containing string processing policies.
namespace NStrPol {

/// String literal policy for skipping over literals. Use with FindSmart().
struct Literal_Skip {
	bool inliteral_, escaped_;
	char endmarker_;

	Literal_Skip(): inliteral_(false), escaped_(false), endmarker_(0) {}

	void HandleLiteral(const safestring& s, safestring::size_type& i, const safestring::size_type pos, const safestring::size_type endpos) {
		char x=s[i];
		if ((x=='"' && !escaped_ && endmarker_==0) || (x==endmarker_ && inliteral_ && i<endpos-1 && s[i+1]=='"')) {
			inliteral_=!inliteral_; // string constant starting or ending
			if (inliteral_) {
				if (i-pos>=2 && s[i-1]=='"' && s[i-2]=='"') {
					++i;
					endmarker_ = MatchMarker(s[i]);
				} else {
					endmarker_ = 0;
				}
				// Bugfix 19.01.2004: handle correctly triple-quote string literal.
			} else if (endmarker_!=0) {
				++i;
				endmarker_ = 0;
			}
		}
		if (inliteral_) {
			if (escaped_) {
				escaped_=false;
			} else {
				if (x=='\\' && endmarker_==0) {
					escaped_=true;	// escape next char
				}
			}
		}
	}
	bool Skipping() {return inliteral_;}
};

/// String literal policy for traversing into the literals. Use with FindSmart().
struct Literal_Traverse {
	void HandleLiteral(const safestring& s, safestring::size_type& i, const safestring::size_type pos, const safestring::size_type endpos){}
	bool Skipping() {return false;}
};

/// Bracket policy for traversing into any brackets. Use with FindSmart().
struct Bracket_Skip_None {
	void HandleBracket(const char x) {}
	bool Skipping() {return false;}
};

/// Bracket policy base class for skipping some brackets. Use derived classes.
template<char opening_bracket, char closing_bracket>
struct Bracket_Skip {
	int deep_;
	Bracket_Skip(): deep_(0) {}
	bool Skipping() {return deep_>0;}
	void HandleBracket(const char x) {if (x==opening_bracket) ++deep_; else if (x==closing_bracket) --deep_;}
};

/// Bracket policy base class for skipping over square brackets []. Use with FindSmart().
struct Bracket_Skip_Square: public Bracket_Skip<'[',']'> {};

/// Bracket policy base class for skipping over round parens (). Use with FindSmart().
struct Bracket_Skip_Round: public Bracket_Skip<'(',')'> {};

/// Bracket policy base class for skipping over braces {}. Use with FindSmart().
struct Bracket_Skip_Brace: public Bracket_Skip<'{','}'> {};

/// Bracket policy for combining several bracket policies.
template<class Pol1=Bracket_Skip_None, class Pol2=Bracket_Skip_None, class Pol3=Bracket_Skip_None>
struct Bracket_Policy {
	Pol1 pol1;
	Pol2 pol2;
	Pol3 pol3;
	bool Skipping() {return pol1.Skipping() || pol2.Skipping() || pol3.Skipping();}
	void HandleBracket(const char x) {
		pol1.HandleBracket(x);
		pol2.HandleBracket(x);
		pol3.HandleBracket(x);
	}
};

/// Hit policy class for finding first occurence of any character in the specified set. Use with FindSmart().
template<class Char_Traits=safestring::traits_type>
struct Hit_One_Of {
	static bool Hit(const safestring& s, const safestring& what, safestring::size_type pos, safestring::size_type endpos) {
		return Char_Traits::find(what.data(), what.length(), s[pos])!=NULL;
	}
};

/// Hit policy class for finding first occurence of the specified substring. Use with FindSmart().
template<class Char_Traits = safestring::traits_type>
struct Hit_Substring {
	static bool Hit(const safestring& s, const safestring& what, safestring::size_type pos, safestring::size_type endpos) {
		if (pos+what.length()>endpos) {
			return false;
		} else {
			return Char_Traits::compare(s.data()+pos, what.data(), what.length())==0;
		}
	}
};

/// Policy class for traversing into // comments at the end of lines. Use with FindSmart().
struct Comment_Traverse {
	static bool HandleComment(const safestring& s, safestring::size_type& i) {return false;}
};

/// Policy class for skipping // comments at the end of lines. Use with FindSmart().
struct Comment_Skip {
	static bool HandleComment(const safestring& s, safestring::size_type& i) {
		if (s[i]=='/' && i+1<s.length() && s[i+1]=='/') {
			// comment begins
			i = s.find_first_of("\n\r", i);
			return true;
		} else {
			return false;
		}
	}
};

} // namespace NStrPol

/**
* @brief Generic find - find a character or substring in another string. There are several policy template parameters, which are defined in the namespace NStrPol.
* The search has several features used for Acapella script parsing.
* Usage examples:
* @code
	safestring::size_type k = FindSmart<Hit_One_Of<>, Bracket_Skip_Brace, 
		Comment_Traverse, Literal_Traverse>()("big lazy dog", "xyz");
	
	safestring::size_type l = FindSmart<>()(script, modulename, pos);
	
* @endcode
* @param HitPolicy Template parameter to specify whether to search any character in 'what' or the substring 'what'. 
*			Pass either Hit_One_Of or Hit_Substring here. These both take a char_traits template
*			parameter, you can pass either safestring::traits_type or std::string::traits_type for case-sensitive comparison.
* @param BracketPolicy Template parameter to specify which brackets to traverse and which to skip.
*			Pass Bracket_Skip_None, Bracket_Skip_Square, Bracket_Skip_Round, Bracket_Skip_Brace or
*			Bracket_Policy combining any of these.
* @param CommentPolicy Template parameter to specify whether to skip // comments in the end of lines.
*			Pass either Comment_Traverse or Comment_Skip here.
* @param LiteralPolicy Template parameter to specify whether to skip or traverse string literals in Acapella syntax (both simple and triple-quote).
*			Pass either Literal_Traverse or Literal_Skip here.
*/
template<
	class HitPolicy=NStrPol::Hit_Substring<>, 
	class BracketPolicy=NStrPol::Bracket_Policy<NStrPol::Bracket_Skip_Square, NStrPol::Bracket_Skip_Round>, 
	class CommentPolicy=NStrPol::Comment_Skip, 
	class LiteralPolicy=NStrPol::Literal_Skip>
struct FindSmart {

/** Perform the actual search. Search is always from left to right, backward search is not implemented because of Acapella code cannot be parsed right to left unambiguously.
* @param s The string where to search.
* @param what What to search for. Either the whole what string is searched for, or any of the letters, dependig on the HitPolicy template parameter.
* @param pos The starting position to search (incl.)
* @param endpos The ending position of search (excl.)
* @return The position of found match, or safestring::npos if no match found.
*/
safestring::size_type operator()(const safestring& s, const safestring& what, safestring::size_type pos=0, safestring::size_type endpos=Nbaseutil::safestring::npos) const {
	if (endpos>s.length()) endpos=s.length();
	LiteralPolicy litpol;
	BracketPolicy brackpol;

	for (safestring::size_type i=pos; i<endpos; ++i) {
		if (!litpol.Skipping() && !brackpol.Skipping() && HitPolicy::Hit(s, what, i, endpos)) {
			return i;
		}
		litpol.HandleLiteral(s, i, pos, endpos);
		if (!litpol.Skipping()) {
			brackpol.HandleBracket(s[i]);
			if (CommentPolicy::HandleComment(s, i)) {
				if (i==s.npos) break;
			}
		}
	}
	return s.npos;
}
};

/**
* Finds first occurence of any character present in 'separators', skipping string constants in double quotes and parens () []. See also: FindSmart template.
* Returns index of found character, or s.npos.
* This function is case-insensitive.
* @param s The searched string.
* @param separators A string containing all characters to search for. Only ASCII characters are supported.
* @param pos If pos is present, starts at this position. No parsing of preceding string part is done.
*/
DI_baseutil safestring::size_type find_first_of_smart(const safestring& s, const safestring& separators, safestring::size_type pos=0);

/// Finds first occurence of c at or behind pos, skipping string constants and constructs in brackets [], and comments in the end of lines. See also: FindSmart template.
DI_baseutil safestring::size_type find_smart_skip_brackets(const safestring& s, char c, safestring::size_type pos=0);

/// Finds first occurence of substring at or behind pos, skipping string constants and constructs in brackets [], and comments in the end of lines. See also: FindSmart template.
DI_baseutil safestring::size_type find_smart_skip_brackets(const safestring& s, const safestring& substring, safestring::size_type pos=0);

/// Finds first occurence of substring at or behind pos, skipping string constants and constructs in brackets [] and parens (). Case sensitive! See also: FindSmart template.
DI_baseutil safestring::size_type find_smart_skip_brackets_and_parens(const safestring& s, const safestring& substring, safestring::size_type pos=0);

/// Replace occurences of old_substring in s by new_substring. Case-insensitive.
DI_baseutil void Replace(safestring& s, const safestring& old_substring, const safestring& new_substring); 

/// Replace occurences of old_substring in s by new_substring. Case sensitive.
DI_baseutil void ReplaceCase(safestring& s, const safestring& old_substring, const safestring& new_substring); 

/** Replace specified format codes found in s0 by substitutions given in replacements. 
* The codes usually begin with the percent (%) character.
* The codes and replacements must contain the same number of elements.
* The s0 string may contain special codes %( and %). These delimit a region which should contain
* one or more replacable codes present in 'codes' parameter. If all of those codes were replaced
* by empty strings (present in 'replacements' parameter), then the whole region is deleted from 
* the output string. Otherwise (if some code was replaced by non-empty string), only the codes 
* %( and %) are deleted.
*
* If trans_src and trans_dst are present, then each inserted replacement string is at first fed
* itself through SafeStringReplaceFormat with trans_src and trans_dst acting as 'codes' and 'replacements'
* parameter.
*/
DI_baseutil safestring SafeStringReplaceFormat( const safestring& s0, const safestringlist& codes, const safestringlist& replacements, const safestringlist* trans_src=NULL, const safestringlist* trans_dst=NULL);

/// If the string contains a colon, return the string part before colon.
DI_baseutil safestring GetDataBlock(const safestring& s);	

/// Return base name of a file name. For "C:\program files/test\abc/mopsa.doc.exe" returns "mopsa.doc".
DI_baseutil safestring GetBaseName(const safestring& s);	

/// If the string contains a colon, remove the part before colon and the colon itself.
DI_baseutil void RemDataBlock(safestring& s);



///////////////////////  Quoting 

/** Escape certain characters in the s by backslashes. The escaped characters
* are defined by Acapella script language: TAB, CR, LF, double quote and backslash itself.
*/
DI_baseutil safestring QuoteAdd(const safestring& s);

/** Escape certain characters in the s by backslashes. The escaped characters
* are defined by Acapella script language: TAB, CR, LF, double quote and backslash itself.
* The string is modified in-place.
*/
DI_baseutil void ApplyQuoting(safestring& s);

/** Opposite to QuoteAdd().
* @param s The input string.
* @param isfilename If true, then returns s unmodified.
*/
DI_baseutil safestring QuoteRemove(const safestring& s, bool isfilename=false);

/** Opposite to ApplyQuoting().
* @param s The input string.
* The string is modified in-place.
*/
DI_baseutil void RemoveQuoting(safestring& s);

/// Prefix each occurence of chars in pszChars with pszPrefix string. Used for backslashing things.	
DI_baseutil safestring Prepend(const safestring& s, const char *pszPrefix, const char *pszChars); 

/// Removes backslashes from any occurences of \" combinations in s. See also QuoteRemove().
DI_baseutil void ReplaceQuoteEscapes(safestring& s);

/**
* Quote any string so that the result is a valid Acapella variable name. The name will begin
* with the '@' character and contains \@xx (at sign followed by 2 hexadecimal digits) hexadecimal codes for any otherwise illegal character.
* If the input name is already a valid Acapella name, then does nothing.
*/
DI_baseutil void QuoteName(safestring& name);

/**
* Opposite to the QuoteName(). If the input parameter is a valid @-quoted name, then unquotes it, otherwise 
* does nothing. Returns false if the name begins with '@', but is not a valid @-quoted name.
*/
DI_baseutil bool UnquoteName(safestring& name);


////////////////////////  Tokenizing

/** Find the first token in s, separated by whitespace.
*	Delete it from s and return it.
*   Can be applied to any std::basic_string specialization.
*/
template<class T>
T SplitFirstToken(T& s) {
	strlen_t k = s.find_first_not_of(" \t\n\r");
	strlen_t l = s.find_first_of(" \t\n\r", k);
	T token;
	if (l==s.npos) {
		if (k==s.npos) {
			token = "";
		} else {
			token = s.substr(k);
		}
		s = "";
	} else {
		token = s.substr(k,l-k);
		s = s.substr(l);
	}
	return token;
}


/** Find a single token from the string. 
* @param s The analysed string.
* @param k Index of the token to extract. k=0 is the first token.
* @param TailMarker Output parameter for feeding into later TokenTail call.
*/
DI_baseutil safestring Token(const safestring& s, int k, unsigned int& TailMarker);	

/// Return a tail after last found token. TailMarker must be obtained from a preceding Token() call.
DI_baseutil safestring TokenTail(const safestring& s, unsigned int& TailMarker); 


/////////////////////////////////////////////////////////////
/////////////  safestringlist ///////////////////////////////
/////////////////////////////////////////////////////////////

/** Iterator for iterating a safestringlist. One can obtain the valid iterators only 
* from safestringlist::begin() and safestringlist::end(). The iterators are valid
* only while the corresponding safestringlist is live and not changed.
*/
class safestringlist_iterator: public mb_malloced {
public:

	// provide STL iterator interface
    typedef std::forward_iterator_tag iterator_category;
    typedef const char* value_type;
    typedef void difference_type;
    typedef const char** pointer;
    typedef const char*& reference;


	// p_ points to the terminating 0 of the previous substring.
	// it may point also before the beginning of safestringlist.
	// therefore dereferencing p_ is not allowed.

	/// Advance the iterator. Advancing the end() iterator is not allowed.
	void operator++() const { p_ = strchr(p_+1,0);}

	/// Advance the iterator. Advancing the end() iterator is not allowed.
	safestringlist_iterator operator++(int) const { safestringlist_iterator x(*this); ++(*this); return x;}

	/// Dereference the iterator and return const char* ASCIIZ string pointer. Trims whitespace in the beginning of the item.
	const char* operator*() const {
		const char* p = p_+1;
		while(*p==' ') ++p;
		return p;
	}

	/// Synonym for operator*.
	const char* c_str() const {return operator*();}

	// Enh 12.03.2004: add non-trimming iterator.

	/**
	* Dereference the iterator and return const char* ASCIIZ string pointer, without trimming whitespace in the beginning of the item.
	* Commonly used together with SplitStringSmartStrict().
	*/
	const char* c_str_strict() const {
		const char* p = p_+1;
		return p;
	}

	/// Compare the iterators, usually with the end() iterator.
	bool operator!=(const safestringlist_iterator& b) const {
		if (p_ == b.p_) return false;
		if (*(p_+1)==' ') {
			const char* q = *(*this);
			return q!=b.p_;
		}
		return true;
	}
	/// Compare the iterators, usually with the end() iterator.
	bool operator==(const safestringlist_iterator& b) const {return p_ == b.p_;}

	/// Default ctor. Only allowed operations on such iterator are assignment and destruction.
	safestringlist_iterator(): p_(NULL) {}

	/// Copy ctor
	safestringlist_iterator(const safestringlist_iterator& b): p_(b.p_) {}

	// Default assignment operator is ok.
private:
	/// This is the ctor used by safestringlist
	safestringlist_iterator(const char* p): p_(const_cast<char*>(p)) {}

	/// Mutable dereference for use by implementation.
	char* deref() {
		char* p = p_+1;
		while(*p==' ') ++p;
		return p;
	}

	mutable char* p_;	/// Actual raw pointer to the beginning of the item.
	friend class safestringlist;
};

/** Class of lists of strings. Actually this is itself a safestring, with embedded zero characters
* marking the string delimiters. The list elements are always trimmed, i.e. one cannot 
* fetch from the list a string containing space characters in the beginning 
* or in the end. Usually the safestringlist is created by SplitString() function which also
* trims tab, cr and lf characters by default.
* There are padding spaces used in the implementation which are not visible outside.
*/
class safestringlist: public safestring {
	bool empty_;	// to distuingish inbetween empty list and a list containing one empty string.
public:
	/// Default ctor, makes an empty list
	safestringlist(): empty_(true) {}

	/** Construct a list containing one element, unless the src string contains embedded zero characters. 
	* In the latter case the zero characters are interpreted as item delimiters 
	* and the resulting safestringlist will contain more elements.
	*/
	safestringlist(const safestring& src): safestring(src), empty_(src.empty()) {c_str();}

	safestringlist(const safestringlist& src): safestring(src), empty_(src.empty_) {c_str();}

	typedef const safestringlist_iterator const_iterator;
	typedef safestringlist_iterator iterator;

	/// Return iterator to the first element of the list. If this compares equal to end() then the list is empty.
	const_iterator begin() const { return safestringlist_iterator(c_str()-1); }

	/// Return iterator after the end of the list. Only allowed operation with this iterator is comparing to another iterator.
	const_iterator end() const {
		if (empty() && empty_) {return begin();}
		return safestringlist_iterator(c_str()+length());
	}

	/// Return iterator to the first element of the list. If this compares equal to end() then the list is empty.
	iterator begin() { return safestringlist_iterator(c_str()-1); }

	/// Return iterator after the end of the list. Only allowed operation with this iterator is comparing to another iterator.
	iterator end() {return const_cast<const safestringlist*>(this)->end();}

	/// Return number of elements in the list.
	unsigned int count() const {
		const_iterator a = begin(), b=end();
		unsigned int k=0;
		while (a!=b) {
			++k;
			++a;
		}
		return k;
	}

	/// Erase an element from the list
	void erase(iterator it) {
		// logically erase the string pointed by it from the list.
		char* p = it.deref();
		if (!p) return;
		while (*p) *p++ = ' ';
		if (p<c_str()+length()) {
			*p=' ';	
		} else if (it.p_ >= c_str()) {
			// Bugfix 12.03.2004: correctly erase the last element in the list, so that count() method works.
			safestring::erase(it.p_ - c_str());
		}
	}

	/// Return an element by an index. This is O(k) operation. If index is too high, empty string will be returned.
	const char* at(int k) const {
		const_iterator p = begin(), pend=end();
		for (int i=0; i<k; ++i) {
			if (p==pend) return "";
			++p;
		}
		if (p==pend) return "";
		return *p;
	}

	/** Add an element to the end of the list. This invalidates all iterators.
	* If b contains embedded zero characters, then it is considered to contain multiple elements.
	* In particular, if b is itself a safestringlist, then list concatenation is performed.
	*/
	safestringlist& operator +=(const safestring& b) {
		// add a new entry to the list.
		if (!empty() || !empty_) {
			safestring::operator+=(safestring(1,0));
			safestring::operator+=(b);
		} else {
			safestring::operator+=(b);
			empty_ = false;
		}
		return *this;
	}

	/// Add an element to the end of the list. This invalidates all iterators.
	safestringlist& operator +=(const char* b) {
		// add a new entry to the list.
		if (!empty() || !empty_) {
			safestring::operator+=(safestring(1,0));
			safestring::operator+=(b);
		} else {
			safestring::operator+=(b);
			empty_ = false;
		}
		return *this;
	}

	/** Check if the list contains item it. The comparison is case-insensitive, always in C locale.
	* Return the pointer to the found element, or NULL.
	*/
	const char* finditem(const safestring& item) const {
		for (const_iterator p=begin(), pend=end(); p!=pend; ++p) {
			if (item == *p) {
				return *p;
			}
		}
		return NULL;
	}
	/// Remove the content, the list will be empty afterwards.
	void erase() {
		safestring::erase();
		empty_ = true;
	}
private:
	// unimplemented because of unclear meaning; use length() to obtain string representation length and count() to get number of list elements.
	unsigned int size() const;	
};


///////////////////////// safestringlist utility functuons ////////////////////////////

/**
* Split a safestring into substrings and create a safestringlist. Subsequent separators 
* are merged together, i.e. in the output list there will appear no empty substrings.
* For entering an empty string into a safestringlist one must use safestringlist::operator+=().
* Consequently, an empty source string or one containing only separators and trimmable whitespace characters 
* will create an empty safestringlist.
*
* Whitespace is effectively trimmed from the beginning and end of substrings.
* @param s The source string.
* @param separators The list of characters acting as separators. Only ASCII characters (codes 0-127) are currently supported.
* @param preservetabsandlinefeeds Do not trim tab, cr and lf characters from the items. Spaces are always trimmed.
*/
DI_baseutil safestringlist SplitString(const safestring& s, const safestring& separators, bool preservetabsandlinefeeds=false);


/// Same as SplitString, but does not insert breaks inside string constants in double quotes and inside parens () [].
DI_baseutil safestringlist SplitStringSmart(const safestring& s, const safestring& separators, bool preservetabsandlinefeeds=false);

/// Same as SplitStringSmart, but no whitespace merging or string end trimming is done.
DI_baseutil safestringlist SplitStringSmartStrict(const safestring& s, const safestring& separators);

/// Same as SplitString, but does insert any find separator characters as separate list elements at appropriate positions.
DI_baseutil safestringlist SplitStringKeepSeparators(const safestring& s, const safestring& separators);


/**
* Create a safestring from a safestringlist by joining substrings together.
* @param sl the list to join
* @param sep will be inserted inbetween substrings.
*/
DI_baseutil safestring JoinString(const safestringlist& sl, const safestring& sep);


/// A helper class for easy composing of safestringlist's by hand.
class MakeListObject: public mb_malloced {
	safestringlist s_;
public:
	MakeListObject(): s_("") {}
	MakeListObject& operator()(const safestring& t) {s_ += t; return *this;}
	operator safestringlist() {return s_;}
};

/** A helper function for easy composing of safestringlist objects by hand.
* Typical usage:
* @code
* safestringlist lst = MakeList("abc")("def")("kala")("mopsa");
* @endcode
*/
inline MakeListObject MakeList(const safestring& t) {
	return MakeListObject()(t);
}

/// Overloaded operator for streaming safestring objects out to a standard stream.
DI_baseutil std::ostream& operator<<(std::ostream& os, const safestring& s);

/// A 16-bit character type.
typedef wchar_t char16;

/// A 16-bit character string class. This is the same as std::wstring except that memory allocation always happens in heap of baseutil dll.
typedef std::basic_string<char16, std::char_traits<char16>, mb_allocator_typedef<char16>::allocator> aca_wstring;

/// A convenience wrapper for translating a UTF-8 string to Windows internal UTF-16 format. See also Utf8Translator::TranslateFromUtf().
DI_baseutil aca_wstring Utf2Win(const safestring& utf8_string);

/// A convenience wrapper for translating a UTF-8 filename to Windows internal UTF-16 format. It also replaces slashes with backslashes and performs other translations so that the filename is suitable for passing to the CreateFile() Windows SDK function.
DI_baseutil aca_wstring Utf2WinFileName(const safestring& filename_utf8);

/// A convenience wrapper for translating a short zero-terminated Windows internal UTF-16 string to UTF-8. For potentially large texts use Utf8Translator::TranslateToUtf() instead.
DI_baseutil safestring Win2Utf(const char16* utf16_string);

/// A convenience wrapper for translating a Windows internal UTF-16 string to UTF-8.
DI_baseutil safestring Win2Utf(const aca_wstring& utf16_string);

/// Opposite of Utf2WinFileName(), replaces backslashes with slashes and strips/translates special windows //?/ style prefixes.
DI_baseutil safestring Win2UtfFileName(const aca_wstring& filename_utf16);

} // namespace

#ifdef ACAPELLA_EVIL_MIN_WAS_DEFINED
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MIN_WAS_DEFINED
#endif
#ifdef ACAPELLA_EVIL_MAX_WAS_DEFINED
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MAX_WAS_DEFINED
#endif

#endif
