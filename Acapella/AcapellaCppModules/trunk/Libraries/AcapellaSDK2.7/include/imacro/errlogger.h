#ifndef x_IMACRO_ERROLOGGER_H_INCLUDED_
#define x_IMACRO_ERROLOGGER_H_INCLUDED_

namespace NIMacro {

	struct ExecutionContext;

	/**
	* ErrLogger is a base class for the Instance class, providing
	* remembering and augmenting errors and warnings occuring through the script run,
	* later reproduction and clearing.
	*
	* Despite its name, it is not much concerned with any log file.
	* See the Logger class in MemBlock library for that.
	*/
	class ErrLogger {
	public:
		/*
		* Report an error to the ErrLogger. Calls virtual DoError() method.
		* In case of Instance class this should be called only for top-level procedures a la runmacro_impl().
		* Low-level functions should throw Exceptions instead.
		*
		* @param Reason Error code, composed of ERR* flags defined in <MemBlock/modreg.h>. 
		*				ERR_ERROR is not necessary. Specifying ERR_WARNING makes this warning instead.
		* @param Message Error message, in a hopefully useful form for the end user.
		*				Use Printf() functions to format the Message.
		*				The derived class can apply additional augmenting to the message.
		*/
		DI_IMacro void Error(unsigned int Reason, const Nbaseutil::safestring& Message);	

		/* 
		* Report a warning to the ErrLogger. Calls virtual DoWarning() method.
		* Note that warnings cannot be propagated via Exceptions, so this is the 
		* preferred function for announicng them. There is also NIMacro::Warning() function,
		* but this is not 100% reliable.
		* @param Reason Warning code, composed of ERR* or WARN* flags defined in <MemBlock/modreg.h>. 
		*				ERR_WARNING is not necessary, including ERR_ERROR makes this an error instead of warning.
		* @param Message Warning message, in a hopefully useful form for the end user.
		*				Use Printf() functions to format the Message.
		*				The derived class can apply additional augmenting to the message.
		*/
		DI_IMacro void Warning(ExecutionContext& ctx, unsigned int Reason, const Nbaseutil::safestring& Message);
		
		/// Return true if any errors have been logged after last reset. Warnings are not taken into account.
		DI_IMacro bool HasErrors() const {return errcount_>0;}		

		/// Return true if any warnings have been logged after last reset. Errors are not taken into account.
		DI_IMacro bool HasWarnings() const {return warncount_>0;}

		/// Return true if any errors or warnings have been logged after last reset.
		DI_IMacro bool HasErrorsOrWarnings() const {return HasErrors() || HasWarnings();}

		/**
		* Return composite error/warning code composed of logged errors and warnings 
		* after the last reset. The return value consists of ERR* and WARN* flags defined in <memblock/modreg.h>.
		* If there are no errors nor warnings, returns 0.
		* If bit ERR_ERROR is raised, then there were errors.
		* If bit ERR_WARNING is raised, then there were warnings.
		*
		* @param Messages If there are any logged errors or warnings, then the texts of those
		*				are appended to the Messages string.
		* @param clear_errors If true, then the ErrLogger object is reset (all logged errors/warnings forgotten).
		*				In this case the caller has the responsibility to transfer the error messages 
		*				to the user, if this is appropriate.
		*/
		DI_IMacro unsigned int GetErrorsAndWarnings(safestring& Messages, bool clear_errors=true); 

		/*
		* Deprecated, one should use GetErrorsAndWarnings() plus the Logger facility instead.
		*
		* Save errors/warnings to a text file; returns error/warning code as GetErrorsAndWarnings()..
		*
		* @param filename Destination (file)name suitable for passing to OutputStream ctor. This means
		*				one can also log into pipelines and IMacro string data items in the current datablock (if there is one).
		* @param clear_errors If true, then the ErrLogger object is reset (all logged errors/warnings forgotten).
		*/
		DI_IMacro unsigned int LogErrors(const char* filename, bool clear_errors=true);

		/*
		* Reset the object state: clear currently logged error/warning flags and messages.
		* Normally GetErrorsAndWarnings() should be used to reset the object state.
		* If this is not needed, then one has to call ClearErrors() before object destruction
		* in order to avoid warning in Instance::~Instance() about ignored errors/warnings.
		*/
		DI_IMacro void ClearErrors();

		/// Base-class dtor does nothing.
		virtual ~ErrLogger() {}

	protected: // virtual methods
		/**
		* Called from Error(). Derived class can override this method to augment
		* and preprocess the error. Derived class method should finally call the 
		* base-class method for actual logging of the error. Derived-class method should not
		* drop bits from the Reason flags.
		*
		* @param Reason Error code, at least one of ERR_ERROR or ERR_WARNING are set. 
		*				If ERR_ERROR is not set, then this event should be considered a warning.
		* @param Message Error message as passed into Error().
		*/
		DI_IMacro virtual void DoError(unsigned int Reason, const safestring& Message);	

		/**
		* Called from Warning(). Derived class can override this method to augment
		* and preprocess the warning. Derived class method should finally call the 
		* base-class method for actual logging of the warning. Derived-class method should not
		* drop bits from the Reason flags.
		*
		* @param ctx The current execution context.
		* @param reason Warning code, at least one of ERR_ERROR or ERR_WARNING are set. 
		*				If ERR_ERROR is set, then this event should be considered an error.
		* @param message Warning message as passed into Warning().
		*/
		DI_IMacro virtual void DoWarning(ExecutionContext& ctx, unsigned int reason, const safestring& message);

		/**
		* Called from ClearErrors(). Derived class can override this method.
		* Derived class method should finally call the base-class method for actual clearing.
		*/
		DI_IMacro virtual void DoClearErrors();

	protected: // data
		unsigned int errwarn_, errcount_, warncount_;
		safestring errwarnmsg_;
	};

} //namespace
#endif
