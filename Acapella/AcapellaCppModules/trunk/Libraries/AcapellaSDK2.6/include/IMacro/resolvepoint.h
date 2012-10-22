#ifndef x_ACAPELLA_IMACRO_RESOLVEPOINT_H_INCLUDED_
#define x_ACAPELLA_IMACRO_RESOLVEPOINT_H_INCLUDED_

namespace NIMacro {

	struct ModReg;
	class Module;
	class ModuleCall;
	class ResolvePoint;
	struct ResolveHelper;
	class TextHolder;
	class Script;
	class Monitor;
	//class ResolvePointListener;
	typedef ThreadSharablePointer<TextHolder> PTextHolder;
	typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;
	typedef ThreadSharablePointer<Module> PModule;
	typedef ThreadSharablePointer<Script> PScript;
	class Package;
	typedef ThreadSharablePointer<Package> PPackage;

	class ScanModulesVisitor {
	public:
		virtual bool Visit(const safestring& modulename, const safestring& packagename, PModule module)=0;
		virtual ~ScanModulesVisitor() {}
	};

	class ScanModulesContainer {
	public:
		enum scanflags_t {
			scan_recursive=1<<0,
			scan_includehidden=1<<1,
			scan_namesonly=1<<2,
			scan_duplicates_allowed=1<<3,
		};
		virtual bool ScanModules(ScanModulesVisitor& visitor, const safestring& package_mask="*", const safestring& propername_mask="*", unsigned int scanflags=scan_recursive)=0;
		virtual ~ScanModulesContainer() {}
	};

	/// An abstract base class. Multiple derive from that class for using the Monitor class.
	class MonitorCallback {
	public:
		/// An enum for specifying message types
		enum mon_event_t {
			/// A file was changed or added in the monitored directory
			mon_changed,
			/// A file was removed from the monitored directory
			mon_removed,
			/// The monitoring of the directory has been terminated by any reason
			mon_finished,
		};

		virtual ~MonitorCallback();
		virtual void NotifyMonitor(const safestring& monitored_path, const safestring& partial_path, mon_event_t event)=0;
	};

	/**
	* Each resolvepoint corresponds to a "name lookup context" where the module names of a given TextHolder
	* are looked up by script parsing. The TextHolder object maintains a smartpointer to the primary ResolvePoint.
	* Each ResolvePoint also has a link to the parent ResolvePoint, which is consulted if the name was not found in this 
	* ResolvePoint object. There is a single root ResolvePoint without parent, returned by the CoreResolvePoint9) static member function.
	* One special resolvepoint corresponds to StdLib content.
	*
	* The ResolvePoint objects are shared between multiple threads. All methods contain internal locking
	* so there is no need for external synchronization when calling ResolvePoint methods. Thus all methods are sufixed 
	* by "_mt" to indicate they are multithread-safe.
	*/
	class DI_IMacro ResolvePoint: public ThreadSharable, public ScanModulesContainer, public MonitorCallback {
	public: // static interface

		/**
		* Return the core resolve point containing system modules implemented in the IMacro DLL. 
		* The core resolvepoint does not have a parent.
		* All other resolvepoints are directly or indirectly the children of the core resolvepoint.
		*/
		static PResolvePoint CoreResolvePoint();

		/**
		* Return the resolvepoint maintaining the module and procedure libraries under global StdLib paths.
		* This is a direct child of core resolvepoint.
		*/
		static PResolvePoint StdLibResolvePoint();

		/**
		* Return the resolvepoint maintaining the module and procedure libraries under global UserLib paths.
		* This is a direct child of StdLib resolvepoint. This resolve point should be used for resolving 
		* scripts in general.
		* @param reset Read the config from registry again and create a new ResolvePoint object.
		*/
		static PResolvePoint UserLibResolvePoint(bool reset=false);

		/**
		* For performance reasons parsed procedures are cached in the ResolvePoint. 
		* This method deletes the parsed procedures, it is supposed to be called when
		* the the procedure texts have been changed by any reason in the ScriptFile objects.
		*/
//		static void FlushAllParsedSubscripts(std::set<ResolvePoint*>& processed);



	public: // interface

		/// Construct a ResolvePoint as a child of another ResolvePoint. The dbg_resolvepoint_name parameter is ignored in Release build.
		ResolvePoint(ResolvePoint& static_parent, const safestring& dbg_resolvepoint_name);

		/**
		* Construct the ResolvePoint as a child of another ResolvePoint and maintaining module/proc libraries in the specified library directories. 
		* @param static_parent Static parent is consulted automatically if the module is not found in this resolvepoint.
		* @param dbg_resolvepoint_name Informal name for the resolvepoint, used in error and log messages. This parameter is ignored in Release build.
		* @param libdirs A list of directory specifications this ResolvePoint will manage. Each entry in the list may be a list of directories by itself, 
		*                separated by a platform-specific separator (';' in Windows, ':' elsewhere).
		*/
		ResolvePoint(ResolvePoint& static_parent, const safestring& dbg_resolvepoint_name, const safestringlist& libdirs);

		~ResolvePoint();

		/**
		* Add a Package object into this ResolvePoint. The Package modules and procedures 
		* will be available through the ResolvePoint.
		* This function is multithread-safe.
		*/
		void AddPackage_mt(PPackage package);

		/// Clears all local modules and releases all packages. Should be called only in Debug build cleanup code to avoid bogus memory leak warnings.
		void Clear_mt();

		/// Return i-th registered package, NULL if i is out of bounds.
		PPackage GetPackage_mt(int i);

		/// Return the parent ResolvePoint, or NULL for the core resolvepoint.
		PResolvePoint GetStaticParent_mt() {return static_parent_;}

		/// Set the static parent of this resolvepoint. Can be called only if the resolvepoint currently does not have a parent. The parameter may not be NULL.
		void SetStaticParent_mt(PResolvePoint static_parent);

		/**
		* @brief Add a using directive for use in resolving the names.
		* @param usingdirective Directive specifying the absolute package name,
		*			i.e. the directive is considered to begin with "::" even if it does not.
		*			The directive may optionally contain module name wildcarded mask after last "::".
		*/
		void AddUsingDirective_mt(const safestring& usingdirective);

		/// Check if this resolve point has any associated using directives.
		bool HasUsingDirectives_mt() const {return !using_.empty();}

		/// Register this module under this resolvepoint. ResolvePoint takes ownership of m.
		bool AddModule_mt(const safestring& packagename, const safestring& modulename, Module* m);

		/// Request this module/procedure to be removed from under this resolvepoint. Returns the number of removed modules (0 or 1).
		int RemoveModule_mt(const safestring& packagename, const safestring& modulename);

		/**
		* Resolves un unqualified (no colons) module call name as written in the script, returns NULL in case of failure.
		* This is called from ModuleCall::ResolveName(). The name lookup algorithm is the following:
		*   - "default_package::proper_name" is looked up in this and parent resolvepoints, by .guide files only (without libraries' reload).
		*   - for all Using() directives in force for this resolvepoint, "using_package::proper_name" is looked up in this and parent resolvepoints, by .guide files only (without libraries' reload).
		*   - the global namespace is tried: "::proper_name" is looked up in this and parent resolvepoints, by .guide files only (without libraries' reload).
		*   - the above three steps are repeated, now allowing the system to try libraries' reloading, in order to recognize new and changed modules.
		* @param default_package The package where the module call appears in.
		* @param proper_name The unqualified module names as appearing in the script text. This name may not contain colons.
		*/
		PModule ResolveUnqualifiedName_mt(const safestring& default_package, const safestring& proper_name);

		/**
		* Same as ResolveUnqualifiedName_mt(), but returns all matching modules, in the order of priority. 
		* This is used for potential name clash detection.
		* Package reload is attempted only if no match was found by the guide files.
		*/
		std::vector<PModule> ResolveUnqualifiedNameMulti_mt(const safestring& default_package, const safestring& propername);

		/**
		* @brief Try to resolve a qualified module name, or return NULL.
		*
		* This function tries to find the module according to all of the Acapella
		* name lookup rules, assuming that the name is used in the context of this resolve point.
		*
		* @param absolute_packagename The package name, empty string is the main package.
		* @param proper_modulename The bare module name.
		* @param try_library_reload If the name is not found by the guide, the actual module/procedure libraries will be loaded into memory, to discover new or changed modules.
		*/
		PModule ResolveQualifiedName_mt(const safestring& absolute_packagename, const safestring& proper_modulename, bool try_library_reload=true);

		/**
		* Finds if the module is currently registered in this resolvepoint. Parent resolvepoints are not queried. Libraries are not reloaded. 
		* This function is used only internally by Acapella when managing the module lookup system.
		*/
		PModule IsInLocalMap_mt(const safestring& package, const safestring& propername) const;


		/// Return true if this resolvepoint does not currently have any module registered. Parent resolvepoints are not queired, libraries not reloaded.
		bool IsEmpty() const {return localmap_.empty();}

		/**
		* Calls Package::GetGuideLibraryName() for all contained Package objects. If a library is found, inserts it
		* into libnames map under the key "package::propername". Parent resolvepoints are not consulted. This function
		* is called from ComposeLastSeen_mt().
		* @param package The package name, empty string is the main package.
		* @param propername The bare module name.
		* @param libnames The container to be updated.
		*/
		void GetGuideLibraryNames_mt(const safestring& package, const safestring& propername, std::map<safestring, safestring>& libnames) const;

		/**
		* Composes a message about in which libraries the module was last encountered according to the .guide files.
		* If the callname parameter is unqualified (no colons), then the Using() directives and global namespace
		* are also considered, in addition to default_package. 
		* @param default_package The package where the module call appears in.
		* @param callname The module name as appearing in the script text, with or without the package name.
		* @param lastseen The output parameter. If any hits were found, this will be in format: " ("package::name" last seen in "library"; ...)". If there were no hits, libnames will be an empty string after the call.
		*/
		void ComposeLastSeen_mt(const safestring& default_package, const safestring& callname, safestring& lastseen) const;

		/// Check for .proc files controlled by packages of this resolvepoint and changed on disk; return the number of such .proc files.
		int CheckForChangedProcFiles_mt(std::set<ResolvePoint*>& processed);

		/**
		* Useful for tab completion. Finds all modules which could be called in the context of this ResolvePoint
		* with call names starting with the specified text.
		*
		* This only uses guide files, libraries are not reloaded. If needed, library reload has to be done beforehand.
		*
		* @param default_package Default package name to try if callname_start does not contain a package name.
		* @param callname_start The start of the module call name, may or may not include package name.
		* @param results Found matching module or package names are inserted here.
		* @param add_type Append token type in the end of each result string: "?1" - module name, "?5" - package name.
		*/
		void CompleteCallName_mt(const safestring& default_package, const safestring& callname_start, std::set<safestring>& results, bool add_type) const;

		/// Register the resolve point by a filesystem change monitor, to get immediate notifications about changed proc files.
		void RegisterByMonitor(Monitor& monitor);

	public: // virtual overrides
		virtual PSharable DoClone() const;
		virtual safestring GetDescription() const;
		virtual bool ScanModules(ScanModulesVisitor& visitor, const safestring& package_mask="*", const safestring& propername_mask="*", unsigned int scanflags=scan_recursive);
		virtual void NotifyMonitor(const safestring& monitored_path, const safestring& partial_path, mon_event_t event);

	private: // implementation
		ResolvePoint();
		void LogEntry(const safestring& msg) const;
		ResolvePoint(const ResolvePoint& b); // not implemented
		void operator=(const ResolvePoint& b); // not implemented
		PModule ResolveQualifiedNameByPackageReload_mt(const safestring& package, const safestring& propername);
		void CompleteProperName_mt(const safestring& package_name, const safestring& propername_start, std::set<safestring>& results, const safestring& addprefix="", bool add_type=false) const;
		void Swap(ResolvePoint& b);
		PPackage& Loader_lk(size_t i);
		const PPackage& Loader_lk(size_t i) const;

	private: // data protected by mx_
		mutable Nbaseutil::boost_recursive_mutex mx_; // protecting all data in this 'private' section

		typedef std::vector<safestring> using_t;
		using_t using_;

		friend class PackageScanVisitor;
		typedef std::vector<std::pair<safestring, PModule> > localentry_t;
		typedef std::map<safestring, localentry_t> localmap_t;
		localmap_t localmap_;

		PResolvePoint static_parent_;

		/// When calling loaders_ methods, this->mx_ must be locked to ensure consistent mutex lock order
		std::vector<PPackage> loaders_;

		typedef std::set<safestring> known_packages_t;
		known_packages_t known_packages_;

#ifdef _DEBUG
		safestring dbg_resolvepoint_name_;
#endif
	};

	/// Split a module name as appearing in the using directive.
	void SplitUsingName(const safestring& name, safestring& packagename, safestring& modulename);

	/// Split a module name as appearing in the script, into package name and proper module name. Return true if package name was absolute.
	bool SplitModuleName(const safestring& name, safestring& packagename, safestring& modulename);


} //namespace
#endif
