#ifndef x_PKI_CT_BASEUTIL_CONFIG_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_CONFIG_H_INCLUDED_

// For endianness detection:
#include <boost/version.hpp>
#if (!defined(__x86_64) && !defined(__x86_64__)) || BOOST_VERSION/100000>1 || BOOST_VERSION/100%1000>=34
#include <boost/detail/endian.hpp>
#endif

namespace Nbaseutil {

// Acapella build configuration.
// All exported macros start with ACAPELLA, to avoid clashes.
// The aim is to collect any build and runtime environment specific decisions here,
// and use only ACAPELLA* macros for conditional compilation in other places.

/**
* Specifies the Acapella version (two numbers).
* The main version number can be obtained by ACAPELLAVER/10.
* The subversion number can be obtained by ACAPELLAVER%10.
*
* One may want to keep this in sync with ACCPROXY_VERSION (EvoPro/accproxy/accproy.h)
*/
#define ACAPELLAVER 27

// select compiler
#ifdef _MSC_VER
/**
* This macro is defined when compiling by a MSVC compiler or similar.
* More exactly, this is defined if the compiler predefines _MSC_VER macro.
* This is done for example by icc on Windows.
* This macro is used for working around core compiler limitations and differences.
*/
#define ACAPELLA_COMPILER_MSVC

#elif defined(__GNUC__)
/**
* This macro is defined when compiling by a gcc or similar compiler.
* More exactly, this is defined if the compiler predefines __GNUC__ macro.
* This is done for example by icc on Linux.
* This macro is used for working around core compiler limitations and differences.
*/
#define ACAPELLA_COMPILER_GCC
#else
#error Unsupported compiler, only MSVC/gcc/icc supported currently.
#endif

#ifdef __INTEL_COMPILER
/// This is defined if the actual compiler is icc.
#define ACAPELLA_COMPILER_VARIANT_ICC
#endif

#if defined(_WIN32)
/**
* This macro is defined when Win32 API is available at both
* compile and run time. This is used for choosing how to do
* something not covered by C++ standard.
* Note that ACAPELLA_PLATFORM_WINDOWS and ACAPELLA_PLATFORM_POSIX can be defined both
* at the same time, e.g. when compiling under CygWin.
*/
#define ACAPELLA_PLATFORM_WINDOWS
/// Defined when compiling Acapella for Windows
#define ACAPELLA_TARGET_WINDOWS
#endif

#if defined(__CYGWIN32__)
/**
* This macro is defined when Win32 API is available at both
* compile and run time. This is used for choosing how to do
* something not covered by C++ standard.
* Note that ACAPELLA_PLATFORM_WINDOWS and ACAPELLA_PLATFORM_POSIX can be defined both
* at the same time, e.g. when compiling under CygWin.
*/
#define ACAPELLA_PLATFORM_WINDOWS
/// Defined when compiling Acapella for CygWin (not supported yet!)
#define ACAPELLA_TARGET_CYGWIN
#endif

#if defined(__unix__)
/**
* This macro is defined when Posix API is available at both
* compile and run time, e.g. <unistd.h> header.
* This is used for choosing how to do
* something not covered by C++ standard.
* Note that ACAPELLA_PLATFORM_WINDOWS and ACAPELLA_PLATFORM_POSIX can be defined both
* at the same time, e.g. when compiling under CygWin.
*/
#define ACAPELLA_PLATFORM_POSIX
#ifdef __linux__
/// Defined if compiling Acapella for Linux
#define ACAPELLA_TARGET_LINUX
#endif
#endif

#if defined(__APPLE__) && defined(__MACH__)
#define ACAPELLA_PLATFORM_POSIX
/// Defined when compiling Acapella for Mac OS X
#define ACAPELLA_TARGET_OSX
#endif

// Check that at least one platform to compile against is defined.
#if !defined(ACAPELLA_PLATFORM_WINDOWS) && !defined(ACAPELLA_PLATFORM_POSIX)
#error Unsupported platform.
#endif

#ifdef ACAPELLA_PLATFORM_WINDOWS
/// This is defined if __declspec(dllexport) is needed for exporting functions from a shared library.
#define ACAPELLA_REQUIRES_DLLEXPORT
#ifndef ACAPELLA_DLLEXPORT
/// This is defined to be __declspec(dllexport) in Windows builds, and empty otherwise.
#define ACAPELLA_DLLEXPORT  __declspec(dllexport)
#endif
#ifndef ACAPELLA_DLLIMPORT
/// This is defined to be __declspec(dllimport) in Windows builds, and empty otherwise.
#define ACAPELLA_DLLIMPORT  __declspec(dllimport)
#endif
#else
#ifndef ACAPELLA_DLLEXPORT
/// This is defined to be __declspec(dllexport) in Windows builds, and empty otherwise.
#define ACAPELLA_DLLEXPORT
#endif
#ifndef ACAPELLA_DLLIMPORT
/// This is defined to be __declspec(dllimport) in Windows builds, and empty otherwise.
#define ACAPELLA_DLLIMPORT
#endif
#endif

#ifdef _MSC_VER
/**
* This is defined when using the MSVC C/C++ runtime library, or one
* using the implementation-defined symbols compatible to MSVC++, like _S_IFREG.
*/
#define ACAPELLA_RTL_MSVC
#endif

#ifdef __GNUC__
/**
* This is defined when using the Gnu (GLIBC) C/C++ runtime library, or one
* using the implementation-defined symbols compatible to GLIBC.
*/
#define ACAPELLA_RTL_GLIBC
#endif

#if !defined(ACAPELLA_RTL_MSVC) && !defined(ACAPELLA_RTL_GLIBC)
#error Unsupported C/C++ runtime library!
#endif

#ifdef __GNUC__
/// The RTL conforms to ISO C99 standard (ISO/IEC 9899:1999).
#define ACAPELLA_HAS_C99
#endif

// Give Makefile some chance of override
#ifdef REDEFINE_ACAPELLAVER
#undef ACAPELLAVER
#define ACAPELLAVER REDEFINE_ACAPELLAVER
#endif
#ifdef UNDEF_ACAPELLA_COMPILER_MSVC
#undef ACAPELLA_COMPILER_MSVC
#endif
#ifdef UNDEF_ACAPELLA_COMPILER_GCC
#undef ACAPELLA_COMPILER_GCC
#endif
#ifdef UNDEF_ACAPELLA_COMPILER_VARIANT_ICC
#undef ACAPELLA_COMPILER_VARIANT_ICC
#endif
#ifdef UNDEF_ACAPELLA_PLATFORM_WINDOWS
#undef ACAPELLA_PLATFORM_WINDOWS
#endif
#ifdef UNDEF_ACAPELLA_PLATFORM_POSIX
#undef ACAPELLA_PLATFORM_POSIX
#endif
#ifdef UNDEF_ACAPELLA_REQUIRES_DLLEXPORT
#undef ACAPELLA_REQUIRES_DLLEXPORT
#endif
#ifdef UNDEF_ACAPELLA_RTL_MSVC
#undef ACAPELLA_RTL_MSVC
#endif
#ifdef UNDEF_ACAPELLA_RTL_GLIBC
#undef ACAPELLA_RTL_GLIBC
#endif
#ifdef UNDEF_ACAPELLA_HAS_C99
#undef ACAPELLA_HAS_C99
#endif
#ifdef UNDEF_ACAPELLA_TARGET_WINDOWS
#undef ACAPELLA_TARGET_WINDOWS
#endif
#ifdef UNDEF_ACAPELLA_TARGET_CYGWIN
#undef ACAPELLA_TARGET_CYGWIN
#endif
#ifdef UNDEF_ACAPELLA_TARGET_LINUX
#undef ACAPELLA_TARGET_LINUX
#endif
#ifdef UNDEF_ACAPELLA_TARGET_OSX
#undef ACAPELLA_TARGET_OSX
#endif


#define ACAPELLA_STRINGIZE(s1) #s1
/// A macro for converting a preprocessor number into a preprocessor string.
#define ACAPELLA_INT2STRING(num) ACAPELLA_STRINGIZE(num)

#if defined(_MSC_VER) && _MSC_VER>=1400
#	ifdef _DEBUG
#		ifndef _SCL_SECURE_NO_WARNINGS
#			if defined(_SECURE_SCL) && defined(_XSTRING_)
				// It seems the client code has included <string> or STL headers before <acapella/DI_acapella.h>,
				// but has not defined _SCL_SECURE_NO_WARNINGS before doing that. This will produce
				// massive avalanche of nonsense warnings C6996 under Visual Studio 2005 and 2008,
				// unless the warnings are prohibited by a #pragma.
				// It seems it is too late to prohibit the warnings here.
#				pragma message(__FILE__ "(" ACAPELLA_INT2STRING(__LINE__) "): Warning: a lot of C4996 warnings may appear later.")
#				pragma message(__FILE__ "(" ACAPELLA_INT2STRING(__LINE__) "): Consider to #define _SCL_SECURE_NO_WARNINGS in the client library code before including any STL headers to avoid this.")
#				pragma message(__FILE__ "(" ACAPELLA_INT2STRING(__LINE__) "): Alternatively, #include <acapella/DI_acapella.h> as the first include in the client library code.")
#				pragma warning(disable:4996)
#			endif
#			define _SCL_SECURE_NO_WARNINGS
#		endif
#	else
#		if defined(_SECURE_SCL) && _SECURE_SCL!=0
#			error For performance reasons the Release build should not use checked iterators, please    #define _SECURE_SCL 0    before including any STL or Acapella headers!
#		endif
#		undef _SECURE_SCL
#		define _SECURE_SCL 0
#	endif
#endif

#define BOOST_ALL_DYN_LINK 1
#if defined(_MSC_VER) && _MSC_VER==1500 && !defined(__INTEL_COMPILER)
// MSVC 9.0 not recognized by Boost 1.34.1, force the correct library sufix
#define BOOST_LIB_TOOLSET "vc90"
#endif

#if defined(ACAPELLA_COMPILER_MSVC) && !defined(ACAPELLA_COMPILER_VARIANT_ICC)
/// MSVC newer versions recognize the sealed keyword useful for marking virtual member functions non-overridable.
#define ACAPELLA_SEALED sealed
/// MSVC newer versions recognize the override keyword useful for marking overridden virtual member functions.
#define ACAPELLA_OVERRIDE override

#else
/// MSVC newer versions recognize the sealed keyword useful for marking virtual member functions non-overridable.
#define ACAPELLA_SEALED
/// MSVC newer versions recognize the override keyword useful for marking overridden virtual member functions.
#define ACAPELLA_OVERRIDE
#endif

#if defined(BOOST_LITTLE_ENDIAN)
/// Defined if the compilation takes place on a little-endian architecture.
#define ACAPELLA_LITTLE_ENDIAN
#elif defined(BOOST_BIG_ENDIAN)
/// Defined if the compilation takes place on a big-endian architecture.
#define ACAPELLA_BIG_ENDIAN
#elif defined(__x86_64) || defined(__x86_64__)
# // Boost 1.33 does not know about x86_64, a major target for us!
/// Defined if the compilation takes place on a little-endian architecture.
#define ACAPELLA_LITTLE_ENDIAN
#else
#error Unknown or unsupported machine endianness (problem with including <boost/detail/endian.hpp>?)
#endif

// Give Makefile some chance of override
#ifdef UNDEF_ACAPELLA_LITTLE_ENDIAN
#undef ACAPELLA_LITTLE_ENDIAN
#endif
#ifdef UNDEF_ACAPELLA_BIG_ENDIAN
#undef ACAPELLA_BIG_ENDIAN
#endif

} // namespace
#endif
