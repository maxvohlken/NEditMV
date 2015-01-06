# Microsoft Developer Studio Generated NMAKE File, Based on nedit.dsp
!IF "$(CFG)" == ""
CFG=nedit - Win32 Debug
!MESSAGE No configuration specified. Defaulting to nedit - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "nedit - Win32 Release" && "$(CFG)" != "nedit - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

EXCEED=..\..\exceed
XDK=$(EXCEED)\XDK
MOTIF=$(XDK)\Motif12

!IF "$(XMLINK)" == "Dynamic"
XMSTATICDEF=
XMLIBS=HCLXm.lib HCLXt.lib
!ELSE
XMSTATICDEF=/D "XMSTATIC"
XMLIBS=XmStatic.lib XmStatXt.lib
!ENDIF

!IF  "$(CFG)" == "nedit - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\nedit.exe"


CLEAN :
	-@erase "$(INTDIR)\clearcase.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\highlight.obj"
	-@erase "$(INTDIR)\highlightData.obj"
	-@erase "$(INTDIR)\interpret.obj"
	-@erase "$(INTDIR)\macro.obj"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\nedit.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\preferences.obj"
	-@erase "$(INTDIR)\regularExp.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\selection.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server_common.obj"
	-@erase "$(INTDIR)\shell.obj"
	-@erase "$(INTDIR)\shift.obj"
	-@erase "$(INTDIR)\smartIndent.obj"
	-@erase "$(INTDIR)\tags.obj"
	-@erase "$(INTDIR)\text.obj"
	-@erase "$(INTDIR)\textBuf.obj"
	-@erase "$(INTDIR)\textDisp.obj"
	-@erase "$(INTDIR)\textDrag.obj"
	-@erase "$(INTDIR)\textSel.obj"
	-@erase "$(INTDIR)\undo.obj"
	-@erase "$(INTDIR)\userCmds.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\nedit.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /D "NDEBUG" /D "_WINDOWS" /D "WIN32" $(XMSTATICDEF) /Fp"$(INTDIR)\nedit.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "NDEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nedit.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/libpath:$(MOTIF)\lib /libpath:"$(XDK)\lib" $(XMLIBS) HCLXmu.lib Xlib.lib xlibgui.lib xlibcon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /incremental:no /pdb:"$(OUTDIR)\nedit.pdb" /machine:I386 /out:"$(OUTDIR)\nedit.exe" 
LINK32_OBJS= \
	"$(INTDIR)\clearcase.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\highlight.obj" \
	"$(INTDIR)\highlightData.obj" \
	"$(INTDIR)\interpret.obj" \
	"$(INTDIR)\macro.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\nedit.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\preferences.obj" \
	"$(INTDIR)\regularExp.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\selection.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\server_common.obj" \
	"$(INTDIR)\shell.obj" \
	"$(INTDIR)\shift.obj" \
	"$(INTDIR)\smartIndent.obj" \
	"$(INTDIR)\tags.obj" \
	"$(INTDIR)\text.obj" \
	"$(INTDIR)\textBuf.obj" \
	"$(INTDIR)\textDisp.obj" \
	"$(INTDIR)\textDrag.obj" \
	"$(INTDIR)\textSel.obj" \
	"$(INTDIR)\undo.obj" \
	"$(INTDIR)\userCmds.obj" \
	"$(INTDIR)\window.obj" \
	"..\util\Debug\util.lib"

"$(OUTDIR)\nedit.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "nedit - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\nedit.exe"


CLEAN :
	-@erase "$(INTDIR)\clearcase.obj"
	-@erase "$(INTDIR)\file.obj"
	-@erase "$(INTDIR)\help.obj"
	-@erase "$(INTDIR)\highlight.obj"
	-@erase "$(INTDIR)\highlightData.obj"
	-@erase "$(INTDIR)\interpret.obj"
	-@erase "$(INTDIR)\macro.obj"
	-@erase "$(INTDIR)\menu.obj"
	-@erase "$(INTDIR)\nedit.obj"
	-@erase "$(INTDIR)\parse.obj"
	-@erase "$(INTDIR)\preferences.obj"
	-@erase "$(INTDIR)\regularExp.obj"
	-@erase "$(INTDIR)\search.obj"
	-@erase "$(INTDIR)\selection.obj"
	-@erase "$(INTDIR)\server.obj"
	-@erase "$(INTDIR)\server_common.obj"
	-@erase "$(INTDIR)\shell.obj"
	-@erase "$(INTDIR)\shift.obj"
	-@erase "$(INTDIR)\smartIndent.obj"
	-@erase "$(INTDIR)\tags.obj"
	-@erase "$(INTDIR)\text.obj"
	-@erase "$(INTDIR)\textBuf.obj"
	-@erase "$(INTDIR)\textDisp.obj"
	-@erase "$(INTDIR)\textDrag.obj"
	-@erase "$(INTDIR)\textSel.obj"
	-@erase "$(INTDIR)\undo.obj"
	-@erase "$(INTDIR)\userCmds.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\window.obj"
	-@erase "$(OUTDIR)\nedit.exe"
	-@erase "$(OUTDIR)\nedit.ilk"
	-@erase "$(OUTDIR)\nedit.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /GX /ZI /Od /I ".." /I "$(MOTIF)\include" /I "$(XDK)\include" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" $(XMSTATICDEF) /Fp"$(INTDIR)\nedit.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
MTL_PROJ=/nologo /D "_DEBUG" /mktyplib203 /o "NUL" /win32 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nedit.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/libpath:$(MOTIF)\lib /libpath:"$(XDK)\lib" $(XMLIBS) HCLXmu.lib Xlib.lib xlibgui.lib xlibcon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\nedit.pdb" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"$(OUTDIR)\nedit.exe" /pdbtype:sept  
LINK32_OBJS= \
	"$(INTDIR)\clearcase.obj" \
	"$(INTDIR)\file.obj" \
	"$(INTDIR)\help.obj" \
	"$(INTDIR)\highlight.obj" \
	"$(INTDIR)\highlightData.obj" \
	"$(INTDIR)\interpret.obj" \
	"$(INTDIR)\macro.obj" \
	"$(INTDIR)\menu.obj" \
	"$(INTDIR)\nedit.obj" \
	"$(INTDIR)\parse.obj" \
	"$(INTDIR)\preferences.obj" \
	"$(INTDIR)\regularExp.obj" \
	"$(INTDIR)\search.obj" \
	"$(INTDIR)\selection.obj" \
	"$(INTDIR)\server.obj" \
	"$(INTDIR)\server_common.obj" \
	"$(INTDIR)\shell.obj" \
	"$(INTDIR)\shift.obj" \
	"$(INTDIR)\smartIndent.obj" \
	"$(INTDIR)\tags.obj" \
	"$(INTDIR)\text.obj" \
	"$(INTDIR)\textBuf.obj" \
	"$(INTDIR)\textDisp.obj" \
	"$(INTDIR)\textDrag.obj" \
	"$(INTDIR)\textSel.obj" \
	"$(INTDIR)\undo.obj" \
	"$(INTDIR)\userCmds.obj" \
	"$(INTDIR)\window.obj" \
	"..\util\Debug\util.lib"

"$(OUTDIR)\nedit.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

MVversion.h: ..\win32\MVversion.h
	copy ..\win32\MVversion.h .

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("nedit.dep")
!INCLUDE "nedit.dep"
!ELSE 
!MESSAGE Warning: cannot find "nedit.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "nedit - Win32 Release" || "$(CFG)" == "nedit - Win32 Debug"
SOURCE=.\clearcase.c

"$(INTDIR)\clearcase.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\file.c

"$(INTDIR)\file.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\help.c

"$(INTDIR)\help.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\highlight.c

"$(INTDIR)\highlight.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\highlightData.c

"$(INTDIR)\highlightData.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\interpret.c

"$(INTDIR)\interpret.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\macro.c

"$(INTDIR)\macro.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\menu.c

"$(INTDIR)\menu.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\nedit.c

"$(INTDIR)\nedit.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\parse.c

"$(INTDIR)\parse.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\preferences.c

"$(INTDIR)\preferences.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\regularExp.c

"$(INTDIR)\regularExp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\search.c

"$(INTDIR)\search.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\selection.c

"$(INTDIR)\selection.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\server.c

"$(INTDIR)\server.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\server_common.c

"$(INTDIR)\server_common.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\shell.c

"$(INTDIR)\shell.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\shift.c

"$(INTDIR)\shift.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\smartIndent.c

"$(INTDIR)\smartIndent.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tags.c

"$(INTDIR)\tags.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\text.c

"$(INTDIR)\text.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\textBuf.c

"$(INTDIR)\textBuf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\textDisp.c

"$(INTDIR)\textDisp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\textDrag.c

"$(INTDIR)\textDrag.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\textSel.c

"$(INTDIR)\textSel.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\undo.c

"$(INTDIR)\undo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\userCmds.c

"$(INTDIR)\userCmds.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\window.c

"$(INTDIR)\window.obj" : $(SOURCE) "$(INTDIR)"

MVversion.h: ..\win32\MVversion.h
	copy ..\win32\MVversion.h .


!ENDIF 

