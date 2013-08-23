#ifndef x_ACAPELLA_IMACRO_DEBUGGER_H_INCLUDED_
#define x_ACAPELLA_IMACRO_DEBUGGER_H_INCLUDED_

#include "executive.h"

namespace NIMacro {

class DebuggerImpl;
class DataBlockCallback;

/**
* @brief Script debugging support class. Create an instance of this class for each debugging sesson.
* Normally the Debugger object is coupled with the Executive object under whose control the script is run.
* The Executive object should call the Debugger methods in several occasions. Mostly the Executive
* calls just forward to the Debugger method all parameters and return back the result returned by the Debugger method.
*
* There is a DebuggableExecutive class which does all these tasks automatically.
*/
class DI_IMacro Debugger: public mb_malloced {
	DebuggerImpl& impl_;
public: // construction/destruction
	/**
	* @brief Construct a Debugger object.
	* @param disable_optimizations Switch off the Acapella optimization feature that items having the names of module 
	*        output parameters are deleted from the datablock already when entering the module.
	*        If set to true, then one can retry the module call in case of exceptions.
	* @param break_on_exception Breaks into the debugger in case of errors raised in the script.
	* @param break_immediately Breaks before the first modulecall in the script.
	* @param break_in_end Breaks after the execution of the script.
	*/
	Debugger(bool disable_optimizations, bool break_on_exception, bool break_immediately, bool break_in_end);
	
	/// Copy ctor, performs a deep copy of DebuggerImpl member.
	Debugger(const Debugger& b);

	/// Virtual dtor.
	virtual ~Debugger();

public:  // forwarded functions

	/**
	* Handles several "dbg_" commands sent by Acapella. Executive::DoVerb() should forward to this method each time
	* the verb begins with "dbg_" prefix.
	*/
	virtual DataItem Command(ExecutionContext& ctx, const safestring& cmd, const SafeValue& param1=SafeValue(), const SafeValue& param2=SafeValue(), const SafeValue& param3=SafeValue(), const SafeValue& param4=SafeValue());

	/// Should be forwarded from Executive::NotifyDebugBeforeModuleCall().
	bool NotifyDebugBeforeModuleCall(ExecutionContext& ctx, RunFrame& runframe);

	/// Should be forwarded from Executive::NotifyDebugAfterModuleCall().
	void NotifyDebugAfterModuleCall(ExecutionContext& ctx, RunFrame& runframe);

	/// Should be forwarded from Executive::NotifyModuleCallBegins().
	void NotifyModuleCallBegins(ExecutionContext& ctx, const ModuleCall* call, int no);

	/// Should be forwarded from Executive::NotifyScriptStarted().
	virtual void NotifyScriptStarted(ExecutionContext& ctx);

	/// Should be forwarded from Executive::NotifyScriptFinishedNormally().
	virtual void NotifyScriptFinishedNormally(ExecutionContext& ctx);

	/// Should be forwarded from Executive::NotifyScriptAborted().
	virtual void NotifyScriptAborted(ExecutionContext& ctx, int mode);

public: // enums


	/// Actions which can be taken in debugging situations.
	enum action_t {
		act_break,
		act_ignore,
		act_continue,
		act_tryagain,
		act_exit,
		act_none,
	};

	/// Exact reason why the BreakPoint() method is called.
	enum reason_t {
		reas_none,
		reas_stepping,
		reas_line_bp,
		reas_mod_bp,
		reas_dataw_bp,
		reas_datar_bp,
		reas_exception,
		reas_handled_exception,
		reas_exiting_bp,
		reas_exiting_script,
	};

	enum msg_t {
		msg_error,
		msg_warning,
		msg_watch,
		msg_loc,
		msg_code,
		msg_trace,
		msg_curline,
		msg_bpdef,
		msg_break_on_bp,
		msg_break_on_error,
	};

public: // virtual interface

	/**
	* @brief This method is called for stopping the script execution on breakpoints or when an exception occurs.
	* The breakpoint may be called before or after or the module call. The module call may have run correctly or
	* exited by an exception. The reason parameter determines the call state.
	* 
	* The base class implementation executes an Interactive() module call. This calls back to Executive::DoVerb("interactive"),
	* which the client can override.
	*
	* @param ctx The current script execution context. If the output parameter optimisation is in use, then the
	*      items with module output parameter names may be missing in the datablock. The module call input parameters
	*      and last module call output parameters are present in the containers __lastoutput and __nextinput.
	* @param reason Exact reason why the BreakPoint() method is called.
	* @return The user-chosen action: act_exit, act_continue, act_ignore or act_again. The last two have special meaning only
	*      if reson is reas_exception or reas_handled_exception, otherwise they are the same as act_continue. 
	*      The act_exit return terminates the whole script immediately.
	*/
	virtual action_t BreakPoint(ExecutionContext& ctx, reason_t reason);

	/// This method is called for outputting several debugging-related messages.
	virtual void Message(ExecutionContext& ctx, msg_t reason, const safestring& msg) const {
		ctx.exec.ConsoleOut(ctx, msg.c_str());
	}

	/// Make a copy of this object, to be passed over thread borders.
	virtual Debugger* Clone() const=0; 

public: // interface

	/// Return current script location: procedure name and filename (with or without path).
	safestring GetLoc(ExecutionContext& ctx, bool longfilename=false) const;

	/// Add line breakpoint. Return breakpoint number. Pathname part of the filename is ignored. If temporary, the Message() method is not called.
	safestring AddBpLine(const safestring& filename, int lineno, bool temporary=false);

	/// Add module breakpoint. Return breakpoint number. 
	safestring AddModBp(const safestring& name);

	/// Add data write/change breakpoint. Return breakpoint number. 
	safestring AddDataChangedBp(const safestring& name);

	/// Add data read/use breakpoint. Return breakpoint number.
	safestring AddDataUsedBp(const safestring& name);

	/// To be called from Executive::GetDataBlockCallback(). Returns a new DataBlockCallback or existing_callback.
	DataBlockCallback* GetDataBlockCallback(ExecutionContext& ctx, DataBlockCallback* existing_callback);

	/// Delete specified breakpoint. Argument -1 deletes all breakpoints.
	safestring DeleteBp(int no=-1);

	/// Make a list of all existing breakpoints.
	safestring ListBp() const;

	/// List script code (this is reconstructed from module calls.
	void ListCode(ExecutionContext& ctx) const;

	/// List script backtrace.
	void ListBackTrace(ExecutionContext& ctx) const;

	/// Add a watch command. This can be debugger command or Acapella script. It is executed at each breakpoint.
	safestring AddWatch(ExecutionContext& ctx, const safestring& expr);

	/// Delete a watch command. no==-1: delete all watches.
	safestring DeleteWatch(int no);

	/**
	* @brief Executes all watches. Returns console output produced by the execution. 
	*
	* If a watch command fails by any reason, then the console output of the full watch command is not
	* included in the result.
	* @param ctx The current exectuion context.
	* @param verbose Display watch number and watch command before execution of each watch. 
	*                Also display the error message if the watch failed.
	*/
	safestring DisplayWatch(ExecutionContext& ctx, bool verbose) const;

	/**
	* @brief Return current action policy in case of exceptions.
	* @param handled Specifies which kind of exceptions to consider: handled or unhandled. 
	*     Handled exceptions are handled by catch_error() modules in the script.
	*/
	action_t GetExcPolicy(bool handled) const;

	/// Set action policy in case of exceptions.
	/**
	* @param handled Specifies which kind of exceptions to consider: handled or unhandled. 
	*     Handled exceptions are those for which there are relevant catch_error() or eval() modules in the script.
	* @param action Specifies the action to carry out in the case of the exception.
	*/
	void SetExcPolicy(bool handled, action_t action);

	/// Convert string to action_t or throw.
	static action_t ActionEnum(const safestring& s);

	/// Convert action_t value to a string.
	static safestring ActionStr(action_t x);

private:
	/// Assignment op is not implemented, use the Clone() member function instead.
	void operator=(const Debugger&);
};

/**
* @brief In console program, goes into interactive mode.
*
* This function is called by the Interactive() module if the executive doesn't override 
* the "interactive" verb.
* Accepts commands from the user and executes them. This mode works tightly together with the 
* Executive object and the Debugger, if Executive forwards to the Debugger. 
* Each unknown command is passed to Executive::DoVerb("dbg_interactive_cmd").
* @param ctx Current execution context. The databblock ctx.db can be modified by the function.
* @param prompt Prompt to display in the beginning of the line. 
* @param quiet For frequent calling: do not display 1-line help message in the start.
*/
void DI_IMacro InteractiveConsole(ExecutionContext& ctx, const safestring& prompt, bool quiet);

} // namespace

#endif
