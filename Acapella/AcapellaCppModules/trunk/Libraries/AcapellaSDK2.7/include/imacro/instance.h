#ifndef _IMACRO_INSTANCE_H_INCLUDED_
#define _IMACRO_INSTANCE_H_INCLUDED_

#include "script.h"
#include "datablock.h"
#include "errlogger.h"

namespace NIMacro {

	// Instance class state is determined by script. 
	// Everything one can change by script should be included in the Instance class state.
	// In particular, input-output-current data blocks are considered to belong to the Instance class object.

	// Forward declaration for the smartpointer
	class Instance;
    
	// Smartpointer to Instance; Plain Instance pointers/references should be avoided.
	typedef ThreadSharablePointer<Instance> PInstance;

	class Aliases;
	typedef RefcountedPointer<Aliases> PAliases;

	// A registration list for non-stdlib procedures.
	class ModuleMap;
	typedef ThreadSharablePointer<ModuleMap> PModuleMap;

	class AbortHandler;

	/**
	* In former Acapella version this was an important class for client software.
	* In Acapella 2.1 usage of this class has been reduced. Instead one can create 
	* directly a Script object by one of Script::Create() methods and execute it by one 
	* of Script::Run() methods.
	*
	* The Instance class is similar to the ExecutionContext class, in the sense it is 
	* to be used in a single thread only and stores script execution state. The difference
	* is that there is a separate ExecutionContext object for each script-level stack frame,
	* e.g. a procedure call, but usually only one Instance object. The Instance object thus 
	* acts like a master ExecutionContext frame.
	*/
	class DI_IMacro Instance: public ThreadSharable, public ErrLogger {
	public:

		/// Create a new Instance.
		static PInstance Create();	

		/// Return a reference to a common "empty" Instance. Modification not allowed.
		static Instance& Void();

		/** Attach a datablock to the Instance. The datablock is considered to be owned
		* by the Instance, in the sense that it cannot be attached to another Instance
		* at the same time (see also DataBlock::SetInstance()). The lifetime of the DataBlock
		* is controlled by the common smartpointer mechanism though.
		*
		* @param block The datablock itself. It is constantly modified during the script run.
		*/
		void SetDataBlock(PDataBlock block);

		/// Obsolete, do not use.
		void SetDataBlock(const safestring& name, PDataBlock block) {SetDataBlock(block);}

		/// Obsolete, do not use.
		bool ExistsDataBlock(const safestring& name) const;

		/**
		* Equivalent to GetDataBlock(), preserved for back-compatibility.
		* @param name If "system", then throws, use DataBlock::GetSysItem instead.
		*/
		PDataBlock GetDataBlock(const safestring& name) const {
			if (name=="system") {
				throw Exception(ERR_PROGRAM_ERROR, "Instance::GetDataBlock(\"system\") is obsolete call, use DataBlock::GetSysItem() instead.");
			} else {
				return GetDataBlock();
			}
		}

		/**
		* Return pointer to the attached datablock.
		* If no datablock is attached to the instance, an empty datablock is created and attached.
		*/ 
		PDataBlock GetDataBlock() const;


		/** Detach a datablock from the Instance. 
		* If there are no other smartpointers to the DataBlock, it is deleted.
		* Returns true, if there was a datablock attached.
		*/
		bool DeleteDataBlock(const safestring& name);	


		/** Attach a Script object to the Instance. 
		* The same Script object may be attached to several Instance objects 
		* at the same time, and those Instance objects can be run parallel
		* in different threads.
		*/
		void SetScript(PScript script);

		/// Return pointer to the attached Script object, or NULL.
		PScript GetScriptPrivate() const {return script_;}

		/**
		* This is a more automagic variant of the other Run overload (Run(ExecutionContext&, Executive&)).
		* It creates a temporary ExecutionContext object and calls the other Run overload with it.
		* It also takes care of copying ExecutionContext metadata (mostly provideinput() settings) from 
		* one Run() call to another, acting as a transparent execution context for executing several script pieces 
		* one after another in the same context.
		*
		* @param exec The executive supervisor, which determines effectively every 
		*		aspect of script running via overloading virtual Executive class member functions.
		*		The Executive base class object can be used for "normal" run.
		*/
		void Run(Executive& exec);


		/**
		* Calls Script::Run() on the attached Script. 
		
		* In case of any errors exceptions will be thrown.
		* Currently also stop() module throws an exception (of type NIMacro::Stopped), 
		* which may be thrown out of Instance::Run() method.
		* The local datablock context in place of the stop() module is transported 
		* out of recursive procedure calls by attaching it to the NIMacro::Stopped exception 
		* and can be inspected/used by the calling program.
		*
		* Throwing an NIMacro::Stopped exception is considered to mark a successful termination of 
		* the script. The caller has to distinguish between this exception and other exceptions.
		* For that there are 2 ways:
		*
		* 1. Add a catch handler for NIMacro::Stopped doing nothing.
		* 
		* 2. Call Instance::RunNoThrow() instead of Run() and check for errors by a call to 
		* Instance::HasErrors() and Instance::GetErrorsAndWarnings() member functions. 
		*
		* @param parent_ctx The parent execution context to run the script within. For transporting
		*		provideinput() settings out the running script into parent_ctx, the parent_ctx has 
		*		to be marked as willing to act transparently, by calling SetAsAttachmentParent(true) on it.
		* @param exec The executive supervisor, which determines effectively every 
		*		aspect of script running via overloading virtual Executive class member functions.
		*		The Executive base class object can be used for "normal" run.
		*/
		void Run(ExecutionContext& parent_ctx, Executive& exec);

		/** Same as Run() except does not throw and returns an error code instead.
		* The error code is composed of ERR_* flags defined in <baseutil/imacroexception.h>
		* One should call GetErrorsAndWarnings() if the return value is not zero.
		*/
		unsigned int RunNoThrow(Executive& exec) throw();

		/** Same as Run() except does not throw and returns an error code instead.
		* The error code is composed of ERR_* flags defined in <baseutil/imacroexception.h>
		* One should call GetErrorsAndWarnings() if the return value is not zero.
		*/
		unsigned int RunNoThrow(ExecutionContext& parent_ctx, Executive& exec) throw();

		/** Break current Run() ASAP (at most on the next module boundary).
		* Can be called from another thread. 
		* @param mode Aborting mode: 
					1=silent (Run() is finished as though script was over); 
					2=message (Exception thrown: Script aborted)
					3=forced, abortion cannot be catched by catch_abort() module.
		*/
		void Abort(int mode=2) volatile;	

		/** Return current aborting mode of the Instance.
		* Can be called from another thread.
		* Return value: 0 = not aborting. 1=silent aborting requested; 2=throwing aborting requested.
		* This function is called from Script::Run(). 
		*/
		int Aborted() const volatile;

		/// Announce an error. The error message is decorated with the current script location info.
		void Error(ExecutionContext& ctx, unsigned int Reason, const safestring& Message);

	public: // ErrLogger virtual overrides

		/** Override of ErrLogger base class function.
		* Adds current location info (in the script) to the message
		* and forwards to the base class method.
		*/
//		virtual void DoError(unsigned int Reason, const safestring& Message);	
		
		/** Override of ErrLogger base class function.
		* Location info (in the script) is added to the message.
		* During Run(), if the Executive used for Run() claims to handle the warning
		* via the ProcessWarning() function, nothing more is done.
		* Otherwise, ErrLogger base class method is called.
		*/
		virtual void DoWarning(ExecutionContext& ctx, unsigned int Reason, const safestring& Message);

		/** Override of ErrLogger base class function.
		* In addition to this object's errors, calls global ::ClearErrors().
		*/
		virtual void DoClearErrors();

		// Extra feature support
		enum feature_t {
			feature_conversion,
			feature_attributecreation,
			feature_attributerecalculation,
			abort_on_inconsistency,
			check_illegal_input_modification,	// before module call remember input item MD5 hash, check later that it is not changed.

			feature_t_count, // must be last enum value!
		};
		enum support_t {
			support_silent=0,
			support_warn=1,
			support_off=2,
		};

		/// Return the current support level for the given feature.
		support_t GetFeatureSupport(feature_t feature) const;

		/// Set the current support level for the given feature.
		void SetFeatureSupport(feature_t feature, support_t support);

		/** Search for given MemBlock object in all attached DataBlock's and
		* return the name(s) under which it appears. Return empty string if not found.
		* Used for diagnostic purposes.
		*/
		Nbaseutil::safestring GetItemName(const MemBlock* addr) const;


		/** Set verbosity levels for debugging. 
		* Currently not supported very throrougly/uniformly.
		* @param level  0 = no output, 99 = full output.
		*/
		void SetVerbosity(int level);

		/// Return current verbosity level.
		int Verbosity();

		/// Emit a verbosity message at given level.
		void Verbose(const Nbaseutil::safestring& msg, int level=99);

		/// Return a new DataBlockCallback object for script execution, or NULL, if not needed. Used for aliases support
		DataBlockCallback* GetDataBlockCallback();

		/// Add a data item alias. db should be the current datablock.
		void AddAlias(const safestring& itemname1, const safestring& itemname2, DataBlock& db);

		/// Check if the name has any aliases.
		bool IsAliased(const safestring& itemname) const;

		/// Synchronize any data item alias changes
		// Use inlined forwarder only if aliases present
		void SyncAliases(ExecutionContext& ctx, DataBlock& db) {if (aliases_) SyncAliases_internal(ctx, db);}

		//void SetTransparent(bool transparent);

	private: // implementation

		/// The only ctor; see Create().
		Instance();

		/// Friend declaration for allowing destruction 
		friend class ThreadSharablePointer<Instance>;

		/// Dtor made private to force smartpointer usage only.
		~Instance();

		/// Unimplemented copy ctor to avoid copying of the entity object
		Instance(const Instance& b); // unimplemented

		/// Unimplemented assignment op to avoid copying of the entity object
		Instance& operator=(const Instance& b); // unimplemented.

		void SyncAliases_internal(ExecutionContext& ctx, DataBlock& db);

	protected: // implementation
		virtual PSharable DoClone() const;

	private: // data
		int verbosity_;
		PDataBlock db_;
		PScript script_;
		support_t support_[feature_t_count];
		volatile int aborted_;    // if instance is being aborted
		PAliases aliases_;
		ExecutionContext::metadata_t metadata_; // store metadata (provideinputs) from one Run() to another.
	};

	/** A throwable object for early exiting of the script. 
	* throwing this is considered a normal stop.
	* See stop module.
	*/
	struct Stopped: public std::runtime_error {
		PDataBlock datacontext;
		/** Ctor
		* @param msg Arbitrary message
		* @param db Current datablock. This is transported in the Stopped object out of any 
		*		recursively called subscripts, etc., and displayed in the EvoShell Data Explorer tab.
		*/
		Stopped(const char* msg, PDataBlock db): std::runtime_error(msg), datacontext(db) {}
		~Stopped() throw() {}
	};

} // namespace

#endif // include guard
