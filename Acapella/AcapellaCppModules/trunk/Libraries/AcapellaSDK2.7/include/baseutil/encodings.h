#ifndef x_ACAPELLA_BASEUTIL_ENCODINGS_H_INCLUDED
#define x_ACAPELLA_BASEUTIL_ENCODINGS_H_INCLUDED

#include "carrier.h"

namespace Nbaseutil {

	/// Compress data by the zlib algorithm. Uses zlib library. The zlib format differs from gzip by metadata headers.
	DI_baseutil Carrier Encode_zlib(const void* data, size_t data_size);

	/// Uncompress data in zlib format. Uses zlib library. The zlib format differs from gzip by metadata headers.
	DI_baseutil Carrier Decode_zlib(const void* data, size_t data_size);

	/// Compress data by the gzip algorithm. Uses zlib library. The zlib format differs from gzip by metadata headers.
	DI_baseutil Carrier Encode_gzip(const void* data, size_t data_size);

	/// Uncompress data in gzip format. Uses zlib library. The zlib format differs from gzip by metadata headers.
	DI_baseutil Carrier Decode_gzip(const void* data, size_t data_size);

	/// Encode data by MIME Base64 algorithm. Uses string-encoders library by Nick Galbreath.
	DI_baseutil Carrier Encode_base64(const void* data, size_t data_size);

	/// Decode data encoded by MIME Base64 algorithm. Uses string-encoders library by Nick Galbreath.
	DI_baseutil Carrier Decode_base64(const void* data, size_t data_size);

	/**
	* Encode XML special entities: <>&"'. These are replaced by relevant ampersand-constructions.
	* @param src The input string.
	* @param srclen The length of the input string.
	* @param buffer The encoded content is appended to the buffer.
	*/
	DI_baseutil void EncodeXmlEntities(const char* src, int srclen, safestring& buffer);

	/// Reverse of EncodeXmlEntities().
	DI_baseutil void DecodeXmlEntities(const char* src, int srclen, safestring& buffer);

} // namespace

#endif
