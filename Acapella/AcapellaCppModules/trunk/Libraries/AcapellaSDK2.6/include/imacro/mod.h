#ifndef _IMACRO_MOD_H_INCLUDED_
#define _IMACRO_MOD_H_INCLUDED_

#include <stddef.h>

#if defined(_MSC_VER) && !defined(_CPPRTTI)
#if _MSC_VER<=1200
#error RTTI (Run-time type info) is required. Switch on RTTI in Project:Settings:C/C++/C++ Language.
#else
#error RTTI (Run-time type info) is required. Switch on RTTI in Project property pages: C/C++: Language.
#endif
#endif

namespace NIMacro {

class Module;
class ModuleCall;
class Instance;
class DataBlock;
class Executive;
class Script;
class ModParTrier;
class ModPar;
class proc_support;
struct ExecutionContext;
struct ModRegHelper;
class UnitTest;
class ErrLogger;
class ScriptFile;
class TextHolder;
typedef ThreadSharablePointer<Module> PModule;
typedef ThreadSharablePointer<ScriptFile> PScriptFile;
typedef ThreadSharablePointer<DataBlock> PDataBlock;
typedef ThreadSharablePointer<Script> PScript;
typedef ThreadSharablePointer<TextHolder> PTextHolder;

/**
* Mod: base class for C++ style IMacro modules.
*
* Derive your own class from here and override Declare() and Run() methods.
* then add a line for your class into ExportedModules() function.
* The Run() method of this instance will be called for running the module.
*
* The derived class will declare private or protected member variables
* which serve as the module parameters. When the module is called, Acapella
* fills in the variables corresponding to the input parameters. After the 
* module Run() method returns, Acapella copies back the data from the variables
* declared as module output parameters.
*
* For any module typically only one Mod-derived object exists. If the module is
* called in several places in the script, then the object is just reused. If the
* module is called recursively or parallely (from other thread), a new temporary 
* instance of the class is created by Acapella. Thus the module cannot count on
* that the next call will be made on the same object.
*
* It is not suggested to define extra member variables, which do not correspond
* to the module parameters. The reason is that such parameters are not automatically
* cleaned up after the module call, and the memory taken up by them may
* remain unreleased indefinitely. Of course, this is an issue only for MemBlock hierarchy
* objects and other large arrays.
*/
//class DI_IMacro Mod: public SingleThreadAlloced {
class DI_IMacro Mod: public mb_malloced {
public:
	/// Default constructor.
	/**
	* It is suggested that the derived class defines their own *private* default constructor.
	* See Create() function description for reasons.
	*/
	Mod(): Module_(NULL), ModuleName_(&s_dummy_), p_ctx_(NULL), helper_(NULL) {} // , parentstate_(NULL) {} //, instance_(NULL) {}

	/// Override to release any specific resources.
	virtual ~Mod();

	/** @brief This method returns the full name (package::propername) under which the module is registered.
	*
	* No need to override. Does not work in the constructor.
	*/
	const Nbaseutil::safestring& Name() const {return *ModuleName_;}

	/// The static creator function.
	/** This method has to be overriden and return a pointer to a new derived class instance, allocated by new operator.
		The object will be deleted with the 'delete' operator by IMacro.
		Usually it just reads: { return new derivedclass();}
		The address of this method has to be registered in ExportedModules() function of the DLL.

		The creator function might actually be a free function. However, by making it a static
		member and by defining the constructor private, one ensures that the object can be created
		only by the correct Create() function. This helps if someone makes a new module 
		by copy-pasting and forgets to change the Create() function.
	*/
	static Mod* Create();

	/**
	* Override for lengthy modules for supporting abortion in the middle of the module. 
	* This function will be called from Instance::Abort(), typically from another thread than the execution thread,
	* be sure to use appropriate locking. This function will be called only while there exists a corresponding AbortHandler
	* object, usually created as a local variable in module's Run() method.
	*
	* The overridden Abort() method should signal a condition or set a flag so that the currently executing module's Run()
	* method could return ASAP. The Run() method should just return without throwing any exception. The module output parameters
	* may remain missing (NULL). The Run() method can call the AbortHandler::IsAborted() function to find out if the abortion has 
	* occurred.
	*
	* @param mode Abortion or signalling mode. See AbortableExecutive::abortmode_t for possible values.
	* @param abort_data This is the callback value used by the creation of the corresponding AbortHandler object.
	*/
	virtual void Abort(int mode, void* abort_data=NULL) {}

	/** Return the format string which specifies what is to be displayed in the box on the
	* block diagram view. In the format the following codes can be used:
	*  %n - module full name (together with package name, if any).
	*  %m - module bare name (without package name, if any)
	*  %p - package name
	*  %c - full call as appearing in the script
	*  %a - explicit arguments
	* The text inside brackets %{ ... %} will be deleted if the code replacements
	* inside brackets only produce empty strings.
	*/
	virtual const char* GetBoxFormat() const {return "%n";}

	/** Arbitrary extensions. If the class does not handle the verb, it must forward the call to the base class.
	* @param ctx The valid execution context for a case when DoVerb() is called outside of 
	* the module run. DoVerb() can access local data members only if &ctx==&GetExecutionContext().
	* @param pszVerb The command verb.
	* @param arg1 An additional argument, meaning depends on the verb.
	* @param arg2 An additional argument, meaning depends on the verb.
	*/
	virtual SafeValue DoVerb(ExecutionContext& ctx, const char* pszVerb, const SafeValue& arg1=SafeValue(), const SafeValue& arg2=SafeValue());

protected:
	/**
	* Declare module attributes and parameter names, using module(), input() and output() methods.
	*
	* This method is generally overriden in each derived class.
	* This method is called from Register().
	*/
	virtual void Declare()=0;	// Must not be const, to allow distinguishing const and nonconst parameters!

public:
	//@{
	// Module implementation.
	//
	// This method is called by IMacro when executing the module.
	//@}
	virtual void Run()=0;

	//@{
	// Syntax check run.
	//
	// This method is called by IMacro when executing the script in syntax check mode.
	// This method is usually not overriden.
	// The method should mimic the actual run in regard of parameter names and types.
	// In most cases this can be done by IMacro according to parameter declarations,
	// so there is no need to override this method.
	//@}
	virtual void SyntaxRun();

	// service functions for use by derived modules
	// Return current execution context. If no context, throw.
	ExecutionContext& GetExecutionContext() const;
	bool HasExecutionContext() const {return p_ctx_!=NULL;}
	Instance& GetInstance() const; // Return the current IMacro instance.
	const ModuleCall& GetModuleCall() const; // Return current module call.
	ModuleCall& GetModuleCall() {return const_cast<ModuleCall&>(const_cast<const Mod*>(this)->GetModuleCall());}
	DataBlock& GetCurrentDataBlock() const; // Return the "current" data block.
	Executive& GetExecutive() const;	// Return the current executive.
	const Nbaseutil::safestring& GetModuleName() const {return Name();}	// Return the name under which the module is registered.
	PModule GetModule() const {return Module_;}	// Return pointer to the IMacro internal Module object.
	const ModPar* GetModParByAddr(int relative_addr) const;	// return parameter by it's address in a Mod object. See RecalcMod::FillDynamicParameter() in Cells library.

	/*
	* Called from module Declare() method to announce that the module pariticipates in branching constructs.
	* This enables and requires the module to call GetExecutionContext().SetToBranch() 
	* from the Run() method. If jumping is not needed, the module must call GetExecutionContext().SetToBranch(false).
	* The behavior in case of branching depends on the flags specified here.
	* @param flags Composed of branching_flags enum bit-flags:
	*   - IF, TRY, SWITCH: branch_begin
	*   - ELSE, CASE, CATCH_ERROR, CATCH_WARNING, CATCH_ABORT, CATCH_STOP: branch_break
	*   - ELIF: branch_break|branch_elif
	*   - WHILE, FOREACH, EVENTLOOP: branch_begin | branch_looping
	*   - BREAK: branch_break|branch_looping
	*   - CONTINUE: branch_continue|branch_looping
	*   - END: branch_end
	*/
	void AnnounceBranchingSupport(unsigned int flags);

	// NOTE: must not define values creater than 1<<13!
	/// Bit-flags to be used with AnnounceBranchingSupport().
	enum branching_flags {
		branch_begin=1,		// this module acts as a branching construct beginning. this is the default.
		branch_end=2,		// this module acts as a branching construct end module
		branch_break=4,		// this module appears inside a branching construct or a procedure. if the branching occurs, then the execution will continue after the end() module.
		branch_continue=8,	// this module appears inside a branching construct or a procedure. if the branching occurs, the execution will be started again from the previous branch_looping module.
		branch_looping=16,	// for branch_begin modules: the execution should jump back to this module from the matching end() module, if the branching did not occur.
							// for branch_break and branch_continue: operate on most enclosed looping cycle, instead of the most recent construct.
		branch_labelled=32,	// the module supports an optional label, which must textually coincide with the corresponding expression in the corresponding construct starting module call.
		branch_dynamic=64,	// the jump is calculated at runtime by DoVerb("__get_branch_jump__");
		branch_elif=128,	// special ELIF semantics
	};

protected:
	// Methods called from Declare.
	//@{
	// Specify module attributes and description.
	// Module name is not specified here, but in ExportedModules() function instead.
	// @param Flags		Combined from MOD_* flags in <MemBlock/modreg.h>.
	// @param Groups	Comma-delimited list of informal group names the module belongs to.
	// @param Description	A pointer to a static or permanent ASCIIZ string giving a short module description.
	//@}
	void module(unsigned int Flags, const char* Groups, const char* Description) const {
		module_0( Flags, Groups, Description, NULL);
	}

	//@{
	// Declare an integer input parameter with default value.
	// @param Value		A const int data member of your derived class. This will be set appropriately when entering Run().
	// @param Name		Pointer to the static or permanent string specifying the parameter name.
	// @param Flags		Parameter flags. See modreg.h for details. PAR_INPUT|PAR_INTEGER is added automatically. PAR_OUTPUT is not allowed.
	// @param Descr		A pointer to static/permanent string describing the parameter, in HTML format. If the string is not static, then it must be prepended by a tilde character (~).
	// @param Default	Default value for the parameter.
	// @param DynaDefault	Data item name for default value, if PAR_DYNADEFAULT flag is used.
	//@}
	ModPar* input(const int& Value, const char* Name, unsigned int Flags, const char* Descr, int Default) const;
	ModPar* input(const int& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;

	//@{
	// Declare an integer input parameter without default value.
	// See the other integer input method for more details.
	//@}
	ModPar* input(const int& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare an integer output parameter.
	// See integer input methods for more details.
	// @param Value An int data member of your derived class. You must set this before returning from Run().
	// @param Flags Parameter flags. See modreg.h for details. PAR_OUTPUT|PAR_INTEGER is added automatically. PAR_INPUT is not allowed.
	//@}
	ModPar* output(int& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	// cast unsigned int to int in order to avoid registering temporaries!
	ModPar* input(const unsigned int& Value, const char* Name, unsigned int Flags, const char* Descr, int Default) const {
		return input( *((const int*) &Value), Name, Flags, Descr, Default);
	}
	ModPar* input(const unsigned int& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const {
		return input( *((const int*) &Value), Name, Flags, Descr, DynaDefault);
	}
	ModPar* input(const unsigned int& Value, const char* Name, unsigned int Flags, const char* Descr) const {
		return input( *((const int*) &Value), Name, Flags, Descr);
	}

	ModPar* input(const Nbaseutil::int64& Value, const char* Name, unsigned int Flags, const char* Descr, Nbaseutil::int64 Default) const;
	ModPar* input(const Nbaseutil::int64& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(const Nbaseutil::int64& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(Nbaseutil::int64& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	ModPar* input(const double& Value, const char* Name, unsigned int Flags, const char* Descr, double Default) const;
	ModPar* input(const double& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(const double& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(double& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare an ASCIIZ string input parameter. The constness of Value pointer may not be violated.
	// @param Default	A pointer to static or permanent ASCIIZ string specifying the default value for the parameter.
	//@}
	ModPar* input(const char* const& Value, const char* Name, unsigned int Flags, const char* Descr, const char* Default) const;
	ModPar* input(const char* const& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare an ASCIIZ string output parameter.
	// @param Value A char* data member of your derived class. Store a pointer to the output static or permanent string there before
	//		returning from Run(). Use TempString() or TempStringFrom() functions to produce permanent strings.
	//@}
	ModPar* output(const char*& Value, const char* Name, unsigned int Flags, const char* Descr) const;


	//@{
	// Declare a safestring input parameter. The main advantage compared to const char* is
	// that this string type is a "real" C++ type and easier to manipulate.
	// @param Default	A pointer to a string specifying the default value for the parameter.
	//@}
	ModPar* input(const safestring& Value, const char* Name, unsigned int Flags, const char* Descr, const char* Default) const;
	ModPar* input(const safestring& Value, const char* Name, unsigned int Flags, const char* Descr, const safestring& Default) const {
		return input(Value, Name, Flags, Descr, Default.c_str());
	}
	ModPar* input(const safestring& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare a safestring output parameter.
	// @param Value An Nbaseutil::safestring type data member of your derived class. Store the returned string here before
	//		returning from Run().
	//@}
	ModPar* output(Nbaseutil::safestring& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare a MemBlock input parameter.
	//@}
	ModPar* input(const NIMacro::PMemBlock& Value, const char* Name, unsigned int Flags, const char* Descr, const NIMacro::PMemBlock Default) const;
	ModPar* input(const NIMacro::PMemBlock& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(const NIMacro::PMemBlock& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(NIMacro::PMemBlock& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare a boolean input parameter.
	//@}
	ModPar* input(const bool& Value, const char* Name, unsigned int Flags, const char* Descr, bool Default) const;
	ModPar* input(const bool& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(const bool& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(bool& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare a raw pointer parameter.
	//@}
	ModPar* input(void* const& Value, const char* Name, unsigned int Flags, const char* Descr, const void* Default) const;
	ModPar* input(void* const& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(void* const& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(void*& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	//@{
	// Declare a polytype parameter.
	//@}
	ModPar* input(const SafeValue& Value, const char* Name, unsigned int Flags, const char* Descr, SafeValue Default) const;
	ModPar* input(const SafeValue& Value, const char* Name, unsigned int Flags, const char* Descr, const char* DynaDefault) const;
	ModPar* input(const SafeValue& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* output(SafeValue& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	// Sharable parameter
	ModPar* input(const PSharable& Value, const char* Name, unsigned int Flags, const char* Descr) const;
	ModPar* input(const PSharable& Value, const char* Name, unsigned int Flags, const char* Descr, PSharable Default) const;
	ModPar* output(PSharable& Value, const char* Name, unsigned int Flags, const char* Descr) const;

	/// Return ModPar pointer by the parameter argument inside the same object, or NULL.
	ModPar* GetModParAt(const void* p_arg) const;

public:
	//@{
	// For announcing warnings during module execution.
	// In case of errors throw an NIMacro::Exception instead
	//@}
	void Warning(unsigned int Reason, const Nbaseutil::safestring& Message) const;

	//@{
	// Called by IMacro only
	//@}
	void Register(NIMacro::Module* m);

	/// Declare a parameter of type 'stype' as specified by the input() module:
	ModPar* inputany(const char* sname, const char* stype, const char* sdesc, const SafeValue& defvalue);

	/// Assign new executioncontext.
	void SetExecutionContext(ExecutionContext* ctx);

	/// Return a string describing the current script position. There is also a namespace-level function with the same name, returning an empty string.
	safestring GetScriptLocation(const safestring& format="") const;

private:
	// implementation
	void module_0(unsigned int Flags, const char* Groups, const char* Description, void* dummy) const;
	bool CheckModuleProperlyLocked();
protected:
	inline ModPar* param(void* Value, const char* Name, NIMacro::ItemType Type, unsigned int Flags, const char* Descr, const void* Default) const;
	void SetModuleName(const Nbaseutil::safestring* s) {ModuleName_=s;}

	/// Copy the Mod part of the object
	void CloneBase(Mod& clone) const;

	/// Execute a piece of script text and return the script output datablock. May be called only during module run.
	PDataBlock RunSubScript(const safestring& scriptname, const safestring& scripttext, PDataBlock inputdata=NULL);

private:
	// Data
	mutable PModule Module_;
	const Nbaseutil::safestring* ModuleName_;
	// SetModuleName() is called only from:
	friend class ::NIMacro::Module;

	friend class proc_support;
	ExecutionContext* p_ctx_;	// pointer to the current execution context. this is valid only during Run()/SyntaxRun().
	ModRegHelper* helper_; // valid in Declare() only.

protected:
	//Instance* instance_;	// A pointer to the current IMacro instance, or NULL.
//	ModuleMapper* parentstate_;
public:
	/// If the module calls a subscript, then return pointer to the text holder of its text, otherwise NULL.
	virtual PTextHolder GetSubScriptTextHolder() {return NULL;}

	/**
	* If the module parses the module call line by itself or uses some other non-explicit means for getting input data from the input datablock,
	* then it should perform such activity in the PrepareCustomArguments() virtual function. Failure to do that means 
	* that the module cannot be called in parallel mode. The base class implementation does nothing.
	* When this function is called all explicit arguments are already passed to the module.
	*/
	virtual void PrepareCustomArguments() {}

public:
	// Unit test support
	/**
	* NB! Unit test support not yet implemented, i.e. this function is not in use currently.
	* Return a new-allocated object capable of varying input values for specified parameter. Return NULL to use predefined rules for varying that parameter.
	*/
	virtual ModParTrier* CreateModParTrier(const safestring& paramname, const ModPar& param) const {return NULL;}

	/**
	* NB! Unit test support not yet implemented, i.e. this function is not in use currently.
	* Verify that the input parameters of 'this' are acceptable for the module.
	* If not, log error message(s) through log.Error() and return false.
	* You can call this method also from Run():
	* @code
	*	if (!PreCondition(GetInstance()) throw Exception(ERR_BADDATA, "Invalid input data.");
	* @endcode
	*/
	virtual bool PreCondition(ErrLogger& log) const {return true;}

	/**
	* NB! Unit test support not yet implemented, i.e. this function is not in use currently.
	* Verify that module output (given by 'this' object) is valid and corresponds to the input parameters (given as the 'input' object).
	* If some errors found, report these through log.Error() and return false.
	*/
	virtual bool PostCondition(const Mod& input, ErrLogger& log) const {return true;}

	/** 
	* NB! Unit test support not yet implemented, i.e. this function is not in use currently.
	*/
	virtual void TestCases(Mod& input, UnitTest& tester) {}

private: // static data
	static const Nbaseutil::safestring s_dummy_;
private:
	//void SetInstance(Instance* p) {instance_=p;}
//	void SetParentState(ModuleMapper* state) {parentstate_=state;}
	// are called only from
	friend class ModuleCall;
	
};

/*
* A helper class for supporting abort in the middle of module execution.
* In module's Run() method, declare a local object of type AbortHandler in lengthy sections of code.
* While the AbortHandler object lives, Mod::Abort() virtual function will be called in case of abortion.
* The abort_data parameter is passed to the Mod::Abort() virtual function.
*
* The abort in the middle of the module happens only if the current Executive supports it 
* (is derived from AbortableExecutive or handles the "set_abort_hook" verb otherwise).
*
* There is no need for using any mutex for accessing AbortHandler, locking is performed by the AbortHandler class
* internally. The virtual Abort() function is guaranteed to be called only after AbortHandler local object creation;
* also, the code section containing the local AbortHandler object is not terminated while
* there is a virtual Abort() call underway in another thread. This means that if you have locked a mutex before creating
* the AbortHandler object and attempt to lock the same mutex in the Abort() virtual function, the program will deadlock,
* avoid this kind of usage.
*/
class DI_IMacro AbortHandler {
public:
	AbortHandler(ExecutionContext& ctx, Mod& caller, void* abort_data=NULL);
	~AbortHandler();
	/// Can be called from modules Run() method to find out if an abortion has occurred. If yes, the Run() method should return ASAP without throwing any exception.
	bool IsAborted();
private:
	void Abort(int mode);
	static void AbortHook(int mode, void* p_this);
private:
	ExecutionContext& ctx_;
	Mod& caller_;
	void* abort_data_;
	DataItem prev_handler_;
};

// type of static Mod::Create() method:
typedef Mod* (*ModCreatorFunc)();

// A struct for using in ExportedModules functions
struct ModReg {
	const char* ModName;
	ModCreatorFunc ModCreator;
};

#define MOD_CLASS_LIBRARY (1<<29) // Return from ExportedModules to indicate that pointers to Mod::Create static methods are in the return list.

// An example of ExportedModules function:
// DI_IMacro macro is defined in dllexport.h
//
//	DI_IMacro int ExportedModules( ModReg** x, int *Count) {
//		static ModReg p[] = {
//			{"read_hdf", read_hdf::Create},
//			{"info_hdf", info_hdf::Create},
//			{"write_hdf", write_hdf::Create},
//		};
//		*x = p;
//		*Count = sizeof(p)/sizeof(ModReg);
//		return MOD_CLASS_LIBRARY;
//	}

// Resolve script-relative file names.
// If s starts with a colon, replace the colon with the current script file path.
// To be used in Mod::Run() methods.
DI_IMacro safestring ResolveScriptRelativeFilename(Mod* m, const safestring& s);


/**
* A base class for modules concerned with the data exchange inbetween script and host environment, e.g. input().
* The module should declare also MOD_INPUT_MODULE or MOD_PROVIDEINPUT_MODULE flag.
*/
class DI_IMacro IO_module: public Mod {
	typedef Mod super;
public:

	/**
	* A placeholder for a virtual function to be added in the future; do not use.
	*/
	virtual ModPar* Reserved1(Mod* mod) {return NULL;}

	/**
	* A placeholder for a virtual function to be added in the future; do not use.
	*/
	virtual ModPar* Reserved2() {return NULL;}

	
	/**
	* @brief Return the label of the input data item.
	*
	* The function has to dig out the data item label from the specified module call. 
	* This is called at the parsing stage, but the module call explicit arguments have been
	* parsed already so the function can use ModuleCall::GetParameter() and ModuleCall::GetExplValue().
	*
	* If the label cannot be found or is not applicable, an empty string should be returned.
	*/
	virtual safestring GetLabel(ModuleCall& call) const {return "";}

	/// List possible return values from FindEnviroMode().
	enum enviro_mode_t {
		/// No environment setting in force
		env_default,
		/// Client environment; the operation should be forwarded to the AccProxy client interface.
		env_client,
		/// Native environment; the operation should be performed on this computer.
		env_local,
	};
	
	/**
	* Declares the "__env" parameter; to be called from the overridden virtual Declare().
	* @param pathname_parameter_name If the module has a filename-style parameter which can have client:// or server:// prefixes, then pass its name here.
	*/
	void DeclareEnv(const safestring& pathname_parameter_name);

	/**
	* Returns the current running environment preference, and strips the environment prefix from the 
	* pathname argument. If Acc::ClientCommand() module is not loaded/loadable, returns env_local always.
	* The following sources of information are checked, in this order:
	*   * The prefix of the 'pathname' argument, if present.
	*	* The explicit '__env' parameter override.
	*   * The system datablock '__env' parameter.
	*   * The 'default_value' argument is returned.
	* @param pathname A file or directory name which can contain "client://" or "local://" prefix. This will be stripped away, if found.
	* @param default_value If the environment mode cannot be determined otherwise, the default value is returned.
	*/
	enviro_mode_t FindEnviroMode(safestring& pathname, enviro_mode_t default_value);

	/**
	* Returns the current running environment preference.
	* If Acc::ClientCommand() module is not loaded/loadable, returns env_local always.
	* The following sources of information are checked, in this order:
	*	* The explicit '__env' parameter override.
	*   * The system datablock '__env' parameter.
	*   * The 'default_value' argument is returned.
	* @param default_value If the environment mode cannot be determined otherwise, the default value is returned.
	*/
	enviro_mode_t FindEnviroMode(enviro_mode_t default_value);

private:
	safestring env_;
};

// Some helper functions for other DLL-s to create modules based on Mod classes.
// The Module class itself is not exported from IMacro.dll.

/// Create a Module object and return it's pointer. The module is not registered in IMacro global module map.
DI_IMacro const Module* CreateModule(const char* ModuleName, void* dllhandle, ModCreatorFunc pfCreator);

/// Delete a Module object.
DI_IMacro void DeleteModule(const Module* mod);

/// Return associated Mod by Module, or NULL.
//DI_IMacro const Mod* GetModByModule(const Module* m);

/// Return module type of m.
DI_IMacro int GetModuleType(const Module* m);

/// Get module flags
DI_IMacro unsigned int GetModuleFlags(const Module* m);

/// Add module flags
DI_IMacro void AddModuleFlags(Module* m, unsigned int additional_flags);

DI_IMacro PTextHolder GetModuleTextHolder(const Module* m);

DI_IMacro Nbaseutil::safestring GetModuleName(const Module* m);

DI_IMacro Nbaseutil::safestring GetFullModuleName(const Module* m);

// Scan all parameters and call x.Visit() each time.
class ParamVisitor;
DI_IMacro void ScanParameters(const Module* m, ParamVisitor& x, bool bynumber=true);

DI_IMacro safestring GetModuleDllName(const Module* m);

//DI_IMacro void CopyModuleInputParameters(ExecutionContext& ctx, RunFrame& runframe, const Module* m, DataBlock& db);

/**
* @brief [Linux only] A structure for supporting module library versioning under Linux.
*
* A module library can define a function 
* @code
* const NIMaco::AcaVersionInfo& GetAcaVersion();
* @endcode
* returning a reference to a static instance of AcaVersionInfo struct,
* which has been filled according to the .rc file. There is a separate
* tool for generating GetAcaVersion() functions from .rc files in Linux.
*/
struct AcaVersionInfo {
	int version, subversion, buildmajor, buildminor;
	const char *builddate, *comments, *description;
};

class eval_module_base: public Mod {
public: // overridden base class methods
	virtual void Run();
	virtual void SyntaxRun();

	virtual PTextHolder GetSubScriptTextHolder();
public: 
	void Run(Executive& exec);
protected:
	//PScript ParseScriptIfNeeded(Executive& exec);
	//PScript GetEvalScript() const;
	safestring GetHash() const;
	bool PrepareText(ExecutionContext& ctx, safestring& buffer);
protected:
	safestring EvalCmd;
	safestring ErrMsg, ErrMsgXml;
	int errorcode, warningcode;
	bool yAbort, yFwdLic;
};

/// Return an empty string. Cf. Mod::GetScriptLocation(). This is to be used in macros like LOGGER().
DI_IMacro safestring GetScriptLocation(const safestring& format="");

#ifdef ACAPELLA_DEFINE_LOGGER
#ifndef ACAPELLA_LOGGER_DISABLED
#define ACAPELLA_LOGGER_ENABLED
#endif
#ifdef ACAPELLA_LOGGER_ENABLED
/**
* LOGGER macro for helping in logging messages to Acapella log file via the Logger class facility.
* This macro is defined only if one has #define ACAPELLA_DEFINE_LOGGER macro defined before including Acapella headers.
*
* If one has defined an ACAPELLA_LOGGER_DISABLED macro, and ACAPELLA_LOGGER_ENABLED macro is NOT defined, then the LOGGER macro is expanded to nothing.
*
* The macro checks first if the specified topic is logged at the specified level, and only if this is the case,
* evaluates the message argument and calls the actual log function. If called from inside an Acapella module (a member function 
* of a class derived from Mod), then also the current script location is added to the log message.
*
* @param topic Topic enum constant as defined in the Logger class, e.g. topic_acapella. Do not prepend Logger:: class name, it is prepended by the macro.
* @param subtopic Subtopic name as a string, e.g. "timezone".
* @param level Verbosity level constant as defined in the Logger class. Do not prepend Logger:: class name, it is prepended by the macro.
* @param message The log message (as a C string pointer or a safestring), in free format.
*/
#define LOGGER(topic,subtopic,level,message) \
	if (Logger::IsLogging(Logger::topic, Logger::level)) {\
		Logger::Log(Logger::topic, subtopic, Logger::level, safestring(message)+safestring(1, 29)+GetScriptLocation());\
	}
#else
#define LOGGER(topic,subtopic,level,message)
#endif
#endif

} //namespace NIMacro

#endif
