#ifndef PKI_CTG_BOOST_BINARY_CONFIG_HPP_INCLUDED
#define PKI_CTG_BOOST_BINARY_CONFIG_HPP_INCLUDED

// PHelde 01.02.2008: the Release mode Boost DLL-s in svn are built with _SECURE_SCL=0, ensure the client code follows this!
#if defined(_MSC_VER) && _MSC_VER>=1400
#	ifndef _DEBUG
#		if defined(_SECURE_SCL) && _SECURE_SCL!=0
#			pragma message("PHelde 01.02.2008: The Boost Release-mode libraries are built with _SECURE_SCL=0 setting.")
#			pragma message("This implies the client code must be compiled with the same setting, which is not the case now.")
#			pragma message("Please add _SECURE_SCL=0 to the Preprocessor Definitions in the Release build project settings!")
#			error Incompatible build options with Boost libraries, follow the above guidelines to resolve!
#		endif
#		undef _SECURE_SCL
#		define _SECURE_SCL 0
#   else
#		if defined(_SECURE_SCL) && _SECURE_SCL==0
#			error Incompatible build options with Boost libraries, _SECURE_SCL should be not 0 in Debug builds!
#		endif
#	endif
#endif

#endif
