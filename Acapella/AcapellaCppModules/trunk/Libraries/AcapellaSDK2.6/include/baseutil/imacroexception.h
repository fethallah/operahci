#ifndef x_PKI_CT_BASEUTIL_IMACROEXCEPTION_H_INCLUDED_
#define x_PKI_CT_BASEUTIL_IMACROEXCEPTION_H_INCLUDED_

#include <exception>
#include <string.h>
#include <set>
#ifndef ACAPELLA_DISABLE_DEBUG_CAST
#	include <boost/numeric/conversion/cast.hpp>
#endif

#ifdef _DEBUG
#include <limits>
#endif

#include "mb_malloc.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4275)
#endif

#include "safestring.h"

#ifdef min
#define ACAPELLA_EVIL_MIN_WAS_DEFINED
#undef min
#endif
#ifdef max
#define ACAPELLA_EVIL_MAX_WAS_DEFINED
#undef max
#endif

namespace Nbaseutil {


	/// Returns last system error in string format. This uses either Windows SDK GetLastError() or POSIX errno.
	DI_baseutil safestring GetLastErrorString();						

	/// Returns system error message corresponding to the error code (GetLastError() code on Windows, errno otherwise).
	DI_baseutil safestring GetLastErrorString(unsigned int error_code);

	/**
	* Error and warning flags to be used in Exception throwing and in Error() and Warning() modules.
	* These are bit flags so the several flags can be combined.
	* Note: The ERR_ or WARN_ prefix is not actually distinguishing between errors and warnings.
	* One can use all flags either in case of errors or warnings.
	*
	* Currently the error/warning flags are not used very much. Some errors, like ERR_PROGRAM_ERROR
	* and ERR_NOT_IMPLEMENTED are automatically stored aside by EvoShell plugin and
	* a dialog is popped up for the user to offer tp send feedback to PerkinElmer. 
	* The ERR_USER_INTERVENTION flag is actually not an 'error' per se and is silently 
	* discarded in some contexts.
	*
	* However, in the future the error flags might find more use, so it is encouraged to
	* specify them as accurately as possible.
	*
	* Most errors are ERR_BADPARAMETERS or ERR_BADDATA. Sometimes it is hard to distinguish
	* inbetween them.
	*/
	enum ErrorAndWarningCodes {

// general flags
		/// No error, no warning
		ERR_SUCCESS_ACAPELLA=0,			
		/// Warning(s) occurred
		ERR_WARNING=1,			
		/// Error occurred
		ERR_ERROR=2,			

// specific flags
// usually for errors: throw Exception(...), or Error(ERR_ERROR|..., ...)
		/// An input file does not exist, cannot be read or is in a wrong/corrupted format, or an output file cannot be created or written.
		ERR_BADFILE=(1<<2),		
		/// The module does not conform to the standards. Supposedly a module will not raise this error itself.
		ERR_BADMODULE=(1<<3),	
		/// Not enough memory to complete operation.
		ERR_NOMEMORY=(1<<4),	
		/// Missing or bad parameters for DLL or module
		ERR_BADPARAMETERS=(1<<5),
		/// Hardware-reported errors: bad memory access, division by zero, etc.
		ERR_HARDWARE=(1<<6),	
		/// Programmer error, as opposed to user or environment caused error. ERR_HARDWARE is in principle also programmer error.
		ERR_PROGRAM_ERROR=(1<<7),
		/// Script syntax is probably erroneous
		ERR_MACROSYNTAX=(1<<8),	
		/// The data (image, parameters, etc.) items seem to be contradictory or illegal.
		ERR_BADDATA=(1<<9),		
		/// Timeout in Mutex. 
		ERR_TIMEOUT=(1<<10),	
		/// Input/output errors (file does not exist, access denied, etc:)
		ERR_IO=(1<<11),			
		/// The requested functionality is not (yet) implemented.
		ERR_NOTIMPLEMENTED=(1<<12),	
		/// Automatic creation/recalculation of cell list attributes is switched off
		ERR_NOAUTOMATION=(1<<13),	
		/// user has aborted the script run.
		ERR_USER_INTERVENTION=(1<<14),	
		/// The user is missing required licenses.
		ERR_LICENSE=(1<<15),		

		// usually for warnings: Mod::Warning(), Instance::Warning(), Error(ERR_WARNING|..., ...)

		/// The parameters for the module (or any other function) seem to be strange. 
		WARN_SUSPICIOUS_PARAMETERS=(1<<16),	
		/// The macro syntax seems strange.
		WARN_MACROSYNTAX=(1<<17),	
		/// Memory leak detected, cured, if possible.
		WARN_MEMORYLEAK=(1<<18),	
		/// Possible memory corruption
		WARN_MEMORYCORRUPTION=(1<<19),	
		/// Data conversion occures. This warning is not logged unless ConversionWarnings(1) has been called.
		WARN_CONVERSION=(1<<20),	
		/// Performance degradation
		WARN_PERFORMANCE=(1<<22),	
		/// The exception has been handled (debugged) by the user and does not need further debugging breaks.
		NOTICE_ERR_HANDLED=(1<<23),
		/// Security-related problems
		ERR_SECURITY=(1<<24),

// internal
		/// used for noticing the caller parent to make a clone of the dataitem and retry the operation. See MemBlock::SetSubItem().
		NOTICE_MAKE_CLONE_AND_RETRY=(1<<27),	
		/// pseudocode for avoiding throw in Error() function call.
		ERR_NOTHROW=(1<<28),

// reserved
		/// Reserved, do not use that code.
		ERR_RESERVED_FOR_CLASS_MODULES=(1<<29),	

		/// use together with ERR_USER_INTERVENTION to turn the exception non-catchable by the catch_abort() module.
		FLAG_IGNORE_CATCH_ABORT=(1<<30),  
	};


// Change 16.01.2005: remove dependence on fixed-size buffer.

/// The Acapella exception class. Almost all exceptions thrown be Acapella code are of this class.
class DI_baseutil Exception: public std::exception {
public: // static interface

	/// Obsolete, use SetLoggerCallbackBase()/SetLoggerFilter() instead.
	static void SetLogging(FILE* f, unsigned int reason_mask=0xffffffff);

	/// Obsolete, use SetLoggerCallbackBase()/SetLoggerFilter() instead.
	static void GetLogging(FILE*& f, unsigned int& reason_mask);

public: // interface
	/**
	* Construct an exception
	* @param reason Error code composed of ERR_* and WARN_* flags defined in imacroexception.h. 
	*               ERR_ERROR is added automatically.
	* @param errmsg The error message.
	* @param object If the exception is related to a certain C++ object, then pass the pointer to it here.
	*               This is used e.g. in module calls for identifying the exception with a certain module parameter.
	*/
	Exception(unsigned int reason, const safestring& errmsg, const void* object=NULL) throw();
	Exception(const Exception& b) throw();
	Exception& operator=(const Exception& b) throw();
	virtual ~Exception() throw();

	/// Return the error message buffer. This can be XML or ASCII, or ASCII followed by XML.
	const char* buffer() const {return p_;}

	/// Return the error message in ASCII
	virtual const char* what() const throw();

	/// Return the error formatted in XML. 
	safestring xml() const;

	/// Return the error message in ASCII
	safestring ascii() const;

	/// Return the error code passed to the ctor. ERR_ERROR bit is always raised.
	unsigned int Reason() const throw() {return reason_;}
	/**
	* Return the object pointer passed to the ctor. The returned pointer
	* cannot be dereferenced as the pointed object might be already 
	* destructed. It can only be compared with other pointers. Note that
	* even the comparing is formally undefined behaviour in this case,
	* but should work well at least in non-segmented address spaces.
	*/
	const void* GetObject() const throw() {return object_;}
#ifdef _CRTDBG_MAP_ALLOC
	void* operator new(size_t n, int blocktype, const char* filename, int lineno) {return mb_malloc_dbg(n, blocktype, filename, lineno);}	// enable exchange inbetween DLL-s.
#else
	void* operator new(size_t n) {return mb_malloc(n);}	// enable exchange inbetween DLL-s.
#endif
	void operator delete(void* p) {mb_free(p);}
private: // data 
	char* p_; // pointer to the error message
	unsigned int reason_; // error code (bitflag)
	const void* object_;
	static char s_buffer_[512]; // a static buffer to use if mb_malloc fails.
private: // implementation
	int& RefCount() const;
};



class DI_baseutil ConnectionResetByPeer: public Exception {
public:
	ConnectionResetByPeer(): Exception(ERR_IO, "recv() failed: connection reset by peer") {}
};


/**
* An helper class for more flexible error handling. 
* Instead of throwing an Exception, call Throw(Exception(...)).
* This may throw indeed or execute some other action via a throw handler.
* One can pass a custom handler as the second parameter. 
* Throw() returns the result returned by the handler.
*/
class ThrowHandlerBase: public mb_malloced {
public:
	/**
	* Used for reporting an error. Override in a derived class to do what's needed. 
	* If the operator returns normally then it should return e.Reason().
	*/
	virtual int operator()(const Exception& e)=0;
	virtual ~ThrowHandlerBase() {}
	/// Same as operator().
	int Throw(const Exception& e) {return operator()(e);}

	enum verb_t {
		vrb_create_missing_subelems_automatically=1,
		/// If ThrowHandler does not make use the passed Exception object, it should return 1 for this verb. This helps the caller to avoid unneeded Exception object construction.
		vrb_exception_object_not_used=2,
		/// When resolving subitems like .class, the object may not attempt automatic translation to .@class. Used be Defined() function.
		vrb_avoid_backward_compatibility=3,
		/// Used by the assert() module
		vrb_expr_stack_reconstruction=4,
		/// Used by the VirtualImage class to trigger the closure evaluation
		vrb_trigger_virtual_evaluation=5,
		/// Used to reset the throwhandler to the initial (non-failure) state.
		vrb_reset=6,
	};
	/// Generic backdoor for extensions.
	virtual int DoVerb(verb_t verb) {return 0;}
};

/// This enum identifies "stock throwhandler" objects. These values can be passed to functions expecting a ThrowHandlerFunc argument.
enum ThrowHandlerFunc_Constant {
	/// Creates a throwhandler which always actually throws the exception.
	ThrowIndeed,
	/// Creates a throwhandler which silently ignores all exceptions.
	ThrowNever,
	/// A synonym for ThrowNever, present for back-compatibility only.
	DoNothing=ThrowNever,
};

/**
* A lightweight wrapper class for passing throwhandler objects with minimal syntax, mostly for back-compatibility. 
* This is a reference class which should be passed by value. 
* The functions having a parameter for throwhandler should declare it as taking a ThrowHandlerFunc
* argument by value.
*/
class ThrowHandlerFunc {
	public:
		/// Construct from an object of a type derived from ThrowHandlerBase. The object is not copied and must be alive during the lifetime of ThrowHandlerFunc object.
		ThrowHandlerFunc(ThrowHandlerBase& ref): ref_(ref) {}
		/// Construct a stock throwhandler object by passing a predefined enum constant. 
		DI_baseutil ThrowHandlerFunc(ThrowHandlerFunc_Constant c);
		/// Forwards to ThrowHandlerBase::operator().
		int operator()(const Exception& e) {return ref_(e);}
		/// Same as operator(). Forwards to ThrowHandlerBase::Throw().
		int Throw(const Exception& e) {return ref_.Throw(e);}
		/// A boolean test. If the throw handler does something, it evaluates to true inside an if() clause. If the throw handler does nothing (i.e. is an instance of ThrowNever object), it evaluates to false inside an if() clause.
		operator const void*() const {return (&ref_ == &ThrowHandlerFunc(ThrowNever).ref_)? NULL: this;}
		/// Forwards to ThrowHandlerBase::DoVerb().
		int DoVerb(ThrowHandlerBase::verb_t verb) const {return ref_.DoVerb(verb);}
	private:
		ThrowHandlerBase& ref_;
};

/** A helper function for sending an Exception object to a ThrowHandler object.
* Instead of using this function it is suggested to call ThrowHandlerFunc::operator() directly.
* @param e Exception object to be thrown or processed.
* @param throwhandler The throwhandler object for processing the exception. 
*         This can be a predefined global handler like ThrowNever or ThrowIndeed,
*		  but also any other object derived from ThrowHandlerBase.
* @return If the handler decided not to throw, the return value of its operator() is returned.
*/
DI_baseutil int Throw(const Exception& e, ThrowHandlerFunc throwhandler);

DI_baseutil void ThrowAssertException(const char* filename, int line, const char* expression);

#ifdef DEFINE_BASEUTIL_ASSERT

#ifndef ASSERT
/// If ASSERT is not yet defined, then this macro defines an assert which reports failures via throwing an Exception. This is defined only if DEFINE_BASEUTIL_ASSERT is defined before including baseutil headers.
#define ASSERT(x) if (!(x)) Nbaseutil::ThrowAssertException(__FILE__, __LINE__, #x);
#endif

#ifdef _DEBUG
/// A macro for Debug-build assertions. Calls ASSERT macro in Debug builds; does nothing in Release builds. The tested condition should not have any side effects. This is defined only if DEFINE_BASEUTIL_ASSERT is defined before including baseutil headers.
#define DEBUG_ASSERT(cnd) ASSERT(cnd)
#else
/// A macro for Debug-build assertions. Calls ASSERT macro in Debug builds; does nothing in Release builds. The tested condition should not have any side effects. This is defined only if DEFINE_BASEUTIL_ASSERT is defined before including baseutil headers.
#define DEBUG_ASSERT(cnd)
#endif

#endif // DEFINE_BASEUTIL_ASSERT

#ifndef ACAPELLA_DISABLE_DEBUG_CAST
/**
* A cast for converting larger numeric types into smaller. Throws an exception if the number does not fit in the integral destination type.
* If ACAPELLA_DISABLE_DEBUG_CAST has been defined, then this will be equivalent to static_cast. This might be useful for 
* avoiding pulling the <boost/numeric/conversion/cast.hpp> header into the compilation.
*/
template<typename T, typename U> inline T checked_cast(U x) {
	// Boost code yields runtime failures for MS "smaller type check"
	// if U is smaller type than T, avoid this.
	if (double(std::numeric_limits<U>::min())>=double(std::numeric_limits<T>::min()) && double(std::numeric_limits<U>::max())<=double(std::numeric_limits<T>::max())) {
		return static_cast<T>(x);
	} else {
		return boost::numeric_cast<T,U>(x);
	}
}
#else
/**
* A cast for converting larger numeric types into smaller. Throws an exception if the number does not fit in the integral destination type.
* If ACAPELLA_DISABLE_DEBUG_CAST has been defined, then this will be equivalent to static_cast. This might be useful for 
* avoiding pulling the <boost/numeric/conversion/cast.hpp> header into the compilation.
*/
template<typename T, typename U> inline T checked_cast(U x) {
	return static_cast<T>(x);
}
#endif


/**
* A cast for converting larger numeric types into smaller. In debug build throws an exception if the number does not fit in the integral destination type.
* The template parameters may be built-in types, DWORD and size_t.
*/
template<typename T, typename U> inline T debug_cast(U x) {
#ifdef _DEBUG
	return checked_cast<T,U>(x);
#else
	return static_cast<T>(x);
#endif
}

} // namespace

#ifdef ACAPELLA_EVIL_MIN_WAS_DEFINED
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MIN_WAS_DEFINED
#endif
#ifdef ACAPELLA_EVIL_MAX_WAS_DEFINED
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#undef ACAPELLA_EVIL_MAX_WAS_DEFINED
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif
