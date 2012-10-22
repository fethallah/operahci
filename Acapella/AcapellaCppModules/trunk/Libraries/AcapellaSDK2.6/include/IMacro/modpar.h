#ifndef x_IMACRO_MODPAR_H_INCLUDED_
#define x_IMACRO_MODPAR_H_INCLUDED_

#include <boost/noncopyable.hpp>

namespace NIMacro {

class Mod;
class Module;
typedef ThreadSharablePointer<Module> PModule;
class ModuleCall;
class Executive;
struct ExecutionContext;
struct inputitem_check;
class RunFrame;
class ResolvePoint;
typedef ThreadSharablePointer<ResolvePoint> PResolvePoint;


/** 
* Internal class for holding module parameter info for exchange with module implementation.
* In the module implementation the parameter appears just as a simple variable (int, double, PMemBlock).
* 
* Each ModPar object is owned by a certain Module object.
*/
class DI_IMacro ModPar: public mb_malloced, public boost::noncopyable {
public: // query interface

	/// Return parameter name (note this is const char*).
	const char* Name() const {return name_;}

	/// Return parameter description (note this is const char*).
	const char* Description() const;

	/// Return parameter flags, composed of PAR_xxx bits defined in modreg.h header.
	unsigned int Flags() const {return flags_;}

	/// Return zero-based position index of the parameter, or -1 for non-positional parameters.
	int PosIndex() const {return posindex_;}

	/// Return logical type of the parameter. This differs from the physical type in case of procedures, where all physical types are SafeValue/PolyType.
	ItemType GetLogicType() const;

	/// Return logic type name for procedure parameters, as specified in the procedure header.
	safestring GetLogicTypeName() const; 

	/// Return physical type of the parameter. This differs from the logical type in case of procedures, where all physical types are SafeValue/PolyType.
	ItemType GetPhysicalType() const {return type_;}

	/// Return a reference to the default value. If there is no default, the defaultvalue is Undefined.
	const DataItem& DefaultValue() const {return defaultvalue_;}

	/// Returns the list of accepted memblock classes for this parameter.
	safestring GetReqClassList(const char* delim=", ") const;

	/**
	* Return the required parameter representation. 
	* The info may be returned either in string form or as a vector of allowed vector element type constants.
	* There are two output parameters, the info is returned only in one of them, depending on whether the
	* original representation string specified only vector datatypes or contained something more.
	* @return True, if there is a required representation. In this case one of the parameters returns non-NULL.
	* @param s_repr Returns the original representation string or NULL.
	* @param t_repr Returns NULL or a pointer to an array of type Vector::DataType, terminated by Vector::Void.
	*/
	bool GetReqRepr(const char*& s_repr, const Vector::DataType*& t_repr) const;

	/// Return required representation in string format.
	safestring GetReqRepr() const;

	/**
	* During a module call, return the current value of a module parameter inside the module.
	* @param mod Module object reference, this can be obtained by ExecutionContext::GetMod() member function.
	* @return Current parameter value inside the module object.
	*/
	DataItem GetPassedValue(Mod& mod) const;

public: // mutator interface

	// safestring Describe(unsigned int Mask, int ShowDefault=1);	// Returns parameter description if flags_ & Mask.


	// Returns the parameter default value as a string. If 
	//safestring DefaultToString() const;	

	/// Replace the current parameter description with a new one. This is used by module decorator templates in acapella_base library.
	void SetDescription(const safestring& descr);

	/// Add a memblock class name, e.g. "vector" to the list of accepted memblock classes. This is used by module decorator templates in acapella_base library.
	void AddReqClass(const safestring& reqclass);

	/**
	* Process the specifed parameter representation string as specified in the module or procedure declaration, and remember the results in the parameter.
	* @param repr_spec The representation specification. This may not be empty.
	*/
	void SetReqRepr(const Nbaseutil::safestring& repr_spec);

	/// For internal usage: make deep copy for thread-passing of the parameter residing inside the module.
	void MakeCopy(RunFrame& runframe);

public: // interface for internal use by Acapella

	/// Return zero-based id of the parameter; this is unique in the scope of given module.
	int Id() const {return id_;}

	/// For internal use by Acapella: checks if the module parameter requires the literal call expression to be sent by GiveName().
	bool WantsName() const {return valuenameaddr_!=0;}

	/// For internal use by Acapella: pass the literal call expression to the parameter before module run.
	void GiveName(const safestring& s, RunFrame& runframe) const;

	/// For internal use by Acapella: returns the physical offset of the parameter in bytes inside the module object.
	int GetOffset() const {return offset_;}

	/// Resolve and resend the input parameter to the module. This is used in Cells library by object list attribute recalculations.
	void Resend(Mod* mod, ExecutionContext& ctx, RunFrame& runframe) const;

	/// For internal debugging support: assign new thread id to the MemBlock parameter passed into the function.
	void AssignThreadId(RunFrame& runframe, thread_id_t thread_id);

private: // friend interface for internal use by Acapella

	friend class Module;
	friend class ModuleCall;
	friend class ExplicitArgument;
	friend class proc_support;
	friend class Closure;

	/** ModPar constructor.
	* @param name Pointer to the ASCIIZ string containing parameter name. 
	*             For Memory type the name must be followed by requested memblock class and optional representation
	*			  specification, e.g. "image (image:8bpp,16bpp)".
	*             The ASCIIZ string must be static. If it is not static, then prepend it with a tilde ~, the ctor will make a copy of the name.
	* @param type Type of the item. Cannot be Undefined.
	* @param offset  Offset of the parameter location inside the Mod object or in the parameter passing area.
	* @param defaultvalue Pointer to the default value. The default value will be copied into ModPar during ctor, i.e. default value is not required to be static.
	*				For Memory types DefValue must point to a PMemBlock smartpointer. Only the pointer is copied, not the pointed MemBlock.
	* @param flags Any PAR_xxx flags applicable to the parameter. In particular, at least one of PAR_INPUT and PAR_OUTPUT should be present.
	* @param description Parameter description - an ASCIIZ string. The ASCIIZ string must be static. If it is not static, then prepend it with a tilde ~, the ctor will make a copy.
	* @param owner Pointer to the owning module.
	*/
	ModPar(const char* name, ItemType type, int offset, const void *defaultvalue, unsigned int flags, const char* description, Module* owner);

	/// Dtor
	~ModPar();

	/// Return true if this parameter has been already sent to the module.
	bool HasBeenSent(RunFrame& runframe) const;
	void StartSending(RunFrame& runframe);	// to be called before sending parameters to the module, clears Sended flag.
	void Cleanup(RunFrame& runframe);							// to be called after module execution, to clean up memory etc.
	void Send(const DataItem& x, ExecutionContext& ctx, RunFrame& runframe, const safestring* ExplicitArgErrMsg=NULL) const; // Send a value to module implementation
	void SendDefault(ExecutionContext& ctx, RunFrame& runframe) const;						// Send the default value to module implementation
	void Send(ExecutionContext& ctx, RunFrame& runframe, const ModPar* localcontext) const;		// Send dynamic default value to module implementation.
	void SendZero(RunFrame& runframe) const; // Send zero value to module implementation. Handles output parameters and no-default parameters.
	void ReleaseMemBlock(RunFrame& runframe);			// Set PMemBlock variable in module to NULL.

	bool CheckInputItemHash(safestring& msg, inputitem_check& chk);	// check if the input parameter was not changed by the module. Is called after module run only if consitencychecks are switched on.

	// Value getting.
	DataItem Receive(ExecutionContext& ctx, RunFrame& runframe) const;				// Copy the output value from module implementation

private: // enums

	/// Bitmask constants for extraflags_.
	enum extraflags_t {
		// name_ is allocated dynamically and must be relased by mb_free().
		name_delete_required = 1<<0,
		// description_ is allocated dynamically and must be relased by mb_free().
		descr_delete_required = 1<<1,
		// req_repr_ is a pointer to Vector::DataType array.
		reqrepr_is_typearray = 1<<2,
		/// For procedure parameters of type "integer". As PAR_INTEGER==0, one cannot encode this information in flags_.
		is_procpar_integer = 1<<3,
		/// For procedure parameters of type "int64". 
		is_procpar_integer64 = 1<<4,
	};

private: // implementation

	void DynAlloc(const char*& ptr, const safestring& text, extraflags_t& flags, unsigned int delete_required_flag);
	void DynAlloc(const char*& ptr, const char* text, extraflags_t& flags, unsigned int delete_required_flag);

private: // data
	const char* name_;			// Parameter name.
	ItemType type_;				// Parameter physical type. Shall not be Undefined.
	extraflags_t extraflags_;	// Extra flags
	unsigned short offset_;		// Offset to the parameter storage inside the module.
	unsigned short valuenameaddr_;	// If not 0, a pointer to the actual data item name or expression needs to be stored at this offset before calling the module.
	DataItem defaultvalue_;			// Default value. If Undefined, then parameter has no default.
    const char* description_;		// Parameter description. Resides physically in module.
    unsigned int flags_;			// additonal flags
	short posindex_;			// The zero-based index of position, if this is a positional parameter.
	short id_;					// number of the parameter in the module.
	//int RefCount_;
	const char* req_repr_;		// Required MemBlock class representation, or NULL. 
								// If reqrepr_is_typearray flag is set, then a pointer to the array of Vector::DataType of possible types, terminated by Vector::Void.
	// Buffers for storing required class/representation for Memory type parameters:
	safestringlist reqclass_;

};

/// A helper base class for processing parameters, see ScanParameters().
class ParamVisitor: public mb_malloced {
public:
	/**
	* Visit() will be called separately for input parameters and for output parameters.
	* For an input-output parameter Visit() will be called twice.
	* @param p The currently scanned parameter.
	* @param input True during the input parameter list scanning.
	*/
	virtual void Visit(const ModPar& p, bool input)=0;

	/// Virtual dtor for silencing compiler warnings.
	virtual ~ParamVisitor() {}
};

// Some Module-related declarations here because module.h is not exported.

/** Check if the specified module has been registered in the global StdLib module map.
*   Return pointer to it, or NULL if not found.
* @param ModuleName The name of the module. Can be prepended by the library name and colon.
* @param exec The executive to be used by module registration. Can be NULL, 
				in this case standard Executive is used. See Module::DelayedRegister().
* @param registered Obsolete, not used.
*/
DI_IMacro Module *FindModule( const safestring& ModuleName, Executive* exec=NULL, bool registered=true);

/** Same as FindModule, but also tries to load module from the IMacro's StdLib.
*   As a side-effect all StdLib module libraries may get loaded.
*   The returned module is always fully registered. If you have an ExecutionContext& present,
*   use another overload with ExecutionContext& parameter instead.
*/
DI_IMacro Module* FindOrLoadModule( const safestring& ModuleName, Executive* exec=NULL);

/**
* Same as the other FindOrLoadModule() overload, but finds modules from the current 
* main script module map as well.
*/
DI_IMacro Module* FindOrLoadModule(ExecutionContext& ctx, const safestring& ModuleName);

// Finds an existing module or tries to load/register it from an external dll.
DI_IMacro Module *FindOrLoadModule(const safestring& ModuleName, bool ensure_registered, Executive* exec=NULL);	

/**
* A more advanced module find function. 
* @param ctx Local execution context. This is used if context="__local" parameter is passed.
* @param modname The module name to look up. This name is resolved in the context determined by the Context parameter. In a nonlocal context it is suggested to use fully qualified names for more robust name lookup.
* @param context The context for module name lookup. You can pass a script file name, a closure or script object, or a predefined string "__stdlib" (pure stdlib context), \"__userlib\" (stdlib+userlib) or "__local" (context specified by the ctx parameter).
* @param resolvepoint An output parameter: pointer to the resolve point where the module was looked up.
* @return A non-null module pointer. 
*/
DI_IMacro PModule FindOrLoadModule(ExecutionContext& ctx, const Nbaseutil::safestring& modname, const DataItem& context, PResolvePoint& resolvepoint);

} // namespace
#endif
