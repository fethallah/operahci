#ifndef x_ACAPELLA_IMACRO_SCRIPTFILE_H_INCLDUED_
#define x_ACAPELLA_IMACRO_SCRIPTFILE_H_INCLDUED_

#include <vector>
#include <iostream>
#include <istream>
#include <ostream>

#include "textholder.h"


namespace NIMacro {

// using declarations
using Nbaseutil::safestring;

// forward declarations
class ResolvePoint;
typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;
class ScriptFile;
typedef ThreadSharablePointer<ScriptFile> PScriptFile;
struct ExecutionContext;
class Script;
typedef ThreadSharablePointer<NIMacro::Script> PScript;
class scriptfilekey_t;


/// Enum constants to be broadcast by ScriptFile::NotifyListeners().
enum ScriptFileEvent {
	/// The text of the ScriptFile object is changed in memory
	script_file_changed,

	/// The ScriptFile object is deleted
	script_file_released,

	/// Used in the ScriptFile::Scan method.
	script_file_scan,

	/// Script file has been saved in permanent storage.
	script_file_saved,

	/// The defined procedure names in the scriptfile have been changed
	script_file_procnames_changed,

	script_file_input_params_possibly_changed,
};

/**
* A structure holding file modification time on disk.
* On Windows, the content is the same as for FILETIME structure.
* After defualt ctor or if the disk file modiftime is not accessible,
* the content is zero. Such zero FileTime compares unequal to any
* other FileTime, including another zero FileTime.
*/
struct FileTime {
  unsigned long dwLowDateTime;
  unsigned long dwHighDateTime;

public: // interface
  /// Construct by filename. If there is an error when accessing the file, a level 4 Logger message is generated.
  DI_IMacro FileTime(const safestring& filename);

  /// Contruct by filename. If there is an error when accessing the file and log_errors==false, the no message is logged.
  DI_IMacro FileTime(const safestring& filename, bool log_errors);

  /// Construct by filename, support Executive override via GetInput(..., "filetime")
  DI_IMacro FileTime(const safestring& filename, ExecutionContext& ctx);

  FileTime(): dwLowDateTime(0), dwHighDateTime(0) {}
  FileTime(const FileTime& b): dwLowDateTime(b.dwLowDateTime), dwHighDateTime(b.dwHighDateTime) {}
  FileTime& operator=(const FileTime& b) {dwLowDateTime=b.dwLowDateTime; dwHighDateTime=b.dwHighDateTime; return *this;}
  bool operator==(const FileTime& b) const {return (dwLowDateTime!=0 || dwHighDateTime!=0) && dwLowDateTime==b.dwLowDateTime && dwHighDateTime==b.dwHighDateTime;}
  bool operator!=(const FileTime& b) const {return !operator==(b);}
  bool operator<(const FileTime& b) const {return dwHighDateTime<b.dwHighDateTime || (dwHighDateTime==b.dwHighDateTime && dwLowDateTime<b.dwLowDateTime);}
  bool operator<=(const FileTime& b) const {return *this<b || (dwLowDateTime==b.dwLowDateTime && dwHighDateTime==b.dwHighDateTime);}
  bool operator>(const FileTime& b) const {return !(*this<=b);}
  bool operator>=(const FileTime& b) const {return !(*this<b);}

  friend void operator>>(std::istream& is, FileTime& ft) {is >> ft.dwLowDateTime >> ft.dwHighDateTime;}
  friend void operator<<(std::ostream& os, const FileTime& ft) {os << ft.dwLowDateTime << " " << ft.dwHighDateTime;}

  /// Init the content by the modification time of a disk file.
  DI_IMacro void Init(const safestring& filename, bool log_errors);

  /// Format the time as "YYYY-MM-DD HH:MM:SS TIMEZONE". If gmt==true, then TIMEZONE will be GMT.
  DI_IMacro safestring Format(bool gmt) const;
};

/**
* A base interface class for listening ScriptFile events. The derived classes are registered via
* ScriptFile::RegisterListener() and ScriptFile::RegisterListenerStatic(), and the callbacks will be executed
* for certain events.
*
* In the callback function it is allowed to unregister the current listener, but no other listeners. It is allowed
* to register new listeners, but these won't get called back in this event.
*
* The callback may happen in another thread than the registration.
*/
class ScriptFileListener {
public:
	virtual ~ScriptFileListener() {}
	virtual void NotifyScriptFileEvent(ScriptFile& sender, ScriptFileEvent evt)=0;
};

/// A base class used in ScriptFile::Scan()
class ScriptFileCallback {
public:
	virtual ~ScriptFileCallback() {}
	virtual void NotifyScriptFileCallback(ScriptFile& sf, void* callback_data)=0;
};


class Undoer;

/** A class for representing a disk script file or opera database script file.
*   This integrates the capability of undo/redo and single representation of the script file in the memory.
*
*   This class has no knowledge about files or databases. There is a global map of ScriptFiles.
*   The map keys are arbitrary strings, which are called filenames generally. In case of disk files these
*	keys coincide with the full path filename of the script. You access the global map via ScriptFile::Get
*	static method.
*
*	The ScriptFile can be initialized by Reset() method.
*
*	The ScriptFile content can be changed by replace(), undo() and redo() methods.
*
*	The ScriptFile also maintains the modification flag for external storage support. The flag is raised if the
*	content has been changed. When storing to external storage the client should call SetModified(false). This
*	does not flush undo buffer.
*
*	A script file generally contains a main script (which may be empty) and an arbitrary number of procedure definitions.
*   There is a Script object corresponding to each of those. You access them by GetScript() method.
*
*	After every reparsing all registered listeners are notified about script_file_change event.
*
*	For deleting a ScriptFile call it's Unregister() method. This will remove it from the global registry.
*	When all smartpointers to the ScriptFile are dead, the ScriptFile will be deleted. By deletion it notifies
*	all registered listeners about script_file_delete event.
*
*/
class DI_IMacro ScriptFile: public TextHolder {
public:
	/**
	* Find or create a ScriptFile in the global map. Can be called in any thread.
	* If a new ScriptFile is created then tries to load it from the file, throws if this doesn't succeed.
	* @param filename  Unique identification tag for the ScriptFile; any characters allowed.
	* @param create If there is no such ScriptFile yet, then if 'create', a new ScriptFile will be created; else NULL is returned.
	*/
	static PScriptFile Get(const safestring& filename, bool create=true);

	/**
	* Find or create a ScriptFile in the global map. Can be called in any thread.
	* If a new ScriptFile is created then tries to load it from the file, throws if this doesn't succeed.
	* @param filename  Unique identification tag for the ScriptFile; any characters allowed.
	* @param ctx Execution context
	* @param create If there is no such ScriptFile yet, then if 'create', a new ScriptFile will be created; else NULL is returned.
	*/
	static PScriptFile Get(const safestring& filename, ExecutionContext& ctx, bool create=true);

	/**
	* Same as other versions of Get(), but can access also other scripts than the first in a multi-script file like .res or .msr.
	*/
	static PScriptFile Get(const safestring& filename, int no, ExecutionContext& ctx, bool create=true);

	/**
	* Same as other Get() except that the disk file is not touched; file content is used instead.
	* If a ScriptFile with the same name is already present, then it's content is changed and its listeners notified about that.
	* Use the other Get() with create=false parameter to check if the ScriptFile is already present.
	*/
	static PScriptFile Get(const safestring& filename, const safestring& content);

	/// Same as Get(const safestring& filename, const safestring& content), only content may be swapped away into the object.
	static PScriptFile Get(const safestring& filename, safestring& content);

	/**
	* Find or create a ScriptFile corresponding to a procedure file. Can be called in any thread.
	* If a new ScriptFile is created then tries to load it from the file, throws if this doesn't succeed.
	* @param filename  Unique identification tag for the ScriptFile; any characters allowed.
	* @param rp The resolvepoint acting as a static parent for the resolve point of the procedure file.
	*/
	static PScriptFile GetProcFile(const safestring& filename, ResolvePoint& rp);

	/**
	* The first version of GetFromXml() reads XML data from filename, the second one gets the XML data from
	* the second parameter. filename is just used to generate filenames for the PScriptFile objects in scripts.
	* The Xml data may contain several scripts. All these scripts are extracted and stored in the
	* scripts parameter.
	* If at least one script could be extracted, the result is true, false otherwise.
	*/
	static bool GetFromXml(const safestring& filename, std::vector<PScriptFile>& scripts);
	static bool GetFromXml(const safestring& filename, const safestring& xml, std::vector<PScriptFile>& scripts);

	/// Create an unnamed ScriptFile which does not appear in the global map.
	static PScriptFile CreateUnnamed(const safestring& content);

	/// Create an unnamed ScriptFile which does not appear in the global map. The content parameter is swapped away into the object.
	static PScriptFile CreateUnnamed(const safestring& surrogate_name, safestring& content);

	static int GetGlobalVersion();

	/// Loads or reloads the content from the file, or throws if the file cannot be read. This is called from first version of Get() if needed.
	void LoadFromFile();

	/// Same as LoadFromFile(), but provides support for Executive override. This is called from the second version of Get() as needed.
	void LoadFromFile(ExecutionContext& ctx);

	/** Stores the content into the file, or throws if the file cannot be written.
	* @param force Force save also when the modified flag is not raised.
	*/
	void StoreToFile(bool force=false) const;

	/// Dtor
	~ScriptFile();

	void RegisterListener(ScriptFileListener& listener);
	void RemoveListener(ScriptFileListener& listener);

	static void RegisterListenerStatic(ScriptFileListener& listener);
	static void RemoveListenerStatic(ScriptFileListener& listener);

	/// Scan all files in the global map
	static void Scan(ScriptFileCallback& callback, void* callback_data);

	/// Scan all files accessible through this ScriptFile.
//	void ScanThis(ScriptFileListener& listener, void* callback_data);

	/** Modified flag is raised in any content-changing function and is cleared in StoreToDisk() member function.
	*   This shows that the script content is not saved in the disk file or database.
	*   If the caller saves the content otherwise it should call SetModified(false).
	*/
	void SetModified(bool modified=true);

	/// Return true if the script is changed via replace(), undo() or redo() and not saved to disk file or database.
	bool GetModified() const;


	void SetForceNewFileName(bool forceNewFileName = true);
	bool GetForceNewFileName() const { return forceNewFileName_; }

	/// Switch on Undo support
	void SetCanUndo(bool canundo);
	bool HasUndoer() const {return undoer_!=NULL;}

//	void replace(size_t pos, size_t deletelen, const safestring& replacement, bool notify_listeners);
	void undo(bool notify_listeners);
	void redo(bool notify_listeners);
	bool canundo() const;
	bool canredo() const;

	// To group multiple replace() calls into one undo/redo action, call BeginTransaction()/EndTransaction() before and after replace() calls.
	void BeginTransaction();
	void EndTransaction();

//	static bool Active() {return true;}
//	static void SetActive(bool active=true) {} // Change 12.11.2004: ScriptFile support activated always, cannot switch off!

	void NotifyListeners(ScriptFileEvent evt);

	class ScanListenersCallback {
	public:
		virtual bool ListenerCallback(ScriptFileListener& lstner, void* callback_data)=0;
		virtual ~ScanListenersCallback() {}
	};
	bool ScanListeners(ScanListenersCallback& callback, bool scan_static=true, bool scan_nonstatic=true, void* callback_data=NULL);

	/// Retrieve file modification timestamp on disk
	const FileTime& GetFileModif() const {return filetime_;}

	/// Check if the file appears modified on the disk after last LoadFromFile().
	bool IsModifiedOnDisk() const;

	void Unregister();

	void SetIsStdLibProc(bool stdlibproc=true) {stdlibproc_=stdlibproc;}

	/// Return the index of the script in the disk file, usually 0.
	int GetNo() const {return no_;}

	/// Delete previous "parameters" attachments from the script text and replace with the passed string in provideinput() format. Listeners are notified.
	void SetAttachedParams(const safestring& settings_string);

protected: // overridden virtual methods
	virtual PSharable DoClone() const;
//	virtual void NotifySingleRef();
	virtual bool DoReplace(size_t pos, size_t deletelen, const safestring& replacement, bool notify_listeners);

public: // overridden virtual methods
	virtual const safestring& GetFileName() const;

private: // implementation
	ScriptFile(const scriptfilekey_t& key, ResolvePoint& parent_resolvepoint);

	/// Set file modification timestamp on disk
	void SetFileModif(const FileTime& filemodif) {filetime_=filemodif;}

//	void ScanThisPrivate(ScriptFileListener listener, void* callback_data, std::set<ScriptFile*>& scanned);
//	void ScanScript(PScript script, ScriptFileListener listener, void* callback_data, std::set<ScriptFile*>& scanned);

private: // static data
	static int s_globalversion_;

private: // data
	mutable Mutex data_mutex_;		// protects all data except listeners
	Undoer* undoer_;		// undo/redo support; pointer only as in non-ui environment the undo/redo support is not needed.
	bool readonly_, modified_, forceNewFileName_, stdlibproc_;
	const safestring filename_;	// actual filename
	FileTime filetime_;	// timestamp of the disk file when it was last reparsed.

	typedef std::set<ScriptFileListener*> listeners_t;

	Mutex listeners_mutex_;		// protects listeners
	listeners_t listeners_;

	static Mutex static_listeners_mutex_;		// protects static listeners
	static listeners_t static_listeners_;

	const int no_;	// in case of multi-script files, the index of this script in the file, otherwise 0.
};

} // namespace

#endif
