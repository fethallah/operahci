#ifndef _IMACRO_EXECUTIVE_H_INCLUDED_
#define _IMACRO_EXECUTIVE_H_INCLUDED_

#include <set>
#include <string>

namespace NIMacro {

	// Instance class state is determined by script. 
	// Everything one can change by script should be included in the Instance class state.

	// Executive class state (including the behaviour of virtual methods) is determined by the 
	// calling method of IMACRO.DLL and other C++-level twiddling.


	// forward declarations
	class Script;
	typedef ThreadSharablePointer<Script> PScript;

	class DataBlock;
	typedef ThreadSharablePointer<DataBlock> PDataBlock;

	class Instance;
	typedef ThreadSharablePointer<Instance> PInstance;

	class DataBlockCallback;
	class Module;
	class ModuleCall;
	class Executive;
	class SafeValue;
	class DataItem;
	class IO_module;
	class Mod;
	struct ExecutionContext;
	class Executive;
	class RunFrame;
	class Debugger;
	Nbaseutil::safestring GetScriptLocation(ExecutionContext* ctx);

	/**
	* Intrusive smartpointer for Executive objects. Normally not used as Executive objects
	* are mostly created on stack. Note that one has to call Executive::SetAutoDelete() 
	* for smartpointers to delete the object automatically. If Executive is not autodelete,
	* then PExecutive smartpointers are functionally equivalent to ordinary pointers.
	*/
	typedef RefcountedPointer<Executive> PExecutive;

	/**
	* A class (called struct because of better back-compatibility) encapsulating the current execution state, 
	* primarily in the procedure call hierarchies.
	* Inside modules the object is accessible by Mod::GetExecutionContext() function.
	* In recursive call trees a dynamic linked chain of ExecutionContext objects is built, which can be navigated
	* by the GetParent() member function.
	*/
	struct DI_IMacro ExecutionContext {
	public: // interface
		
		/// Construct a master context object (without parent). The Instance object has to have correct Script and DataBlock attached.
		ExecutionContext(Instance& inst, Executive& e, bool parsing=false);

		/// Construct a master context object (without parent) for running the specified ModuleCall. The Instance object has to have correct DataBlock attached.
		ExecutionContext(Instance& inst, const ModuleCall& call, Executive& exec, bool parsing=false);

		/// Construct a child context object for executing specified script.
		ExecutionContext(ExecutionContext& parent, const Script& s, DataBlock& d, Executive& e, bool parsing=false); 

		/// Construct the context object for executing a single module call c, possibly not encapsulated in any Script object.
		ExecutionContext(ExecutionContext& parent, const ModuleCall& call, Executive& exec, bool parsing=false);

		/// Construct a copy of an ExecutionContext object with changed DataBlock.
		ExecutionContext(ExecutionContext& original, DataBlock& db);

		/// Return a reference to a global dummy ExecutionContext object. This should only be used as a placeholder if the real ExecutionContext is not available.
		static ExecutionContext& Void();

		/// Returns true if this context object is the master one (with no parent).
		bool IsMainScript() const {return parent_==NULL;}

		/// Return true if the object is used for script parsing stage.
		bool IsParsing() const {return parsing_;}

		/// Return the parent context object, or NULL for master context.
		ExecutionContext* GetParent() const {return parent_;}

		/// Return the toplevel parent of this ExecutionContext
		ExecutionContext& GetMaster();

		/// Return the calling ModuleCall object in the parent script context, or NULL for master context.
		const ModuleCall* GetParentCall() {return parent_? parent_->GetCall(): NULL;}

		/// Return the current module call underway, or NULL if the context object is not associated with a running module call.
		const ModuleCall* GetCall() const {return call_;}

		/// Set the current module call pointer. The module call object must remain alive during the lifetime of the ExecutionContext object. The module call should be contained in the Script object associated with the ExecutionContext object.
		void SetCall(const ModuleCall* call) {call_ = call;}

		/// Return reference to the current module call, or reference to ModuleCall::Void()  if the context object is not associated with a running module call.
		const ModuleCall& Call() const;

		/// Return reference to the current Script object.
		const Script& GetScript() const {return script_;}

		/// Return a reference to the Instance object associated with the current execution stack.
		Instance& GetInstance() {return inst;}

		/// Return a reference to the DataBlock object associated with the current context.
		DataBlock& GetDataBlock() {return db;}

		/// Return a reference to the Executive object associated with the current context.
		Executive& GetExecutive() {return exec;}

		/// If a modulecall is run in parallel regime (see parallel() function), returns the total number of parallel executions needed, otherwise 1.
		int GetParallelSize() const {return parallel_size_;}

		/// If a modulecall is run in parallel regime (see parallel() function), returns index of the current parallel execution.
		int GetParallelIndex() const {return parallel_index_;}

		/// During a module run, returns reference to the Mod module object, otherwise throws.
		Mod& GetMod() const {
			if (mod_) {
				return *mod_;
			} else {
				throw Nbaseutil::Exception(ERR_PROGRAM_ERROR, "ExecutionContext::GetMod() called from outside of module call.");
			}
		}

		/// During a module run, returns pointer to the corresponding Mod object, otherwise NULL.
		Mod* GetModPtr() const {return mod_;}

		/// Announce an input value for later Input() module calls. This is called from ProvideInput() module, for example.
		void SetProvidedInput(const Nbaseutil::safestring& label, const DataItem& value);

		/// Query an input value set by SetProvidedInput() call. This context object and all its parents are examined. Undefined item is returned if the input is not found.
		DataItem GetProvidedInput(const Nbaseutil::safestring& label) const;

		/// Associates a data item with a given module call. 
		void SetModuleCallMetaItem(const ModuleCall* call, const Nbaseutil::safestring& label, const DataItem& value);

		/// Returns a data item previously set by SetModuleCallMetaItem().
		DataItem GetModuleCallMetaItem(const ModuleCall* call, const Nbaseutil::safestring& label) const;

		/** 
		* Called by module implementation in order to announce it's decision to branch the script execution.
		* This can and must be done only by modules which have declared themselves as branching (see Mod::AnnounceBranching()).
		* @param tobranch Specify whether branching should occur or not.
		* @param branch_jump If not 0, overrides the default branch jump (stored in ModuleCall object).
		*/
		void SetToBranch(bool tobranch, int branch_jump=0) {tobranch_=tobranch; branch_jump_=branch_jump;}	

		/// Return the currently running module call index.
		int GetCurrentModuleNo() const {return current_module_no_;}

		/// Return the index of previously run module call, or -1 if this is the first module call running in the current Script.
		int GetPreviousModuleNo() const {return prev_module_no_;}

		/// Add the current line number and module name to the error message
		Nbaseutil::safestring AddLocationInfo(const Nbaseutil::safestring& msg) const; 

		/// For internal use by Acapella engine. Sets the execution context in a parallel execution mode, if parallel_size>1. Used by the ModuleCallJob class.
		void SetParallelSize(int parallel_size);

		/// For internal use by Acapella engine. Assigns job index to the execution context in a parallel execution mode. Used by the ModuleCallJob class.
		void SetParallelIndex(int parallel_index) {parallel_index_=parallel_index;}

		/**
		* For internal use by Acapella engine. Returns true if the last module call decided to branch the execution. In this case the branch jump is also returned (value 0 indicates default branch jump). 
		* This function returns correct answer also during the next module call, until next SetToBranch() call. 
		* The branching modules must call SetToBranch() explicitly, whereas for normal modules it is called automatically after the module call.
		*/
		bool ToBranch(int& branch_jump) const {
			if (tobranch_) {
				branch_jump = branch_jump_; 
			}
			return tobranch_;
		}

		///  For internal use by Acapella engine. Assigns the currently executed module call index to the execution context.
		void SetCurrentModuleNo(int k, bool reset_prev_module_no=false) {
			if (current_module_no_>=0) {
				prev_module_no_ = current_module_no_;
			}
			if (reset_prev_module_no) {
				prev_module_no_ = -1;
			}
			current_module_no_=k;
		}

		/// For internal use.
		bool SetAsAttachmentParent(bool is_attachment_parent) {
			bool prev_value = is_attachment_parent_;
			is_attachment_parent_ = is_attachment_parent;
			return prev_value;
		}

		/**
		* Return the host and possibly also port suitable for forming URL-s for the client associated with the current execution context.
		* For example, if the current script is running in an Acc session, the host and port from the Host field in the initial
		* session creation packet are returned. 
		* If there is no host information associated with the current execution context, this function forwards to 
		* Nbaseutil::GetDefaultHostName().
		*/
		safestring GetHost();

	private: // implementation
		ExecutionContext(const ExecutionContext&); // unimplemented - no copy possible!
		void operator=(const ExecutionContext&); // unimplemented - no copy possible!
		void Init();
	public: // data
		/// Reference to the associated Instance.
		Instance& inst; 
	private:
		const ModuleCall* call_;
	public:
		/// Reference to the associated DataBlock.
		DataBlock& db;   
		/// Reference to the associated Executive.
		Executive& exec; 
	private: // friends
		friend class Module;
	public: // typedefs
		struct datalabel_t {
			const ModuleCall* call_;
			Nbaseutil::safestring label_;
			datalabel_t(const ModuleCall* call, const Nbaseutil::safestring& label): call_(call), label_(label) {}
			bool operator<(const datalabel_t& b) const {return call_<b.call_ || (call_==b.call_ && label_<b.label_);}
		};
		typedef std::map<datalabel_t, DataItem> metadata_t;
	public: // internal interface
		/// For internal use.
		void SwapMetaData(metadata_t & metadata) {
			metadata_.swap(metadata);
		}
	private: // data
		const Script& script_;
		ExecutionContext* parent_;
		metadata_t metadata_;
		int parallel_size_;
		int parallel_index_;
		bool parsing_;
		bool tobranch_;			// if module decided to branch in the last call.
		bool is_attachment_parent_; // the ProvideInput() calls in a child script should populate this parent object instead.
		bool in_recurse_;
		Mod* mod_;
		int branch_jump_;
		int current_module_no_;
		int prev_module_no_;

		// in_recurse_ is used by GetScriptLocation(ExecutionContext*)
		friend Nbaseutil::safestring GetScriptLocation(ExecutionContext* ctx);
	};

	/** 
	* A callback base class for driving and monitoring the script execution.
	* This class represents the "execution environment" for the script.
	* A reference to this class is passed along in the exec member of ExecutionContext struct.
	*
	* There are three distinctive usage patterns for Executives; script parsing,
	* syntax-check run and actual run. During script parsing stage only few Executive
	* methods are called, these are described accordingly. In what concerns 
	* syntax-check and actual run, then these appear mostly identical to the Executive,
	* in a matter of fact the Executive itself decides whether this is a syntax-check
	* or an actual run (see Executive::CallSyntaxRun()).
	* For syntax-check run executives there is a convienience SyntaxCheckerBase base class
	* providing common overrides for such executives.
	*
	* The base class Executive realizes ordinary, screening-type execution.
	* This is the one which runmacro() interface function uses.
	* One also uses the direct Executive objects for script parsing, because
	* there is little need to override anything during script parsing.
	*
	* The Executive base class is essentially stateless, which makes it easy to implement
	* derived classes - there are no invariants to worry about. Calling base-class
	* methods is not necessary and functions are quite orthogonal.
	*
	* All methods take a ExecutionContext argument, enabling overridden methods
	* to get info about the current environment, if necessary. The exact content of the structure
	* depends on the method. References can be always dereferenced, but can in some
	* case point to the Void() objects. ctx.exec should be usually identic to *this.
	*/
	class Executive: public mb_malloced {
	public:
		/**
		* Default ctor; assumes creation on stack, usually as a local variable.
		* If you construct it on heap instead, call SetAutoDelete() and use 
		* PExecutive smartpointers to access the object. This is usually not 
		* necessary.
		*/
		Executive(): refcount_(0), autodelete_(false) {}

		/**
		* When creating any Executive-derived object on heap with the 'new'-expression, call this method and 
		* set it autodeletable. If there appear any PExecutive smartpointers to this object,
		* then the object will be autodeleted when the last smartpointer goes out of scope.
		* If there are no smartpointers, then the client code must delete the object explicitly.
		*/
		void SetAutodelete(bool autodelete=true) {autodelete_ = autodelete;}

		/// Return true if autodelete is switched on. Useful for debugging asserts.
		bool GetAutodelete() const {return autodelete_;}

		/**
		* Called for running sub-scripts (procedures, include()-s, eval()-s, etc.)
		* The method must return a reference to an Executive-derived object, which
		* remains valid during the subscript execution. The base class method returns 
		* just *this, thus using the same Executive object for sub-scripts as well.
		*/
		virtual Executive& GetSubExecutive(ExecutionContext& ctx) {return *this;}

		/**
		* Called when the currently executable Script object changes (starting script,
		* entering and leaving subscripts, terminating script. In case of errors
		* the leaving and termination notices can be absent. The base-class method does nothing.
		* @param ctx The current execution context.
		* @param new_script The script whose execution is started. If NULL, then 
		*				the script or subscript is terminating normally.
		*/
		virtual void NotifyCurrentScriptChanges(ExecutionContext& ctx, const PScript new_script) {}

		/**
		* Called when the current DataBlock changes (starting script,
		* entering and leaving subscripts, terminating script. In case of errors
		* the leaving and termination notices can be absent. Currently appears 
		* synchronuously with NotifyCurrentScriptChanges().
		* @param ctx The current execution context.
		* @param new_db New 'current' datablock. If NULL, then the script or subscript is terminating.
		*/
		virtual void NotifyCurrentDataBlockChanges(ExecutionContext& ctx, PDataBlock new_db) {}

		/**
		* Called when the current Instance changes (starting script,
		* entering and leaving subscripts, terminating script. In case of errors
		* the leaving and termination notices can be absent. Currently appears 
		* synchronuously with NotifyCurrentScriptChanges().
		* @param ctx The current execution context.
		* @param new_instance Instance for starting the execution. If NULL, then the script or subscript is terminating.
		*/
		virtual void NotifyCurrentInstanceChanges(ExecutionContext& ctx, PInstance new_instance) {}

		/**
		* Called before each module call.
		* 
		* @param ctx The current execution context.
		* @param call The ModuleCall to run; never NULL.
		* @param no	  Index of the modulecall in the current Script.
		*/
		DI_IMacro virtual void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no);

		/**
		* Called after each module call. If errors occurred, the notification may be absent.
		* 
		* @param ctx The current execution context.
		* @param call The ModuleCall which was run; never NULL.
		* @param no	  Index of the modulecall in the current Script.
		*/
		virtual void NotifyModuleCallEnds(ExecutionContext& ctx, const ModuleCall* call, int no) {}	// no is module call index in the current Script.

		/**
		* Called immediately before the beginning of execution of each Script.
		*/
		virtual void NotifyScriptStarted(ExecutionContext& ctx) {}

		/**
		* Called immediately after the normal termination of each Script.
		*/
		virtual void NotifyScriptFinishedNormally(ExecutionContext& ctx) {}

		/**
		* Called if the user has aborted the script. This is called in the execution 
		* thread after the modulecall end, if the user-abortion request is detected.
		*
		* @param ctx The current execution context.
		* @param mode 1=silent stop, 2=non-silent abort, 3=non-catchable. See Instance::Abort().
		*/
		virtual void NotifyScriptAborted(ExecutionContext& ctx, int mode) {}

		// Change 21.04.2004: replace unused NotifyBeforeEnteringSubScript() and 
		// NotifyAfterLeavingSubScript() with debug support callbacks.

		/**
		* Called after the input parameters have been copied to the module space,
		* and immediately before launching the module. Not called in syntax run.
		* This callback is meant for debugging and autoresuming executives.
		* Return value is ignored/obsolete.
		*/
		virtual bool NotifyDebugBeforeModuleCall(ExecutionContext& ctx, RunFrame& runframe) {return false;}

		/*
		* Called after returning from the module call, after the output 
		* parameters of the module have been inserted in the datablock,
		* but before committing the datablock. This enables the executive
		* to study the values of the data both before and after the call.
		* This function is not called in syntax run and in case of errors 
		* occuring during module call.
		*/
		virtual void NotifyDebugAfterModuleCall(ExecutionContext& ctx, RunFrame& runframe) {}

		/**
		* Obsolete, not used.
		*/
		virtual void NotifyParsingProcedure(ExecutionContext& ctx, const safestring& procname, const safestring& filename, const safestring& procbody, int startpos, int lineno) {}

		/**
		* Called if an exception occurs in the module call during script execution.
		* This function has to return 'true' to ignore the exception and continue 
		* with the next module. This is usually done for syntax-check executives.
		* If the function returns false, then the exception is logged appropriately 
		* and the script execution is aborted. The base-class method just returns false.
		*
		* NB! THIS VIRTUAL METHOD IS CALLED FROM CATCH BLOCKS DURING STACK UNWINDING.
		* BY THIS REASON THIS METHOD MAY NOT THROW ANY EXCEPTIONS!
		*
		* @param ctx The current execution context.
		* @param e The occurred exception. If the method returns false this will be re-thrown.
		*/
		virtual bool HandleException(ExecutionContext& ctx, const Exception& e) {return false;}

		/**
		* Called after module calls which could possible 
		* branching the script execution flow (if/else/end/foreach/while etc. modules).
		* Return true to take the branch. The meaning of 'taking the branch' depends
		* on the module, but is opposite to the 'continue with the next module call in the script.'
		*
		* Return false to continue execution by the next module call in the Script. This is 
		* appropriate for the syntax-checking executives. 
		*
		* The baseclass implementation just returns default_decision.
		* 
		* @param ctx The current execution context.
		* @param default_decision The decision taken by the module itself. In syntax-check run this is 
		*				usually false, as the module didn't execute at all.
		*/
		virtual bool TakeBranch(ExecutionContext& ctx, bool default_decision) {return default_decision;}

		/**
		* This method is called before each modulecall run, after NotifyModuleCallBegins().
		* If this method returns true, then the module is called in the syntax run mode,
		* usually meaning that the actual module implementation is not executed at all.
		* If an Executive-derived class object returns true from this mehotd, then one calls
		* this Executive a "syntax-check executive". 
		*
		* If this method returns false, then normal module run is performed.
		*
		* The baseclass implementation just returns false.
		*/
		virtual bool CallSyntaxRun(ExecutionContext& ctx) {return false;}

		/**
		* Called before passing an explicit input argument to the module.
		* This happens before evaluating the explicit expression given for the argument.
		*
		* Return true to allow argument sending to the module. In this case there follows
		* a SendParameterValue() call for this parameter.
		*
		* Return false to not send argument to the module. This is appropriate
		* mostly for the syntax-check executives as the parameter location in the module
		* remains uninitialized and can contain garbage value. Also syntax-check 
		* modules should return true for PAR_SYNTAX or PAR_SYSTEM parameters as these 
		* might be used in the Mod::SyntaxRun() methods.
		*
		* The baseclass implementation just returns true.
		*
		* @param ctx The current execution context.
		* @param argname Module parameter name
		* @param explicit_expression The explicit value for the argument, as appearing in the module call.
		* @param flag_PAR_SYSTEM True if the module parameter has been registered with the PAR_SYSTEM flag.
		* @param flag_PAR_SYNTAX True if the module parameter has been registered with the PAR_SYNTAX flag.
		*/
		virtual bool SendExplicitParameter(ExecutionContext& ctx, const char* argname, const char* explicit_expression, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return true;}

		/**
		* Called for implicit (non-explicit) parameters before trying to find them in the pipeline state.
		* Return true to allow argument sending to the module. In this case there follows
		* a SendParameterValue() call for this parameter.
		*
		* Return false to not send argument to the module. This is appropriate
		* mostly for the syntax-check executives as the parameter location in the module
		* remains uninitialized and can contain garbage value. Also syntax-check 
		* modules should return true for PAR_SYNTAX or PAR_SYSTEM parameters as these 
		* might be used in the Mod::SyntaxRun() methods.
		*
		* The baseclass implementation just returns true.
		*
		* @param ctx The current execution context.
		* @param argname Module parameter name
		* @param flag_PAR_SYSTEM True if the module parameter has been registered with the PAR_SYSTEM flag.
		* @param flag_PAR_SYNTAX True if the module parameter has been registered with the PAR_SYNTAX flag.
		*/
		virtual bool SendImplicitParameter(ExecutionContext& ctx, const char* argname, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return true;}

		/**
		* Called for all parameters directly before sending the parameter to the module,
		* if the SendExplicitParameter() or SendImplicitParameter() has returned true.
		* 
		* If the method returns true, the program goes on with sending the parameter 
		* to the module. If argvalue is Undefined, this results in an Exception.
		*
		* If the method returns false, the parameter is skipped. This is appropriate
		* mostly for the syntax-check executives as the parameter location in the module
		* remains uninitialized and can contain garbage value. Also syntax-check 
		* modules should return true for PAR_SYNTAX or PAR_SYSTEM parameters as these 
		* might be used in the Mod::SyntaxRun() methods. Note that the parameter value 
		* is already calculated and the cost of passing it into the module is not high,
		* so it makes sense to return false only for Undefined values, and only in 
		* syntax-check run.
		* 
		* The baseclass implementation just returns true.
		*
		* @param ctx The current execution context.
		* @param argname Module parameter name
		* @param argvalue The parameter value. If Undefined, then the program was not able to determine the input value. 
		*				This is normal in syntax-check runs.
		* @param flag_PAR_SYSTEM True if the module parameter has been registered with the PAR_SYSTEM flag.
		* @param flag_PAR_SYNTAX True if the module parameter has been registered with the PAR_SYNTAX flag.
		*/
		virtual bool SendParameterValue(ExecutionContext& ctx, const char* argname, const DataItem& argvalue, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return true;}


		/**
		* Called in the case when module returns NULL for some output MemBlock parameter.
		* Return true to consider this as a request to delete this dataitem from
		* the current datablock, if any.
		* Return false to output an "empty" item produced by MemBlock::Factory() instead.
		* 
		* The baseclass implementation just returns true.
		*
		* @param ctx The current execution context.
		* @param argname Module parameter name
		*/
		virtual bool DeleteNullOutputParam(ExecutionContext& ctx, const char* argname) {return true;}

		/**
		* Called immediately after creating the 'current' datablock for (sub)script 
		* execution, before NotifyCurrentDataBlockChanges().
		* Return pointer to the callback to install in the "current" datablock, or NULL.
		* By default, the callback will be  deleted automatically together with the datablock.
		* See DataBlockCallback class for more details.
		*
		* The baseclass implementation just forwards to Instance::GetDataBlockCallback().
		*/
		DI_IMacro virtual DataBlockCallback* GetDataBlockCallback(ExecutionContext& ctx);

		/**
		* The first-preference hook for alternative module location algorithm during
		* script parsing.
		* Return pointer to the Module object in the current process.
		* Return NULL to locate the module in standard way.
		*
		* The baseclass implementation just returns NULL (use standard lookup rules). 
		*
		* @param ctx The current execution context.
		* @param modulename - the name of the module as appeared in the script, trimmed, original case, may contain library name and colon.
		*/
		virtual const Module* FindModule(ExecutionContext& ctx, const safestring& modulename) {return NULL;}

		/**
		* Called when a warning occurs, e.g. from Instance::Warning().
		* Return true if the warning has been fully processed and does not need any further storing.
		* Return false to indicate that the Executive is not able to deliver the warning to the user.
		*
		* The baseclass implementation just returns false (caller should store/process the warning by itself). 
		*
		* @param ctx The current execution context.
		* @param reason Warning code, composed of WARN* or ERR* flags in <MemBlock/modreg.h>.
		*				If ERR_ERROR is set, this should be considered as an error instead.
		* @param message Warning message.
		*/
		DI_IMacro virtual bool ProcessWarning(ExecutionContext& ctx, unsigned int reason, const char* message);

		/**
		* Called from various modules for outputting text to console (e.g. from printf()).
		*
		* @param ctx The current execution context.
		* @param text The text to output, in ASCII. Should be output "as is", as much as possible.
		*/
		DI_IMacro virtual void ConsoleOut(ExecutionContext& ctx, const Nbaseutil::safestring& text);


		/**
		* Return true if it's appropriate to maintain snapshots in the snapshot() module.
		* This is currently not really used/supported/tested.
		* The baseclass implementation just returns false.
		*/
		virtual bool MakeSnapshots(ExecutionContext& ctx) {return false;}

		/*
		* Called from output(), imageview(), graphview(), tableview() modules
		* for handing over data to display in the user interface.
		* The baseclass implementation does nothing.
		*
		* @param ctx The current execution context.
		* @param label Output item label (not dataitem name!).
		* @param item  Output item itself. This can be a number or string, or 
		*				a DataBlock wrapped in a Sharable DataItem. DataBlocks
		*				are meant for different view panes in the player window.
		*				The contained item 'viewtype' determines the exact view 
		*				("imageview", "graphview", "tableview"). Alternative 
		*				way to pass the view type is to prepend it to the label,
		*				followed by a colon. 
		*				The content of DataBlock depends on the view, see 
		*				TabPanePlayer::SetPane() implementation for more details.
		*
		*				The OutputItem method is allowed to cast away the constness of item 
		*				and set it to Undefined value.
		*/
		virtual void OutputItem(ExecutionContext& ctx, const char* label, const SafeValue& item) {} // handle UI output data 

		virtual ~Executive() {}

		typedef safestring imagetype_t;

		/**
		* Called by an IO_module during SyntaxRun to announce a script-level
		* input parameter, which the framework should care about.
		* Should return true, if the framework will provide input value.
		* @param ctx The current execution context.
		* @param label Some end-user-readable name for the input.
		* @param caller Pointer to the calling module.
		* @param input_type \"i\" = integer, \"d\" = double, \"s\" = string, \"f\" = file name, \"p\" = directory name, \"y\" = boolean (on-off), \"m\" = memblock, \"w\" = well data (see singlewell() module).
		*/
		virtual bool AnnounceInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type) {return false;}

		/**
		* Called by an IO_module during Run to fetch the actual value of the input.
		* Should return the value, or Undefined value to indicate that the module should 
		* use it's default value instead, or that the given input_type parameter is not supported.
		* Instead of directly returning Undefined, it is suggested to call the parent class 
		* implementation of GetInput() instead.
		*
		* @param ctx The current execution context.
		* @param label Some end-user-readable name for the input.
		* @param caller Pointer to the calling module.
		* @param input_type \"i\" = integer, \"d\" = double, \"s\" = string, \"f\" = file name, \"p\" = directory name, \"y\" = boolean (on-off), \"m\" = memblock, \"w\" = well data (see singlewell() module).
		*
		* If input_type is "file_r", then the script is going to open a file for reading.
		* The label parameter gives the filename as appearing in the script.
		* This is a hook mechanism which gives the Executive an opportunity to override
		* normal file lookup mechanism. The Executive can return:
		* - file content encapsulated in a Vector of type Vector::Byte.
		* - new physical file name as a string item (redirection support).
		* - OS file handle as an integer item (as returned by the fileno() function, e.g.)
		* - Undefined value to indicate that it does not provide the file and it should be 
		*     looked up in the normal fashion.
		*/
		virtual DI_IMacro SafeValue GetInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type);

		/**
		* For binary-compatible extensions of Executive class functionality.
		* If the executive doesn't handle the verb, it should call super-class
		* implementation. Finally, Executive::DoVerb() returns Undefined value 
		* to indicate that the executive object doesn't know how to handle the verb.
		* In this case the caller has to fallback on some default behaviour, depending
		* on the verb, i.e. the script execution may not be aborted just because of dumb
		* Executive.
		*
		* @param ctx The current execution context.
		* @param pszVerb The command verb. Most of the verbs are handled only by the NESPMacroEditor::RunExecutive.
		*	Currently (June 2008) the following verbs are supported:
		* - "askpassword" (string message) In an interactive environment, the executive
		*				should present the message to the user and ask for a password.
		*				The password string is returned. See DbOpen() module.
		* - "interactive" (void) Executive should return 1 in the interactive environment, where user can e.g. respond to dialog boxes.
		* - "sendalarm" (string msg, int severity (1..5), int errcode, string descr).
						Deprecated, use SendAlarmIMacro()/SetSendAlarmHandler() mechanism instead.
		* - "supports_graphview" (void) Executive returns 1 if it is capable and wants to display graphview data. See GraphView() module. Otherwise graphview data is not formed.
		* - "supports_imageview" (void) Executive returns 1 if it is capable and wants to display imageview data. See ImageView() module. Otherwise imageview data is not formed.
		* - "supports_imageview_html" (void) Executive returns 1 if it is capable and wants to display HTML-style imageview data. See HtmlImageView() module. Otherwise imageview data is not formed.
		* - "supports_tableview" (void) Executive returns 1 if it is capable and wants to display tableview data. See TableView() module. Otherwise tableview data is not formed.
		* - "ui_window_handle" (void) If the executive is provided by a GUI client, then on Windows it should return a HWND handle (casted to int), which is suitable for e.g. passing in ::MessageBox().
		* @param param1 Additional parameter, meaning depends on the verb.
		* @param param2 Additional parameter, meaning depends on the verb.
		* @param param3 Additional parameter, meaning depends on the verb.
		* @param param4 Additional parameter, meaning depends on the verb.
		*/
		DI_IMacro virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue());

		/** Called during parsing the 'script' when a procedure definition
		*   has been extracted from the script text and added to the global
		*   procedure map.
		*/
		virtual void NotifyProcedureParsed(const safestring& procname, PScript parentscript) {}

		/** 
		* This can be used by long-running modules for reporting the execution progress and for supporting early termination
		* by the user. Returns true, if the module should continue the execution. Returns false, if the user has requested abort.
		* In the latter case the module should terminate the execution as soon as possible and throw Exception(ERR_USER_INTERVENTION, ...).
		* Default implementation just outputs the info to console and returns true.
		* 
		* @param ctx The current execution context.
		* @param percent The execution progress in percents (0.0..100.0). Special values: if -1, then the progress bar, if any, is removed from the display, if possible. 
		*     If -2, then no progress is not yet made after the last Progress() call, the function is just so that the executive can return false to abort the script.
		* @param message If non-empty, then the message will be displayed to the user, typically in the console window.
		* @param reserved Reserved for future use.
		*/
		DI_IMacro virtual bool Progress(ExecutionContext& ctx, double percent, const safestring& message, const SafeValue& reserved=SafeValue());

		/// If the executive keeps track on parent scripts of the current script, set the output params and return true.
		virtual bool GetParentScripts(Script*& parents, int& parentcount) {return false;}

		/**
		* Returns a copy of Executive object suitable for running a child script in a different thread, if the cloning is supported by the actual Executive class.
		* For such a support the Executive class has to implement "get_childthread_executive" verb (with param1=forward).
		* @param forward A notification parameter, specifying what types of information should be forwarded from the child script to the parent script.
		* @return A pointer to the new object or NULL, if the Executive class does not support cloning.
		*/
		DI_IMacro Executive* Clone(ExecutionContext& ctx, PContainer forward=NULL);

	public: // static interface
		/// A typedef used by ArrangeThreadRendezvous().
		typedef void (*rendezvous_func)(ExecutionContext& ctx, unsigned int rendezvous_no, void* callback_data);

		/**
		* Issue a new rendezvous request with all threads running Acapella scripts.
		* Each thread will call the passed rendezvous function once for each issued rendezvous 
		* request, when it notices it first. If the thread is stuck in a non-Acapella function
		* or a lengthy/pausing module call, it might not respond to the rendezvous request in a timely manner.
		* @param f The function to call back.
		* @param callback_data An opaque pointer passed back to the rendezvous function.
		* @param wake_sleeping Notify all threads waiting in Sleep(), EventLoop() etc. modules to wake up and meet the rendezvous. Otherwise, only actively running scripts will notice the rendezvous.
		* @return The number of the scheduled rendezvous. This is passed also to the rendezvous function.
		*/
		DI_IMacro static unsigned int ArrangeThreadRendezvous(rendezvous_func f, void* callback_data, bool wake_sleeping=true);

		/// Cancel a previously scheduled rendezvous. The rendezvous function will not be called any more by the threads which have not called it yet.
		DI_IMacro static void CancelThreadRendezvous(unsigned int rendezvous_no);

	protected: // protected interface
		/// Copy ctor, to be called by derived classes' copy ctors. The autodelete flag is initialized to false.
		Executive(const Executive&)
			: refcount_(0)
			, autodelete_(false)
		{}

	private:
		mutable int refcount_;
		bool autodelete_;
	public:
		/// Increase refcount, used by PExecutive smartpointer
		void Capture() const {++refcount_;}
		/// Decrease refcount, and finally delete this, if autodelete switched on. Used by PExecutive smartpointer.
		void Release() const {
			try {
				if (--refcount_<=0 && autodelete_) delete this;
			} catch(...) {
				fprintf(stderr, "Exception in Executive destructor\n");
				// do not propagate destructor exception.
			}
		}
	};



	/**
	* For better abortion support derive your script running executive from AbortableExecutive.
	* For aborting the execution call the Abort() member function, usually from another thread than the script is running in.
	* The script will be aborted at the next module boundary. If the module makes use of the AbortHandler class, it can be aborted
	* also in the middle of the module.
	*/
	class DI_IMacro AbortableExecutive: public Executive {
		typedef Executive super;
	public:
		AbortableExecutive();
                
		AbortableExecutive(const AbortableExecutive& b);

		/// Constants for using with the Abort() method.
		enum abortmode_t {
			/// Stop the script execution as if by the Stop() module call. This is considered a normal script finish, no errors reported.
			abort_silent_stop=1,
			/// Normal abort, can be catched by Catch_Abort() module in script higher levels.
			abort_normal=2,
			/// Program shutdown, the abort cannot be catched by Catch_Abort(), all script levels are terminated.
			abort_shutdown=3,
			/// No abort; if the module is waiting on a condition, signal it; the script is supposed to call Executive::Progress(-2) when waken up this way, and to continue the waiting afterwards.
			abort_signal_only=4,
			/// Terminate the thread; the abort cannot be catched by Catch_Abort(), all script levels are terminated. This is a normal termination, no error will be reported.
			abort_stop_thread=5,
		};

		/**
		* Call this function asynchronously from another thread to abort the script execution ASAP, or to signal a waiting a module to wake up.
		* In abortion mode, this 
		*
		* @param mode The abortion mode. See abortmode_t enum for possible values.
		*/
		void Abort(int mode);

	protected: // overridden virtuals

		virtual void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no);
		virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1, const SafeValue& param2, const SafeValue& param3, const SafeValue& param4);
		virtual bool Progress(ExecutionContext& ctx, double percent, const safestring& message, const SafeValue& reserved);

	private: // implementation
		void CheckAborting(ExecutionContext& ctx, const ModuleCall* call);

	private: // data
		mutable Nbaseutil::boost_recursive_mutex mx_; // protects abort_handler_ and abort_data_.
		typedef	void (*AbortHookHandler)(int mode, void* data);
		AbortHookHandler abort_handler_;
		void* abort_data_;
		int mode_;
	};

	/**
	* An executive which can be used for debugging a script. 
	*/
	class DI_IMacro DebuggableExecutive: public AbortableExecutive {
		typedef AbortableExecutive super;

	public: // static interface

		/** 
		* Set debugging regime on; can be called from any thread. Other threads may not notice the changes immediately.
		* In debugging regime the scripts may run significantly slower, so this function should not be called without a need.
		* This function can be called repeatedly, for example for updating breakpoinst info stored by the debugger object.
		* @param debugger The debugger object. This is cloned and the clone will be stored.
		*/
		static void StartDebugging_mt(const Debugger& debugger);

		/** 
		* Set debugging regime off; can be called from any thread. Other threads may not notice the changes immediately.
		* Switching the debugging regime off may enhance scripts' performance.
		*/
		static void StopDebugging_mt();

		/**
		* Return a clone of the currently set debugger. 
		* If StartDebugging_mt() has never been called, NULL is returned.
		* Calling StopDebugging_mt() does not affect this function.
		* @return A pointer to a clone of the current debugger, or NULL. The caller has to delete the object when done.
		*/
		static Debugger* GetCloneOfCurrentDebugger_mt();

	public: // interface

		DebuggableExecutive();
		DebuggableExecutive(const DebuggableExecutive& b);
		~DebuggableExecutive();

	protected: // overridden virtuals

		virtual bool NotifyDebugBeforeModuleCall(ExecutionContext& ctx, RunFrame& runframe);
		virtual void NotifyDebugAfterModuleCall(ExecutionContext& ctx, RunFrame& runframe);
		virtual void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no);
		virtual void NotifyScriptStarted(ExecutionContext& ctx);
		virtual void NotifyScriptFinishedNormally(ExecutionContext& ctx);
		virtual void NotifyScriptAborted(ExecutionContext& ctx, int mode);
		virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1, const SafeValue& param2, const SafeValue& param3, const SafeValue& param4);

	private: // implementation
		void operator=(const DebuggableExecutive& b); // not implemented
		bool IsDebugging() {
			// no locking, as immediate recognition of changed value is not needed.
			if (s_debugging_) {
				return CheckVer();
			} else {
				return false;
			}
		}
		bool CheckVer();

	private: // static data
		static boost_mutex* s_mx_;
		static const Debugger* s_debugger_;
		static int s_ver_;
		static iptr_t s_debugging_;

	private: // data
		int ver_;
		Debugger* attached_debugger_;
	};

	/**
	* Base class for syntax-check type executives.
	*/
	class SyntaxCheckerBase: public Executive {
		typedef Executive super;
		std::set<safestring, std::less<safestring>, mb_allocator_typedef<safestring>::allocator > processed_procs_;
	public:
		/// Overridden virtual method, returns true
		virtual bool CallSyntaxRun(ExecutionContext& ctx) {return true;}

		/// Overridden virtual method, returns always false, to scan each module call.
		virtual bool TakeBranch(ExecutionContext& ctx, bool default_decision) {return false;}

		/// Overridden virtual method, returns true. Note that this may be expensive if the explicit expressions are complex.
		virtual bool SendExplicitParameter(ExecutionContext& ctx, const char* argname, const char* explicit_expression, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {
			return true;
		}

		/// Overridden virtual method, returns true. Note that this is not the fastest option.
		virtual bool SendImplicitParameter(ExecutionContext& ctx, const char* argname, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {
			return true;
		}

		/**
		* Overridden virtual method, returns true for PAR_SYSTEM or PAR_SYNTAX arguments.
		* Note that this leads to script abort, if the value is Undefined.
		*/
		virtual bool SendParameterValue(ExecutionContext& ctx, const char* argname, const DataItem& argvalue, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {
			return flag_PAR_SYSTEM || flag_PAR_SYNTAX;
		}

		/// Overridden virtual method, returns false in order to create a dummy output variable in the datablock.
		virtual bool DeleteNullOutputParam(ExecutionContext& ctx, const char* argname) {return false;}

		/**
		* Process "__proc__already__processed__" verb. The procedure name is passed as param1.
		* Returns 0 in the first call, 1 on any subsequent calls with the same procedure name.
		* This is used in syntax check run to avoid multiple entrance into the same procedure, 
		* to speed up the interface. Use verb "__reset__processed__" for restoring the initial
		* state of the object.
		*/
		DI_IMacro virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue());

		// Offline ctor-dtor to force creating the std::set member in imacro.dll, for MSVC++6 support.
		DI_IMacro SyntaxCheckerBase();
		DI_IMacro ~SyntaxCheckerBase();
	};

	/**
	* A syntax checker which does not panic on missing module parameters
	* and ignores any exceptions.
	*/
	class DI_IMacro ScriptScanner: public NIMacro::SyntaxCheckerBase {
		typedef NIMacro::SyntaxCheckerBase super;
	public: // interface
		ScriptScanner(bool auto_undo=true): errcode_(0), auto_undo_(auto_undo) {}
	protected: // overridden virtuals
		virtual void NotifyScriptStarted(NIMacro::ExecutionContext& ctx);
		virtual bool SendParameterValue(NIMacro::ExecutionContext& ctx, const char* argname, const NIMacro::DataItem& argvalue, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX);
		virtual bool HandleException(NIMacro::ExecutionContext& ctx, const NIMacro::Exception& e);
		virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue());
	public: // data	
		unsigned int errcode_;
		Nbaseutil::safestring errmsg_;
		bool auto_undo_;
	};

	/** A base class for Executive proxies.
	* This proxy transfers all calls to another Executive handler, which is
	* available only at runtime. If the overridible executive is known 
	* at compile-time, then one should use ordinary derivation for better
	* performance.
	* 
	* To override some behaviour of another handler, derive from this class
	* and override the desired methods. You should call or not call the superclass
	* implementation, depending on the situation.
	*
	* See also: Script::SetExecutiveProxy(), provideinput() module.
	*/
	class ExecutiveProxy: public Executive {
	private:
		PExecutive exec_;	// handler
	public:
		/**
		* Construct proxy executive. 
		* @param exec The handler for default behaviour. This is a smartpointer,
		*			in order to support transparently both autodelete and stack-created
		*			executives. Just pass executive's address in here.
		*			If you pass NULL (the default), then the executive is not usable
		*			before the proper handler is installed by SetHandler(). 
		*/
		ExecutiveProxy(PExecutive exec=NULL): exec_(exec) {}

		/// Return current handler
		PExecutive GetHandler() const {return exec_;}

		/// Replace current handler. Parameter may not be NULL.
		DI_IMacro virtual void SetHandler(PExecutive exec);

		// Bugfix 31.05.2004: return *this as the default subexec; this allows for forwarding provideinput()/providesinglewell() into procedures.
		virtual Executive& GetSubExecutive(ExecutionContext& ctx) {return *this;}
		virtual void NotifyCurrentScriptChanges(ExecutionContext& ctx, const PScript new_script) {exec_->NotifyCurrentScriptChanges(ctx, new_script);}
		virtual void NotifyCurrentDataBlockChanges(ExecutionContext& ctx, PDataBlock new_db) {exec_->NotifyCurrentDataBlockChanges(ctx, new_db);}
		virtual void NotifyCurrentInstanceChanges(ExecutionContext& ctx, PInstance new_instance) {exec_->NotifyCurrentInstanceChanges(ctx, new_instance);}
		virtual void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no) {exec_->NotifyModuleCallBegins(ctx, call, no);}
		virtual void NotifyModuleCallEnds(ExecutionContext& ctx, const ModuleCall* call, int no) {exec_->NotifyModuleCallEnds(ctx, call, no);}
		virtual void NotifyScriptStarted(ExecutionContext& ctx) {exec_->NotifyScriptStarted(ctx);}
		virtual void NotifyScriptFinishedNormally(ExecutionContext& ctx) {exec_->NotifyScriptFinishedNormally(ctx);}
		virtual void NotifyScriptAborted(ExecutionContext& ctx, int mode) {exec_->NotifyScriptAborted(ctx, mode);}
		virtual bool NotifyDebugBeforeModuleCall(ExecutionContext& ctx, RunFrame& runframe) {return exec_->NotifyDebugBeforeModuleCall(ctx, runframe);}
		virtual void NotifyDebugAfterModuleCall(ExecutionContext& ctx, RunFrame& runframe) {exec_->NotifyDebugAfterModuleCall(ctx, runframe);}
		virtual void NotifyParsingProcedure(ExecutionContext& ctx, const safestring& procname, const safestring& filename, const safestring& procbody, int startpos, int lineno) {exec_->NotifyParsingProcedure(ctx, procname, filename, procbody, startpos, lineno);}
		virtual bool HandleException(ExecutionContext& ctx, const Exception& e) {return exec_->HandleException(ctx, e);}
		virtual bool TakeBranch(ExecutionContext& ctx, bool default_decision) {return exec_->TakeBranch(ctx, default_decision);}
		virtual bool CallSyntaxRun(ExecutionContext& ctx) {return exec_->CallSyntaxRun(ctx);}
		virtual bool SendExplicitParameter(ExecutionContext& ctx, const char* argname, const char* explicit_expression, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return exec_->SendExplicitParameter(ctx, argname, explicit_expression, flag_PAR_SYSTEM, flag_PAR_SYNTAX);}
		virtual bool SendImplicitParameter(ExecutionContext& ctx, const char* argname, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return exec_->SendImplicitParameter(ctx, argname, flag_PAR_SYSTEM, flag_PAR_SYNTAX);}
		virtual bool SendParameterValue(ExecutionContext& ctx, const char* argname, const DataItem& argvalue, bool flag_PAR_SYSTEM, bool flag_PAR_SYNTAX) {return exec_->SendParameterValue(ctx, argname, argvalue, flag_PAR_SYSTEM, flag_PAR_SYNTAX);}
		virtual bool DeleteNullOutputParam(ExecutionContext& ctx, const char* argname) {return exec_->DeleteNullOutputParam(ctx, argname);}
		virtual DataBlockCallback* GetDataBlockCallback(ExecutionContext& ctx) {return exec_->GetDataBlockCallback(ctx);}
		virtual const Module* FindModule(ExecutionContext& ctx, const safestring& modulename) {return exec_->FindModule(ctx, modulename);}
		virtual bool ProcessWarning(ExecutionContext& ctx, unsigned int Reason, const char* Message) {return exec_->ProcessWarning(ctx, Reason, Message);}
		virtual void ConsoleOut(ExecutionContext& ctx, const Nbaseutil::safestring& text) {exec_->ConsoleOut(ctx, text);}
		virtual bool MakeSnapshots(ExecutionContext& ctx) {return exec_->MakeSnapshots(ctx);}
		virtual void OutputItem(ExecutionContext& ctx, const char* label, const SafeValue& item) {exec_->OutputItem(ctx, label, item);}
		virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue()) {return exec_->DoVerb(ctx, pszVerb, param1, param2, param3, param4);}
		virtual bool AnnounceInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type) {return exec_->AnnounceInput(ctx, label, caller, input_type);}
		virtual SafeValue GetInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type) {return exec_->GetInput(ctx, label, caller, input_type);}
		virtual bool Progress(ExecutionContext& ctx, double percent, const safestring& message, const SafeValue& reserved=SafeValue()) {return exec_->Progress(ctx, percent, message, reserved);}
	};

	class DI_IMacro ForwardMask {
	public: // enums
		/**
		* Flags for controlling which events are forwarded. 
		* By construction of the ForwardMask object the settings container is converted into the bitmask of these flags.
		* The corresponding item name in the settings container is the same as the enum constant name without the 'fwd_' prefix, e.g. "consoleout".
		*/
		enum fwd_t {
			/// Forward the messages intended for console output (Executive::ConsoleOut()).
			fwd_consoleout=(1<<0),
			/// Forward the notifications about starting and ending the module call. This may produce noticable overhead, so it is not suggested to use it in a normal run.
			fwd_notify_modulecall=(1<<1),
			/// Forward the notifications about entering and leaving subscripts.
			fwd_notify_scriptchange=(1<<2),
			/// Forward the notifications about starting and terminating a script.
			fwd_notify_scriptrun=(1<<3),
			/// Forward image views
			fwd_imageview=(1<<4),
			/// Forward table views
			fwd_tableview=(1<<5),
			/// Forward graph views
			fwd_graphview=(1<<6),
			/// Forward html views
			fwd_htmlview=(1<<7),
			/// Forward simple outputs (Output() module).
			fwd_output=(1<<8),
			/// Forward warnings
			fwd_notify_warning=(1<<9),
			/// Forward DoVerb() (with waiting for the answer!)
			fwd_execverb=(1<<10),
			/// Forward progress messages
			fwd_progress=(1<<11),
			/// Forward singlewell requests
			fwd_singlewell=(1<<12),
			/// Forward input requests
			fwd_input=(1<<13),
			/// Forward sourcedatajump requests
			fwd_sourcedatajump=(1<<14),
		};
	public: // interface
		ForwardMask(unsigned int init=0): fwd_mask_(init) {}
		ForwardMask(const Container& fwd_mask_settings);
		void SetForwarded(fwd_t flag, bool forwarded=true) {
			if (forwarded) {
				fwd_mask_ |= flag;
			} else {
				fwd_mask_ &= ~flag;
			}
		}
		bool IsForwarded(fwd_t flag) const {return (fwd_mask_ & flag)!=0;}
	private: // data
		unsigned int fwd_mask_;
	};


	/**
	* A class template for creating forwarding executives. 
	* All events and commands can be forwarded to a specific FORWARDER object and/or to an executive proxy.
	* It is assumed that calling the executive proxy is cheap, everything is forwarded to the proxy if one 
	* is present (unlike in ExecutiveProxy class, the executive proxy may be missing here).
	* OTOH, calling the FORWARDER object is assumed to be expensive. Special data containers are prepared
	* for each call, containing no physical (smart)pointers as these are primarily meant for interprocess communication.
	* By the construction one has to specify which events and notifications have to be forwaded to the FORWARDER
	* object. See the ForwardMask class documentation for details.
	*
	* @param FORWARDER The template parameter specifying the FORWARDER class. This class has to implement a Forward method with the following signature: PContainer Forward(const safestring&, PContainer&).
	* @param BASE_EXECUTIVE_CLASS The Executive base class from where the template instantiation class is derived. All 
	*/ 
	template<class FORWARDER, class BASE_EXECUTIVE_CLASS=Executive>
	class ExecutiveForwarder: public BASE_EXECUTIVE_CLASS, public ForwardMask {
		typedef BASE_EXECUTIVE_CLASS super;
		FORWARDER fwd_;
	public:
		ExecutiveForwarder(const FORWARDER& fwd, const Container& fwd_mask_settings)
			: ForwardMask(fwd_mask_settings)
			, fwd_(fwd)
		{}
		ExecutiveForwarder(const ExecutiveForwarder& b) 
			: BASE_EXECUTIVE_CLASS(b)
			, ForwardMask(b) 
			, fwd_(b.fwd_)
		{
			fwd_.MakeCopy();
		}
		virtual void NotifyCurrentScriptChanges(ExecutionContext& ctx, const PScript new_script);
		virtual void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no);
		virtual void NotifyModuleCallEnds(ExecutionContext& ctx, const ModuleCall* call, int no);
		virtual void NotifyScriptStarted(ExecutionContext& ctx);
		virtual void NotifyScriptFinishedNormally(ExecutionContext& ctx);
		virtual void NotifyScriptAborted(ExecutionContext& ctx, int mode);
		virtual bool ProcessWarning(ExecutionContext& ctx, unsigned int Reason, const char* Message);
		virtual void ConsoleOut(ExecutionContext& ctx, const Nbaseutil::safestring& text);
		virtual SafeValue GetInput(ExecutionContext& ctx, const char* label, IO_module* caller, const char* input_type);
		virtual void OutputItem(ExecutionContext& ctx, const char* label, const SafeValue& item);
		virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue()) {
			if (IsForwarded(fwd_execverb)) {
				PContainer c = Cnt("verb", pszVerb);
				if (param1) c->Put(param1, "param1");
				if (param2) c->Put(param2, "param2");
				if (param3) c->Put(param3, "param3");
				if (param4) c->Put(param4, "param4");
				DataItem result = fwd_.ForwardAndWait(ctx, "execverb", c);
				if (result) {
					return result;
				}
			}
			return super::DoVerb(ctx, pszVerb, param1, param2, param3, param4);
		}

		virtual bool Progress(ExecutionContext& ctx, double percent, const safestring& message, const SafeValue& reserved=SafeValue()) {
			if (IsForwarded(fwd_progress)) {
				PContainer c = Cnt("percent", percent, "message", message);
				if (reserved) {
					c->Put(reserved, "reserved");
				}
				fwd_.Forward(ctx, "progress", c);
			}
			return super::Progress(ctx, percent, message, reserved);
		}
	};

	DI_IMacro DataItem SendForwardedMessageToExec(const Nbaseutil::safestring& eventname, PContainer eventdata, ExecutionContext& ctx);


} // namespace
#endif
