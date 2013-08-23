#ifndef _IMACRO_SCRIPT_H_INCLUDED_
#define _IMACRO_SCRIPT_H_INCLUDED_

#include <list>
#include <vector>
#include "datablock.h"
#include "executive.h"
#include "modulecall.h"

namespace NAcapellaHttp {
	class XmlNode;
}

namespace NIMacro {

	// forward declarations
	class Script;
	typedef ThreadSharablePointer<Script> PScript;
	class ScriptFile;
	typedef ThreadSharablePointer<ScriptFile> PScriptFile;
	class ModPar;
	class Mod;
	struct ParserState;
	class Module;
	class proc_support;
	class proc_entry;
	class proc_support;
	class ModuleCall;
	class TextHolder;
	typedef ThreadSharablePointer<TextHolder> PTextHolder;
	class ResolvePoint;
	typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;
	class ErrorListener;
	typedef void (*UnregCallback)(ErrorListener* el, void* callback_data);

	/// Callback interface to be used with Script::RegisterExitListener().
	class DI_IMacro ErrorListener {
		UnregCallback callback_;
		void* callback_data_;
	public:
		ErrorListener(): callback_(NULL), callback_data_(NULL) {}
		virtual void Notify(ExecutionContext& ctx, unsigned int errcode, const char* errmsg, Script& script)=0;
		virtual ~ErrorListener();
		void SetUnregCallback(UnregCallback callback, void* callback_data);
	};

	/// Callback interface for Script::ScanModuleCalls().
	class ScanModuleCallsVisitor {
	public:
		virtual bool Visit(ModuleCall& call, void* callback_data)=0;
		virtual ~ScanModuleCallsVisitor() {}
	};

	/// A struct for holding a script attachment. 
	struct ScriptAttachment {
	public: // interface
		ScriptAttachment() {}
		ScriptAttachment(const safestring& name, const safestring& type, const safestring& action, const DataItem& value)
			: name_(name), type_(type), action_(action), value_(value) {}
		void swap(ScriptAttachment& a) {
			a.name_.swap(name_);
			a.type_.swap(type_);
			a.action_.swap(action_);
			a.value_.swap(value_);
		}
	public: // data
		/// Acapella item name or path for storing the attachment under. If the name is empty, any store request is ignored.
		safestring name_;
		/// A type for distinguihing different types of attachments. Nomenclature not fixed, some defined values: "parameters", "inputiterator".
		safestring type_;
		/**
		* What to do with the script attachment when starting the script:
		*   * "store": store under the specified name (the default).
		*   * "puch": push to a vector with the specified name.
		*   * "evaluate/acapella": evaluate as Acapella expression and store under the specified name.
		*   * "execute/acapella": execute as Acapella script text in the started script context.
		*   * "ignore": attachment is ignored by the Script::Run() method.
		*/
		safestring action_;
		/// The attachment content itself. This may be of any type. For evaluate/acapella and execute/acapella actions it should be a string.
		DataItem value_;
	};

	/// A callback base class for Script::ScanAttachments().
	class ScanAttachmentsCallback {
	public:
		/// Will be called for each attachment by Script::ScanAttachments(). Return false to break the loop early.
		virtual bool operator()(int index, ScriptAttachment& attachment)=0;
		virtual ~ScanAttachmentsCallback() {}
	};


	/** 
	* The Script class holds a linearized module sequence from a script.
	* This is held as a std::vector of ModuleCall's.
	* After initialization, Script is considered to be more or less immutable
	* until next Init() or destruction. There are some exceptions.
	*
	* Script is refcounted, meaning that one should access it only through 
	* the PScript smartpointers.
	*/
	class DI_IMacro Script: public ThreadSharable {
		typedef ThreadSharable super;
	public:
		class ParsingContext;

		/// Create a script from a textholder, which can be also a ScriptFile or proc_entry.
		static PScript Create(PTextHolder textholder);

		/// Obsolete, use Create(PTextHolder) instead.
		static PScript Create(PTextHolder textholder, const ModuleCall* owner) {
			return Create(textholder);
		}

		/**
		* @brief Create a script based on a disk file.
		* @param filename File name or other label which can be resolved by ctx.exec.
		*/
		static PScript Create(const safestring& filename); 

		/**
		* @brief Create a Script object from in-memory script content.
		* @param surrogate_filename Surrogate filename for use in error messages. It is suggested to put the name in braces {...}.
		* @param scripttext Script content. This is swapped away, the parameter will be empty after return.
		* @param owner If present, provides the name lookup context for resolving module names in the script. If absent, the StdLib resolvepoint is used.
		*/
		static PScript Create(const safestring& surrogate_filename, safestring& scripttext, const ModuleCall* owner=NULL);

		/**
		* @brief Create a Script object from in-memory script content. This does not support XML formats, call CreateFromContent() instead if XML support is needed.
		* @param surrogate_filename Surrogate filename for use in error messages. It is suggested to put the name in braces {...}.
		* @param scripttext Script content. This is swapped away, the parameter will be empty after return.
		* @param name_lookup_context Create the script in the context of specified ResolvePoint.
		*/
		static PScript Create(const safestring& surrogate_filename, safestring& scripttext, PResolvePoint name_lookup_context);

		/**
		* @brief Create a Script object from an AccScript XML structure.
		* @param ctx Execution context for executing the extract-from-xml script.
		* @param surrogate_filename Surrogate filename for use in error messages. If empty, the 'name' attribute of the XML node is used.
		* @param acc_script_node The XML structure conforming to AccScript syntax.
		* @param name_lookup_context Create the script in the context of specified ResolvePoint.
		*/
		static PScript Create(ExecutionContext& ctx, const safestring& surrogate_filename, const NAcapellaHttp::XmlNode& acc_script_node, PResolvePoint name_lookup_context);

		/**
		* @brief Create a Script object by an Acapella string or XmlNode dataitem, or by external file/url reference.
		* @param ctx Execution context for looking up Acapella dataitems and executing the extract-from-xml script.
		* @param surrogate_filename Surrogate filename for use in error messages. If empty, the 'name' attribute of the XML node is used.
		* @param src Either filename, URL or Acapella item specification in "<name" or "<<name" syntax.
		* @param name_lookup_context Create the script in the context of specified ResolvePoint.
		* @param script_index If the source appears to be XML (a .msr file), it can contain multiple scripts, use the script_index variable to choose the right one.
		*/
		static PScript Create(ExecutionContext& ctx, const safestring& surrogate_filename, const safestring& src, PResolvePoint name_lookup_context, int script_index=0);

		/**
		* @brief Create a Script object by in-memory file content. Supports AccJob and MacroScript (.msr) XML formats.
		* @param ctx Execution context for executing the extract-from-xml script.
		* @param surrogate_filename Surrogate filename for use in error messages.
		* @param scripttext Script or XML text; the data is possibly swapped away and this parameter will be empty after return.
		* @param name_lookup_context Create the script in the context of specified ResolvePoint.
		* @param script_index If the source appears to be XML (a .msr file), it can contain multiple scripts, use the script_index variable to choose the right one.
		*/
		static PScript CreateFromContent(ExecutionContext& ctx, const safestring& surrogate_filename, safestring& scripttext, PResolvePoint name_lookup_context, int script_index=0);

		/// Obsolete, use Create(ExecutionContext&, const safestring&, const ModuleCall* = NULL) instead.
		static PScript Create(Executive& exec, const safestring& filename, int startpos=0, int firstline=1, int lastline=0) {
			return Create(filename);
		}

		/// Obsolete, use Create(const safestring& surrogate_filename, const safestring& scripttext, const ModuleCall* owner=NULL) instead.
		static PScript CreateEx(Executive& exec, const safestring& surrogate_filename, const safestring& scripttext, Script* mainscript=NULL, Script* fwd_lic=NULL, int startpos=0, int firstline=1, const safestring& procname="", PScriptFile sf=NULL, const ModuleCall* owner=NULL) {
			safestring tmp(scripttext);
			return Create(surrogate_filename, tmp, owner);
		}

		/// Obsolete, use Create(const safestring& surrogate_filename, const safestring& scripttext, const ModuleCall* owner=NULL) instead.
		static PScript Create(Executive& exec, const safestring& surrogate_filename, const safestring& scripttext, int startpos=0, int firstline=1, const safestring& procname="", ParserState* parentstate=NULL) {
			safestring tmp(scripttext);
			return Create(surrogate_filename, tmp, NULL);
		}

		static PScript Create(const ModuleCall& call, PResolvePoint resolvepoint);

		/// Return a reference to a global dummy Script object. This object may not be modified.
		static Script& Void();

		/// Initialize with a textholder
		void Init(PTextHolder textholder);

		/// Obsolete, use Init(PTextHolder) instead.
		void Init(PTextHolder textholder, const ModuleCall* owner) {
			Init(textholder);
		}

//		/// Swap the content of two Script's. Used in ScriptFile::Reset().
//		void Swap(Script& b);

		/// Forget any parsed subscripts. To be called if the called procedures might have been changed.
		void FlushParsedSubscripts();

		// Override ThreadSharable method
		PScript Clone() const;

		/**
		* @brief Scan all module calls in this and in subscripts, if recursive=true specified.
		* @param visitor The visitor object
		* @param callback_data Optional callback data pointer to forward to the Visit() call.
		* @param modflag_mask Call only modules whose module flags include at least one of the bits in the mask.
		* @param recursive Perform deep recursion into subscripts. If subscripts are created at runtime only, then this migh not succeed always.
		*/
		bool ScanModuleCalls(ScanModuleCallsVisitor& visitor, void* callback_data=NULL, unsigned int modflag_mask=0xffffffff, bool recursive=true);

		/// Return pointer to the ResolvePoint which is used for resolving module names in this Script.
		PResolvePoint GetNameLookupContext() const;

		/// Return reference to the TextHolder object from which this Script has been created.
		TextHolder& GetTextHolder() {return *textholder_;}

		/// Return reference to the TextHolder object from which this Script has been created.
		const TextHolder& GetTextHolder() const {return *textholder_;}

		/// Return the text the TextHolder is holding.
		safestring GetLastParsedText() const;

		/**
		* Return the pointer to the nearest enclosing ScriptFile object or NULL, if there is no ScriptFile object.
		* If the associated TextHolder is a ScriptFile, then this function just returns the TextHolder,
		* otherwise it will look up in the static parent chain of the textholder.
		*/
		PScriptFile GetScriptFile() const;

		/**
		* If this is a procedure, returns the starting character position 
		* (index of 'p' in "proc") in the script file. Otherwise returns 0.
		*/
		int GetStartPos() const;

		/**
		* Return info about module call filename, line number and module name.
		* If ModuleNo is out of bounds, return empty string
		* @param ModuleNo Index of the module call.
		* @param format 0 = "filename(lineno) [module]", 1 = filename, 2=lineno, 3=modulename.
		*/
		safestring GetCallInfo(int ModuleNo, int format=0) const;


		/// Return pointer to the contained ModuleCall object, or NULL if ModuleNo index is out of bounds.
		const ModuleCall* GetCall(int ModuleNo) const;

		/// Return pointer to the contained ModuleCall object, or NULL if ModuleNo index is out of bounds.
		ModuleCall* GetCall(int ModuleNo);

		/**
		* Return pointer to the nearest ModuleCall object, or NULL if there is no such ModuleCall.
		* @param bytepos The text position to look, in bytes, in the TextHolder units.
		*/
		ModuleCall* GetCallByBytePos(int bytepos);

		/// Add an empty call to the end of the script.
		ModuleCall* AddCall();

		/// Constants used in this class
		enum constants {
			/// See ScriptFile::replace() and Script::InsertText().
			find_changed_content = -1,
		};

		// 
		/**
		* Make change to the script. This will update the script text and 
		* try to initialize calls_ array via Init().
		* If this does not succeed, an exception is thrown unless exec 
		* absorbs the exception via HandleException() call.
		*
		* NB! This call violates the immutability rule of Script after initialization.
		* Use with care and ensure that other threads are not using the Script object at the same time.
		*
		* @param exec	Executive supervisor; can provide module lookup override via FindModule()
		*				and suppress exceptions via HandleException() method.
		* @param text	The text to be inserted. One can pass full script text by specifying 
		*				before=0 and deletecalls=find_changed_content.
		* @param before The index of the module call before what to insert new text.
		*				Can be procheader_signature constant (-111) to begin insertion before the 
		*				procedure definition corresponding to this Script.
		* @param deletecalls How many module calls to delete from the current Script.
		*				Can be the find_changed_content constant in order to automatically find out 
		*				the relevant part to change. Identification of changed parts is important
		*				for minimizing the size of the UnDo stack.
		*/
		void InsertText(Executive& exec, const char* text, int before, int deletecalls);

		/// To group multiple InsertText() calls into one undo/redo action, call BeginTransaction()/EndTransaction() before and after InsertText() calls.
		void BeginTransaction();

		/// To group multiple InsertText() calls into one undo/redo action, call BeginTransaction()/EndTransaction() before and after InsertText() calls.
		void EndTransaction();


		/** Return the number of module calls in the Script.
		* @param recursive Include the subscripts in the count.
		*/
		int GetModuleCount(bool recursive=false) const;	


		/**
		* Run a master script under the specified Executive object.
		* @param exec The executive object.
		* @param db The datablock providing initial data values and containing the result data after the script run. If NULL is passed, an empty datablock is created for the script run.
		*/
		void Run(Executive& exec, PDataBlock db=NULL) const;

		/**
		* Run a child script in the parent script context, under the same Executive as the parent script.
		* @param parent_ctx The execution context of the parent script.
		* @param db The datablock providing initial data values and containing the result data after the script run. If NULL is passed, the datablock of the parent script is used.
		*/
		void Run(ExecutionContext& parent_ctx, PDataBlock db=NULL) const;

		/**
		* Run a child script in the parent script context, under the specified Executive.
		* @param parent_ctx The execution context of the parent script.
		* @param exec The executive object for controlling the script execution.
		* @param db The datablock providing initial data values and containing the result data after the script run. If NULL is passed, the datablock of the parent script is used.
		*/
		void Run(ExecutionContext& parent_ctx, Executive& exec, PDataBlock db=NULL) const;

		/// Return the filename where this Script is contained in. Names prefixed by "db:" designate Opera database items.
		const safestring& GetFileName() const;

		/// If this Script corresponds to a procedure, return the procedure name, otherwise empty string.
		safestring GetProcName() const;

		/**
		* Return subscript of this script corresponding to module call indices loc[pos], 
		* loc[pos+1] in the first-level subscript, ...
		* Each index refers recursively to next subscript. 
		* If there are no indices (pos==loc.size()), returns this script.
		* If any of the encountered modules does not have an associated subscript, return NULL.
		*/
		PScript GetSubScript(const std::vector<unsigned int>& loc, unsigned int pos=0) const; 

		/**
		* Recompose the script text from the parsed ModuleCall list. This is used by
		* the Opera plugin before distributing the script to RMCA network.
		* 
		* @param exec		Executive object, which can override module name resolving.
		* @param mergeprocs Include UserLib procedure definitions in the returned text,
		*					making the script self-contained. Starting from Acapella 2.6 
		*					StdLib procedures are not merged any more.
		*					If this parameter is false, then only the procedures present
		*					in the original script file are included in the returned text.
		* @param removecomments Remove all comments from the returned text. Note that
		*					some comments present in the original script file may be lost 
		*					in any case.
		* @param addlicensegrant Add a GrantLicense() module call in the beginning of the 
		*					returned script and all contained procedures, enabling 
		*					running the returned script in the RMCAnalyzer.exe program 
		*					without access to the License Server. The licenses granted
		*					are those available for this machine at the time of call.
		*/
		safestring ComposeText(Executive& exec, bool mergeprocs=false, bool removecomments=false, bool addlicensegrant=false) const;

		/// Same as the other ComposeText() but uses default Executive object.
		safestring ComposeText(bool mergeprocs=false, bool removecomments=false, bool addlicensegrant=false) const;

		/// Return modulecall index of call in the script, or -1 if the call is not present in this Script.
		int GetCallNo(const ModuleCall* call) const;

		/// Return list of missing licenses at the moment, which would be needed for running all modules present in the script and parsable subscripts.
		safestring GetMissingLicenses() const;

		bool IsFrozen() const {return frozen_;}

		/// Return true if the script is not frozen and the scriptfile is newer than script.
		bool NeedsReparsing(bool deep_reparsing=false) const;

		/// Return true if the script is frozen and the scriptfile is newer than script.
		bool IsObsolete() const;

		/// Replace the module call. NB! this method violates the rule of Script immutability after creation, use with care!
		void ReplaceModuleCall(const ModuleCall& call, int moduleno);

		/// Return pointer to the parent script, or NULL, if this is not a subscript.
		const Script* GetParentScript(ExecutionContext& ctx) const;

		/// Return pointer to the parent script, or NULL, if this is not a subscript.
		Script* GetParentScript(ExecutionContext& ctx) {return const_cast<Script*>(const_cast<const Script*>(this)->GetParentScript(ctx));}

		/// Return reference to the main startup script
		const Script& GetMainScript(ExecutionContext& ctx) const;

		/// Return reference to the main startup script		
		Script& GetMainScript(ExecutionContext& ctx) {return const_cast<Script&>(const_cast<const Script*>(this)->GetMainScript(ctx));}

		/// Obsolete, do not use.
		Script& GetMainScript();

		bool IsMainScript(ExecutionContext& ctx) const {return ctx.IsMainScript();}

		bool IsEncrypted() const;

		// Prereserve place for specified amount of module calls to be entered later in the script.
		void Reserve(int modcount) {calls_.reserve(modcount);}

		/**
		* This function is called in the main script object after any Script::Run, including subscripts.
		* If the run is terminated by an error then errcode & ERR_ERROR==true.
		* The function passes the notification to all listeners registered via RegisterScriptExitListener().
		* @param ctx The current execution context.
		* @param errcode The error/warning code resulting from the exiting script execution.
		* @param errmsg The error/warning messages.
		* @param exiting_script The script being exited (*this is the main script, where the listeners are registered).
		*/
		void NotifyScriptExit(ExecutionContext& ctx, unsigned int errcode, const char* errmsg, Script& exiting_script);

		/**
		* Registers or unregisters error listeners. These will be notified
		* about script exit error in NotifyScriptExit().
		*/
		void RegisterScriptExitListener(ErrorListener* el, bool register_el=true);

		/// Callback interface to use with Script::Scan()
		class DI_IMacro CallbackModuleCall {
		public:
			virtual ~CallbackModuleCall() {}
			virtual bool NotifyScriptScan(ModuleCall& call, void* callback_data)=0;
		};

		/**
		* Scan the script and call the callback repeatedly during the process.
		* If callback returns false, then cancel the scan and return false.
		* @param callback An object derived from ScriptScanCallback.
		* @param recursive Descend into subscripts. The subscript is *not* created automatically if it doesn't exist at the moment.
		*		The callback is called for the subscript-owning ModuleCall before descending,
		*		so the callback has an option to create the subscript via the ModuleCall::GetSubScript() call.
		* @param subscript_only Call the callback only for ModuleCall's with MOD_SUBSCRIPT flag.
		* @param callback_data Will be passed back to the callback.
		*/
		bool Scan(CallbackModuleCall& callback, bool recursive, bool subscript_only, void* callback_data=NULL);

		/// Callback interface to use with Script::ScanScripts()
		class DI_IMacro CallbackScript {
		public:
			virtual ~CallbackScript() {}
			virtual bool NotifyScriptScan(Script& script, void* callback_data)=0;
			/// Called for modulecalls, which should have subscripts attached, but are currently not.
			virtual bool NotifyScriptScanMissingSubScript(ModuleCall& call, void* callback_data) {return true;}
		};

		/// Notifies callback for this Script and all contained subscripts, recursively.
		bool ScanScripts(CallbackScript& callback, void* callback_data=NULL);

		/// Callback interface to use with Script::ScanScriptFiles()
		class DI_IMacro CallbackScriptFile {
		public:
			virtual ~CallbackScriptFile() {}
			virtual bool NotifyScriptScan(ScriptFile& scriptfile, Script& script, void* callback_data)=0;
		};

		/// Notifies callback for the ScriptFile of this Script and for all distinct ScriptFiles of contained subscripts, recursively.
		bool ScanScriptFiles(CallbackScriptFile& callback, void* callback_data=NULL);

		void Freeze();

		void GetInputsScript(safestring& buffer) const;

		bool HasSinglewell() const {return contains_singlewell_;}

		bool IsAas() const {return aas_script_;}

		//void SetOwner(ModuleCall* owner);

		/// Return true when called after BeginTransaction() and before EndTransaction().
		bool InTransaction() const {return in_transaction_;}

		void SetIsThreadMaster(bool is_thread_master) {is_thread_master_ = is_thread_master;}

		bool IsUsingExplicitLanguage() const {return explicit_;}

		/// Adds a new attachment. The attachment is swapped away inside the Script object. Returns the index of the attachment.
		int AddAttachment_mt(ScriptAttachment& attachment);

		/// Swaps the existing attachment with the parameter. Throws if i is out of range. 
		void SwapAttachment_mt(int i, ScriptAttachment& attachment);

		/// Returns the number of script attachments.
		int GetAttachmentCount_mt() const;

		/// Returns i-th attachment. Throws if i is out of range. The returned attachment content is deep-copied for avoiding multithreading problems.
		void GetAttachment_mt(int i, ScriptAttachment& attach) const;

		/// Scan attachments and call callback.operator() on each of them. A mutex is locked during this, so the callback should not do anything slow or complicated.
		void ScanAttachments_mt(ScanAttachmentsCallback& callback);

		/// Move attachments from the Script object and to the \a detached list. If \a pattern is a default-constructed object, all attachments will be moved, otherwise only those matching the non-empty fiels in \a pattern.
		void DetachAttachments_mt(const ScriptAttachment& pattern, std::list<ScriptAttachment>& detached);

		/// Returns true if there is a catch_warning block upstream.
		bool AreWarningsCatched(int current_call_index) const;

	private: // implementation

		// constructors
		Script();
		Script(const Script& b);
		~Script();

		void SetCallsParent();
		void operator=(const Script&);

		bool ForwardLic() const;


		/**
		* Reparse the contained script text.
		* @param deep Reparse called procedures in deep recursion.
		*/
		void Reparse(bool deep=false);

		void ParseFirstStage(ParserState& state);
		void ParseSecondStage(ParserState& state);

		void ExecuteAttachments_mt(ExecutionContext& ctx) const;

	protected: // virtual overrides

		virtual PSharable DoClone() const;
		virtual Nbaseutil::safestring GetDescription() const;
		virtual SafeValue DoVerb(const char* verb, const SafeValue& arg1, const SafeValue& arg2);

	private: // implementation
		int FindCatchHandler(int curr_moduleno, const safestring& handler) const;
		void EnterCatchHandler(ExecutionContext& ctx, const safestring& msg, int errcode, int catch_module_no) const;
		static safestring FindErrorHandlerInParentContextsOf(ExecutionContext& ctx0);

	private: // data
		safestring procname_;
		PTextHolder textholder_;	// must be before calls_, because ModuleCall destructors might want to access it.

		typedef Nbaseutil::mb_allocator_typedef< ModuleCall >::allocator calls_allocator_t;
		typedef std::vector<ModuleCall, calls_allocator_t> calls_t;
		calls_t calls_;				// list of module calls in the script.

		bool in_rollback_state_;
		bool frozen_;	// 04.02.2004, for debug-build consistency checks.
		bool contains_singlewell_; // set true in parsing stage if the script or any subscript contains a MOD_SINGLEWELL module.
		bool aas_script_; // set true in parsing stage if the script represents an Acapella Assay Sequence.

		typedef Nbaseutil::mb_allocator_typedef< std::map<safestring, safestring>::value_type >::allocator procs_allocator_t;
		typedef std::map<safestring, safestring, procs_allocator_t> procs_t;
		void ComposeText(Executive& exec, bool mergeprocs, bool removecomments, procs_t& procs, safestring& text) const;

		typedef Nbaseutil::mb_allocator_typedef< std::pair<safestring, safestring> >::allocator parsed_inputs_allocator_t;
		typedef std::vector<std::pair<safestring, safestring>, parsed_inputs_allocator_t > parsed_inputs_t;
		parsed_inputs_t parsed_inputs_;

		// const ModuleCall* owner_; // in subscripts, pointer to the owner modulecall in the parent script. In main script - NULL.
		int parse_version_;
		int deep_parse_version_;
		typedef Nbaseutil::mb_allocator_typedef< ErrorListener* >::allocator ErrorListeners_allocator_t;
		typedef std::vector<ErrorListener*, ErrorListeners_allocator_t> ErrorListeners_t;
		ErrorListeners_t ErrorListeners_;

		bool in_transaction_;
		bool is_thread_master_;
		bool explicit_;
		bool attachments_modified_;

		friend class ModuleCall;
		typedef std::map<const Module*, PScript> parsed_subscripts_t;
		parsed_subscripts_t parsed_subscripts_;

		/// This mutex protects all data in the Script object which might mutate in multiple threads.
		mutable Nbaseutil::boost_recursive_mutex mx_;
		std::vector<ScriptAttachment> attachments_;

		void* reserved_;
	};

	void ParseArgument(const safestring& Arg, safestring& Name, safestring& Value);
	// aux function

	/// Find s1, s2, t1, t2 such that s.replace(s1, s2-s1, t.substr(t1,t2-t1)) == t
	DI_IMacro void find_changed_part(const safestring& s, const safestring& t, int& s1, int& s2, int& t1, int& t2);

	DI_IMacro safestringlist GetRequiredLicenses(const Script* s);

	/// A struct to use with FindGrantLicenseCalls().
	struct gl_pos {
		safestring::size_type pos_, procstart_, procend_;
		safestring procname_;
		gl_pos(safestring::size_type pos=0, safestring::size_type procstart=0, safestring::size_type procend=Nbaseutil::safestring::npos, const safestring& procname="")
			: pos_(pos), procstart_(procstart), procend_(procend), procname_(procname) {}
	};

	/**
	* Find GrantLicense() module calls in the main script and in the contained procedures.
	*/
	DI_IMacro void FindGrantLicenseCalls(const safestring& script, std::vector<gl_pos>& positions);

	/// Return module name in form "package::propername" by the Module pointer.
	DI_IMacro Nbaseutil::safestring GetFullModuleName(const Module* m);

	template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyCurrentScriptChanges(ExecutionContext& ctx, const PScript new_script) {
			if (IsForwarded(fwd_notify_scriptchange)) {
				PContainer c = Cnt("filename", new_script? new_script->GetFileName(): "", "procname", new_script? new_script->GetProcName(): "");
				fwd_.Forward(ctx, "notify_scriptchange", c);
			} 
			super::NotifyCurrentScriptChanges(ctx, new_script);
		}

        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no) {
			if (IsForwarded(fwd_notify_modulecall)) {
				PContainer c = Cnt("modulename", call? GetFullModuleName(&call->GetModule()): "", "filename", call? call->GetFileName(): "", "lineno", call? call->GetLineNo(): 0);
				fwd_.Forward(ctx, "notify_modulecall_begins", c);
			} 
			super::NotifyModuleCallBegins(ctx, call, no);
		}
       template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyModuleCallEnds(ExecutionContext& ctx, const ModuleCall* call, int no) {
			if (IsForwarded(fwd_notify_modulecall)) {
				PContainer c = Cnt("modulename", call? GetFullModuleName(&call->GetModule()): "", "filename", call? call->GetFileName(): "", "lineno", call? call->GetLineNo(): 0);
				fwd_.Forward(ctx, "notify_modulecall_ends", c);
			} 
			super::NotifyModuleCallEnds(ctx, call, no);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyScriptStarted(ExecutionContext& ctx) {
			if (IsForwarded(fwd_notify_scriptrun)) {
				PContainer c = Cnt();
				fwd_.Forward(ctx, "notify_script_started", c);
			} 
			super::NotifyScriptStarted(ctx);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyScriptFinishedNormally(ExecutionContext& ctx) {
			if (IsForwarded(fwd_notify_scriptrun)) {
				PContainer c = Cnt();
				fwd_.Forward(ctx, "notify_script_finished", c);
			} 
			super::NotifyScriptFinishedNormally(ctx);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::NotifyScriptAborted(ExecutionContext& ctx, int mode) {
			if (IsForwarded(fwd_notify_scriptrun)) {
				PContainer c = Cnt("mode", mode);
				fwd_.Forward(ctx, "notify_script_aborted", c);
			}
			super::NotifyScriptAborted(ctx, mode);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    bool ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::ProcessWarning(ExecutionContext& ctx, unsigned int Reason, const char* Message) {
			if (IsForwarded(fwd_notify_warning)) {
				PContainer c = Cnt(
					"modulename", GetFullModuleName(&ctx.Call().GetModule()), 
					"filename", ctx.Call().GetFileName(), 
					"lineno", ctx.Call().GetLineNo(),
					"warning", Message,
					"flags", int(Reason));
				fwd_.Forward(ctx, "notify_warning", c);
			}
			return super::ProcessWarning(ctx, Reason, Message);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::ConsoleOut(ExecutionContext& ctx, const Nbaseutil::safestring& text) {
			if (IsForwarded(fwd_consoleout)) {
				PContainer c = Cnt("text", text);
				fwd_.Forward(ctx, "consoleout", c);
			} else {
				super::ConsoleOut(ctx, text);
			}
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
		SafeValue ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::GetInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type) {
			if (safestring(input_type)=="w") {
				if (IsForwarded(fwd_singlewell)) {
					PContainer c = Cnt("label", label);
					return fwd_.ForwardAndWait(ctx, "singlewell", c);
				}
			} else {
				if (IsForwarded(fwd_input)) {
					PContainer c = Cnt("label", label, "type", input_type);
					return fwd_.ForwardAndWait(ctx, "input", c);
				}
			}
			return super::GetInput(ctx, label, caller, input_type);
		}
        template<class FORWARDER, class BASE_EXECUTIVE_CLASS>
	    void ExecutiveForwarder<FORWARDER,BASE_EXECUTIVE_CLASS>::OutputItem(ExecutionContext& ctx, const char* label, const SafeValue& item) {
			if (item.GetType()==Sharable) {
				PDataBlock content = dynamic_cast<DataBlock*>(item.GetSharable().Pointer());
				if (content) {
					Nbaseutil::safestring view = content->GetString("viewtype");
					ForwardMask::fwd_t flag=ForwardMask::fwd_t(0);
					if (view=="imageview") {
						flag = fwd_imageview;
					} else if (view=="tableview") {
						flag = fwd_tableview;
					} else if (view=="graphview") {
						flag = fwd_graphview;
					} else if (view=="htmlview") {
						flag = fwd_htmlview;
					}
					if (IsForwarded(flag)) {
						PContainer c = content->ToContainer();
						c->Put(label, "label");
						fwd_.Forward(ctx, view, c);
					}
				}
			} else {
				if (IsForwarded(fwd_output)) {
					PContainer c = Cnt("label", label, "value", item);
					fwd_.Forward(ctx, "output", c);
				} 
			}
			super::OutputItem(ctx, label, item);
		}


} // namespace

#endif
