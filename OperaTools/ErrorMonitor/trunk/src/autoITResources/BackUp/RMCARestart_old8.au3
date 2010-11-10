#Region ;**** Directives created by AutoIt3Wrapper_GUI ****
#AutoIt3Wrapper_icon=.\sys.ico
#AutoIt3Wrapper_outfile=..\..\Resources\RMCARestart.exe
#AutoIt3Wrapper_Compression=0
#AutoIt3Wrapper_UseX64=n
#AutoIt3Wrapper_Change2CUI=y
#AutoIt3Wrapper_Res_Description=This tool provides support for restarting the RMCAs.
#AutoIt3Wrapper_Res_Fileversion=0.1.0.16
#AutoIt3Wrapper_Res_Fileversion_AutoIncrement=y
#AutoIt3Wrapper_Res_Language=1033
#AutoIt3Wrapper_Run_Tidy=y
#EndRegion ;**** Directives created by AutoIt3Wrapper_GUI ****

#include <Constants.au3>
;When running as an administrator, the Secondary Logon (RunAs) service must be enabled or this function will fail. This does not apply when running as the SYSTEM account.

If $CmdLine[$CmdLine[0]] = '?' Or $CmdLine[$CmdLine[0]] = '-?' Or $CmdLine[$CmdLine[0]] = '/?' Or $CmdLine[$CmdLine[0]] = '--help' Then Usage()

If $CmdLine[0] < 1 Or $CmdLine[0] > 3 Then Usage()


If $CmdLine[0] = 3 Then $UserID = StringSplit($CmdLine[2], "\\")

If ($UserID[0] = 2) Then
	$runErrors = RunAs($UserID[2], $UserID[1], $CmdLine[3], 2, 'Shutdown /r /f /t 0 /d u:4:5 /c /m ' & $CmdLine[1], @SystemDir, @SW_HIDE, $STDERR_CHILD + $STDOUT_CHILD)
	ConsoleWrite('restarting')
Else
	ConsoleWrite('restarting')
	$runErrors = Run('Shutdown /r /f /t 0 /d u:4:5 /c /m ' & $CmdLine[1], @SystemDir, @SW_HIDE, $STDERR_CHILD + $STDOUT_CHILD)

EndIf

Local $line
While 1
	$line = StderrRead($runErrors)
	If @error Then ExitLoop
	ConsoleWriteError($line)
WEnd

Exit (0)

Func Usage()
	ConsoleWrite(@ScriptName & ' allows to restart remote servers' & @CRLF & @CRLF & 'Usage: RMCARestart ServerName [Optional:UserID PWD]' & @CRLF & 'Note: UserID should containd domain or machine name ex. devs\UserID')
	Exit (1)
EndFunc   ;==>Usage