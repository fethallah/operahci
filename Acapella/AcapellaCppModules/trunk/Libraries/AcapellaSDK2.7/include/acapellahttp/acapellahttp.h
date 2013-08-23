#ifndef x_ACAPELLAHTTP_H_INCLUDED
#define x_ACAPELLAHTTP_H_INCLUDED

// This is the first exported header, disable some silly level 4 warning
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4100 4275 4251)
#endif
#include <set>

#include <memblock/DI_memblock.h>

#ifndef ACAPELLAVER
#error ACAPELLAVER not defined, include <baseutil/DI_baseutil.h> beforehand!
#endif

#if !defined(_MSC_VER) && !defined(DI_AcapellaHttp)
#define DI_AcapellaHttp
#endif


namespace NAcapellaHttp {

/**
* @brief Convert plain text to HTML. All HTML special characters are quoted according to the HTML rules.
* @param text Text buffer to convert.
* @param linefeeds If true, then linefeeds in text are converted to hard line breaks in HTML output (&lt;BR&gt;).
*/
DI_AcapellaHttp void Quote(Nbaseutil::safestring& text, bool linefeeds=false);

/// Convert HTML text to plain text. This is reverse of Quote() function.
DI_AcapellaHttp void Unquote(Nbaseutil::safestring& text);

/// Quotes the text to be included inside PHP '..' single quotes. Single quotes and a backslashes are escaped by prepending a backslash.
DI_AcapellaHttp void PhpSingleQuote(Nbaseutil::safestring& text);

/// Reverse of the PhpSingleQuote() function. Backslashes occuring before a single quote or a backslash are removed.
DI_AcapellaHttp void PhpSingleUnQuote(Nbaseutil::safestring& text);

/// Quote the text by the URL/HTTP encoding rules. All special characters are replaced by a percent and hexadecimal character code.
DI_AcapellaHttp void QuoteHttp(Nbaseutil::safestring& s);

/// Unquote the URL/HTTP-encoded text. This is reverse of QuoteHttp() function.
DI_AcapellaHttp void UnquoteHttp(Nbaseutil::safestring& s);


/// Bit flags for the redirected_channels parameter of ExecRedirected() function. One can compose these constants by the bitwise or '|' operator.
enum RedirectedChannels {
	/// Redirect STDIN channel.
	redirect_stdin = 1,
	/// Redirect STDOUT channel.
	redirect_stdout = 2,
	/// Redirect STDERR channel.
	redirect_stderr = 4,
	/// Redirect STDIN, STDOUT and STDERR channels.
	redirect_all = redirect_stdin|redirect_stdout|redirect_stderr,
};

/**
Execute a hidden process with stdin, stdout and stderr optionally redirected.
* @param cmdline The command line. This can start by a colon, in this case the path
*   of acapellahttp DLL is substituted for the colon. If colon is used, the (relative) path and
*   filename of the executable may not contain spaces. If colon is not used and the path or filename
*   contain spaces, then you need to surround them by double quotes on Windows platform.
*   The cmdline cannot contain redirection specs (<, >, 2>) if corresponding channels
*	are redirected by the redirected_channels parameter.
* @param input If stdin redirected, pass here the input data
* @param output If stdout redirected, here appears the stdout output of the program
* @param stderror If stderr redirected, here appears the stderr output of the program
* @param redirected_channels Specifes redirected channels, compose of enum flags.
* @param timeout Timeout in seconds for waiting for the program completion, 0=wait indefinitely.
* @param max_outputsize Maximum allowed size in bytes for the stdout or stderr output, if the program
*		produces more, an exception is thrown.
*/
DI_AcapellaHttp int ExecRedirected(const Nbaseutil::safestring& cmdline,
					const Nbaseutil::safestring& input,
					Nbaseutil::safestring& output,
					Nbaseutil::safestring& stderror,
					RedirectedChannels redirected_channels = redirect_all,
					int timeout=0,
					unsigned int max_outputsize=10*1000*1000
					);

/**
* Temporary files manager. One can use this class for managing temporary files and directories which will be automatically deleted.
* The files are deleted by destruction of the TmpFileNamer object. If the global object (see Instance() static member function)
* is used then this happens in the end of the program.
* If the program crashes, the files may remain on the disk. These will be automatically deleted by the Instance() member function later.
*/
class DI_AcapellaHttp TmpFileNamer: public NIMacro::TimerCallback {
public: // static interface

	/// Return the global TmpFileNamer. On first call, also deletes any temporary files accidentally left on the disk, which are more than 7 days old.
	static TmpFileNamer& Instance(bool alive=true);

public: // interface
	/// Create new TmpFileNamer (not usually needed)
	TmpFileNamer(const Nbaseutil::safestring& prefix = "acapella_");

	/**
	* Get a temporary filename.
	* @param suffix A string part to add in the end of filename.
	* @param extension Filename extension.
	* @param create_empty Create an empty file with that name so other programs know this name is taken.
	*/
	Nbaseutil::safestring GetName(const Nbaseutil::safestring& suffix, const Nbaseutil::safestring& extension = "tmp", bool create_empty=true);

	/// Registers an existing file or directory for automatic deletion.
	void RegisterName(const Nbaseutil::safestring& filename);

	/// Deletes all files with the filenames issued by this object.
	void DeleteIssuedNames();

	/// Calls DeleteIssuedNames().
	~TmpFileNamer();

private: // virtual overrides
	virtual void OnTimer() ACAPELLA_OVERRIDE;

private: // implementation
	void RegisterName_lk(const Nbaseutil::safestring& filename);

	void CleanupOld(int days);

	bool DeleteName(const Nbaseutil::safestring& name, Nbaseutil::safestring& errormsg, bool debug_diagnostics=true);

	const Nbaseutil::safestring& GetRegistryFile();

private: // data
	Nbaseutil::boost_mutex mx_;
	Nbaseutil::safestring dir_, process_id_, prefix_, registry_file_;
	bool needsquoting_;
	std::set<Nbaseutil::safestring> issued_names_;
};

DI_AcapellaHttp NIMacro::DataItem AcapellaHttp_DoVerb(const Nbaseutil::safestring& verb, NIMacro::DataItem arg1, NIMacro::DataItem arg2);

/// Return true if the argument is a DNS name or IP address (in dotted-decimal or IPv6 format) resolves to the local computer.
DI_AcapellaHttp bool IsLocalHost(const Nbaseutil::safestring& name_or_ip);

/// Normalizes a path name
DI_AcapellaHttp void MakeShortestPath(Nbaseutil::safestring& s);

// for back-compatibility, import some names from Nbaseutil:
using Nbaseutil::AcceptedSocket;
using Nbaseutil::SocketBase;
using Nbaseutil::ServerSocket;

} // namespace
#endif
