#ifndef x_ACAPELLA_IMACRO_MODULECALL_H_INCLUDEDF_
#define x_ACAPELLA_IMACRO_MODULECALL_H_INCLUDEDF_

#include "explicitargument.h"

namespace NIMacro {

	using Nbaseutil::safestring;
	class Module;
	typedef ThreadSharablePointer<Module> PModule;
	struct ExecutionContext;
	class RunFrame;
	class DataBlock;
	typedef ThreadSharablePointer<DataBlock> PDataBlock;
	class Mod;
	class ModPar;
	class Script;
	typedef ThreadSharablePointer<Script> PScript;
	class Executive;
	struct ParserState;
	class ResolvePoint;

	/**
 	* This class is mostly for internal Acapella core use. For client code only informational queries might be useful.
	*
	* ModuleCall object represent a parsed module call in the script.
	* Each ModuleCall is owned by a certain Script object.
	* ModuleCall's are created during Script::Init() and are immutable after that.
	* ModuleCall's are destroyed at next Script::Init() or at Script's dtor.
	* 
	* ModuleCall's are not reference-counted, so be careful when
	* remembering pointers/references to them. It is adviced to remember 
	* the ModuleCall index (module no) in the Script object instead.
	*/
	class DI_IMacro ModuleCall: public mb_malloced {
	public:

		/// Simple ctor for temporary placeholder objects.
		ModuleCall();

		/// Initialize simple attributes. The args argument is swapped into the object.
		void Init(const safestring& callname, const safestring& package, int line, int pos_in_script, safestring& args, bool strict=false);

		/// Initialize during script text parsing. This is for internal Acapella use.
		void Parse(ParserState& state);

		/// Is it known which module to call when the module call is run?
		bool IsResolved() const;

		/// Try to resolve the module name as appearing in the script text, in the position of this module. Returns NULL if not found.
		PModule ResolveName(const safestring& callname) const;

		/**
		* @brief Link the module call to another module.
		* @param module Pointer to the module, may be NULL.
		*/
		void ChangeModule(PModule module);

		/// Comparison op. Return true if the ModuleCall's are created by the same module call in some script file.
		bool operator==(const ModuleCall& b) const;

		/// Dtor
		~ModuleCall();

		/** Execute the module call. This is 'const' reflecting the fact that ModuleCall
		*	is immutable after initialization.
		* @return The skip in the script to the next module, usually 1.
		* @param ctx The current execution context, all fields must be valid.
		* @param ySyntaxRun Pass true for syntax check mode run. 
		*			Result of ctx.exec.CallSyntaxRun() call is normally passed here.
		*/
		int Run(ExecutionContext& ctx, bool ySyntaxRun=false) const;

		/**
		* Run the module call in the "closure" mode. This is used by the Closure obejcts.
		* @param ctx The execution context for the call. Precondition: ctx.GetCall()==this.
		* @param bound_args Arguments bound by the closure. Warnings are logged for unknown arguments.
		* @param expl_args Extra explicit arguments. Warnings are logged for unknown or already bound arguments.
		* @param suppresswarnings Do not generate warnings about extra arguments which are not present in the called module interface, or which are already bound.
		*/
		void RunForClosure(ExecutionContext& ctx, const Container& bound_args, const Container& expl_args, bool suppresswarnings) const;

		/// Called mostly from inside Run(). Passes all defined parameters to the module and also calls Mod::PrepareCustomArguments().
		void RunFirstStage(ExecutionContext& ctx, RunFrame& runframe, bool ySyntaxRun) const;

		/// Called mostly from inside Run(). Actual execution of the module. In case of parallel execution this may occur in another thread than RunFirstStage().
		int RunSecondStage(ExecutionContext& ctx, RunFrame& runframe, bool ySyntaxRun) const;

		/// Fetch single dynamic metadata item, which has been stored by SetMetaItem() earlier.
		DataItem GetMetaItem(const Nbaseutil::safestring& itemname, ExecutionContext& ctx) const;

		/// Store dynamic metadata item; this is per-Instance data. This applies for the ModuleCall in the context of the specified ExecutionContext; if the execution of this Script is terminated, the ExecutionContext object and the metadata will be deleted.
		void SetMetaItem(const Nbaseutil::safestring& itemname, const DataItem& item, ExecutionContext& ctx) const;

		/// Compose the module call (without comments, but with linefeeds embedded as in the original script text).
		safestring GetCall() const;

		/// Return command-line arguments as sent in to the ctor. These don't contain comments, but may contain embedded linefeeds.
		safestring GetArgs() const;

		/** If this is procedure call, return the procedure header as appearing in the script. 
		* Otherwise throw ERR_PROGRAM_ERROR.
		* @param removecomments Remove comments from the returned header.
		*/
		safestring GetProcHeader(bool removecomments) const;

		/**
		* Return relative jump in the Script's module list for the case
		* if this modulecall caused a branching jump. These jumps are precalculated
		* during the Script initialization. This function always returns the same
		* number regardless of script execution state. See also ModuleCall::Run(), 
		* Mod::AnnounceBranchingSupport(), Instance::SetToBranch().
		*/
		int GetBranchJump() const {
			if (branchjump_==0) {
				throw Exception(ERR_PROGRAM_ERROR, "Branching jumps not prepared.");
			}
			return branchjump_;
		}

		/** Store the jump for branching case, to be fetched later by GetBranchJump().
		*	This is called from ::ProcessModuleCall() and return_module::Run().
		*/
		void SetBranchJump(int bj) {branchjump_=bj;}

		/// Return the filename this module call appears in
		const safestring& GetFileName() const; 

		/// Return the 1-based line number in the script file where this module call starts.
		int GetLineNo() const {return line_;}

		/// Return the module name of this module call, exactly as appearing in the script text.
		const safestring& GetCallName() const;

		/// Return the package name this module call appears in.
		const safestring& GetSurroundingPackageName() const {return package_;}

		/// Return proper module name. Kept in this class mostly for convenience.
		safestring GetModuleName() const;

		/// Return the full module name in format PACKAGE::PROPERNAME
		safestring GetFullModuleName() const;

		/** Return the per-module comment (comment appearing immediately after 
		*   the module call closing paren in the script text). There is no way
		*	to return embedded comments inside the module call.
		*/
		Nbaseutil::safestring GetComment() const;

		/** Return the Mod object pointer for C++-styled modules, otherwise NULL.
		* Should be called during module running, so that the ctx.argbase member is correct.
		* If called elsewhere, then a "shared" Mod object will be returned which might
		* be in improper state and not associated with this ModuleCall.
		*/
		Mod* GetMod(RunFrame& ctx) const;

		/// Return Module object reference for this ModuleCall. Throws if module cannot be resolved.
		const Module& GetModule() const;

		/// Return the Script which owns this module call.
		Script& GetScript() const {return *parent_;}

		/// Assign new Script parent; this is called from Script::Swap().
		void SetParent(Script& parent) {parent_ = &parent;}

		/** Return explicit expression string for that parameter, as appearing
		*   in the script text, or "" if p is not a parameter for this module, or it is not 
		*	explicitly specified in the module call. For PAR_MULTI parameters
		*	returns a comma-separated list of expressions.
		* @param p The searched parameter; can be NULL, in this case empty string is returned.
		*/
		safestring GetExplValue(const ModPar* p) const;

		/** Just forwards to Module::GetBranchingFlags(). Defined here because Module class is not DLL-exported.
		*  See mod.h about possible branching flags applicable to a module.
		*/
		unsigned int GetBranchingFlags() const;

		/** Just forwards to Module::HasBranching(). Defined here because Module class is not DLL-exported.
		* Returns true if the script execution can be in principle branched at this module.
		*/
		bool HasBranching() const;

		/// Returns true if the module call is parsed in the strict mode.
		bool IsStrict() const {return strict_;}

		/** Just forwards to Module::ParamCountPositional(). Defined here because Module class is not DLL-exported.
		* Returns the number of positional parameters of the module.
		*/
		int ParamCountPositional() const;

		/// Just forwards to Module::GetParameter(). Defined here because Module class is not DLL-exported.
		const ModPar* GetParameter(const char* ParamName) const; 
	
		/// Return the index of this modulecall in the script, or a negative value in the case of error (broken backlink -- should not happen).
		int GetModuleNo() const; 

		/**
		* Set the explicit parameter expression. Return true, if any changes have been made to ModuleCall.
		* NB! This method violates the immutability rule of ModuleCall after initialization!
		* Use with care and ensure that other threads are not using the ModuleCall at the same time!
		*
		* @param exec Not used.
		* @param p Identifies the parameter whose explicit value is set. This must be already present in the ModuleCall.
		* @param value The expression string as appearing in the script.
		*    For PAR_USERNAMED or PAR_MULTI params value has to be a comma-separated list of expressions.
		* @param no The index of the ModuleCall in it's Script. 
		*	If >=0, then SetExplValue sends the changes, if any, 
		*	also to the Script, and the script text is updated. 
		*	The 'no' value must coincide with the index of the ModuleCall in it's script.
		*/
		bool SetExplValue(Executive& exec, const ModPar* p, const char* value, int no);

		/**
		* Set all explicit parameters as appearing in the module call. Return true, if any changes have been made to ModuleCall.
		* NB! This method violates the immutability rule of ModuleCall after initialization!
		* Use with care and ensure that other threads are not using the ModuleCall at the same time!
		* If update_script==true, then this method also sends the changes, if any, to the Script, and the script text is updated. 
		*/
		bool SetExplArgs(const safestring& explicit_args, bool update_script);

		/**
		* Specify output renaming argument. Returns true if any changes are made.
		* If expr is empty, then deletes the corresponding output renaming param. Note that deleting all 
		* of the output renaming params does not reset the output-renamed flag, use SetOutputRenamed(false) for that.
		*
		* @param exec Not used.
		* @param newname Must be a valid Acapella data item name. This will be output from the module call.
		* @param expr The expression for calculating the output data item. If empty, then the output argument 'newname' is deleted, 
		*             if it was present before.
		*/
		bool SetOutputArgument(Executive& exec, const safestring& newname, const safestring& expr);

		/// Sets the output-renamed flag for this module call.
		void SetOutputRenamed(bool output_renamed);


		/**
		* Return the label of this module call suitable for displaying in the Block Diagram Editor.
		* Uses Mod::GetBoxFormat() for C++-style modules.
		*/
		safestring GetBoxLabel() const;

		/**
		* Return the string character index of parent_.script_, where this module call begins.
		* @param checkstring Script text for verification. If the module name is not found here at the known position,
		*			an Exception is thrown. Should not happen in principle.
		*/
		int GetStartPos(const safestring& checkstring) const;


		/**
		* Specify the comment for the module call. This is directly following the 
		* the enclosing brace of the module call in the script text.
		* @param comment The comment text.
		* @param append If true, then the comment text is appended to the existing module call. 
		*				This is used for continuation lines after the main comment line.
		*/
		void SetComment(const char* comment, bool append=false);

		/**
		* Return reference to an "empty" modulecall. Modification of this is not allowed.
		*/
		static ModuleCall& Void();

		/// Same as GetSubScript(void) overload, the ctx argument is not used. Retained for backward compatibility.
		PScript GetSubScript(ExecutionContext& ctx) const;

		/// Same as GetSubScript(void) overload, the exec argument is not used. Retained for backward compatibility.
		PScript GetSubScript(Executive& exec) const;

		/// Return subscript. If the subscript has not been parsed, then reparse is attempted in the context of mainscript. 
		PScript GetSubScript() const;

		/// For restricted use in case of invalid module calls - see ::ProcessModuleCall() function.
		void SetApparentModuleName(const safestring& modulename) {modname_ = modulename;}

		/// Returns true if the module call contains output renaming syntax.
		bool HasOutputRenamed() const {return output_renamed_;}

		/// Receives output-renamed outputs after the module run. Used internally from the Module class.
		void ReceiveOutputParams(ExecutionContext& ctx, DataBlock& module_output, DataBlock& db, bool syntax_run) const;

		/**
		* Return a list of possible completions of partial module name, according to packages and local procedures
		* visible at the point of this ModuleCall.
		* @param callname_start Beginning of the module name, possibly together with the package name, as appearing in the script.
		* @param results Output array of found possible completions.
		* @param add_type Append token type in the end of each completion: "?1"=module name, "?5"=package name.
		*/
		void CompleteName(const safestring& callname_start, std::set<safestring>& results, bool add_type) const;

	private: // implementation.

		void Cleanup();

		/**
		* Should be called before accessing internal data, if procversion_ is not zero.
		* If this ModuleCall corresponds to a procedure, then resets the secondary data
		* in this object, if the procedure has been reparsed meanwhile.
		* @param exec Obsolete, not used.
		*/
		void FixProcedureCallIfNecessary(Executive& exec);

		/**
		* Implementation procedure of parsing explicit module arguments. This is called from Init().
		* @param argstring Comma-separated list of module arguments as appearing in the script
		*					text, without comments and surrounding parenthesis, but with possible
		*					linefeeds embedded.
		*/
		void ParseExplicitArgs();

		/**
		*	Compose the argument part of the module call. In the ctor the modargs_ member 
		*	is just set to the explicit argument list as present in the initial script text.
		*   This function modifies this member so that it lists the expressions
		*	for positional parameters first, in positional order, followed by the
		*	name=value expressions for any nonpositional parameters having nontrivial
		*	value. This function is used for composing the CallView control content in 
		*	the EvoShell plugin when the user is changing individual parameter values.
		*   The resulting argument list is returned by the GetArgs() member function.
		*
		*   NB! Note that this method is violating the immutability requirement of the
		*	ModuleCall after initialization. Use with care and ensure that other threads
		*	are not accessing this ModuleCall at the same time!
		*/
		void ComposeArgs(Executive& exec);	

		void PassExplicitParams(ExecutionContext& ctx, const Container& args, RunFrame& runframe, bool suppresswarnings) const;
		void RunFirstStageForClosure(ExecutionContext& ctx, RunFrame& runframe, const Container& bound_args, const Container& expl_args, bool suppresswarnings) const;

		/// Try to resolve the module name using the passed resolvepoint. Returns NULL if not found.
		PModule ResolveName(const safestring& callname, ResolvePoint& rp) const;

	private: // data
		// branching support
		safestring modname_, modargs_, comment_, package_;
		PModule mod_;			// pointer to the registered module
		typedef std::vector<ExplicitArgument, mb_allocator_typedef<ExplicitArgument>::allocator > explargs_t;
		explargs_t explargs_;	// explicit arguments occuring in the call.
		typedef std::vector<OutputArgument, mb_allocator_typedef<OutputArgument>::allocator > outargs_t;
		outargs_t outargs_;	// output renaming arguments occuring in the call.
		Script* parent_;		// pointer to the script the call is in.
		int branchjump_;		// branching jump in the Script in case of branching.
		int line_, pos_;				// line number in the file, and character position in parent_.script_
		int procversion_;			// remember proc_support::version_ here for procedure calls. ==0 for non-procedures.
		bool output_renamed_;		// module call contains output renaming part
		bool strict_; // strict mode

	private: // static data
		static ModuleCall* voidcall;
	};


} // namespace
#endif
