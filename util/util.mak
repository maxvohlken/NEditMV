# Microsoft Developer Studio Generated NMAKE File, Based on util.dsp
!IF "$(CFG)" == ""
CFG=util - Win32 Debug
!MESSAGE No configuration specified. Defaulting to util - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "util - Win32 Release" && "$(CFG)" != "util - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "util.mak" CFG="util - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "util - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "util - Win32 Debug" (based on "Win32 (x86) Static Library")
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

!IF  "$(CFG)" == "util - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\util.lib"


CLEAN :
	-@erase "$(INTDIR)\DialogF.obj"
	-@erase "$(INTDIR)\fileUtils.obj"
	-@erase "$(INTDIR)\fontsel.obj"
	-@erase "$(INTDIR)\getfiles.obj"
	-@erase "$(INTDIR)\managedList.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\prefFile.obj"
	-@erase "$(INTDIR)\printUtils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(OUTDIR)\util.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /EHsc /O2 /D "NDEBUG" /D "_WINDOWS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" $(XMSTATICDEF) /Fp"$(INTDIR)\util.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\util.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\util.lib" 
LIB32_OBJS= \
	"$(INTDIR)\DialogF.obj" \
	"$(INTDIR)\fileUtils.obj" \
	"$(INTDIR)\fontsel.obj" \
	"$(INTDIR)\getfiles.obj" \
	"$(INTDIR)\managedList.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\prefFile.obj" \
	"$(INTDIR)\printUtils.obj"

"$(OUTDIR)\util.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "util - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\util.lib"


CLEAN :
	-@erase "$(INTDIR)\DialogF.obj"
	-@erase "$(INTDIR)\fileUtils.obj"
	-@erase "$(INTDIR)\fontsel.obj"
	-@erase "$(INTDIR)\getfiles.obj"
	-@erase "$(INTDIR)\managedList.obj"
	-@erase "$(INTDIR)\misc.obj"
	-@erase "$(INTDIR)\prefFile.obj"
	-@erase "$(INTDIR)\printUtils.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(OUTDIR)\util.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MD /W3 /EHsc /ZI /Od /I ".." /I "$(MOTIF)\include" /I "$(XDK)\include" /D "_DEBUG" /D "_WINDOWS" /D "WIN32" /D "_CRT_SECURE_NO_WARNINGS" $(XMSTATICDEF) /Fp"$(INTDIR)\util.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\util.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\util.lib" 
LIB32_OBJS= \
	"$(INTDIR)\DialogF.obj" \
	"$(INTDIR)\fileUtils.obj" \
	"$(INTDIR)\fontsel.obj" \
	"$(INTDIR)\getfiles.obj" \
	"$(INTDIR)\managedList.obj" \
	"$(INTDIR)\misc.obj" \
	"$(INTDIR)\prefFile.obj" \
	"$(INTDIR)\printUtils.obj"

"$(OUTDIR)\util.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
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
!IF EXISTS("util.dep")
!INCLUDE "util.dep"
!ELSE 
!MESSAGE Warning: cannot find "util.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "util - Win32 Release" || "$(CFG)" == "util - Win32 Debug"
SOURCE=.\DialogF.c

"$(INTDIR)\DialogF.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fileUtils.c

"$(INTDIR)\fileUtils.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fontsel.c

"$(INTDIR)\fontsel.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getfiles.c

"$(INTDIR)\getfiles.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\managedList.c

"$(INTDIR)\managedList.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\misc.c

"$(INTDIR)\misc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\prefFile.c

"$(INTDIR)\prefFile.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\printUtils.c

"$(INTDIR)\printUtils.obj" : $(SOURCE) "$(INTDIR)"



!ENDIF 

