#ifndef x_ACAPELLA_IMACRO_PACKGAE_H_INCLCDED_
#define x_ACAPELLA_IMACRO_PACKGAE_H_INCLCDED_

#include "resolvepoint.h"
#include "scriptfile.h"

namespace NIMacro {

	class Module;
	class ResolvePoint;
	class PackageManager;
	class TextHolder;
	class Monitor;
	struct ResolveHelper;
	typedef ThreadSharablePointer<Module> PModule;
	typedef ThreadSharablePointer<TextHolder> PTextHolder;
	typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;
	class LoadTrierRegistrar;

	/**
	* This class manages a single StdLib directory. It is capable to load and reload the
	* .aml and .proc libraries and register them in a ResolvePoint.
	* It remembers the locations of modules in a .guide file beneath user HOME directory,
	* so if the module is required at next run it can load only a single library.
	*
	* The modules in the maintained directory go to the global namespace; packages correspond
	* to the subdirectories. Despite of the name of this class both the global namespace
	* and packages are handled by a single Package object.
	*
	* Only the following files are attempted to load:
	*   - *R.aml
	*   - *.proc
	*
	* The Debug version of Acapella uses D instead of R. 
	*
	* In Windows, also .LNK shortcuts are supported in the directory. The shortcut name must
	* follow the same rules as library names, only the .LNK extension is added (this is hidden
	* by the Windows Explorer). There are no restrictions on the name of the file pointed by
	* the shortcut. On Linux, soft- and hardlinks work as usually.
	*
	*/
	class DI_IMacro Package: public ThreadSharable, public ScanModulesContainer {
	public:

		/// Return the maintained directory name. This is in normalized form: uses forward slashes and does not end with a slash.
		safestring GetLibDir() const {return libdir_;}

		/**
		* Restricts the Package object to load only libraries matching
		* one of the specified filters in the list.
		* The compared library filenames contain also path.
		* The filter strings may contain * and ? metacharacters.
		* The filter does not affect already loaded libraries.
		* See -libfilter option of Acapella command-line.
		* If this function is not called, all libraries are loaded.
		*/
		void AddLibFilter(const safestringlist& libfilterlist);

		/**
		* Attempt to load module package::propername. Should be called only after module lookup failed.
		* If the load succeeds, the module is registered in resolvepoint and the module pointer is returned.
		*
		* This function can load a single library or all libraries from a single directory.
		*
		* Precondition: resolvepoint.ResolveQualifedName(package, propername)==NULL.
		*
		* PostCondition: resolvepoint.ResolveQualifedName(package, propername)==return_value.
		*
		* @param package Package name of the module. Alphanumeric or empty. Empty string means global namespace modules (present directly in the library directory).
		* @param propername Module proper name (does not contain "::"). Alphanumeric, may not be empty.
		* @param resolvepoint ResolvePoint where to register all loaded modules.
		*/
		PModule TryToLoadByNamespace(const safestring& package, const safestring& propername, ResolvePoint& resolvepoint);

		/**
		* This member function is called only if the module has not yet been loaded.
		* It attempts to load the module, assuming it is present in this Package. This can succeed if the module is registered in the guide file
		* of the Package, or any procedure file has been marked out-of-date (via e.g. PrepareOutOfDate() or RemoveModulesInProc()
		* member functions).
		* 
		* @param package Module namespace.
		* @param propername Module proper name.
		* @param resolvepoint Resolve point for registering modules from the loaded libraries.
		* @param[out] module_disappeared If the module should be present in this Package according to the guide file, but was not found, this output parameter is set to true; otherwise it is left unchanged.
		* @return Module pointer if it was successfully loaded by the member function, otherwise NULL.
		*/
		PModule TryToLoadByGuide(const safestring& package, const safestring& propername, ResolvePoint& resolvepoint, bool& module_disappeared);

		void LoadMatchingLibraries(const safestring& package_mask, const safestring& propername_mask, ResolvePoint& resolvepoint);

		void WriteGuide(bool force=false);

		safestring GetGuideLibraryName(const safestring& package, const safestring& propername) const;

		/**
		* Attempt to load module package::propername. Should be called only after module lookup failed.
		* If the load succeeds, the module is registered in resolvepoint and the module pointer is returned.
		*
		* This function can load a single library or all libraries from a single directory.
		*
		* Precondition: resolvepoint.ResolveQualifedName(package, propername)==NULL.
		*
		* PostCondition: resolvepoint.ResolveQualifedName(package, propername)==return_value.
		*
		* @param package Package name of the module. Alphanumeric or empty. Empty string means global namespace modules (present directly in the library directory).
		* @param propername Module proper name (does not contain "::"). Alphanumeric, may not be empty.
		* @param resolvepoint ResolvePoint where to register all loaded modules.
		*/
		PModule TryToLoad(const safestring& package, const safestring& propername, ResolvePoint& resolvepoint);

		/// Check for .proc files controlled by this package and changed on disk; return the number of such .proc files.
		int CheckForChangedProcFiles(ResolvePoint& caller);

		/**
		* Return a list of libraries managed by this Package object.
		* @param liblist Full pathnames of managed library files are appended here.
		* @param shortcuts For each library, if the library was found through a Windows shortcut, the name of the schortcut file (.lnk) is appended here, otherwise an empty string is appended.
		*/
		void ListLibraries(std::vector<safestring>& liblist, std::vector<safestring>& shortcuts) const;

		/**
		* Loads the library to the specified resolvepoint if it is managed by this Package object (according to the guide file).
		* @param libname The library filename.
		*				This can be full pathname, relative or just proper name.
		*				The filename extensions and proper name suffix like "R" (regex "[rdRD]") may be absent.
		*				In case of Windows shortcuts in package directories this must match the shortcut name, not the actual library name.
		* @param resolvepoint Specifies the resolve point where to register the library.
		* @return If the library is a dll/so and it was loaded during this function call, handle to it is returned, otherwise NULL.
		*/
		hmodule_t LoadLibraryIfFoundInGuide(const safestring& libname, ResolvePoint& resolvepoint);

		void CompleteProperNameByGuide(const safestring& package_name, const safestring& propername_start, std::set<safestring>& results, const safestring& addprefix="", bool add_type=false) const;

		void CompletePackageNameByGuide(const safestring& package_name_start, std::set<safestring>& results, const safestring& addprefix="", bool add_type=false) const;

		/// By the Guide file, look up modules corrently registered in the proc file specified by \a partial_path, and remove these from \a caller.
		void RemoveModulesInProc(const safestring& partial_path, ResolvePoint& caller);

		/// Remove the module entry from the guide file, if any.
		void DeleteFromGuide(const safestring& package_name, const safestring& propername);

		void ResetProcVersion();

	public: // virtual overrides
		virtual PSharable DoClone() const;
		virtual safestring GetDescription() const;
		virtual bool ScanModules(ScanModulesVisitor& visitor, const safestring& package_mask="*", const safestring& propername_mask="*", unsigned int scanflags=0);

	private: // friend interface
		friend class PackageManager;
		/**
		* Construct a Package object to maintain the specified StdLib directory.
		* @param libdir Absolute directory name (local or UNC). Can contain backslashes and can end by a slash.
		*/
		Package(const safestring& libdir);

		friend class LoadTrierRegistrar;
		void AddToGuide(const safestring& propermodulename, const safestring& packagename, bool hidden);

	private: // implementation
		~Package();
		void LoadLib(const safestring& relativefilename, bool forcereload);
		void LoadNamespace(const safestring& package, bool forcereload);
		void LoadLibs( void (Package::*fmeth)(const safestring&, bool), const PVector& globlist, const safestring& filemask, bool forcereload);
		void LoadBinary(const safestring& filename, bool forcereload);
		void LoadProc(const safestring& filename, bool forcereload);
		void PrepareOutOfDate(const safestring& abspath);
		void PrepareOutOfDateHelper(const safestring& filename, bool forcereload);
		void LoadGuide();
		void ParseGuide(const char* s);
		void RegisterFileTimeInGuide(const safestring& absfilename);
		safestring CalcGuideHash() const;
	private:
		mutable Nbaseutil::boost_recursive_mutex mx_;
		bool reloaded_recently_, guide_changed_, guide_write_failed_;
		typedef std::vector<safestring> libfilters_t;
		libfilters_t libfilters_;
		safestring libdir_, guidefilename_, curr_libfilename_;
		typedef std::map<safestring,safestring> guide_t;
		guide_t guide_;
		safestring guide_hash_;
		typedef std::map<safestring, int> procversion_t;
		procversion_t procversion_;
		typedef std::set<safestring> loaded_namespaces_t;
		loaded_namespaces_t loaded_namespaces_;
		typedef std::set<safestring> outofdate_t;
		outofdate_t outofdate_, loaded_matching_libs_;
		LoadTrierRegistrar* registrar_;
#ifdef ACAPELLA_PLATFORM_POSIX
		typedef std::set<ExportedClassesFunc> loaded_classes_t;
		loaded_classes_t loaded_classes_;
#endif
		typedef std::set<safestring> known_packages_t;
		known_packages_t known_packages_;
	};
	typedef ThreadSharablePointer<Package> PPackage;

	class ScanPackagesVisitor {
	public:
		virtual bool Visit(Package& package, void* callback_data)=0;
		virtual ~ScanPackagesVisitor() {}
	};

	/**
	* A singleton class managing packages and shared module libraries.
	* All methods are thread-safe, no external synchronization needed.
	*/
	class DI_IMacro PackageManager: public mb_malloced {
	public: // static interface

		/// Return the singleton reference
		static PackageManager& Instance();

		/// Return the pointer to the singleton, if it has been created, otherwise NULL.
		static PackageManager* InstanceNoCreate();

		/// Destroy the singleton. Can be called in single-threaded regime only.
		static void Destroy();

	public:

		/**
		* Return the package corresponding to the specified StdLib directory.
		* The Package object is created if not yet existing.
		* @param libdir Absolute directory name (local or UNC). Can contain backslashes and can end by a slash.
		*/
		PPackage GetPackage(const safestring& libdir);

		/// Iterates over all existing Package objects, calls back to visitor each time.
		bool ScanPackages(ScanPackagesVisitor& visitor, void* callback_data=NULL);

		/**
		* Return true if the library has been already loaded by LoadDll() member function.
		* @param absolutefilename The absolute file name of the library.
		*/
		bool IsDllLoaded(const safestring& absolutefilename);

		/// Return true if the library has been marked as bad (non-loadable). Also returns the relevant error message.
		bool IsBad(const safestring& absolutefilename, safestring& error_message);

		/// Return information about "bad" libraries.
		int GetBadLibInfo(safestring& buffer, const safestring& format);

		/// Forget the remembered info about bad and non-acapella libraries.
		void ResetBadLibs();

		/// A typedef for the ExportedModules() function exported from the Acapella module libraries.
		typedef int (*ExportedModulesFunc)( ModReg **x, int *Count);

		/**
		* Loads a binary Acapella module library into the process memory space. Does not check if the library has already been loaded or not.
		* The Acapella library has to export an extern "C" function called ExportedModules. If this is not the case,
		* the library is remembered as "not an Acapella module library" and NULL is returned.
		* In case of load failures the library is remembered as "bad", subsequent load attempts return NULL immediately.
		* @param absolutefilename The absolute file name of the library.
		* @param shortcut If the file was found through a Windows shortcut, then the name of the shortcut, for using in warning messages. Otherwise an empty string.
		* @param f_exported_modules Output parameter: if module library successfully loaded, the address of the exported ExportedModules() function is stored here.
		* @param f_exported_classes Output parameter: if module library successfully loaded and it exports an extern "C" ExportedClasses() function, the address of it is stored here. Otherwise, NULL is stored here.
		* @return The library handle if module library successfully loaded, otherwise NULL.
		*/
		hmodule_t LoadDll(const safestring& absolutefilename, const safestring& shortcut, ExportedModulesFunc& f_exported_modules, ExportedClassesFunc& f_exported_classes);

		/**
		* Switch the proc file monitoring on. After monitoring has been switched on,
		* changes to .proc files are recognized immediately, and the files reloaded/reparsed when the relevant procedures 
		* are called later.
		*
		* If file change monitoring is not implemented on the platform, the call does nothing. In this case, one can call 
		* ResolvePoint::CheckForChangedProcFiles_mt() for scanning and recognizing changed files by hand.
		* @return True, if monitoring is supported and will be working or started.
		*/
		bool StartMonitor();


		/// Return pointer to the filesystem changes monitor, or NULL if monitoring has not been started.
		Monitor* GetMonitor() const;

	private: // implementation
		PackageManager();
		~PackageManager();
	private: // types
		struct badlibentry_t {
			FileTime time;
			safestring errmsg;
			badlibentry_t(const FileTime& t, const safestring& s): time(t), errmsg(s) {}
			badlibentry_t(){}
		};
	private: // static data
		static Nbaseutil::boost_recursive_mutex s_mx_;
		static PackageManager* s_themgr_;
	private:
		mutable Mutex mx_;
		typedef std::map<safestring, PPackage> packages_t;
		packages_t packages_;

		mutable boost_mutex libs_mx_;
		typedef std::set<safestring> stringset_t;
		stringset_t loaded_dlls_;
		stringset_t not_acapella_libraries_;
		typedef std::map<safestring,badlibentry_t> badlibs_t;
		badlibs_t badlibs_;

		Monitor* monitor_;
	};

	inline PackageManager& Pck() {return PackageManager::Instance();}

} // namespace

#endif
