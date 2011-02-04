#Region ;**** Directives created by AutoIt3Wrapper_GUI ****
#AutoIt3Wrapper_Icon=.\shutdown.ico
#AutoIt3Wrapper_Outfile=..\..\Resources\OperaShutdown.exe
#AutoIt3Wrapper_Compression=0
#AutoIt3Wrapper_UseX64=n
#AutoIt3Wrapper_Res_Description=This tool allows shutdown the LBC control and the evoshell.
#AutoIt3Wrapper_Res_Fileversion=0.1.0.67
#AutoIt3Wrapper_Res_FileVersion_AutoIncrement=y
#AutoIt3Wrapper_Res_Language=1033
#AutoIt3Wrapper_Run_Tidy=y
#EndRegion ;**** Directives created by AutoIt3Wrapper_GUI ****



$shutdown = MsgBox(4132, "OperaShutdown", "The Opera is about to shutdown do you wish to abort?", 30)
if ($shutdown = 6) Then Exit (0)

If ProcessExists('LBCControl.exe') Then
	WinClose("LBC-Control")
	Sleep(1000)
	If Not WinExists("LBCControl", "Do you really want to close the program") Then
		ProcessClose('LBCControl.exe')
		Run('C:\PerkinElmerCTG\Opera\LBCControl.exe')
		WinWait("LBC-Control")
		WinActivate("LBCControl", "Do you really want to close the program")
		Send("{SPACE}")
		WinClose("LBC-Control")
		WinWait("LBCControl", "Do you really want to close the program")
	EndIf
	WinActivate("LBCControl", "Do you really want to close the program")
	Send("!y")
EndIf

$loop = 0
While $loop <= 10
	If Not WinExists("EvoShell") Then ExitLoop
	Sleep(1000)
	$loop += 1
WEnd

If ProcessExists('EvoShell.exe') Then ProcessClose('EvoShell.exe')

Shutdown(4, "Opera Screen Terminated") ;Force log off