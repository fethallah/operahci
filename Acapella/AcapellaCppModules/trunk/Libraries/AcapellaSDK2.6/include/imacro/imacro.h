#ifndef _IMACRO_H_INCLUDED_
#define _IMACRO_H_INCLUDED_

#include "script.h"
#include "instance.h"

#ifndef ACAPELLAVER
#error ACAPELLAVER not defined, include <baseutil/DI_baseutil.h> beforehand.
#endif

#ifndef _WINDOWS_
#ifdef _MSC_VER
struct HKEY__;
typedef struct HKEY__ *HKEY;
#else
// bugfix 22.10.2003; HKEY defined in gcc.h for Linux, in NIMacro namespace; deleted the global def here.
#endif
#endif


// Doxygen title page
/*! \mainpage Acapella SDK Reference

* \section intro_sec Introduction

PerkinElmer Acapella (TM) SDK contains 5 separate libraries, which are provided as DLL-s. The libraries are described in order of their
dependencies.

The errors in Acapella are mostly reported by throwing exceptions, mostly of type Nbaseutil::Exception.
Some error conditions are normal in certain situations, e.g. a missing container subitem
during a syntax check run. In order to avoid expensive throws and more importantly, to avoid
false alarms in debugger, there are lot of functions which take a Nbaseutil::ThrowHandler
argument. The client can pass a non-throwing handler (Nbaseutil::DoNothing or
Nbaseutil::ThrowNever) to avoid throws.

 * \section baseutil_sec Baseutil library

 The baseutil library provides tha basic utilities needed for anything else. It can be also used by its own
 in other projects not related to Acapella. The following classes are of special importance:

 - Nbaseutil::safestring: a specialization of std::basic_string<char> with case-insensitive comparisons and independent memory mangagement.
	The safestring class is extensively used throughout Acapella.

 - Nbaseutil::Exception: an exception class derived from std::exception. It encapsulates an error message and error code. The Exception class
	is used throughout Acapella for reporting error conditions.

 - Nbaseutil::InputStream: a class abstracting an input file, with extended functionality. Acapella modules typically use InputStreamEx class
	derived from it.

 - Nbaseutil::OutputStream: a class abstracting an output file, with extended functionality. Acapella modules typically use OutputStreamEx class
	derived from it.

 - Nbaseutil::printfer class and Printf() function. These provide typesafe alternative to the C sprintf() function and are commonly used by constructing
	error messages and other texts.

 The baseutil library also defines many convienience functions for string manipulation, described in the safestring.h header file.

* \section memblock_sec MemBlock library

The MemBlock library provides most of the data types used in Acapella. Especially see the following classes:

 - NIMacro::DataItem (and NIMacro::SafeValue, which is almost the same): these provide value objects which can encapsulate a data item of
	several types: either number, string or a memblock hierarchy object.

 - NIMacro::MemBlock class hierarchy with all the derived classes,
	including NIMacro::Vector and NIMacro::Image classes.

* \section acapellattp_sec AcapellaHttp library

The Acapella Http library contains an implementation of a builtin HTTP server and related utilities. Especially see the following classes:

 - NAcapellaHttp::HtmlCmdHandler: this is a base class for all HTTP request handlers. One can add new handlers easily. There are
    also specific base classes for helping to create binary and XML handlers.

- NAcapellaHttp::XmlNode: this is a MemBlock hierarchy class for holding a reference to an XML element, document, text or declaration
    inside an XML DOM tree.

- NAcapellaHttp::WormHole: this is a MemBlock hiareachy class implementing a inter-thread/inter-process communication channel. Acapella structures can
	be pumped through a wormhole to another thread, process or computer.

\section imacro_sec IMacro library

The IMacro.dll library implements the language interpretator engine
for the IMacro scripting language. It makes heavy use of the data
structure classes implemented in MemBlock.dll library.

IMacro library SDK can be roughly split into two parts:

 - SDK for module library developers - see NIMacro::Mod class.
 - SDK for client applications, who want to execute Acapella scripts - see NIMacro::IMacro, NIMacro::Instance, NIMacro::Script and NIMacro::DataBlock classes.

\section cells_sec Cells library

This is an Acapella module library providing an advanced NIMacro::MemBlock hieararchy class - NIMacro::Cells. An object of this class
holds an so-called "object list", which is essentially a table containing different attributes about the objects
as well as their shape and position in the analysed images. This library is needed only for module libraries working with the
object lists.

*/

/*

- C-style - see runmacro*() functions declared in this header.
  This is functionally quite limited.

- C++ style. See Instance, Script and DataBlock classes, declared
  in the accordingly named headers. This is the preferred style of invocation.

Example: calculate 2*x, where x is 3.5.
@code
#include <acapella/DI_acapellaR.h>
using namespace NIMacro;
...
double x = 3.5;
double y = CallAcapella("set(b=2*a)")("a", x)("b").GetFloating();
@endcode

The same can be done in a hard way, which is sometimes more functional:

@code
#include <acapella/DI_acapella.h>
using namespace NIMacro;
...
PInstance inst = Instance::Create();
Executive exec;
PScript scr = Script::Create(exec, "Test", "set(y=2*x)");
PDataBlock db = DataBlock::Create();
db->SetItem("x", 3.5);
inst->SetScript(scr);
inst->SetDataBlock(db);
inst->Run(exec);
double y = db->GetDouble("y");
@endcode

There is also a COM interface present implemented by a separate DLL.
*/

namespace NAcapellaHttp {
	class HtmlCmdHandler;
}

namespace NIMacro {


/**
* A convenience class for calling IMacro from C++ code. NIMacro::Exception's
* are thrown in case of any errors. See CallAcapella() function.
*/
class DI_IMacro IMacro: public mb_malloced {
public:
	/// Construct by the script text.
	IMacro(const safestring& scripttext, const safestring& scriptname="{Acapella inline script}");

	/// Construct by a script file.
	IMacro(InputStream& scriptfile);

	~IMacro();

	/// Set an input data item.
	void Set(const safestring& itemname, const DataItem& item);

	/// Set input data by operator(). See also CallAcapella() function.
	IMacro& operator()(const safestring& itemname, const DataItem& item) {Set(itemname, item); return *this;}

	/// Run the script. Return true if no warnings occur.
	bool Run();

	/// Run the script under the control of an executive object. Return true if no warnings occur.
	bool Run(Executive& exec);

	/// Run the script under the control of an parent execution context. Return true if no warnings occur.
	bool Run(ExecutionContext& parent_ctx);

	/// Run the script under executive control. Returns reference to itself, for operator chaining.See also CallIMacro() function.
	IMacro& operator()(Executive& exec) {Run(exec); return *this;}

	/// Run the script under the control of an parent execution context. Returns reference to itself, for operator chaining.
	IMacro& operator()(ExecutionContext& ctx) {Run(ctx); return *this;}

	/// Return error/warning messages from the last run.
	safestring GetWarn() const;

	/// Fetch output data after script run. Script is run automatically if is has not yet been run. If there is no such data item, an Undefined DataItem is returned.
	DataItem Get(const safestring& itemname) const;

	/// Fetch output data after script run. Script is run automatically if is has not yet been run. If there is no such data item, an Undefined DataItem is returned.
	DataItem operator()(const safestring& itemname);

	/// Check if an output item exists.
	bool Exists(const safestring& itemname) const;

	/// Delete all existing data items. Needed only if one wants to make a new run in a clear data envirionment.
	void Clear();

protected:
	PScript script_;
	PInstance inst_;
	PDataBlock input_, output_;
};

/**
* Same as CallAcapella(), retained for backward compatibility.
*/
inline IMacro CallIMacro(const safestring& scripttext, const safestring& scriptname="{Acapella inline script}") {
	return IMacro(scripttext, scriptname);
}

/**
* Convenience function for constructing an IMacro object. Example:
* double y = CallAcapella("set(y=2*x)")("x", 2.5).Get("y").GetDouble();
*/
inline IMacro CallAcapella(const safestring& scripttext, const safestring& scriptname="{Acapella inline script}") {
	return IMacro(scripttext, scriptname);
}



/**
* A convenience class for calling IMacro from C++ code
* without bothering about exception catching. One must instead
* bother about error return checking.
*/
class DI_IMacro IMacroNoThrow: private IMacro {
	typedef IMacro super;
public:
	IMacroNoThrow() throw();

	/// Init by the script text.
	bool Init(const safestring& scripttext, const safestring& scriptname="{Acapella inline script}") throw();

	/// Init by a script file.
	bool Init(InputStream& scriptfile) throw();

	/// Set input data
	bool Set(const safestring& itemname, const DataItem& item) throw();

	/// Run the script. Return true if no errors occur.
	bool Run() throw();

	/// Run the script under the control of an executive object. Return true if no errors occur.
	bool Run(Executive& exec) throw();

	/// Run the script under the control of an parent execution context. Return true if no warnings occur.
	bool Run(ExecutionContext& parent_ctx) throw();

	bool HasWarnings() const throw();

	bool HasErrors() const throw();

	/// Return error/warning messages from the last Run() or Init().
	safestring GetErrWarn() const throw();

	/// Return error/warning code composed of ERR/WARN flags defined in MemBlock/modreg.h
	unsigned int GetErrWarnCode() const throw();

	/// Fetch output data after Run().
	DataItem Get(const safestring& itemname) const throw();

	/// Check if an output item exists.
	bool Exists(const safestring& itemname) const throw();
private:
	mutable safestring errwarn_;
	mutable unsigned int errwarncode_;
};

/**
* Composes a module description and returns it as a C++ string.
* In case of errors returns error message,
* beginning with the substring "<Error:".
*
* @param ModuleName Name of the module. Cannot contain wildcards.
* @param FormatMode The output format: 0=ASCII, 1=LaTeX, 2=RTF, 3=HTML.
*/
DI_IMacro safestring DescribeModule(const safestring& ModuleName, int FormatMode);


/**
* Format the error as a XML string.
*/
DI_IMacro void FormatXmlException(ExecutionContext& ctx, safestring& xml, unsigned int errcode,
						const safestring& errmsg0, const safestring& filename, int line, const safestring& modulename, const safestring& nested_xml);

/**
* Find a file or directory in or relative to Acapella StdLib directories.
* Windows shortcuts are resolved. The initial filename can contain ".." path elements
* for accessing directories outside of StdLib.
* If the file is not found relative to any StdLib directories, an empty string is returned.
*/
DI_IMacro safestring FindFileInStdLibs(const Nbaseutil::safestring& properfilename);

/// File globbing interface
class DI_IMacro Globber: public mb_malloced {
public: // interface
	Globber(const safestring& filemask);
	int Size();
	bool Get(int index, safestring& filename);
private:
	PVector hits_;
};

/**
* This function takes a err_warn type value as a parameter and returns a string
* which contains description for all raised bits in the error code. This function 
* is maintained only for backward compatibility.
*/
DI_IMacro Nbaseutil::safestring AnalyzeErrorCode(unsigned int error_code);

/**
* If logging is turned on and sent to a file (see Logger::SetLogFile()), 
* this function returns some trailing rows from the end of the current log file.
*/
DI_IMacro Nbaseutil::safestring LogFileTail();


/**
* Checks script syntax by running the script in syntax-check mode. Returns false in case of errors.
* @param scripttext Script text to check
* @param errors Output parameter - error and warning messages.
* @param errorcode Output parameter - code composed of error and warning flags.
*/
DI_IMacro bool CheckSyntax(const safestring& scripttext, safestring& errors, unsigned int& errorcode);

/// Obsolete, use an Acapella script containing a Modules() module call instead.
DI_IMacro int ListModules(Nbaseutil::safestring& buffer);

/// Deprecated; forwards to Logger::SetLogFile(), use the latter in new code.
DI_IMacro int SetLogFilePath(const char* lpszPathName);

/// Deprecated; forwards to Logger::Log(), use the latter directly instead.
DI_IMacro int LogString(const char* lpszString);


#ifndef CALLBACK
#	ifdef _MSC_VER
#		define CALLBACK    __stdcall
#	else
#		define CALLBACK
#	endif
#endif

#ifndef STDCALLFUNC
#	ifdef __GNUC__
#		define STDCALLFUNC __attribute__ ((stdcall))
#	else
#		define STDCALLFUNC
#	endif
#endif


// For parsing the macro string and extracting parameter names/values.
// The info is sent back to the caller via a callback function.
// Callback function prototype:
#if	defined(_MSC_VER)
typedef int (__stdcall *PFNLVPARSE1)(unsigned int nTag, const char* lpszTagVal, iptr_t lParam);
#elif defined(__GNUC__)
typedef int (*PFNLVPARSE1)(unsigned int nTag, const char* lpszTagVal, iptr_t lParam); // __attribute__ ((stdcall));
#else
typedef int (*PFNLVPARSE1)(unsigned int nTag, const char* lpszTagVal, iptr_t lParam);
#endif

/// Deprecated, use the NIMacro::Script class instead.
DI_IMacro unsigned int parse(PFNLVPARSE1 pfnParse, const char* lpszMacro, Nbaseutil::iptr_t lParam);


/// Deprecated, use the NIMacro::Script class instead.
DI_IMacro unsigned int parse_ex(PFNLVPARSE1 pfnParse, const char* lpszMacro, Nbaseutil::iptr_t lParam, safestring& errmsg);

class Mod;

/// A typedef for using with RegisterClassModule() and RegisterModule().
typedef Mod* (*ModCreatorFunc)();
/// A typedef for using with RegisterModule().
typedef Mod* (*ModCreatorFunc2)(Module& caller, PTextHolder callback_data);

class Module;


/**
* This function has been superseded by RegisterModule(), which drops some 
* obsolete parameters and adds parameter for extra module flags. It is 
* suggested to use RegisterModule() in new code.
*
* This function registers a regular C++ Acapella module by the Acapella runtime engine. 
* This function is typically called from the 
* extern "C" ExportedModules() function in an Acapella module library.
* This function is called by Acapella when loading the library.
*
* This function registers a C++ module derived from the NIMacro::Mod class, 
* implemented in the library. Alternatively, the ExportedModules() function 
* can return a pointer to a static array containing the names and creator function 
* pointers to the registered modules.
*
* @param name Name of the module. For modules inside a package/namespace this has to be in the format "PACKAGENAME::PROPERNAME". 
*			The same module library can contain modules in different namespaces. If that is the case, it should be installed in 
*			the StdLib top-level directory. If the module library contains only modules inside a specific package, it could also
*			be installed in a StdLib subdir having the same name as the package.
* @param pfCreator A pointer to a function returning a pointer to a new module object, allocated by 'new'. 
*			This is typically a static Create() function in the module class.
*			Acapella will call this function for creating module objects when necessary.
*			The function is typically not called immediately.
* @param dllname Obsolete, ignored. Pass "" here.
* @param dllhandle Obsolete, ignored. Pass NULL here.
* @return A pointer to the registered internal Module object, corresponding to the module. 
*			This is effectively an opaque handle as Module class is not exported from the IMacro library.
*			The pointer may become invalidated unexpectedly, so it should not be stored for later use.
*/
DI_IMacro Module* RegisterClassModule(const safestring& name, ModCreatorFunc pfCreator, const char* dllname="", hmodule_t dllhandle=NULL);


/**
* A newer version of RegisterClassModule(), provides a more detailed interface and allows 
* for passing initial module flags like MOD_INJECTED.
*
* This function registers a C++ module derived from NIMacro::Mod by Acapella. This function 
* is typically called from the extern "C" ExportedModules() function in an Acapella module library.
* The ExportedModules() function is called by Acapella when loading the library.
*
* @param packagename The package aka namespace name. For modules in the global namespace pass an empty string here.
*			The same module library can contain modules in different namespaces. If that is the case, it should be installed in 
*			the StdLib top-level directory. If the module library contains only modules inside a specific package, it could also
*			be installed in a StdLib subdir having the same name as the package.
* @param propermodulename Proper name of the module, not containing any colons.
* @param pfCreator A pointer to a function returning a pointer to a new module object, allocated by 'new'. 
*			This is typically a static Create() function in the module class.
*			Acapella will call this function for creating module objects when necessary.
* @param initial_flags Initial module flags, which must be known to Acapella before creating and declaring the module object.
*			The following module flags have to be specified here:
*			  - MOD_HIDDEN
*			  - MOD_EXTRA_PARSING
*			  - MOD_INJECTED
*			The same module flags have to be present also in later Mod::module() function call in the module's overridden Declare() method.
* @return A pointer to the registered internal Module object, corresponding to the module. 
*			This is effectively an opaque handle as Module class is not exported from the IMacro library.
*			The pointer may become invalidated unexpectedly, so it should not be stored for later use.
*/
DI_IMacro Module* RegisterModule(const safestring& packagename, const safestring& propermodulename, ModCreatorFunc pfCreator, unsigned int initial_flags=0);

/// Used internally for registering Acapella procedures.
DI_IMacro Module* RegisterModule(const safestring& packagename, const safestring& propermodulename, ModCreatorFunc2 pfCreator, PTextHolder callback_data, unsigned int initial_flags=0);

/// Superseded by AcapellaInit("regbranch", ...) and AcapellaInit("regbranchver", ...), use those in new code.
DI_IMacro void SetAcapellaRegBranch(bool versioned, const safestring& regbranch);

/**
* Returns the Acapella registry branch in force, e.g. "Software\\PerkinElmerCTG\\Acapella\\21". 
* The value can be changed by AcapellaInit("regbranch", ...) or AcapellaInit("regbranchver", ...).
* @param versioned Return the "versioned" registry branch, where version-specific settings are stored. It is suggested to use only version-specific branch.
*				If false, the "common" registry branch is returned, supposed to be shared by all Acapella installations.
*/
DI_IMacro safestring GetAcapellaRegBranch(bool versioned=true);

#ifndef ACAPELLA_PLATFORM_WINDOWS
/// A typedef for registry branch values.
typedef void* HKEY;
#endif 

/**
* Return a value from Windows registry. On Linux, the HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE branches are emulated in the ~/.imacrorc file.
* @param reg_branch Registry branch. This should be either HKEY_CURRENT_USER (0x80000001) or HKEY_LOCAL_MACHINE (0x80000002).
* @param key The key name path in the registry. This can contain either slashes or backslashes.
* @param name The value name in the registry key. Pass empty string to retrieve the default value of the key.
* @return The registry value, converted to string. If the key or value is not existing, not accessible or not convertible, an empty string is returned.
*/
DI_IMacro safestring GetRegKey(const HKEY reg_branch, const safestring& key, const safestring& name);

/**
* Stores a string value in the Windows registry. On Linux, the HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE branches are emulated in the ~/.imacrorc file.
* @param reg_branch Registry branch. This should be either HKEY_CURRENT_USER (0x80000001) or HKEY_LOCAL_MACHINE (0x80000002).
* @param key The key name path in the registry. This can contain either slashes or backslashes. Any intermediate key nodes are created as needed.
* @param name The value name in the registry key. Pass empty string to access the default value of the key.
* @param sz_value The string to store in the value.
* @return True in case of success, false in case of failure.
*/
DI_IMacro bool SetRegKey(const HKEY reg_branch, const safestring& key, const safestring& name, const safestring& sz_value);

/**
* Stores an unsigned 32-bit integer value in the Windows registry. On Linux, the HKEY_CURRENT_USER and HKEY_LOCAL_MACHINE branches are emulated in the ~/.imacrorc file.
* This functions is only needed for interacting with non-Acapella software, as Acapella software always sees registry values as strings.
* @param reg_branch Registry branch. This should be either HKEY_CURRENT_USER (0x80000001) or HKEY_LOCAL_MACHINE (0x80000002).
* @param key The key name path in the registry. This can contain either slashes or backslashes. Any intermediate key nodes are created as needed.
* @param name The value name in the registry key. Empty string is not allowed (default key value is of string type always).
* @param dword_value The value to store.
*/
DI_IMacro bool SetRegKey(const HKEY reg_branch, const safestring& key, const safestring& name, unsigned int dword_value);

/**
* Return a registry key value from Acapella registry branch (see GetAcapellaRegBranch()). First HKCU is searched, then HKLM.
* @param subkey_name A subkey name in the Acapella registry branch, e.g. "UserInterface/EvoShell/Settings". Can be empty.
* @param value_name The value name inside the key. Pass empty string to access the default value of the key.
* @return The value found in registry, or an empty string if not found.
*/
DI_IMacro safestring GetAcapellaRegKey(const safestring& subkey_name, const safestring& value_name);


/// Return MD5 hash of the buffer - this is a 32-character hexadecimal string.
DI_IMacro safestring md5(const char* buffer, size_t bufferlength);

/// Deprecated, use Image::GetMaskVector() or Image::GetStencilVector(), or call Acapella MaskArray() module from a script.
DI_IMacro NIMacro::PIndexVector MaskArray(const NIMacro::PImage im);

/**
* Load all StdLib libraries, if not yet loaded. Should not be called without a reason as this operation is quite heavy.
* @return Error code. If ERR_ERROR bit is set, some libraries may have remained unloaded.
*/
DI_IMacro unsigned int LoadAllStdLibs();

/**
* Report the current information about .AML libraries encountered in StdLib directories, which could not be loaded.
* Calls internally LoadAllStdLibs().
* Returns the number of bad libraries.
* @param buffer Output buffer, the information is appended to it.
* @param format A printf-like format string, should contain two %s tags, for the library name and the error message.
*/
DI_IMacro int GetBadLibInfo(safestring& buffer, const safestring& format = "%s: %s\r\n");


/// A debug support function, do not call from client code.
DI_IMacro void ClearModules();

/// A debug support function, do not call from client code.
DI_IMacro void IMacroDllCleanup();

/// Return the build information date and compiler, e.g. "Jan 31 2009, Microsoft Visual C++ 2008".
DI_IMacro safestring BuildDate(); 

/**
* A debugging/testing support function.
* If IMacro DLL has been compiled with #defined MEASURE_PERFORMANCE symbol,
* then ResetDuration() can be used for quering and resetting the accumulated time in the modules.
* The return value is the accumulated time spent in module Run() methods from the previous call.
* Optionally one can specify logfile to output info for each module call, or NULL.
* If MEASURE_PERFORMANCE has been not defined at compile time, this function returns -1.0;
*/
DI_IMacro double ResetDuration(FILE* logfile);

/// Perform a glob operation. This effectively calls the Glob() module, see its documentation for details.
DI_IMacro PVector Glob(const char* filemask);

/// Perform a glob operation, called from another module. This effectively calls the Glob() module, see its documentation for details.
DI_IMacro PVector Glob(ExecutionContext& ctx, const char* filemask);

/** 
* Sends an alarm message to the handler registered via SetSendAlarmHandler().
* If no handler registered, outputs the message to STDERR and returns false.
* In EvoShell environment this is forwarded to CShellDoc::SendAlarm.
* In command-line environment this will be printed to STDERR, prexifed by an [ALARM] marker.
* 
* @param message The message text.
* @param level The alarm seriousness level, in range 1..5: 1=warning, 5=serious trouble.
* @param error code - see CShellDoc::SendAlarm() documentation in EvoShell project. Can be 0.
* @param help_message - additional help message. Can be an empty string.
* @return true if a handler is registered and message was delivered to the handler.
*/
DI_IMacro bool SendAlarmIMacro(
	const Nbaseutil::safestring& message,
	unsigned int level=1,
	int	errorcode=0,
	const Nbaseutil::safestring& help_message= ""
);

/// A typedef for SetSendAlarmHandler(). See SendAlarmIMacro for parameter meanings.
typedef void (*SendAlarmHandler)(
	const char* message,
	unsigned int level,
	int	errorcode,
	const char* help_message
);

/**
* Replaces the current SendAlarmHandler and returns the previous one. See also: SendAlarmIMacro().
* @param handler The pointer to the new handler function, or NULL for turning the handler off.
* @return The previous alarm handler function, or NULL if no active handler.
*/
DI_IMacro SendAlarmHandler SetSendAlarmHandler(SendAlarmHandler handler);

/**
* Regenerates online help documentation. The online help is placed to the first writable online_help
* directory next to a StdLib directory (stdlib/../online_help). 
* The online help is generated for modules currently present and loadable
* from all StdLib directories. The extended help is taken from the first 'extended' directory found.
* The 'extended' directory is first searhced under all stdlib/../online_help directories for all stdlib dirs,
* then under all stdlib/../../acapella_online_help/online_help directories for all stdlib directories.
*
* This function essentially calls OnlineDoc::Generate() procedure, after initialisation and determining whether
* the documentation is already up-to-date or not.
*
* @param exec The OnlineDoc::Generate() procedure is run under the control of this executive.
* @param force Force regeneration of help, even if no changes detected.
* @param in_separate_thread Launch the regeneration in a separate thread and return immediately.
* @return True if generation was performed or started, false if the documentation appears to be up-to-date and no regeneration was done.
*/
DI_IMacro bool GenerateOnlineDoc(Executive* exec=NULL, bool force=false, bool in_separate_thread=true);

/**
* Regenerates online help for a single module. See GenerateOnlineDoc() for file locations.
* @param modulename The full module name. If not existing, an exception is thrown.
* @param exec The generator procedure is run under this executive.
* @return Always true.
*/
DI_IMacro bool GenerateOnlineDocForModule(const safestring& modulename, Executive& exec);


/**
* Call this function with argument 'false' to prohibit any attempt to contact the SoftDngl license server.
* While license server using is switched off, only unlicensed modules and scripts with explicitly
* granted licenses (containing appropriate GrantLicense() lines) can be run.
*/
DI_IMacro void UseLicenseServer(bool use_server);

/// On windows, if filename is a .lnk file, returns the pointed file name; otherwise returns filename intact.
DI_IMacro Nbaseutil::safestring ResolveShortcut(const Nbaseutil::safestring& filename);

/// Obsolete, use RegisterModule(..., MOD_INJECTED) instead.
DI_IMacro void AddInclusionModule(const Module* m);

/// Obsolete, use ReadImage() module in an Acapella script instead.
DI_IMacro PImage readimage_module_Run(const char* filename, int& availchannels, PVector allchannels, int width=0, int height=0, int image_index=0, const char* imageformat="autodetect", int bpp=8, int skip=0);

/// A generic function for adding extensions without breaking DLL binary compatibility. No verbs defined currently.
DI_IMacro DataItem IMacro_DoVerb(const Nbaseutil::safestring& verb, DataItem arg1, DataItem arg2);

/**
* Proposes possible completions for Acapella module call contained in line, at position pos.
* @param ctx The appropriate execution context for specifying the active script, as well as to take account any local procedures, Using directives and packages.
* @param line The line of completion (1-based).
* @param pos Byte-position of the "cursor" in the line (1-based).
* @param completions Output array of found matches.
* @param startpos Output parameter: determined position of the module name start.
* @param common_prefix_length Output parameter: the length of common prefix among all returned matches.
*/
DI_IMacro void AcapellaCompletion(ExecutionContext& ctx, const safestring& line, int pos, std::vector<safestring, Nbaseutil::mb_allocator_typedef<Nbaseutil::safestring>::allocator>& completions, int& startpos, int& common_prefix_length);


/// Deprecated, forwards to Logger::Log() with level 3. Use Logger::Log() directly in new code.
DI_IMacro void IMacroWarning(unsigned int code, const Nbaseutil::safestring& message);

/**
* Returns a pointer to the entry k in the current IMacro list of UserLib and StdLib directories.
* @param k List index, starting from 0. If k too big, empty string is returned.
*/
DI_IMacro Nbaseutil::safestring GetStdLibDir(int k);

/// Copy all stdlib directories into a buffer; optionally include UserLib and/or StdLib directories.
DI_IMacro void GetStdLibDirs(std::vector<safestring>& buffer, bool include_userlibdir=true, bool include_stdlibdir=true);

/**
* In Windows debug builds, one can call this function in order to have MSVC++ _CrtDumpMemoryLeaks() called at the program exit.
* This will force the appropriate settings at the time of downloading IMacro.dll.
* Earlier setting might cause false-positive leak dumps initiated by downloaded MFC DLL-s, if any.
* @param dump If false, the memory dump is not asked for.
* @param destination 0=stderr, 1=MSVC++ Debug console, 2=file.
* @param filename File name for destination==2 setting.
*/
DI_IMacro void DumpMemoryOnExit(bool dump=true, int destination=0, const char* filename="mem_leak.log");

/**
* Replace or append StdLib directories to search for module libraries and procedures.
* @param stdliblist Semicolon (Win) or colon (Linux) separated list of directories.
*				If NULL and append==false, then forgets any directories set via this
*				interface and restores the usual StdLib dir list. Note that any
*				module libraries or procedures are not unloaded.
* @param append If true, then only appends the directories in the beginning of lookup list.
*				If false, then stdliblist parameter completely overrides the normal StdLib
*				lookup (loaded from the Registry). Use arguments (NULL, false) to restore the normal state.
*/
DI_IMacro void SetStdLibLocation(const char* stdliblist, bool append);

/**
* Analogous to SetStdLibLocation, but sets custom user directories instead. These are separate from StdLib dirs 
* and are searched before StdLib dirs.
*/
DI_IMacro void SetUserLibLocation(const char* userliblist, bool append);

// Tracing support.
#ifdef _DEBUG
#define TRACE_IMACRO(msg) trace_imacro(msg)
#else
#define TRACE_IMACRO(msg)
#endif

/// Obsolete, use Logger::Log() instead.
DI_IMacro void trace_imacro(const Nbaseutil::safestring& msg);

/// Obsolete, use Logger class instead.
typedef void (*TraceFunc_t)(const Nbaseutil::safestring& msg);

/// Obsolete, use Logger class instead.
DI_IMacro TraceFunc_t SetTraceFunc(TraceFunc_t tracefunc);

// retained for any case, in AC2.x always "objects" is used.
#define ACAPELLA_USE_OBJECTS_NAMES

/**
* Pass data to the HTML-based imageview system.
* @param handler Reference to a object of ImageViewHandler class. This is typically obtained by HttpServer::FindHandler("imageview") call.
* @param label Label of the view (arbitrary text).
* @param data The data container containing tables: items, backgrounds, pixelsources. If NULL, only xml settings are applied to the existing view, if found. The data pointer will be NULL after the call.
* @param xml The XML tree containing visual settings. If NULL, only data is changed for the existing view, if found. If both data and xml are NULL, the view is deleted. The xml pointer will be NULL after the call.
*/
DI_IMacro void ImageViewSetContent(NAcapellaHttp::HtmlCmdHandler& handler, const safestring& label, PContainer& data, PMemBlock& xml);

/**
* Update all or part of the data container of an existing ImageView.
* @param handler Reference to a object of ImageViewHandler class. This is typically obtained by HttpServer::FindHandler("imageview") call.
* @param label Label of an existing ImageView. If ImageView does not exist, an exception is thrown.
* @param itempath The relative path of changed item in the Data container. An empty string means the whole Data container.
* @param item Changed item. This will be swapped away, item will be Undefined after the call.
*/
DI_IMacro void ImageViewUpdateData(NAcapellaHttp::HtmlCmdHandler& handler, const safestring& label, const safestring& itempath, DataItem& item);


/**
* Replace the xml visual settings tree of an existing ImageView.
* @param handler Reference to a object of ImageViewHandler class. This is typically obtained by HttpServer::FindHandler("imageview") call.
* @param label Label of an existing ImageView. If ImageView does not exist, an exception is thrown.
* @param xml The XML tree containing visual settings. May not be NULL. The xml pointer will be NULL after the call.
*/
DI_IMacro void ImageViewUpdateXml(NAcapellaHttp::HtmlCmdHandler& handler, const safestring& label, PMemBlock& xml);

/*
* Delete the ImageView object of specified label, if any.
* @param handler Reference to a object of ImageViewHandler class. This is typically obtained by HttpServer::FindHandler("imageview") call.
* @param label Unique label of ImageView, in UTF-8, can contain any characters.
*/
DI_IMacro void ImageViewRemove(NAcapellaHttp::HtmlCmdHandler& handler, const Nbaseutil::safestring& label);

/**
* Returns an empty MemBlock class hierarchy object by class name.
* This forwards to MemBlock::Factory().GetInstance(). If this fails,
* the corresponding module library is attempted to load with the help of the ResolvePoint/Package class mechanisms.
*/
DI_IMacro PMemBlock GetMemBlockClassInstance(const safestring& classname, PResolvePoint resolvepoint=NULL);

/// Return license message produced by the acapella_license.txt script by Acapella startup.
DI_IMacro safestring GetLicenseMsg();

#ifndef MSC_VER
DI_IMacro void SetStartupDir(const safestring& dir);
#endif


/**
* Initializes Acapella run-time system.
* Must be called before entering the multithreaded regime and before any software component calls GetStdLibDir().
*/
DI_IMacro void AcapellaInit(const safestring& propname, const safestring& value, void* reserved=NULL);

DI_IMacro void EnterMultiThreadedRegime();

DI_IMacro bool IsInMultiThreadedRegime();

/**
* Returns the directory of the generated online help. This depends on the registry settings 
* (Settings/LocalAppDir or Settings/StdLibDir). Example return value: "C:/PerkinElmerCTG/Acapella 2.2/online_help".
*/
DI_IMacro safestring GetOnlineHelpOutputDir();

/// A prototype for a callback function for registering Sharable classes.
typedef PSharable (*create_sharable_func)(const DataItem& arg1, const DataItem& arg2);

/**
* Register a sharable class. 
* @param name Name of the class, which can be used in scripts as procedure parameter type name.
* @param create_func Pointer to a function which can be called for creating objects of this type.
*/
DI_IMacro void RegisterSharableClass(const safestring& name, create_sharable_func create_func);

/**
* For a registered sharable class (see RegisterSharableClass()) returns a pointer
* the the creator function, otherwise NULL.
*/
DI_IMacro create_sharable_func GetRegisteredSharableClass(const safestring& name);

/**
* Returns Acapella __localappdatadir__ constant. Throws an exception if the directory does not exist and cannot be created.
*/
DI_IMacro Nbaseutil::safestring GetLocalAppDataDir();

/**
* Returns Acapella __commonappdatadir__ constant. A level 3 warning message is logged if the directory does not exist and cannot be created.
*/
DI_IMacro Nbaseutil::safestring GetCommonAppDataDir();

/**
* Overrides Acapella product name as seen by GetProductName() and GetProductInfo() module.
*/
DI_IMacro void SetProductName(const Nbaseutil::safestring& name);

/**
* Overrides Acapella product version as seen by GetProductName() and GetProductInfo() module.
* @param version Pass a general product version, typically two parts, e.g. "2.2". 
*                Last two parts of Acapella version will be added to this to form the complete product version.
*/
DI_IMacro void SetProductVersion(const Nbaseutil::safestring& version);

/**
* Returns the product name as set by SetProductName(). If not set and dongle is in use,
* returns product name based on the dongle serial no. If unknown serial no or no dongle, returns "Acapella".
*/
DI_IMacro Nbaseutil::safestring GetProductName();

/**
* Returns the product version as set by SetProductVersion() or guessed, plus two last parts of Acapella version. If not set, tries to guess the product version 
* by the current product name and current Acapella version. If guessing fails, an empty string is returned.
* @return A combination of product and Acapella versions, e.g. "2.2.6.65783".
*/
DI_IMacro Nbaseutil::safestring GetProductVersion();

/**
* Returns the four-part Acapella version.
*/
DI_IMacro Nbaseutil::safestring GetAcapellaVersion();

/**
* An abstract base class for classes maintaining data caches in memory. 
* Just derive the cache maintainer class from this one, implement the virtual functions
* and call MemoryCacheBaseCleanup() from the derived class destructor.
* The virtual functions will be called for handling out-of-memory situations. 
*/
class DI_IMacro MemoryCacheBase {
public: // interface
	/// Call from the derived class destructor, this will unregister this object from the out-of-memory handling system.
	void MemoryCacheBaseCleanup();
public: // virtual interface
	/**
	* Report the approximate maximum amount of memory which could be freed from the cache.
	*/
	virtual size_t GetCurrentCacheSize(int reserved=0) const=0;

	/**
	* Is called for releasing memory. 
	* @param requested_size The size of the memory request which triggered the cache release.
	* @return The amount of memory which has been freed.
	*/
	virtual size_t ReleaseCache(size_t requested_size, int reserved=0)=0;

	/// A reserved vtable slot.
	virtual size_t Reserved1(size_t arg1, void* arg2) {return 0;}

protected: // implementation
	MemoryCacheBase();
	virtual ~MemoryCacheBase();
	bool cleanup_called_;
};

/**
* Create a local scope object of this type when calling code from inside an Acapella module 
* which might not handle out-of-memory exceptions well. If an out-of-memory error happens inside of this region
* the handler will try to satisfy the memory request by using the panic reserve.
*/
class DI_IMacro OomUnsafeRegion {
public:
	OomUnsafeRegion();
	~OomUnsafeRegion();
};


} // namespace NIMacro
#endif

