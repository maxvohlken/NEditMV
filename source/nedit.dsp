# Microsoft Developer Studio Project File - Name="nedit" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=nedit - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "nedit.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nedit.mak" CFG="nedit - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nedit - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "nedit - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName "nedit"
# PROP Scc_LocalPath ".."
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "nedit - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /EHsc /O2 /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" /D "NDEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /W3 /EHsc /O2 /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" /D "XMSTATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "nedit - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /EHsc /Zi /Od /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" /D "_DEBUG" /D "_WINDOWS" /YX /FD /c
# ADD CPP /nologo /MD /W3 /EHsc /ZI /Od /I ".." /I "..\..\exceed\xdk\include" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" /D "XMSTATIC" /YX /FD /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 xmstatic.lib XmStatXt.lib HCLXmu.lib Xlib.lib Xlibgui.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib /nologo /subsystem:windows /debug /machine:I386 /nodefaultlib:"libc.lib" /pdbtype:sept /libpath:"..\..\exceed\xdk\lib"
# SUBTRACT LINK32 /pdb:none

!ENDIF 

# Begin Target

# Name "nedit - Win32 Release"
# Name "nedit - Win32 Debug"
# Begin Source File

SOURCE=.\clearcase.c
# End Source File
# Begin Source File

SOURCE=.\file.c
# End Source File
# Begin Source File

SOURCE=.\help.c
# End Source File
# Begin Source File

SOURCE=.\highlight.c
# End Source File
# Begin Source File

SOURCE=.\highlightData.c
# End Source File
# Begin Source File

SOURCE=.\interpret.c
# End Source File
# Begin Source File

SOURCE=.\macro.c
# End Source File
# Begin Source File

SOURCE=.\menu.c
# End Source File
# Begin Source File

SOURCE=.\nedit.c
# End Source File
# Begin Source File

SOURCE=.\nedit.mak
# End Source File
# Begin Source File

SOURCE=.\parse.c
# End Source File
# Begin Source File

SOURCE=.\preferences.c
# End Source File
# Begin Source File

SOURCE=.\regularExp.c
# End Source File
# Begin Source File

SOURCE=.\search.c
# End Source File
# Begin Source File

SOURCE=.\selection.c
# End Source File
# Begin Source File

SOURCE=.\server.c
# End Source File
# Begin Source File

SOURCE=.\server_common.c
# End Source File
# Begin Source File

SOURCE=.\shell.c
# End Source File
# Begin Source File

SOURCE=.\shift.c
# End Source File
# Begin Source File

SOURCE=.\smartIndent.c
# End Source File
# Begin Source File

SOURCE=.\tags.c
# End Source File
# Begin Source File

SOURCE=.\text.c
# End Source File
# Begin Source File

SOURCE=.\textBuf.c
# End Source File
# Begin Source File

SOURCE=.\textDisp.c
# End Source File
# Begin Source File

SOURCE=.\textDrag.c
# End Source File
# Begin Source File

SOURCE=.\textSel.c
# End Source File
# Begin Source File

SOURCE=.\undo.c
# End Source File
# Begin Source File

SOURCE=.\userCmds.c
# End Source File
# Begin Source File

SOURCE=.\window.c
# End Source File
# Begin Source File

SOURCE=..\util\Debug\util.lib
# End Source File
# End Target
# End Project
