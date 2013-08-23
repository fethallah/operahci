#ifndef x_ACAPELLA_IMACRO_UTFTRANSLATOR_H_INCLUDED_
#define x_ACAPELLA_IMACRO_UTFTRANSLATOR_H_INCLUDED_

#include <sys/stat.h>
#include <time.h>

#include "safestring.h"

namespace Nbaseutil {

	namespace UtfTranslator {

#ifdef ACAPELLA_PLATFORM_WINDOWS

		/// A typedef for codepage identificators.
		typedef unsigned int codepage_t;

		/// Defines some often used codepage id-s
		enum codepage_constants {
			cp_utf8 = 65001,
			cp_ucs2_little_endian = 1200,
		};
#else
		/// A typedef for codepage identificators. On Linux the iconv() utility does not use integral id-s, use strings instead.
		typedef safestring codepage_t;
		
		const codepage_t cp_utf8 = "UTF-8";
		const codepage_t cp_ucs2_little_endian = "UCS-2LE";

#endif

		/// Return codepage id which is assumed for texts missing the codepage directive.
		DI_baseutil codepage_t GetDefaultCodePageId();

		/// Finds out if conversions to and from the codepage are supported by the currently running system.
		DI_baseutil bool IsCodePageSupported(codepage_t codepage_id);

		/** 
		* Translate codepage id to a standard IANA charset name.
		* If the codepage_id is not known, just returns the numeric id.
		*/
		DI_baseutil Nbaseutil::safestring GetCharsetName(codepage_t codepage_id);

		/**
		* Translate the IANA charset name to the codepage id. 
		* If charset name is numeric, then just returns this number.
		* Otherwise, if charset name is not known, returns 0.
		*/
		DI_baseutil codepage_t GetCodePageId(const Nbaseutil::safestring& iana_charset_name);

		/** Find out the codepage of the text received from external source (e.g. disk). 
		* @param text The input text as arriving from the external source (e.g. disk image).
		*		The text can be either XML or Acapella source text.
		* @param guessed An output parameter. If the text did not contain explicit codepage specification,
		*            a guess is made the guessed parameter set to true, otherwise it will be false.
		*/
		DI_baseutil codepage_t FindCodePageId(const Nbaseutil::safestring& text, bool& guessed);

		/** Check that the text does not contain invalid UTF-8 characters. This function does not recognize UTF BOM markers,
		* these have to be removed beforehand.
		* @param utf The text to check.
		* @param errindex In case of 'false' return, this will contain the byte index of the invalid UTF-8 sequence.
		* @return True for valid ASCII or UTF-8, false for invalid.
		*/
		DI_baseutil bool IsValidUtf(const Nbaseutil::safestring& utf, Nbaseutil::safestring::size_type& errindex);

		/** Check that the text does not contain invalid UTF-8 characters. 
		* This function does not recognize UTF BOM markers,
		* these have to be removed beforehand.
		* @param utf The text to check.
		* @return True for valid ASCII or UTF-8, false for invalid.
		*/
		DI_baseutil bool IsValidUtf(const Nbaseutil::safestring& utf);

		/// Translate byte index to line number and position in that line (both 1-based).
		DI_baseutil void IndexToLinePos(const Nbaseutil::safestring& utf, Nbaseutil::safestring::size_type byte_index, int& lineno, int& character_pos_in_line);

		/// Translate src from one codepage to another. Either codepage can also be cp_ucs2_little_endian. Both src and dst may be the same string.
		DI_baseutil bool Translate(codepage_t codepage_id_from, codepage_t codepage_id_to, const Nbaseutil::safestring& src, Nbaseutil::safestring& dst);

		/// Translates text from the indicated codepage to UTF-8. Both text and utf may be the same string.
		DI_baseutil void TranslateToUtf(codepage_t codepage_id, const Nbaseutil::safestring& text, Nbaseutil::safestring& utf);

		/// Translates text from UTF-8 to the indicated codepage. If some chars could not be represented, returns false. Both text and utf may be the same string.
		DI_baseutil bool TranslateFromUtf(codepage_t codepage_id, const Nbaseutil::safestring& utf, Nbaseutil::safestring& text);

		/**
		* Extract and return next Unicode character from an UTF-8 stream. 
		* @param utf8 A string in UTF-8 encoding.
		* @param pos The byte position in the string where to start extracting the character. In output the pos will be updated to point to the next character to read (regardless of whether valid or invalid utf-8 sequence has been read).
		* @return The Unicode code point value, or 0 in case of an invalid UTF-8 sequence.
		*/
		DI_baseutil unsigned long ExtractUtfCharacter(const Nbaseutil::safestring& utf8, Nbaseutil::safestring::size_type& pos);

	} // namespace UtfTranslator

	/// A portable access() using UTF-8 strings.
	DI_baseutil int aca_access(const safestring& filename_utf8, int mode);

	/// A portable fopen() using UTF-8 strings.
	DI_baseutil FILE* aca_fopen(const safestring& filename_utf8, const safestring& mode);

	/// A portable remove() using UTF-8 strings.
	DI_baseutil int aca_remove(const safestring& filename_utf8);

	/// A portable rename() using UTF-8 strings.
	DI_baseutil int aca_rename(const safestring& oldname_utf8, const safestring& newname_utf8);

	/// A portable mkdir() using UTF-8 strings. Also creates intermediate directories.
	DI_baseutil int aca_mkdir(const safestring& path, unsigned int mode);

#ifdef _MSC_VER
	typedef struct ::__stat64 aca_stat_struc;
#else
	typedef struct ::stat aca_stat_struc;
#endif

	/// A portable stat() using UTF-8 strings.
	DI_baseutil int aca_stat(const safestring& pathname_utf8, aca_stat_struc& buf);

	/// A portable unlink() using UTF-8 strings.
	DI_baseutil int aca_unlink(const safestring& filename_utf8);

	/// A portable thread-safe gmtime().
	DI_baseutil struct ::tm aca_gmtime(time_t t);

	/// A portable thread-safe localtime().
	DI_baseutil struct ::tm aca_localtime(time_t t);

	/// Returns the last access time of the file. Throws in case of errors.
	DI_baseutil time_t aca_atime(const safestring& filename_utf8);

	/// Sets the last access time of the file. Throws in case of errors.
	DI_baseutil void aca_atime(const safestring& filename_utf8, time_t atime);

	/// Retrieves the total occupied size of a disk directory
	DI_baseutil uint64 aca_diskusage(const safestring& pathname_utf8, unsigned int flags);


} // namespace
#endif


