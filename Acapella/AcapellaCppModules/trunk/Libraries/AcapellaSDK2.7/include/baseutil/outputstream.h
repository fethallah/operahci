
#ifndef x_PKI_CT_BASEUTIL_OUTPUTSTREAM_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_OUTPUTSTREAM_H_INCLUDED_

#include "mb_malloc.h"
#include "carrier.h"
#include "safestring.h"
#include "utftranslator.h"

namespace Nbaseutil {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4251) // needs to have dll-interface to be used by clients of class
#endif

	class OutputStream;

	/// An abstract base class for using custom output functionality with OutputStream class.
	class CustomOutputBufferBase {
	public: // virtual interface
		/// Will be called from the OutputStream constructor.
		virtual void Init(OutputStream& os, const safestring& name, bool append) {}
		/// Will be called for performing the actual output.
		virtual void Write(const void* buffer, size_t size)=0;
		/// Will be called for performing the actual output. Note that the memory ownership is passed over. 
		virtual void Write(Carrier crr) {
			Write(crr.GetPointer(), crr.GetSize());
		}
		/// Will be called if OutputStream::Flush() is called.
		virtual void Flush() {}
		/// Will be called to close the stream. This is the last call to the streambuffer object.
		virtual void Close()=0;
		/// Reserved for future extensions.
		virtual void DoVerb(const char* verb, void* data) {}
		/// Virtual dtor, for any case
		virtual ~CustomOutputBufferBase() {}
	};

/**
* Encapsulates an output file or process
* When calling from an Acapella module, use NIMacro::OutputStreamEx instead
* for enhanced redirection support.
*/
class DI_baseutil OutputStream: public mb_malloced {
public: // static interface

	/// A static function, determines by filename if this corresponds to a regular disk file. An InputStream created by using this name would have <tt>regularfile</tt> mode.
	static bool IsRegularFile(const safestring& filename);

	/// Identifies different possible file types associated with an OutputStream.
	enum Mode_t {
		/// An ordinary disk file
		regularfile, 
		/// Associated with a console (STDOUT or STDERR).
		consolefile, 
		/// Associated with a child process where the data is piped to.
		process, 
		/// Linux only: identifies the parent process after fork(), in the detached child process regime.
		process_parent, 
		/// Linux only: identifies the child process after fork(), in the detached child process regime. In parent process any output sent to the OutputStream object is silently ignored.
		process_child, 
		/// Data is output in an Acapella datablock string item.
		stringitem, 
		/// Data is output in an Acapella datablock vector item.
		vectoritem, 
		/// OutputStream has been closed by calling the Close() member function.
		closed,
		/// A custom stream buffer is used for the actual output.
		custom,
	};

public: // interface
	/** OutputStream ctor.
	* @param name Can be specified in different formats given below, together with corresponding modes of the resulting OutputStream.
	*	- filename - output to a regular file: <tt>regularfile</tt>.
	*	- >>filename - append to a file, even if the 'append' parameter is false: <tt>regularfile</tt>.
	*	- filename.gz		- compress the output: <tt>regularfile</tt>.
	*	- "stdout"		- output to STDOUT channel: <tt>consolefile</tt>.
	*	- "stderr"		- output to STDERR channel: <tt>consolefile</tt>.
	*	- |command        - launch a process and pipe output to it: <tt>process</tt>.
	*	- |command&       - on Linux, create an detached child process by calling fork() before launching process (<tt>process_parent</tt> and <tt>process_child</tt>); on windows, same as the previous option (<tt>process</tt>).
	*	- >number			- write to an open file handle (OS file descriptor number, not FILE* pointer value).
	*	- <name			- writes into a memory buffer; call GetContentAndClear() to retrieve the buffer: <tt>stringitem</tt>.
	*	- <<name			- writes into a memory buffer; call GetContentAndClear() to retrieve the buffer: <tt>vectoritem</tt>.
	* @param append Append to the existing file, if possible. This regime is automatically switched on if the filename starts with ">>" prefix.
	* @param gzip Compress the output with gzip algorithm. This regime is automatically switched on if the filename ends with the ".gz" extension.
	* @param throw_exceptions In case of any problems in this or later operations, throw exceptions instead of returning error codes.
	* @param encoding Specifying non-default encoding will translate the whole file from UTF-8 to this codepage transparently. 
	*/
	OutputStream(const safestring& name, bool append=false, bool gzip=false, bool throw_exceptions=true, UtfTranslator::codepage_t encoding=UtfTranslator::cp_utf8);

	/**
	* The same as another overload, except it has a custom streambuffer pointer parameter.
	* @param custom_buffer Pointer to a custom streambuffer object, or NULL. The virtual methods of streambuffer will be called for performing the actual output.
	*/
	OutputStream(CustomOutputBufferBase* custom_buffer, const safestring& name, bool append=false, bool gzip=false, bool throw_exceptions=true, UtfTranslator::codepage_t encoding=UtfTranslator::cp_utf8);

	/// Duplicates the corresponding ctor parameter in a more explicit way.
	void SetThrowExceptions(bool throw_exceptions=true) {throw_exceptions_=throw_exceptions;}

	/// Emulate fprintf C function. Not type-safe, do not use. Use Nbaseutil::Printf instead.
	int fprintf(const char* format, ...);

	/// Emulate fwrite
	size_t fwrite(const void *buffer, size_t size, size_t count);

	/// Write a memory block to the stream. Note that the memory ownership is transferred to the stream. This function always throws in case of errors.
	void Write(Carrier crr);

	/// Write a string to the output stream.
	void operator<<(const safestring& s) {
		fwrite(&s[0], 1, s.length());
	}

	/// Emulate fflush()
	int flush();

	/// If applicable, prepare the stream for outputting yet more n bytes
	void Reserve(unsigned int n);

	/// Constants for using in setmode() member function.
	enum os_filemode_t {
		os_filemode_text,
		os_filemode_binary
	};

	/** In Windows: set the file OS mode to text or binary. 
	* This is not related to the OutputStream::Mode_t.
	* Has only effect for regular files and stdin.
	* In text mode the CR bytes are automatically stripped off from the read content, in Windows environment.
	* By default all files are opened in binary mode.
	* In Linux this function does nothing and returns os_filemode_binary.
	* @return The previous file OS mode, either OutputStream::os_filemode_text or OutputStream::os_filemode_binary.
	* @param mode Either OutputStream::os_filemode_text or OutputStream::os_filemode_binary.
	*/
	os_filemode_t setmode(os_filemode_t mode);

	/// Return OutputStream mode.
	Mode_t GetMode() const {return mode_;}

	/// Return true if the gzip regime is in force.
	bool IsZipped() const {return gzip_;}

	/**
	* Returns true if this is parent process of fork. 
	* Any output is ignored in the parent process of fork. 
	* By using this function the caller may check for this condition immediately after creating the OutputStream
	* and skip the output generation part of the code.
	*/
	bool IsParentProcess() const {return mode_ == process_parent;}

	/** 
	* Close the stream. This is needed only if you want to close the stream before object destructor.
	* After Close(), GetMode() returns closed. No other member function may be called after that point.
	*/
	void Close();

	/// Cleanup. If this is the child of fork(), the process is terminated here.
	~OutputStream();

	/// Only when mode is stringitem or vectoritem: call this function to get the output result.
	Carrier GetContentAndClear();

	/**
	* Only when mode is stringitem or vectoritem: call this function to get the output result
	* as a string. This is slower than the other GetContentAndClear() (makes copy).
	*/
	void GetContentAndClear(safestring& content);

	/// In case of <tt>stringitem</tt> or <tt>vectoritem</tt> OutputStream, returns the Acapella item name where the content will be available.
	const safestring& GetName() const {return itemname_;}

	/// Encapsulates ferror().
	bool error() const;

	/// Returns error message, in case of error occurred.
	safestring strerror() const;

	/**
	* Return a filehandle the user can write to instead using OutputStream methods.
	* In case of gzipped output and stringitem/vectoritem output a temporary file is created, which is naturally less efficient than native output.
	* The temporary file is not synced with this object, i.e. the user should not use OutputStream output functions after calling GetFileHandle().
	*/
	FILE* GetFileHandle();

	/// Return pointer to the memory buffer for stringitem and vectoritem types.
	void* Buffer(unsigned int& size) const;

private: // data
	union {
		FILE* f_;
		CustomOutputBufferBase* custom_buf_;
	};
	Mode_t mode_;
	bool gzip_;
	bool throw_exceptions_;
	safestring itemname_;
	Carrier* content_;
	unsigned int content_capacity_; // the allocated length of content_ block.
	OutputStream* proxy_;
	UtfTranslator::codepage_t encoding_;
private: // implementation
	void Init(CustomOutputBufferBase* custom_buf, const safestring& name0, bool append);
	OutputStream(const OutputStream&); // not implemented
	void operator=(const OutputStream&); // not implemented
};

#ifdef _MSC_VER
#pragma warning(pop)
#endif


} // namespace Nbaseutil
#endif
