
#ifndef x_PKI_CT_BASEUTIL_INPUTSTREAM_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_INPUTSTREAM_H_INCLUDED_

#include "mb_malloc.h"
#include "carrier.h"
#include "safestring.h"
#include "printfer.h"
#include "utftranslator.h"

struct stat;

namespace Nbaseutil {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // needs to have dll-interface to be used by clients of class
#endif


/**
* Encapsulates an input data provider (file, pipe, memory area, etc.)
* When calling from an Acapella module, use InputStreamEx instead
* for enhanced redirection support (Acapella datablock items and Executive overrides).
*
* The most useful way is usually to read the file is to call the GetContent() member function
* and get the whole file content in one call.
*/
class DI_baseutil InputStream: public mb_malloced {
public:
	/// Identifies different possible file types associated with an InputStream.
	enum Mode_t {
		/// An ordinary disk file.
		regularfile, 
		/// Non-seekable pseudofile associated with a console (STDIN).
		consolefile, 
		/// The data is pumped from/to another process via a pipe.
		process, 
		/// The data is present in a memory area inside the same process.
		memorychunk, 
		/// An ordinary disk file, in gzipped format which will be automatically (de)compressed.
		gzfile, 
		/// The stream has been closed.
		closed,
		/// Used by InitData mechanism for autodetection purposes
		init_autodetect,
		/// An HTTP connection
		http_connection,
		/// The object ensapsulates a Windows file handle. This is for internal use, GetMode() returns regularfile for this case as well.
		windows_handle,
	};
public: // interface
	/**
	* Open a new input stream. 
	*
	* Windows-specific: All files are opened in binary mode (including "stdin"). If you want text mode, use the setmode()
	* member function. Rationale: most of files are binary; DOS text files open in Linux version would need specific processing anyway.
	*
	* @param filename The actual filename or a pseudo-name in one of the following formats:
	* Allowed names and corresponding InputStream types are: 
	*   - actual filenames accepted by fopen(): <tt>regularfile</tt>
	*   - <tt>"stdin"</tt> (alias <tt>"-"</tt>): <tt>consolefile</tt>
	*   - shell pipeline followed by a vertical bar <tt>'|'</tt>: <tt>process</tt>
	*   - memory region in angle brackets notation <tt>"<addr,length>"</tt>: <tt>memorychunk</tt>
	*   - memory region in angle brackets notation <tt>"<addr,length,freeproc>"</tt>: <tt>memorychunk</tt>
	*   - memory region in angle brackets notation <tt>"<addr,length,freeproc,callbackdata>"</tt>: <tt>memorychunk</tt>
	* If the memory region syntax contains <tt>freeproc</tt> and field, then this must 
	* specify the address of a function with signature <tt>void (*freeproc)(void* addr)</tt>
	* which is called upon destruction of InputStream, in order to release the memory.
	* If also the <tt>callbackdata</tt>field is present, then it is converted to a pointer
	* value and passed back to the release function whose signature must be in this case
	* <tt>void (*freeproc)(void* addr, void* callbackdata)</tt>.
    * 
	* @param autounzip 	In case of an actual disk file, if the file begins with the gzip magic header (0x1f, 0x8b) and autounzip parameter is true,
	* the file is open via the zlib library and automatically unzipped. In this case the seeking is supported only
	* partially (see is_seek_supported_partially()). Note that the GetContent() member function supports autounzip also
	* for other stream types. If the file is opened through zlib library, GetMode() returns gzfile.
	*
	* @param throw_exceptions In case of any problems when accessing the InputStream throw an Nbaseutil::Exception instead of returning error code.
	* @param encoding Specifying non-default encoding will translate the whole file from this codepage into UTF-8 transparently. 
	*/
	InputStream(const safestring& filename, bool autounzip=true, bool throw_exceptions=true, UtfTranslator::codepage_t encoding=UtfTranslator::cp_utf8);

	/// An helper class for using with InputStream advanced ctor.
	class InitData {
	public:
		/// Construct InitData for existing memory chunk. Filename is strictly for informational purpose only.
		InitData(const safestring& filename, const Carrier& crr)
			: filename_(filename)
			, mode_(InputStream::memorychunk)
			, crr_(crr)
		{}
		/// Construct InitData for effectively falling back to the common InputStream ctor. The filename is parsed as described in InputStream ctor.
		InitData(const safestring& filename)
			: filename_(filename)
			, mode_(InputStream::init_autodetect)
		{}
	private:
		friend class InputStream;
		safestring filename_;
		Mode_t mode_;
		Carrier crr_;
	};

	/// Advanced constructor; constructs an InputStream from the data passed by init_data structure.
	InputStream(const InitData& init_data, bool autounzip=true, bool throw_exceptions=false);

	/// Duplicates the corresponding ctor parameter in a more explicit way.
	void SetThrowExceptions(bool throw_exceptions=true) {throw_exceptions_=throw_exceptions;}

	/// Constants for using in setmode() member function.
	enum os_filemode_t {
		os_filemode_text,
		os_filemode_binary
	};

	/** In Windows: set the file OS mode to text or binary. 
	* This is not related to the InputStream::Mode_t.
	* Has only effect for regular files and stdin.
	* In text mode the CR bytes are automatically stripped off from the read content, in Windows environment.
	* By default all files are opened in binary mode.
	* In Linux this function does nothing and returns os_filemode_binary.
	* @return The previous file OS mode, either os_filemode_text or os_filemode_binary.
	* @param mode Either os_filemode_text or os_filemode_binary.
	*/
	os_filemode_t setmode(os_filemode_t mode);

	/// Return type of the input stream. This has nothing to do with setmode().
	Mode_t GetMode() const {return mode_==windows_handle? regularfile: mode_;}

	/// Return the filename passed to ctor.
	safestring GetFilename() const;

	/// Dtor closes the actual input file. In case of memory region, the memory may get released as dscribed in the ctor documentation.
	~InputStream();

	/**
	* Read in the whole file and return it. 
	*  If autounzip=true, then any type of file is automatically unzipped in the case it was gzipped.
	*  The parameter has no effect if file has already been opened in gzfile mode.
	*/
	Carrier GetContent(bool autounzip=true); 

	/**
	* File offset type (64-bit); use InputStream::off_t type in the code for better portability.
	*/
	typedef int64 off_t;

	// Emulate low-level file functions, which will work with gzipped files as well.
	
	/**
	* Returns true if seeking and file length determining are supported fully by the underlying input data source.
	* Note that this function returns false for gzipped files, "stdin" and pipelines.
	*/
	bool is_seek_supported_fully() const;

	/**
	* Returns true if seeking is supported partially. In this case one can seek only
	* with origin=SEEK_CUR, and only onwards. Partial seeking is currently supported for all modes.
	*/
	bool is_seek_supported_partially() const;

	/**
	* Seek in the filestream. If the stream does not support seeking at all, returns 0. If the stream supports seeking only
	* partially (see is_seek_supported_partially()) and origin=SEEK_END or SEEK_SET, the behaviour is undefined.
	*
	* Seeking with negative offsets is not supported.
	*/
	off_t seek(off_t offset, int origin);

	/// Return the file length. If is_seek_supported_fully()==false, returns 0.
	off_t filelength() const;

	/// Read up to count bytes into the buffer. Return number of bytes actually read.
	size_t read(void *buf, size_t count);

	/// Check if stream is at eof().
	bool eof() const;

	/**
	* Read a line into the buffer; returns false if stream was at eof. 
	* The LF character (\\n) is included in the end of the buffer, unless file end 
	* has been reached and ther is no newline in the end of file. If the file is 
	* in DOS format, then a CR (\\r) character will be also present before LF,
	* unless the InputStream has been changed into text regime via the setmode() 
	* member function call (this works only on Windows). You can use apply_trim()
	* function to strip off any trailing CR/LF characters from the buffer.
	*/
	bool gets(safestring& buffer); 

	/// Encapsulates ferror(). Returns true if an input-output error has occured earlier.
	bool error() const;

	/// Returns the error message, in case of error occurred.
	safestring strerror() const;

	/** 
	* Close the stream. This is needed only if you want to close the stream before object destructor.
	* After Close(), GetMode() returns closed. No other member function may be called after Close().
	*/
	void Close();
private: // data
	FILE* f_;
	Mode_t mode_;
	Carrier crr_; 
	safestring name_;	// remembered file name
	UtfTranslator::codepage_t encoding_;
	bool throw_exceptions_;
private: // implementation
	InputStream(const InputStream&); // not implemented
	void operator=(const InputStream&); // not implemented
	void ParseFileName(bool autounzip); // parse the filename and initialize the object.
	void FillCarrierIfNeeded();
};

/// Return an open file length by the OS file descriptor. The file must be seekable.
DI_baseutil int64 filelength(int file_descriptor);

/// A virtual base class for deriving classes to be passed to SetLoggerCallbackBase().
class LogBase {
public:
	enum subsys_t {
		// Mask containing no subsystems
		subsys_none = 0,
		// Mask containing all subsystems
		subsys_all = 0xffffffff,
		// Log file operations
		subsys_file=1<<0,
		// Log socket operations
		subsys_socket=1<<1,
		// Log memory operations
		subsys_memory=1<<2,
		// Log exception handling operations
		subsys_exception=1<<3,
	};

	/**
	* The callback function called when a message is logged and has passed level and subsys checks. 
	* It may be called from multiple threads in the same time.
	* It is not allowed to call SetLoggerCallbackBase() from inside the Log() function implemtation.
	* @param subys The subsystem loggign the message.
	* @param level The message verbosity level 1..9.
	* @param info Message text.
	*/
	virtual void Log(subsys_t subsys, int level, const safestring& info)=0;

	virtual ~LogBase() {}
};

/**
* Install a new logger callback class. The pointed object must be alive until program end or until replaced.
* @param loggercallback Pointer to the callback object of a class derived from LoggerCallbackBase. Pass NULL to switch logging off.
* @return Pointer to the previous callback object.
*/ 
DI_baseutil LogBase* SetLoggerCallbackBase(LogBase* loggercallback);

/// Change logger filter settings. Only log messages passing the subsystem mask and max log level filters are passed to the logger callback object.
DI_baseutil void SetLoggerFilter(LogBase::subsys_t subsys_mask=LogBase::subsys_all, int max_logged_level=4);

#ifdef _MSC_VER
#pragma warning(pop)
#endif

} // namespace Nbaseutil
#endif
