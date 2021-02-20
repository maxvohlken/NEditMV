# Microsoft Developer Studio Generated NMAKE File, Based on nc.dsp
!IF "$(CFG)" == ""
CFG=nc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to nc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "nc - Win32 Release" && "$(CFG)" != "nc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "nc.mak" CFG="nc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "nc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "nc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
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

!IF  "$(CFG)" == "nc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\nc.exe"


CLEAN :
	-@erase "$(INTDIR)\clearcase.obj"
	-@erase "$(INTDIR)\nc.obj"
	-@erase "$(INTDIR)\server_common.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\nc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /EHsc /O2 /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" $(XMSTATICDEF) /Fp"$(INTDIR)\nc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=/libpath:$(MOTIF)\lib /libpath:"$(XDK)\lib" $(XMLIBS) HCLXmu.lib Xlib.lib xlibgui.lib xlibcon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\nc.pdb" /machine:I386 /out:"$(OUTDIR)\nc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\clearcase.obj" \
	"$(INTDIR)\nc.obj" \
	"$(INTDIR)\server_common.obj" \
	"..\util\Debug\util.lib"

"$(OUTDIR)\nc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "nc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\nc.exe" "$(OUTDIR)\nc.bsc"


CLEAN :
	-@erase "$(INTDIR)\clearcase.obj"
	-@erase "$(INTDIR)\clearcase.sbr"
	-@erase "$(INTDIR)\nc.obj"
	-@erase "$(INTDIR)\nc.sbr"
	-@erase "$(INTDIR)\server_common.obj"
	-@erase "$(INTDIR)\server_common.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\nc.bsc"
	-@erase "$(OUTDIR)\nc.exe"
	-@erase "$(OUTDIR)\nc.ilk"
	-@erase "$(OUTDIR)\nc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /Gm /EHsc /ZI /Od /I ".." /I "$(MOTIF)\include" /I "$(XDK)\include" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" $(XMSTATICDEF) /FR"$(INTDIR)\\" /Fp"$(INTDIR)\nc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\nc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\clearcase.sbr" \
	"$(INTDIR)\nc.sbr" \
	"$(INTDIR)\server_common.sbr"

"$(OUTDIR)\nc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=/libpath:$(MOTIF)\lib /libpath:"$(XDK)\lib" $(XMLIBS) HCLXmu.lib Xlib.lib xlibgui.lib xlibcon.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib uuid.lib /nologo /subsystem:windows /incremental:yes /pdb:"$(OUTDIR)\nc.pdb" /debug /machine:I386 /nodefaultlib:"libc.lib" /out:"$(OUTDIR)\nc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\clearcase.obj" \
	"$(INTDIR)\nc.obj" \
	"$(INTDIR)\server_common.obj" \
	"..\util\Debug\util.lib"

"$(OUTDIR)\nc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("nc.dep")
!INCLUDE "nc.dep"
!ELSE 
!MESSAGE Warning: cannot find "nc.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "nc - Win32 Release" || "$(CFG)" == "nc - Win32 Debug"
SOURCE=.\clearcase.c

!IF  "$(CFG)" == "nc - Win32 Release"


"$(INTDIR)\clearcase.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nc - Win32 Debug"


"$(INTDIR)\clearcase.obj"	"$(INTDIR)\clearcase.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\nc.c

!IF  "$(CFG)" == "nc - Win32 Release"


"$(INTDIR)\nc.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nc - Win32 Debug"


"$(INTDIR)\nc.obj"	"$(INTDIR)\nc.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\server_common.c

!IF  "$(CFG)" == "nc - Win32 Release"


"$(INTDIR)\server_common.obj" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "nc - Win32 Debug"


"$(INTDIR)\server_common.obj"	"$(INTDIR)\server_common.sbr" : $(SOURCE) "$(INTDIR)"


!ENDIF 


!ENDIF 

