
#ifndef x_ACAPELLA_MEMBLOCK_ERROR_H_INCLUDED_
#define x_ACAPELLA_MEMBLOCK_ERROR_H_INCLUDED_

#ifdef _MSC_VER
struct _EXCEPTION_POINTERS;
#endif

namespace NIMacro {


/// Produce a warning. If called from inside a module call, the warning is delivered to that module, otherwise IMacroWarning() is called, which delivers warning to the SendAlarm mechanism.
DI_MemBlock void Warning(unsigned int warningcode, const Nbaseutil::safestring& message);
	
/**
* Send an alarm message to the caller environment (eg. EvoShell). Returns true if message was succesfully delivered.
* @param pszMessage Error or warning message.
* @param uiLevel Alarm level in range 1 (warning) - 5 (serious trouble).
* @param iErrNr Error code if available - see CShellDoc::SendAlarm().
* @param pszHelpMsg Additional info to the user, if applicable.
*/
DI_MemBlock bool SendAlarmMemBlock(
		const Nbaseutil::safestring& pszMessage, 
		unsigned int uiLevel,
		int			iErrNr=0,
		const Nbaseutil::safestring& pszHelpMsg="");

#ifdef ACAPELLA_PLATFORM_WINDOWS
/**
* A stuctured exception handler function which turns Windows structured exceptions into Acapella C++ exceptions and also adds stack backtraces,
* if possible. Stack backtraces currently work only for 32-bit Windows and if the system can find dbghelp.dll and proper .pdb files.
*/
DI_MemBlock void Acapella_SEH_trans_func( unsigned int, _EXCEPTION_POINTERS* pExp );
#endif

} // namespace

#endif




