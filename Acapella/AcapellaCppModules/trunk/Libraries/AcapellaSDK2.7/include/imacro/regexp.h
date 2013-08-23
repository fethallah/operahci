#ifndef _IMACRO_REGEXP_H_INCLUDED_
#define _IMACRO_REGEXP_H_INCLUDED_

// Minimal regular expression support (only * and ? metacharacters).

#include "mod.h"

namespace NIMacro {

/// Return true if pattern s contains any regexp metachars (currently * and ?)
DI_IMacro bool HasRegExpSymbols(const safestring& s);

/// Return true if pattern matches string s
DI_IMacro bool RegExpMatch(const safestring& s, const safestring& pattern);

/// Return the list of strings in lst which match the pattern
DI_IMacro safestringlist RegExpGrep(const safestringlist lst, const safestring& pattern);

/**
* Replace format codes in string with specified substitutions. Return resulting string.
* This function is mainly used in self-reflection modules (modules(), libraries(), etc.)
* in order to create formatted output strings. Optional parameters provide a possibility
* to encode specific characters in the replacement strings as required by the output 
* format (HTML, LaTeX, etc.).
* 
* Implementation is in system_modules.cpp.
*
* @param s0 The initial string
* @param codes List of format codes to replace. The codes usually start with % symbol, but this is not required.
* @param replacements The list of replacements, should be of the same length as codes list.
* @param trans_src If not NULL, then gives the list of codes which have to be translated in
*					the replacement string beforehand. This is useful for creating 
*					formatted output in HTML, RTF, LaTeX, etc.
* @param trans_dst If trans_src is not NULL, then this parameter must be also present
*					and give substitutions for the codes in trans_src.
*/ 
DI_IMacro safestring ReplaceFormat( const safestring& s0, const safestringlist& codes, const safestringlist& replacements, const safestringlist* trans_src=NULL, const safestringlist* trans_dst=NULL);

/**
* Abstract base class for modules outputting formatted text, 
* in which some symbol sequences have to be translated. These modules
* call the ReplaceFormat() function. The 'translations' module parameter
* provides the trans_src and trans_dst parameters for this function call.
*/
class mod_base_with_translate_support: public Mod {
protected:
	mod_base_with_translate_support(): translations("") {}
	safestringlist srclist_, dstlist_;
	const char* translations;
	/// Should be called from derived class' Declare(). Declares the 'translations' module input parameter.
	void DeclareTranslationList() {
		input(translations, "translations", 0, "Comma-separated list of strings and their translations, with which they have to be replaced in format code substitution. First entry is a string, second it's translation, etc. String is replaced only if it is not immediately preceded by a backslash. Note that to pass backslash and double quotes to the module you need to escape them by another backslash.", "");
	}
	/// Should be called in the beginning of derived class' Run(). Prepares srclist_ and dstlist_ for subsequent use in ReplaceFormat() call.
	void ProcessTranslationList() {
		// process translation list
		safestringlist trans = SplitString(translations, ",", true);
		MakeListObject src,dst;
		for (safestringlist::const_iterator p=trans.begin(), pend=trans.end(); p!=pend; ++p) {
			src(*p);
			++p;
			if (p==pend) {
				throw Exception(ERR_BADPARAMETERS, "There must be even number of entries in the translation list!");
			}
			dst(*p);
		}
		srclist_ = src;
		dstlist_ = dst;
	}		
};


} // namespace

#endif
