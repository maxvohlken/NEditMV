/*******************************************************************************
*									       *
* preferences.c -- Nirvana Editor preferences processing		       *
*									       *
* Copyright (c) 1991 Universities Research Association, Inc.		       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retains a paid-up,      *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warrenty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* April 20, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <config.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include <errno.h>
#include <string.h>
#include <Xm/Xm.h>
#include <Xm/SelectioB.h>
#include <Xm/Form.h>
#include <Xm/List.h>
#include <Xm/SeparatoG.h>
#include <Xm/LabelG.h>
#include <Xm/PushBG.h>
#include <Xm/PushB.h>
#include <Xm/ToggleBG.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/CascadeBG.h>
#include <Xm/Frame.h>
#include <Xm/Text.h>
#include "../util/prefFile.h"
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/managedList.h"
#include "../util/fontsel.h"
#include "textBuf.h"
#include "nedit.h"
#include "text.h"
#include "search.h"
#include "preferences.h"
#include "window.h"
#include "userCmds.h"
#include "highlight.h"
#include "highlightData.h"
#include "help.h"
#include "regularExp.h"
#include "smartIndent.h"
#include "server_common.h"
#include "server.h"

#define PREF_FILE_NAME ".nedit"

/* maximum number of word delimiters allowed (256 allows whole character set) */
#define MAX_WORD_DELIMITERS 256

/* maximum number of file extensions allowed in a language mode */
#define MAX_FILE_EXTENSIONS 20

/* Return values for checkFontStatus */
enum fontStatus {GOOD_FONT, BAD_PRIMARY, BAD_FONT, BAD_SIZE, BAD_SPACING};

/* enumerated type preference strings */
static char *SearchMethodStrings[] = {"Literal", "CaseSense", "RegExp", NULL};
#define N_WRAP_STYLES 3
static char *AutoWrapTypes[] = {"None", "Newline", "Continuous",
    	"True", "False", NULL};
#define N_INDENT_STYLES 3
static char *AutoIndentTypes[] = {"None", "Auto",
    	"Smart", "True", "False", NULL};
char *CheckingModeTypes[] = {"Disabled", "Prompt", "Tail-f", NULL};
char *RepositionDialogsStyles[] = {"False", "True", NULL};


/* suplement wrap and indent styles w/ a value meaning "use default" for
   the override fields in the language modes dialog */
#define DEFAULT_WRAP -1
#define DEFAULT_INDENT -1
#define DEFAULT_TAB_DIST -1
#define DEFAULT_EM_TAB_DIST -1

/* list of available language modes and language specific preferences */
static int NLanguageModes = 0;
typedef struct {
    char *name;
    int nExtensions;
    char **extensions;
    char *recognitionExpr;
    char *delimiters;
    int wrapStyle;	
    int indentStyle;	
    int tabDist;	
    int emTabDist;	
} languageModeRec;
static languageModeRec *LanguageModes[MAX_LANGUAGE_MODES];

/* Language mode dialog information */
static struct {
    Widget shell;
    Widget nameW;
    Widget extW;
    Widget recogW;
    Widget delimitW;
    Widget managedListW;
    Widget tabW;
    Widget emTabW;
    Widget defaultIndentW;
    Widget noIndentW;
    Widget autoIndentW;
    Widget smartIndentW;
    Widget defaultWrapW;
    Widget noWrapW;
    Widget newlineWrapW;
    Widget contWrapW;
    languageModeRec **languageModeList;
    int nLanguageModes;
} LMDialog = {NULL};

/* Font dialog information */
typedef struct {
    Widget shell;
    Widget primaryW;
    Widget fillW;
    Widget italicW;
    Widget italicErrW;
    Widget boldW;
    Widget boldErrW;
    Widget boldItalicW;
    Widget boldItalicErrW;
    WindowInfo *window;
    int forWindow;
} fontDialog;

/* Repository for simple preferences settings */
static struct prefData {
    int wrapStyle;		/* what kind of wrapping to do */
    int wrapMargin;		/* 0=wrap at window width, other=wrap margin */
    int autoIndent;		/* style for auto-indent */
    int autoSave;		/* whether automatic backup feature is on */
    int saveOldVersion;		/* whether to preserve a copy of last version */
    int searchDlogs;		/* whether to show explanatory search dialogs */
    int keepSearchDlogs;	/* whether to retain find and replace dialogs */
    int statsLine;		/* whether to show the statistics line */
    int searchMethod;		/* initial search method as a text string */
    int textRows;		/* initial window height in characters */
    int textCols;		/* initial window width in characters */
    int tabDist;		/* number of characters between tab stops */
    int emTabDist;		/* non-zero tab dist. if emulated tabs are on */
    int insertTabs;		/* whether to use tabs for padding */
    int showMatching;		/* whether to flash matching parenthesis */
    int highlightSyntax;    	/* whether to highlight syntax by default */
#ifdef SGI_CUSTOM
    int shortMenus; 	    	/* short menu mode */
#endif
    char fontString[MAX_FONT_LEN]; /* names of fonts for text widget */
    char boldFontString[MAX_FONT_LEN];
    char italicFontString[MAX_FONT_LEN];
    char boldItalicFontString[MAX_FONT_LEN];
    XmFontList fontList;	/* XmFontLists corresp. to above named fonts */
    XFontStruct *boldFontStruct;
    XFontStruct *italicFontStruct;
    XFontStruct *boldItalicFontStruct;
    RepositionDialogsStyle repositionDialogs;	/* w. to reposition dialogs under the pointer */
    int mapDelete;		/* whether to map delete to backspace */
    int stdOpenDialog;		/* w. to retain redundant text field in Open */
    char tagFile[MAXPATHLEN];	/* name of tags file to look for at startup */
    int maxPrevOpenFiles;   	/* limit to size of Open Previous menu */
    char delimiters[MAX_WORD_DELIMITERS]; /* punctuation characters */
    char shell[MAXPATHLEN];	/* shell to use for executing commands */
    char serverName[MAXPATHLEN];/* server name for multiple servers per disp. */
    char bgMenuBtn[MAX_ACCEL_LEN]; /* X event description for triggering
    	    	    	    	      posting of background menu */
    int showISearchLine;	/* whether to show the incremental search line */
    int warnOnExit;	/* whether to warn about open files when exiting */
    int allowReadOnlyEdits; /* whether the text is editable if the file is not */
    int findReplaceUsesSelection; /* whether the find replace dialog is automatically
                                     loaded with the primary selection */
    char backupSuffix[MAXPATHLEN]; /* The suffix to append to the file name to generate
                                      the name of the backup file. */
	CheckingMode checkingMode; /* The default file checking mode */
    int tailMinusFInterval;	/* interval to check for changes in the tail -f mode */
} PrefData;

/* Temporary storage for preferences strings which are discarded after being
   read */
static struct {
    char *shellCmds;
    char *macroCmds;
    char *bgMenuCmds;
    char *highlight;
    char *language;
    char *styles;
    char *smartIndent;
    char *smartIndentCommon;
} TempStringPrefs;

/* preference descriptions for SavePreferences and RestorePreferences. */
static PrefDescripRec PrefDescrip[] = {
#ifndef VMS
#ifdef linux
    {"shellCommands", "ShellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:EX:\n\
	cat>spellTmp; xterm -e ispell -x spellTmp; cat spellTmp; rm spellTmp\n\
	wc::w:ED:\nset wc=`wc`; echo $wc[1] \"lines,\" $wc[2] \"words,\" $wc[3] \"characters\"\n\
	sort::o:EX:\nsort\nnumber lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
	expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
    	&TempStringPrefs.shellCmds, NULL, True},
#elif __FreeBSD__
    {"shellCommands", "ShellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:EX:\n\
      cat>spellTmp; xterm -e ispell -x spellTmp; cat spellTmp; rm spellTmp\n\
      wc::w:ED:\nset wc=`wc`; echo $wc[1] \"words,\" $wc[2] \"lines,\" $wc[3] \"characters\"\n\
      sort::o:EX:\nsort\nnumber lines::n:AW:\npr -tn\nmake:Alt+Z:m:W:\nmake\n\
      expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
      &TempStringPrefs.shellCmds, NULL, True},
#else
    {"shellCommands", "ShellCommands", PREF_ALLOC_STRING, "spell:Alt+B:s:ED:\n\
    	(cat;echo \"\") | spell\nwc::w:ED:\nset wc=`wc`; echo $wc[1] \"lines,\" $wc[2] \"words,\" $wc[3] \"characters\"\n\
    	\nsort::o:EX:\nsort\nnumber lines::n:AW:\nnl -ba\nmake:Alt+Z:m:W:\nmake\n\
	expand::p:EX:\nexpand\nunexpand::u:EX:\nunexpand\n",
    	&TempStringPrefs.shellCmds, NULL, True},
#endif /* linux, __FreeBSD__ */
#endif /* VMS */
    {"macroCommands", "MacroCommands", PREF_ALLOC_STRING,
	"Complete Word:Alt+D::: {\n\
		# Tuning parameters\n\
		ScanDistance = 200\n\
		\n\
		# Search back to a word boundary to find the word to complete\n\
		startScan = max(0, $cursor - ScanDistance)\n\
		endScan = min($text_length, $cursor + ScanDistance)\n\
		scanString = get_range(startScan, endScan)\n\
		keyEnd = $cursor-startScan\n\
		keyStart = search_string(scanString, \"<\", keyEnd-1, \"backward\", \"regex\")\n\
		if (keyStart == -1)\n\
		    return\n\
		keyString = \"<\" substring(scanString, keyStart, keyEnd)\n\
		\n\
		# search both forward and backward from the cursor position.  Note that\n\
		# using a regex search can lead to incorrect results if any of the special\n\
		# regex characters is encountered, which is not considered a delimiter\n\
		backwardSearchResult = search_string(scanString, keyString, keyStart-1, \\\n\
		    	\"backward\", \"regex\")\n\
		forwardSearchResult = search_string(scanString, keyString, keyEnd, \"regex\")\n\
		if (backwardSearchResult == -1 && forwardSearchResult == -1) {\n\
		    beep()\n\
		    return\n\
		}\n\
		\n\
		# if only one direction matched, use that, otherwise use the nearest\n\
		if (backwardSearchResult == -1)\n\
		    matchStart = forwardSearchResult\n\
		else if (forwardSearchResult == -1)\n\
		    matchStart = backwardSearchResult\n\
		else {\n\
		    if (keyStart - backwardSearchResult <= forwardSearchResult - keyEnd)\n\
		    	matchStart = backwardSearchResult\n\
		    else\n\
		    	matchStart = forwardSearchResult\n\
		}\n\
		\n\
		# find the complete word\n\
		matchEnd = search_string(scanString, \">\", matchStart, \"regex\")\n\
		completedWord = substring(scanString, matchStart, matchEnd)\n\
		\n\
		# replace it in the window\n\
		replace_range(startScan + keyStart, $cursor, completedWord)\n\
	}\n\
"
"Fill Sel. w/Char:::R: {\n\
		if ($selection_start == -1) {\n\
		    beep()\n\
		    return\n\
		}\n\
		\n\
		# Ask the user what character to fill with\n\
		fillChar = string_dialog(\"Fill selection with what character?\", \"OK\", \"Cancel\")\n\
		if ($string_dialog_button == 2)\n\
		    return\n\
		\n\
		# Count the number of lines in the selection\n\
		nLines = 0\n\
		for (i=$selection_start; i<$selection_end; i++)\n\
		    if (get_character(i) == \"\\n\")\n\
		    	nLines++\n\
		\n\
		# Create the fill text\n\
		rectangular = $selection_left != -1\n\
		line = \"\"\n\
		fillText = \"\"\n\
		if (rectangular) {\n\
		    for (i=0; i<$selection_right-$selection_left; i++)\n\
			line = line fillChar\n\
		    for (i=0; i<nLines; i++)\n\
			fillText = fillText line \"\\n\"\n\
		    fillText = fillText line\n\
		} else {\n\
		    if (nLines == 0) {\n\
		    	for (i=$selection_start; i<$selection_end; i++)\n\
		    	    fillText = fillText fillChar\n\
		    } else {\n\
		    	startIndent = 0\n\
		    	for (i=$selection_start-1; i>=0 && get_character(i)!=\"\\n\"; i--)\n\
		    	    startIndent++\n\
		    	for (i=0; i<$wrap_margin-startIndent; i++)\n\
		    	    fillText = fillText fillChar\n\
		    	fillText = fillText \"\\n\"\n\
			for (i=0; i<$wrap_margin; i++)\n\
			    line = line fillChar\n\
			for (i=0; i<nLines-1; i++)\n\
			    fillText = fillText line \"\\n\"\n\
			for (i=$selection_end-1; i>=$selection_start && get_character(i)!=\"\\n\"; \\\n\
			    	i--)\n\
			    fillText = fillText fillChar\n\
		    }\n\
		}\n\
		\n\
		# Replace the selection with the fill text\n\
		replace_selection(fillText)\n\
	}\n\
"
"Quote Mail Reply:::: {\n\
		if ($selection_start == -1)\n\
		    replace_all(\"^.*$\", \"\\\\> &\", \"regex\")\n\
		else\n\
		    replace_in_selection(\"^.*$\", \"\\\\> &\", \"regex\")\n\
	}\n\
	Unquote Mail Reply:::: {\n\
		if ($selection_start == -1)\n\
		    replace_all(\"(^\\\\> )(.*)$\", \"\\\\2\", \"regex\")\n\
		else\n\
		    replace_in_selection(\"(^\\\\> )(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	C Comments>Comment Out Sel.@C@C++:::R: {\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		replace_range(selStart, selEnd, \"/* \" get_selection() \" */\")\n\
		select(selStart, selEnd + 6)\n\
	}\n\
	C Comments>C Uncomment Sel.@C@C++:::R: {\n\
		sel = get_selection()\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		commentStart = search_string(sel, \"/*\", 0)\n\
		if (substring(sel, commentStart+2, commentStart+3) == \" \")\n\
		    keepStart = commentStart + 3\n\
		else\n\
		    keepStart = commentStart + 2\n\
		keepEnd = search_string(sel, \"*/\", length(sel), \"backward\")\n\
		commentEnd = keepEnd + 2\n\
		if (substring(sel, keepEnd - 1, keepEnd == \" \"))\n\
		    keepEnd = keepEnd - 1\n\
		replace_range(selStart + commentStart, selStart + commentEnd, \\\n\
			substring(sel, keepStart, keepEnd))\n\
		select(selStart, selEnd - (keepStart-commentStart) - \\\n\
			(commentEnd - keepEnd))\n\
	}\n\
	C Comments>+ C++ Comment@C++:::R: {\n\
		replace_in_selection(\"^.*$\", \"// &\", \"regex\")\n\
	}\n\
	C Comments>- C++ Comment@C++:::R: {\n\
		replace_in_selection(\"(^[ \\\\t]*// ?)(.*)$\", \"\\\\2\", \"regex\")\n\
	}\n\
	C Comments>+ C Bar Comment 1@C:::R: {\n\
		if ($selection_left != -1) {\n\
		    dialog(\"Selection must not be rectangular\")\n\
		    return\n\
		}\n\
		start = $selection_start\n\
		end = $selection_end-1\n\
		origText = get_range($selection_start, $selection_end-1)\n\
		newText = \"/*\\n\" replace_in_string(get_range(start, end), \\\n\
			\"^\", \" * \", \"regex\") \"\\n */\\n\"\n\
		replace_selection(newText)\n\
		select(start, start + length(newText))\n\
	}\n\
"
"	C Comments>- C Bar Comment 1@C:::R: {\n\
		selStart = $selection_start\n\
		selEnd = $selection_end\n\
		newText = get_range(selStart+3, selEnd-4)\n\
		newText = replace_in_string(newText, \"^ \\\\* \", \"\", \"regex\")\n\
		replace_range(selStart, selEnd, newText)\n\
		select(selStart, selStart + length(newText))\n\
	}\n\
	Make C Prototypes@C@C++:::: {\n\
		if ($selection_start == -1) {\n\
		    start = 0\n\
		    end = $text_length\n\
		} else {\n\
		    start = $selection_start\n\
		    end = $selection_end\n\
		}\n\
		string = get_range(start, end)\n\
		nDefs = 0\n\
		searchPos = 0\n\
		prototypes = \"\"\n\
		staticPrototypes = \"\"\n\
		for (;;) {\n\
		    headerStart = search_string(string, \\\n\
			    \"^[a-zA-Z]([^;#\\\"'{}=><!/]|\\n)*\\\\)[ \\t]*\\n?[ \\t]*{\", \\\n\
			    searchPos, \"regex\")\n\
		    if (headerStart == -1)\n\
			break\n\
		    headerEnd = search_string(string, \")\", $search_end,\"backward\") + 1\n\
		    prototype = substring(string, headerStart, headerEnd) \";\\n\"\n\
		    if (substring(string, headerStart, headerStart+6) == \"static\")\n\
			staticPrototypes = staticPrototypes prototype\n\
		    else\n\
    			prototypes = prototypes prototype\n\
		    searchPos = headerEnd\n\
		    nDefs++\n\
		}\n\
		if (nDefs == 0) {\n\
		    dialog(\"No function declarations found\")\n\
		    return\n\
		}\n\
		new()\n\
		focus_window(\"last\")\n\
		replace_range(0, 0, prototypes staticPrototypes)\n\
	}", &TempStringPrefs.macroCmds, NULL, True},
    {"bgMenuCommands", "BGMenuCommands", PREF_ALLOC_STRING,
       "Undo:::: {\nundo()\n}\n\
	Redo:::: {\nredo()\n}\n\
	Cut:::R: {\ncut_clipboard()\n}\n\
	Copy:::R: {\ncopy_clipboard()\n}\n\
	Paste:::: {\npaste_clipboard()\n}", &TempStringPrefs.bgMenuCmds,
	NULL, True},
#ifdef VMS
/* The VAX compiler can't compile Java-Script's definition in highlightData.c */
    {"highlightPatterns", "HighlightPatterns", PREF_ALLOC_STRING,
       "C:Default\n\
	C++:Default\n\
	Java:Default\n\
	Ada:Default\n\
	Fortran:Default\n\
	Pascal:Default\n\
	Yacc:Default\n\
	Perl:Default\n\
	Python:Default\n\
	Awk:Default\n\
	Tcl:Default\n\
	Sh Ksh Bash:Default\n\
	Csh:Default\n\
	Makefile:Default\n\
	HTML:Default\n\
	LaTeX:Default\n\
	VHDL:Default\n\
	Verilog:Default\n\
	NEdit Macro:Default", &TempStringPrefs.highlight, NULL, True},
#else
    {"highlightPatterns", "HighlightPatterns", PREF_ALLOC_STRING,
       "C:Default\n\
	C++:Default\n\
	Java:Default\n\
	JavaScript:Default\n\
	Ada:Default\n\
	Fortran:Default\n\
	Pascal:Default\n\
	Yacc:Default\n\
	Perl:Default\n\
	Python:Default\n\
	Awk:Default\n\
	Tcl:Default\n\
	Sh Ksh Bash:Default\n\
	Csh:Default\n\
	Makefile:Default\n\
	HTML:Default\n\
	LaTeX:Default\n\
	VHDL:Default\n\
	Verilog:Default\n\
	NEdit Macro:Default", &TempStringPrefs.highlight, NULL, True},
#endif /*VMS*/
    {"languageModes", "LanguageModes", PREF_ALLOC_STRING,
#ifdef VMS
       "C:.C .H::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	C++:.CC .HH .I::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	Java:.JAVA::::::\n\
	Ada:.ADA .AD .ADS .ADB .A::::::\n\
	Fortran:.F .F77 .FOR::::::\n\
	Pascal:.PAS .P .INT::::::\n\
        Yacc:.Y::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	Perl:.PL .PM .P5:\"^[ \\t]*#[ \\t]*!.*perl\":::::\n\
	Python:.PY:\"^#!.*python\":Auto:None:::\n\
	Awk:.AWK::::::\n\
	Tcl:.TCL::::::\n\
	Makefile:MAKEFILE::::::\n\
	HTML:.HTML .HTM::::::\n\
	LaTeX:.TEX .STY .CLS .DTX .INS::::::\n\
	VHDL:.VHD .VHDL .VDL::::::\n\
	Verilog:.V::::::\n\
	X Resources:.XRESOURCES .XDEFAULTS .NEDIT:\"^[!#].*([Aa]pp|[Xx]).*[Dd]efaults\":::::\n\
	NEdit Macro:.NM .NEDITMACRO::::::",
#else
       "C:.c .h::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	C++:.cc .hh .C .H .i::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	Java:.java::::::\n\
	JavaScript:.js::::::\n\
	Ada:.ada .ad .ads .adb .a::::::\n\
	Fortran:.f .f77 .for::::::\n\
	Pascal:.pas .p .int::::::\n\
	Yacc:.y::::::\".,/\\`'!|@#%^&*()-=+{}[]\"\":;<>?~\"\n\
	Perl:.pl .pm .p5:\"^[ \\t]*#[ \\t]*!.*perl\":::::\n\
	Python:.py:\"^#!.*python\":Auto:None:::\n\
	Tcl:.tcl::::::\n\
	Awk:.awk::::::\n\
	Sh Ksh Bash:.sh .ksh .bash .profile .bashrc .bash_profile:\"^#![^ \\t]*/(sh|ksh|bash)\":::::\n\
	Csh:.csh .cshrc .login .logout:\"^[ \\t]*#[ \\t]*![ \\t]*/bin/csh\":::::\n\
	Makefile:Makefile makefile::::::\n\
	HTML:.html .htm::::::\n\
! 	LaTeX:.tex .sty .cls .dtx .ins::::::\n\
	VHDL:.vhd .vhdl .vdl::::::\n\
	Verilog:.v::::::\n\
+ 	X Resources:.Xresources .Xdefaults .nedit:\"^[!#].*([Aa]pp|[Xx]).*[Dd]efaults\":::::\n\
	NEdit Macro:.nm .neditmacro::::::",
#endif
	&TempStringPrefs.language, NULL, True},
    {"styles", "Styles", PREF_ALLOC_STRING, "Plain:black:Plain\n\
    	Comment:gray20:Italic\n\
    	Keyword:black:Bold\n\
    	Storage Type:brown:Bold\n\
    	String:darkGreen:Plain\n\
    	String1:SeaGreen:Plain\n\
    	String2:darkGreen:Bold\n\
    	Preprocessor:RoyalBlue4:Plain\n\
    	Preprocessor1:blue:Plain\n\
    	Character Const:darkGreen:Plain\n\
    	Numeric Const:darkGreen:Plain\n\
    	Identifier:brown:Plain\n\
    	Identifier1:RoyalBlue4:Plain\n\
 	Subroutine:brown:Plain\n\
	Subroutine1:chocolate:Plain\n\
	Library Function:brown:Plain\n\
	Double Quoted String:SeaGreen:Plain\n\
	Single Quoted String:darkGreen:Plain\n\
	Escaped Items:SeaGreen:Plain\n\
	Delimiters:black:Plain\n\
	Operators:black:Plain\n\
   	Ada Attributes:plum:Bold\n\
    	Flag:red:Bold\n\
    	Text Comment:SteelBlue4:Italic\n\
    	Text Key:VioletRed4:Bold\n\
	Text Key1:VioletRed4:Plain\n\
    	Text Arg:RoyalBlue4:Bold\n\
    	Text Arg1:SteelBlue4:Bold\n\
	Text Arg2:RoyalBlue4:Plain\n\
    	Text Escape:gray30:Bold\n\
	LaTeX Math:darkGreen:Plain", &TempStringPrefs.styles, NULL, True},
    {"smartIndentInit", "SmartIndentInit", PREF_ALLOC_STRING,
        "C:Default\n\tC++:Default", &TempStringPrefs.smartIndent, NULL, True},
    {"smartIndentInitCommon", "SmartIndentInitCommon", PREF_ALLOC_STRING,
        "Default", &TempStringPrefs.smartIndentCommon, NULL, True},
    {"autoWrap", "AutoWrap", PREF_ENUM, "Newline",
    	&PrefData.wrapStyle, AutoWrapTypes, True},
    {"wrapMargin", "WrapMargin", PREF_INT, "0",
    	&PrefData.wrapMargin, NULL, True},
    {"autoIndent", "AutoIndent", PREF_ENUM, "Auto",
    	&PrefData.autoIndent, AutoIndentTypes, True},
    {"autoSave", "AutoSave", PREF_BOOLEAN, "True",
    	&PrefData.autoSave, NULL, True},
    {"saveOldVersion", "SaveOldVersion", PREF_BOOLEAN, "False",
    	&PrefData.saveOldVersion, NULL, True},
    {"showMatching", "ShowMatching", PREF_BOOLEAN, "True",
    	&PrefData.showMatching, NULL, True},
    {"highlightSyntax", "HighlightSyntax", PREF_BOOLEAN, "False",
    	&PrefData.highlightSyntax, NULL, True},
    {"searchDialogs", "SearchDialogs", PREF_BOOLEAN, "False",
    	&PrefData.searchDlogs, NULL, True},
    {"retainSearchDialogs", "RetainSearchDialogs", PREF_BOOLEAN, "True",
    	&PrefData.keepSearchDlogs, NULL, True},
#if XmVersion < 1002 /* Flashing is annoying in 1.1 versions */
    {"repositionDialogs", "RepositionDialogs", PREF_ENUM, "False",
    	&PrefData.repositionDialogs, RepositionDialogsStyles, True},
#else
    {"repositionDialogs", "RepositionDialogs", PREF_ENUM, "True",
    	&PrefData.repositionDialogs, RepositionDialogsStyles, True},
#endif
    {"statisticsLine", "StatisticsLine", PREF_BOOLEAN, "False",
    	&PrefData.statsLine, NULL, True},
    {"searchMethod", "SearchMethod", PREF_ENUM, "Literal",
    	&PrefData.searchMethod, SearchMethodStrings, True},
    {"textRows", "TextRows", PREF_INT, "24",
    	&PrefData.textRows, NULL, True},
    {"textCols", "TextCols", PREF_INT, "80",
    	&PrefData.textCols, NULL, True},
    {"tabDistance", "TabDistance", PREF_INT, "8",
    	&PrefData.tabDist, NULL, True},
    {"emulateTabs", "EmulateTabs", PREF_INT, "0",
    	&PrefData.emTabDist, NULL, True},
    {"insertTabs", "InsertTabs", PREF_BOOLEAN, "True",
    	&PrefData.insertTabs, NULL, True},
    {"textFont", "TextFont", PREF_STRING,
    	"-adobe-courier-medium-r-normal--14-*-*-*-*-*-*-*",
    	PrefData.fontString, (void *)sizeof(PrefData.fontString), True},
    {"boldHighlightFont", "BoldHighlightFont", PREF_STRING,
    	"-adobe-courier-bold-r-normal--14-*-*-*-*-*-*-*",
    	PrefData.boldFontString, (void *)sizeof(PrefData.boldFontString), True},
    {"italicHighlightFont", "ItalicHighlightFont", PREF_STRING,
    	"-adobe-courier-medium-o-normal--14-*-*-*-*-*-*-*",
    	PrefData.italicFontString,
    	(void *)sizeof(PrefData.italicFontString), True},
    {"boldItalicHighlightFont", "BoldItalicHighlightFont", PREF_STRING,
    	"-adobe-courier-bold-o-normal--14-*-*-*-*-*-*-*",
    	PrefData.boldItalicFontString,
    	(void *)sizeof(PrefData.boldItalicFontString), True},
    {"shell", "Shell", PREF_STRING, "/bin/csh",
    	PrefData.shell, (void *)sizeof(PrefData.shell), False},
    {"remapDeleteKey", "RemapDeleteKey", PREF_BOOLEAN, "False",
    	&PrefData.mapDelete, NULL, False},
    {"stdOpenDialog", "StdOpenDialog", PREF_BOOLEAN, "False",
    	&PrefData.stdOpenDialog, NULL, False},
    {"tagFile", "TagFile", PREF_STRING,
    	"", PrefData.tagFile, (void *)sizeof(PrefData.tagFile), False},
    {"wordDelimiters", "WordDelimiters", PREF_STRING,
    	".,/\\`'!|@#%^&*()-=+{}[]\":;<>?~",
    	PrefData.delimiters, (void *)sizeof(PrefData.delimiters), False},
    {"serverName", "serverName", PREF_STRING, DEFAULTSERVERNAME, PrefData.serverName,
      (void *)sizeof(PrefData.serverName), False},
    {"maxPrevOpenFiles", "MaxPrevOpenFiles", PREF_INT, "30",
    	&PrefData.maxPrevOpenFiles, NULL, False},
    {"bgMenuButton", "BGMenuButton" , PREF_STRING,
	"~Shift~Ctrl~Meta~Alt<Btn3Down>", PrefData.bgMenuBtn,
      (void *)sizeof(PrefData.bgMenuBtn), False},
#ifdef SGI_CUSTOM
    {"shortMenus", "ShortMenus", PREF_BOOLEAN, "False", &PrefData.shortMenus,
      NULL, True},
#endif
    {"searchLine", "SearchLine", PREF_BOOLEAN, "True",
    	&PrefData.showISearchLine, NULL, True},
    {"warnOnExit", "WarnOnExit", PREF_BOOLEAN, "True",
    	&PrefData.warnOnExit, NULL, True},
    {"allowReadOnlyEdits", "AllowReadOnlyEdits", PREF_BOOLEAN, "False",
    	&PrefData.allowReadOnlyEdits, NULL, True},
    {"findReplaceUsesSelection", "FindReplaceUsesSelection", PREF_BOOLEAN, "False",
    	&PrefData.findReplaceUsesSelection, NULL, False},
    {"backupSuffix", "BackupSuffix", PREF_STRING,
    	".bck", PrefData.backupSuffix, (void *)sizeof(PrefData.backupSuffix), False},
    {"checkingMode", "CheckingMode", PREF_ENUM, "Prompt",
    	&PrefData.checkingMode, CheckingModeTypes, True},
    {"tailMinusFInterval", "TailMinusFInterval", PREF_INT, "1000"/*milliseconds*/,
    	&PrefData.tailMinusFInterval, NULL, False},
    {"logFileMonitorInterval", "logFileMonitorInterval", PREF_INT, "1000"/*milliseconds*/,
    	&PrefData.tailMinusFInterval, NULL, False},
};

static XrmOptionDescRec OpTable[] = {
    {"-wrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"Continuous"},
    {"-nowrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"None"},
    {"-autowrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"Newline"},
    {"-noautowrap", ".autoWrap", XrmoptionNoArg, (caddr_t)"None"},
    {"-autoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"Auto"},
    {"-noautoindent", ".autoIndent", XrmoptionNoArg, (caddr_t)"False"},
    {"-autosave", ".autoSave", XrmoptionNoArg, (caddr_t)"True"},
    {"-noautosave", ".autoSave", XrmoptionNoArg, (caddr_t)"False"},
    {"-rows", ".textRows", XrmoptionSepArg, (caddr_t)NULL},
    {"-columns", ".textCols", XrmoptionSepArg, (caddr_t)NULL},
    {"-tabs", ".tabDistance", XrmoptionSepArg, (caddr_t)NULL},
    {"-font", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
    {"-fn", ".textFont", XrmoptionSepArg, (caddr_t)NULL},
    {"-svrname", ".serverName", XrmoptionSepArg, (caddr_t)NULL},
};

static char HeaderText[] = "\
! Preferences file for NEdit\n\
!\n\
! This file is overwritten by the \"Save Defaults...\" command in NEdit \n\
! and serves only the interactively setable options presented in the NEdit\n\
! \"Preferences\" menu.  To modify other options, such as background colors\n\
! and key bindings, use the .Xdefaults file in your home directory (or\n\
! the X resource specification method appropriate to your system).  The\n\
! contents of this file can be moved into an X resource file, but since\n\
! resources in this file override their corresponding X resources, either\n\
! this file should be deleted or individual resource lines in the file\n\
! should be deleted for the moved lines to take effect.\n";

/* Module-global variable set when any preference changes (for asking the
   user about re-saving on exit) */	
static int PrefsHaveChanged = False;

/* Module-global variable set when user uses -import to load additional
   preferences on top of the defaults.  Contains name of file loaded */
static char *ImportedFile = NULL;

/* Module-global variables to support Initial Window Size... dialog */
static int DoneWithSizeDialog;
static Widget RowText, ColText;

/* Module-global variables for Tabs dialog */
static int DoneWithTabsDialog;
static WindowInfo *TabsDialogForWindow;
static Widget TabDistText, EmTabText, EmTabToggle, UseTabsToggle, EmTabLabel;

/* Module-global variables for Wrap Margin dialog */
static int DoneWithWrapDialog;
static WindowInfo *WrapDialogForWindow;
static Widget WrapText, WrapTextLabel, WrapWindowToggle;

static void setIntPref(int *prefDataField, int newValue);
static void setStringPref(char *prefDataField, char *newValue);
static void sizeOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void sizeCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void tabsHelpCB(Widget w, XtPointer clientData, XtPointer callData);
static void emTabsCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapOKCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapCancelCB(Widget w, XtPointer clientData, XtPointer callData);
static void wrapWindowCB(Widget w, XtPointer clientData, XtPointer callData);
static void reapplyLanguageMode(WindowInfo *window, int mode,int forceDefaults);
static void fillFromPrimaryCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static int checkFontStatus(fontDialog *fd, Widget fontTextFieldW);
static int showFontStatus(fontDialog *fd, Widget fontTextFieldW,
    	Widget errorLabelW);
static void primaryModifiedCB(Widget w, XtPointer clientData,
	XtPointer callData);
static void italicModifiedCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static void boldModifiedCB(Widget w, XtPointer clientData, XtPointer callData);
static void boldItalicModifiedCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static void primaryBrowseCB(Widget w, XtPointer clientData, XtPointer callData);
static void italicBrowseCB(Widget w, XtPointer clientData, XtPointer callData);
static void boldBrowseCB(Widget w, XtPointer clientData, XtPointer callData);
static void boldItalicBrowseCB(Widget w, XtPointer clientData,
    	XtPointer callData);
static void browseFont(Widget parent, Widget fontTextW, char *title);
static void fontDestroyCB(Widget w, XtPointer clientData, XtPointer callData);
static void fontOkCB(Widget w, XtPointer clientData, XtPointer callData);
static void fontApplyCB(Widget w, XtPointer clientData, XtPointer callData);
static void fontDismissCB(Widget w, XtPointer clientData, XtPointer callData);
static void updateFonts(fontDialog *fd);
static int matchLanguageMode(WindowInfo *window);
static int loadLanguageModesString(char *inString);
static char *writeLanguageModesString(void);
static char *createExtString(char **extensions, int nExtensions);
static char **readExtensionList(char **inPtr, int *nExtensions);
static void updateLanguageModeSubmenu(WindowInfo *window);
static void setLangModeCB(Widget w, XtPointer clientData, XtPointer callData);
static int modeError(languageModeRec *lm, char *stringStart, char *stoppedAt,
	char *message);
static void lmDestroyCB(Widget w, XtPointer clientData, XtPointer callData);
static void lmOkCB(Widget w, XtPointer clientData, XtPointer callData);
static void lmApplyCB(Widget w, XtPointer clientData, XtPointer callData);
static void lmDismissCB(Widget w, XtPointer clientData, XtPointer callData);
static int lmDeleteConfirmCB(int itemIndex, void *cbArg);
static int updateLMList(void);
static languageModeRec *copyLanguageModeRec(languageModeRec *lm);
static void *lmGetDisplayedCB(void *oldItem, int explicitRequest, int *abort,
    	void *cbArg);
static void lmSetDisplayedCB(void *item, void *cbArg);
static languageModeRec *readLMDialogFields(int silent);
static void lmFreeItemCB(void *item);
static void freeLanguageModeRec(languageModeRec *lm);
static int lmDialogEmpty(void);
static void setTabDist(WindowInfo *window, int tabDist);
static void setEmTabDist(WindowInfo *window, int emTabDist);

#ifdef SGI_CUSTOM
static int shortPrefToDefault(Widget parent, char *settingName,int *setDefault);
#endif

XrmDatabase CreateNEditPrefDB(int *argcInOut, char **argvInOut)
{
    return CreatePreferencesDatabase(PREF_FILE_NAME, APP_NAME, 
	    OpTable, XtNumber(OpTable), (unsigned int *)argcInOut, argvInOut);
}
    
void RestoreNEditPrefs(XrmDatabase prefDB, XrmDatabase appDB)
{
    XFontStruct *font;

    /* Load preferences */
    RestorePreferences(prefDB, appDB, APP_NAME,
    	    APP_CLASS, PrefDescrip, XtNumber(PrefDescrip));

    /* Parse the strings which represent types which are not decoded by
       the standard resource manager routines */
#ifndef VMS
    LoadShellCmdsString(TempStringPrefs.shellCmds);
    XtFree(TempStringPrefs.shellCmds);
#endif /* VMS */
    LoadMacroCmdsString(TempStringPrefs.macroCmds);
    XtFree(TempStringPrefs.macroCmds);
    LoadBGMenuCmdsString(TempStringPrefs.bgMenuCmds);
    XtFree(TempStringPrefs.bgMenuCmds);
    LoadHighlightString(TempStringPrefs.highlight);
    XtFree(TempStringPrefs.highlight);
    LoadStylesString(TempStringPrefs.styles);
    XtFree(TempStringPrefs.styles);
    loadLanguageModesString(TempStringPrefs.language);
    XtFree(TempStringPrefs.language);
    LoadSmartIndentString(TempStringPrefs.smartIndent);
    XtFree(TempStringPrefs.smartIndent);
    LoadSmartIndentCommonString(TempStringPrefs.smartIndentCommon);
    XtFree(TempStringPrefs.smartIndentCommon);
    
    /* translate the font names into fontLists suitable for the text widget */
    font = XLoadQueryFont(TheDisplay, PrefData.fontString);
	if(font == NULL) {
		fprintf(stderr, "Error loading normal font %s\n", PrefData.fontString);
		exit(1);
	}
    PrefData.fontList = font==NULL ? NULL :
	    XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
    
	PrefData.boldFontStruct = XLoadQueryFont(TheDisplay,
    	    PrefData.boldFontString);
	if(PrefData.boldFontStruct == NULL) {
		fprintf(stderr, "Error loading bold font %s\n", PrefData.boldFontString);
		exit(1);
	}
    
	PrefData.italicFontStruct = XLoadQueryFont(TheDisplay,
    	    PrefData.italicFontString);
	if(PrefData.italicFontStruct == NULL) {
		fprintf(stderr, "Error loading italic font %s\n", PrefData.italicFontString);
		exit(1);
	}
    
	PrefData.boldItalicFontStruct = XLoadQueryFont(TheDisplay,
    	    PrefData.boldItalicFontString);
	if(PrefData.boldItalicFontStruct == NULL) {
		fprintf(stderr, "Error loading bold italic font %s\n", PrefData.boldItalicFontString);
		exit(1);
	}
    
    /* For compatability with older (4.0.3 and before) versions, the autoWrap
       and autoIndent resources can accept values of True and False.  Translate
       them into acceptable wrap and indent styles */
    if (PrefData.wrapStyle == 3) PrefData.wrapStyle = NEWLINE_WRAP;
    if (PrefData.wrapStyle == 4) PrefData.wrapStyle = NO_WRAP;
    if (PrefData.autoIndent == 3) PrefData.autoIndent = AUTO_INDENT;
    if (PrefData.autoIndent == 4) PrefData.autoIndent = NO_AUTO_INDENT;
}

void SaveNEditPrefs(Widget parent, int quietly)
{
    if (!quietly) {
	if (DialogF(DF_INF, parent, 2, ImportedFile == NULL ?
"Default preferences will be saved in the .nedit file\n\
in your home directory.  NEdit automatically loads\n\
this file each time it is started." :
"Default preferences will be saved in the .nedit\n\
file in your home directory.\n\n\
SAVING WILL INCORPORATE SETTINGS\n\
FROM FILE: %s", "OK", "Cancel", ImportedFile) == 2)
	    return;
    }    
#ifndef VMS
    TempStringPrefs.shellCmds = WriteShellCmdsString();
#endif /* VMS */
    TempStringPrefs.macroCmds = WriteMacroCmdsString();
    TempStringPrefs.bgMenuCmds = WriteBGMenuCmdsString();
    TempStringPrefs.highlight = WriteHighlightString();
    TempStringPrefs.language = writeLanguageModesString();
    TempStringPrefs.styles = WriteStylesString();
    TempStringPrefs.smartIndent = WriteSmartIndentString();
    TempStringPrefs.smartIndentCommon = WriteSmartIndentCommonString();
    if (!SavePreferences(XtDisplay(parent), PREF_FILE_NAME, HeaderText,
    	    PrefDescrip, XtNumber(PrefDescrip)))
    	DialogF(DF_WARN, parent, 1,
#ifdef VMS
    		"Unable to save preferences in SYS$LOGIN:.NEDIT", "Dismiss");
#else
    		"Unable to save preferences in $HOME/.nedit", "Dismiss");
#endif /*VMS*/

#ifndef VMS
    XtFree(TempStringPrefs.shellCmds);
#endif /* VMS */
    XtFree(TempStringPrefs.macroCmds);
    XtFree(TempStringPrefs.bgMenuCmds);
    XtFree(TempStringPrefs.highlight);
    XtFree(TempStringPrefs.language);
    XtFree(TempStringPrefs.styles);
    XtFree(TempStringPrefs.smartIndent);
    XtFree(TempStringPrefs.smartIndentCommon);
    
    PrefsHaveChanged = False;
}

/*
** Load an additional preferences file on top of the existing preferences
** derived from defaults, the .nedit file, and X resources.
*/
void ImportPrefFile(char *filename)
{
    XrmDatabase db;
    
    db = XrmGetFileDatabase(filename);
    RestoreNEditPrefs(NULL, db);
    ImportedFile = XtNewString(filename);
}

void SetPrefWrap(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.wrapStyle, state);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->noWrapDefItem, state==NO_WRAP, False);
    	XmToggleButtonSetState(win->newlineWrapDefItem, state==NEWLINE_WRAP, False);
    	XmToggleButtonSetState(win->contWrapDefItem, state==CONTINUOUS_WRAP, False);
    }
}

int GetPrefWrap(int langMode)
{
    if (langMode == PLAIN_LANGUAGE_MODE ||
	    LanguageModes[langMode]->wrapStyle == DEFAULT_WRAP)
    	return PrefData.wrapStyle;
    return LanguageModes[langMode]->wrapStyle;
}

void SetPrefWrapMargin(int margin)
{
    setIntPref(&PrefData.wrapMargin, margin);
}

int GetPrefWrapMargin(void)
{
    return PrefData.wrapMargin;
}

void SetPrefSearch(int searchType)
{
    WindowInfo *win;
    setIntPref(&PrefData.searchMethod, searchType);
    for (win=WindowList; win!=NULL; win=win->next){
    	XmToggleButtonSetState(win->searchLiteralDefItem, searchType==SEARCH_LITERAL, False);
    	XmToggleButtonSetState(win->searchCaseSenseDefItem, searchType==SEARCH_CASE_SENSE, False);
    	XmToggleButtonSetState(win->searchRegexDefItem, searchType==SEARCH_REGEX, False);
    }
}

int GetPrefSearch(void)
{
    return PrefData.searchMethod;
}

void SetPrefAutoIndent(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.autoIndent, state);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->autoIndentOffDefItem, state==NO_AUTO_INDENT, False);
    	XmToggleButtonSetState(win->autoIndentDefItem, state==AUTO_INDENT, False);
    	XmToggleButtonSetState(win->smartIndentDefItem, state==SMART_INDENT, False);
    }
}

int GetPrefAutoIndent(int langMode)
{
    if (langMode == PLAIN_LANGUAGE_MODE ||
	    LanguageModes[langMode]->indentStyle == DEFAULT_INDENT)
    	return PrefData.autoIndent;
    return LanguageModes[langMode]->indentStyle;
}

void SetPrefAutoSave(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.autoSave, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->autoSaveDefItem, state, False);
}

int GetPrefAutoSave(void)
{
    return PrefData.autoSave;
}

void SetPrefSaveOldVersion(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.saveOldVersion, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->saveLastDefItem, state, False);
}

int GetPrefSaveOldVersion(void)
{
    return PrefData.saveOldVersion;
}

void SetPrefSearchDlogs(int state)
{
    setIntPref(&PrefData.searchDlogs, state);
}

int GetPrefSearchDlogs(void)
{
    return PrefData.searchDlogs;
}

void SetPrefKeepSearchDlogs(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.keepSearchDlogs, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->keepSearchDlogsDefItem, state, False);
}

int GetPrefKeepSearchDlogs(void)
{
    return PrefData.keepSearchDlogs;
}

void SetPrefStatsLine(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.statsLine, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->statsLineDefItem, state, False);
}

int GetPrefStatsLine(void)
{
    return PrefData.statsLine;
}

void SetPrefShowISearchLine(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.showISearchLine, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->iSearchLineDefItem, state, False);
}

int GetPrefShowISearchLine(void)
{
    return PrefData.showISearchLine;
}

void SetPrefWarnOnExit(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.warnOnExit, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->warnOnExitDefItem, state, False);
}

int GetPrefWarnOnExit(void)
{
    return PrefData.warnOnExit;
}

void SetPrefAllowReadOnlyEdits(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.allowReadOnlyEdits, state);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->allowReadOnlyEditsDefItem, state, False);
    }
}

int GetPrefAllowReadOnlyEdits(void)
{
    return PrefData.allowReadOnlyEdits;
}

void SetPrefFindReplaceUsesSelection(int state)
{
    setIntPref(&PrefData.findReplaceUsesSelection, state);
}

int GetPrefFindReplaceUsesSelection(void)
{
    return PrefData.findReplaceUsesSelection;
}

void SetPrefMapDelete(int state)
{
    setIntPref(&PrefData.mapDelete, state);
}

int GetPrefMapDelete(void)
{
    return PrefData.mapDelete;
}

void SetPrefStdOpenDialog(int state)
{
    setIntPref(&PrefData.stdOpenDialog, state);
}

int GetPrefStdOpenDialog(void)
{
    return PrefData.stdOpenDialog;
}

void SetPrefRows(int nRows)
{
    setIntPref(&PrefData.textRows, nRows);
}

int GetPrefRows(void)
{
    return PrefData.textRows;
}

void SetPrefCols(int nCols)
{
   setIntPref(&PrefData.textCols, nCols);
}

int GetPrefCols(void)
{
    return PrefData.textCols;
}

void SetPrefTabDist(int tabDist)
{
    setIntPref(&PrefData.tabDist, tabDist);
}

int GetPrefTabDist(int langMode)
{
    if (langMode == PLAIN_LANGUAGE_MODE ||
	    LanguageModes[langMode]->tabDist == DEFAULT_TAB_DIST)
	return PrefData.tabDist;
    return LanguageModes[langMode]->tabDist;
 }

void SetPrefEmTabDist(int tabDist)
{
    setIntPref(&PrefData.emTabDist, tabDist);
}

int GetPrefEmTabDist(int langMode)
{
    if (langMode == PLAIN_LANGUAGE_MODE ||
	    LanguageModes[langMode]->emTabDist == DEFAULT_EM_TAB_DIST)
	return PrefData.emTabDist;
    return LanguageModes[langMode]->emTabDist;
}

void SetPrefInsertTabs(int state)
{
    setIntPref(&PrefData.insertTabs, state);
}

int GetPrefInsertTabs(void)
{
    return PrefData.insertTabs;
}

void SetPrefShowMatching(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.showMatching, state);
    for (win=WindowList; win!=NULL; win=win->next)
    	XmToggleButtonSetState(win->showMatchingDefItem, state, False);
}

int GetPrefShowMatching(void)
{
    return PrefData.showMatching;
}

void SetPrefHighlightSyntax(int state)
{
    WindowInfo *win;
    setIntPref(&PrefData.highlightSyntax, state);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->highlightOffDefItem, state==False, False);
    	XmToggleButtonSetState(win->highlightDefItem, state==True, False);
    }
}

int GetPrefHighlightSyntax(void)
{
    return PrefData.highlightSyntax;
}

void SetPrefRepositionDialogs(RepositionDialogsStyle style)
{
    WindowInfo *win;
    setIntPref((int*)&PrefData.repositionDialogs, (int)style);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->dialogCenterPointerDefItem, (style == REPOSITION_DIALOGS_ON_POINTER), False);
    	XmToggleButtonSetState(win->dialogCenterWindowDefItem, (style == REPOSITION_DIALOGS_DISABLED), False);
	}
}

RepositionDialogsStyle GetPrefRepositionDialogs(void)
{
    return PrefData.repositionDialogs;
}

void SetPrefTagFile(char *tagFileName)
{
    setStringPref(PrefData.tagFile, tagFileName);
}

char *GetPrefTagFile(void)
{
    return PrefData.tagFile;
}

void SetPrefBackupSuffix(char *suffix)
{
    strcpy(PrefData.backupSuffix, suffix);
}

char *GetPrefBackupSuffix(void)
{
	char *start;
	char *end;
	
	/* remove leading and trailing whitespace */
	for(start = PrefData.backupSuffix; *start && isspace(*start); start++) {
	}
	for(end = start + strlen(start) - 1; end >= start && isspace(*end); end--) {
	}
	end++;
	*end = 0;
	return start;
}

char *GetPrefDelimiters(void)
{
    return PrefData.delimiters;
}

/*
** Set the font preferences using the font name (the fontList is generated
** in this call).  Note that this leaks memory and server resources each
** time the default font is re-set.  See note on SetFontByName in window.c
** for more information.
*/
void SetPrefFont(char *fontName)
{
    XFontStruct *font;
    
    setStringPref(PrefData.fontString, fontName);
    font = XLoadQueryFont(TheDisplay, fontName);
    PrefData.fontList = font==NULL ? NULL :
	    XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
}
void SetPrefBoldFont(char *fontName)
{
    setStringPref(PrefData.boldFontString, fontName);
    PrefData.boldFontStruct = XLoadQueryFont(TheDisplay, fontName);
}
void SetPrefItalicFont(char *fontName)
{
    setStringPref(PrefData.italicFontString, fontName);
    PrefData.italicFontStruct = XLoadQueryFont(TheDisplay, fontName);
}
void SetPrefBoldItalicFont(char *fontName)
{
    setStringPref(PrefData.boldItalicFontString, fontName);
    PrefData.boldItalicFontStruct = XLoadQueryFont(TheDisplay, fontName);
}

char *GetPrefFontName(void)
{
    return PrefData.fontString;
}
char *GetPrefBoldFontName(void)
{
    return PrefData.boldFontString;
}
char *GetPrefItalicFontName(void)
{
    return PrefData.italicFontString;
}
char *GetPrefBoldItalicFontName(void)
{
    return PrefData.boldItalicFontString;
}

XmFontList GetPrefFontList(void)
{
    return PrefData.fontList;
}
XFontStruct *GetPrefBoldFont(void)
{
    return PrefData.boldFontStruct;
}
XFontStruct *GetPrefItalicFont(void)
{
    return PrefData.italicFontStruct;
}
XFontStruct *GetPrefBoldItalicFont(void)
{
    return PrefData.boldItalicFontStruct;
}

void SetPrefShell(char *shell)
{
    setStringPref(PrefData.shell, shell);
}

char *GetPrefShell(void)
{
    return PrefData.shell;
}

char *GetPrefServerName(void)
{
    return PrefData.serverName;
}

char *GetPrefBGMenuBtn(void)
{
    return PrefData.bgMenuBtn;
}

int GetPrefMaxPrevOpenFiles(void)
{
    return PrefData.maxPrevOpenFiles;
}

#ifdef SGI_CUSTOM
void SetPrefShortMenus(int state)
{
    setIntPref(&PrefData.shortMenus, state);
}

int GetPrefShortMenus(void)
{
    return PrefData.shortMenus;
}
#endif

void SetPrefCheckingMode(CheckingMode checkingMode)
{
    WindowInfo *win;
    setIntPref((int*)&PrefData.checkingMode, (int)checkingMode);
    for (win=WindowList; win!=NULL; win=win->next) {
    	XmToggleButtonSetState(win->checkingModePromptToReloadDefItem, 
    		(checkingMode == CHECKING_MODE_PROMPT_TO_RELOAD), False);
    	XmToggleButtonSetState(win->checkingModeDisabledDefItem, 
    		(checkingMode == CHECKING_MODE_DISABLED), False);
    	XmToggleButtonSetState(win->checkingModeTailMinusFDefItem, 
    		(checkingMode == CHECKING_MODE_TAIL_MINUS_F), False);
    }    
}

CheckingMode GetPrefCheckingMode(void)
{
    return PrefData.checkingMode;
}

void SetPrefTailMinusFInterval(int interval)
{
    setIntPref(&PrefData.tailMinusFInterval, interval);
}

int GetPrefTailMinusFInterval(void)
{
    return PrefData.tailMinusFInterval;
}

/*
** If preferences don't get saved, ask the user on exit whether to save
*/
void MarkPrefsChanged(void)
{
    PrefsHaveChanged = True;
}

/*
** Check if preferences have changed, and if so, ask the user if he wants
** to re-save.  Returns False if user requests cancelation of Exit (or whatever
** operation triggered this call to be made).
*/
int CheckPrefsChangesSaved(Widget dialogParent)
{
    int resp;
    
    if (!PrefsHaveChanged)
	return True;
    
    resp = DialogF(DF_WARN, dialogParent, 3, ImportedFile == NULL ?
    	    "Default Preferences have changed.\nSave changes to .nedit file?" :
	    "Default Preferences have changed.  SAVING \n\
CHANGES WILL INCORPORATE ADDITIONAL\nSETTINGS FROM FILE: %s",
	    "Save", "Don't Save", "Cancel", ImportedFile);
    if (resp == 2)
	return True;
    if (resp == 3)
	return False;
    
    SaveNEditPrefs(dialogParent, True);
    return True;
}

/*
** set *prefDataField to newValue, but first check if they're different
** and update PrefsHaveChanged if a preference setting has now changed.
*/
static void setIntPref(int *prefDataField, int newValue)
{
    if (newValue != *prefDataField)
	PrefsHaveChanged = True;
    *prefDataField = newValue;
}
static void setStringPref(char *prefDataField, char *newValue)
{
    if (strcmp(prefDataField, newValue))
	PrefsHaveChanged = True;
    strcpy(prefDataField, newValue);
}

/*
** Apply language mode matching criteria and set window->languageMode to
** the appropriate mode for the current file, trigger language mode
** specific actions (turn on/off highlighting), and update the language
** mode menu item.  If forceNewDefaults is true, re-establish default
** settings for language-specific preferences regardless of whether
** they were previously set by the user.
*/
void DetermineLanguageMode(WindowInfo *window, int forceNewDefaults)
{
    Widget menu;
    WidgetList items;
    int n, mode, nItems;
    XtArgVal userData;
    
    /* Set the new language mode and do mode-specific actions */
    mode = matchLanguageMode(window);
    reapplyLanguageMode(window, mode, forceNewDefaults);
    
    /* Select the correct language mode in the sub-menu */
   	XtVaGetValues(window->langModeCascade, XmNsubMenuId, &menu, NULL);
   	XtVaGetValues(menu, XmNchildren, &items, XmNnumChildren, &nItems,NULL);
   	for (n=0; n<nItems; n++) {
   		XtVaGetValues(items[n], XmNuserData, &userData, NULL);
   		XmToggleButtonSetState(items[n], (int)userData == mode, False);
   	}
}

/*
** Return the name of the current language mode set in "window", or NULL
** if the current mode is "Plain".
*/
char *LanguageModeName(int mode)
{
    if (mode == PLAIN_LANGUAGE_MODE)
    	return NULL;
    else
    	return LanguageModes[mode]->name;
}

/*
** Get the set of word delimiters for the language mode set in the current
** window.  Returns NULL when no language mode is set (it would be easy to
** return the default delimiter set when the current language mode is "Plain",
** or the mode doesn't have its own delimiters, but this is usually used
** to supply delimiters for RE searching, and ExecRE can skip compiling a
** delimiter table when delimiters is NULL).
*/
char *GetWindowDelimiters(WindowInfo *window)
{
    if (window->languageMode == PLAIN_LANGUAGE_MODE)
    	return NULL;
    else
    	return LanguageModes[window->languageMode]->delimiters;
}

/*
** Put up a dialog for selecting a custom initial window size
*/
void RowColumnPrefDialog(Widget parent)
{
    Widget form, selBox, topLabel;
    Arg selBoxArgs[2];
    XmString s1;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "customSize", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)sizeOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)sizeCancelCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Initial Window Size", NULL);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, NULL);

    topLabel = XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
    	    	"Enter desired size in rows\nand columns of characters:"), NULL);
    XmStringFree(s1);
 
    RowText = XtVaCreateManagedWidget("rows", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 5,
    	    XmNrightPosition, 45, NULL);
    RemapDeleteKey(RowText);
 
    XtVaCreateManagedWidget("xLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING("x"),
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
    	    XmNtopWidget, topLabel,
    	    XmNbottomWidget, RowText,
    	    XmNleftPosition, 45,
    	    XmNrightPosition, 55, NULL);
    XmStringFree(s1);

    ColText = XtVaCreateManagedWidget("cols", xmTextWidgetClass, form,
    	    XmNcolumns, 3,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNtopWidget, topLabel,
    	    XmNleftPosition, 55,
    	    XmNrightPosition, 95, NULL);
    RemapDeleteKey(ColText);

    /* put up dialog and wait for user to press ok or cancel */
    DoneWithSizeDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithSizeDialog) {
   		XEvent event;
       	XtAppNextEvent(XtWidgetToApplicationContext(parent), &event);
   		ServerDispatchEvent(&event);
	}
    
    XtDestroyWidget(selBox);
}

static void sizeOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int rowValue, colValue, stat;
    
    /* get the values that the user entered and make sure they're ok */
    stat = GetIntTextWarn(RowText, &rowValue, "number of rows", True);
    if (stat != TEXT_READ_OK)
    	return;
    stat = GetIntTextWarn(ColText, &colValue, "number of columns", True);
    if (stat != TEXT_READ_OK)
    	return;
    
    /* set the corresponding preferences and dismiss the dialog */
    SetPrefRows(rowValue);
    SetPrefCols(colValue);
    DoneWithSizeDialog = True;
}

static void sizeCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithSizeDialog = True;
}

/*
** Present the user a dialog for setting tab related preferences, either as
** defaults, or for a specific window (pass "forWindow" as NULL to set default
** preference, or a window to set preferences for the specific window.
*/
void TabsPrefDialog(Widget parent, WindowInfo *forWindow)
{
    Widget form, selBox;
    Arg selBoxArgs[2];
    XmString s1;
    int emulate, emTabDist, useTabs, tabDist;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "customSize", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)tabsOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)tabsCancelCB,NULL);
    XtAddCallback(selBox, XmNhelpCallback, (XtCallbackProc)tabsHelpCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Tabs", NULL);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, NULL);

    TabDistText = XtVaCreateManagedWidget("tabDistText", xmTextWidgetClass,
    	    form, XmNcolumns, 7,
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_FORM, NULL);
    RemapDeleteKey(TabDistText);
    XtVaCreateManagedWidget("tabDistLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Tab spacing (for hardware tab characters)"),
	    XmNmnemonic, 'T',
    	    XmNuserData, TabDistText,
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, TabDistText,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, TabDistText, NULL);
    XmStringFree(s1);
 
    EmTabText = XtVaCreateManagedWidget("emTabText", xmTextWidgetClass, form,
    	    XmNcolumns, 7,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNrightAttachment, XmATTACH_OPPOSITE_WIDGET,
    	    XmNrightWidget, TabDistText, NULL);
    RemapDeleteKey(EmTabText);
    EmTabLabel = XtVaCreateManagedWidget("emTabLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("Emulated tab spacing"),
	    XmNmnemonic, 's',
    	    XmNuserData, EmTabText,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, EmTabText,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, EmTabText, NULL);
    XmStringFree(s1);
    EmTabToggle = XtVaCreateManagedWidget("emTabToggle",
    	    xmToggleButtonWidgetClass, form, XmNlabelString,
    	    	s1=XmStringCreateSimple("Emulate tabs"),
	    XmNmnemonic, 'E',
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, TabDistText,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, EmTabText, NULL);
    XmStringFree(s1);
    XtAddCallback(EmTabToggle, XmNvalueChangedCallback, emTabsCB, NULL);
    UseTabsToggle = XtVaCreateManagedWidget("useTabsToggle",
    	    xmToggleButtonWidgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Use tab characters in padding and emulated tabs"),
	    XmNmnemonic, 'U',
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, EmTabText,
    	    XmNtopOffset, 5,
    	    XmNleftAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);

    /* Set default values */
    if (forWindow == NULL) {
    	emTabDist = GetPrefEmTabDist(PLAIN_LANGUAGE_MODE);
    	useTabs = GetPrefInsertTabs();
    	tabDist = GetPrefTabDist(PLAIN_LANGUAGE_MODE);
    } else {
    	XtVaGetValues(forWindow->textArea, textNemulateTabs, &emTabDist, NULL);
    	useTabs = forWindow->editorInfo->buffer->useTabs;
    	tabDist = BufGetTabDistance(forWindow->editorInfo->buffer);
    }
    emulate = emTabDist != 0;
    SetIntText(TabDistText, tabDist);
    XmToggleButtonSetState(EmTabToggle, emulate, True);
    if (emulate)
    	SetIntText(EmTabText, emTabDist);
    XmToggleButtonSetState(UseTabsToggle, useTabs, False);
    XtSetSensitive(EmTabText, emulate);
    XtSetSensitive(EmTabLabel, emulate);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* Set the widget to get focus */
#if XmVersion >= 1002
    XtVaSetValues(form, XmNinitialFocus, TabDistText, NULL);
#endif
    
    /* put up dialog and wait for user to press ok or cancel */
    TabsDialogForWindow = forWindow;
    DoneWithTabsDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithTabsDialog) {
   		XEvent event;
       	XtAppNextEvent(XtWidgetToApplicationContext(parent), &event);
   		ServerDispatchEvent(&event);
	}
    
    XtDestroyWidget(selBox);
}

static void tabsOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int emulate, useTabs, stat, tabDist, emTabDist;
    WindowInfo *window = TabsDialogForWindow;
    
    /* get the values that the user entered and make sure they're ok */
    emulate = XmToggleButtonGetState(EmTabToggle);
    useTabs = XmToggleButtonGetState(UseTabsToggle);
    stat = GetIntTextWarn(TabDistText, &tabDist, "tab spacing", True);
    if (stat != TEXT_READ_OK)
    	return;
    if (tabDist <= 0 || tabDist > MAX_EXP_CHAR_LEN) {
    	DialogF(DF_WARN, TabDistText, 1, "Tab spacing out of range", "Dismiss");
    	return;
    }
    if (emulate) {
	stat = GetIntTextWarn(EmTabText, &emTabDist, "emulated tab spacing",True);
	if (stat != TEXT_READ_OK)
	    return;
	if (emTabDist <= 0 || tabDist >= 1000) {
	    DialogF(DF_WARN, EmTabText, 1, "Emulated tab spacing out of range",
	    	    "Dismiss");
	    return;
	}
    } else
    	emTabDist = 0;
    
#ifdef SGI_CUSTOM
    /* Ask the user about saving as a default preference */
    if (TabsDialogForWindow != NULL) {
	int setDefault;
	if (!shortPrefToDefault(window->shell, "Tab Settings", &setDefault)) {
	    DoneWithTabsDialog = True;
    	    return;
	}
	if (setDefault) {
    	    SetPrefTabDist(tabDist);
    	    SetPrefEmTabDist(emTabDist);
    	    SetPrefInsertTabs(useTabs);
	    SaveNEditPrefs(window->shell, GetPrefShortMenus());
	}
    }
#endif

    /* Set the value in either the requested window or default preferences */
    if (TabsDialogForWindow == NULL) {
    	SetPrefTabDist(tabDist);
    	SetPrefEmTabDist(emTabDist);
    	SetPrefInsertTabs(useTabs);
    } else {
    	setTabDist(window, tabDist);
    	setEmTabDist(window, emTabDist);
       	window->editorInfo->buffer->useTabs = useTabs;
    }
    DoneWithTabsDialog = True;
}

static void setTabDist(WindowInfo *window, int tabDist)
{
    if (window->editorInfo->buffer->tabDist != tabDist) {
    	window->editorInfo->ignoreModify = True;
    	BufSetTabDistance(window->editorInfo->buffer, tabDist);
    	window->editorInfo->ignoreModify = False;
    }
}

static void setEmTabDist(WindowInfo *window, int emTabDist)
{
    int i;
    
    XtVaSetValues(window->textArea, textNemulateTabs, emTabDist, NULL);
    for (i=0; i<window->nPanes; i++)
	XtVaSetValues(window->textPanes[i], textNemulateTabs, emTabDist, NULL);
}

static void tabsCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithTabsDialog = True;
}

static void tabsHelpCB(Widget w, XtPointer clientData, XtPointer callData)
{
    Help(XtParent(EmTabLabel), HELP_TABS_DIALOG);
}

static void emTabsCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int state = XmToggleButtonGetState(w);
    
    XtSetSensitive(EmTabLabel, state);
    XtSetSensitive(EmTabText, state);
}

/*
** Present the user a dialog for setting wrap margin.
*/
void WrapMarginDialog(Widget parent, WindowInfo *forWindow)
{
    Widget form, selBox;
    Arg selBoxArgs[2];
    XmString s1;
    int margin;

    XtSetArg(selBoxArgs[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(selBoxArgs[1], XmNautoUnmanage, False);
    selBox = XmCreatePromptDialog(parent, "wrapMargin", selBoxArgs, 2);
    XtAddCallback(selBox, XmNokCallback, (XtCallbackProc)wrapOKCB, NULL);
    XtAddCallback(selBox, XmNcancelCallback, (XtCallbackProc)wrapCancelCB,NULL);
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_TEXT));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_SELECTION_LABEL));
    XtUnmanageChild(XmSelectionBoxGetChild(selBox, XmDIALOG_HELP_BUTTON));
    XtVaSetValues(XtParent(selBox), XmNtitle, "Wrap Margin", NULL);
    
    form = XtVaCreateManagedWidget("form", xmFormWidgetClass, selBox, NULL);

    WrapWindowToggle = XtVaCreateManagedWidget("wrapWindowToggle",
    	    xmToggleButtonWidgetClass, form, XmNlabelString,
    	    	s1=XmStringCreateSimple("Wrap and Fill at width of window"),
	    XmNmnemonic, 'W',
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);
    XtAddCallback(WrapWindowToggle, XmNvalueChangedCallback, wrapWindowCB,NULL);
    WrapText = XtVaCreateManagedWidget("wrapText", xmTextWidgetClass, form,
    	    XmNcolumns, 5,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, WrapWindowToggle,
    	    XmNrightAttachment, XmATTACH_FORM, NULL);
    RemapDeleteKey(WrapText);
    WrapTextLabel = XtVaCreateManagedWidget("wrapMarginLabel",
    	    xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Margin for Wrap and Fill"),
	    XmNmnemonic, 'M',
    	    XmNuserData, WrapText,
    	    XmNtopAttachment, XmATTACH_WIDGET,
    	    XmNtopWidget, WrapWindowToggle,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNrightAttachment, XmATTACH_WIDGET,
    	    XmNrightWidget, WrapText,
	    XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
	    XmNbottomWidget, WrapText, NULL);
    XmStringFree(s1);

    /* Set default value */
    if (forWindow == NULL)
    	margin = GetPrefWrapMargin();
    else
    	XtVaGetValues(forWindow->textArea, textNwrapMargin, &margin, NULL);
    XmToggleButtonSetState(WrapWindowToggle, margin==0, True);
    if (margin != 0)
    	SetIntText(WrapText, margin);
    XtSetSensitive(WrapText, margin!=0);
    XtSetSensitive(WrapTextLabel, margin!=0);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* put up dialog and wait for user to press ok or cancel */
    WrapDialogForWindow = forWindow;
    DoneWithWrapDialog = False;
    ManageDialogCenteredOnPointer(selBox);
    while (!DoneWithWrapDialog) {
   		XEvent event;
       	XtAppNextEvent(XtWidgetToApplicationContext(parent), &event);
   		ServerDispatchEvent(&event);
	}
    
    XtDestroyWidget(selBox);
}

static void wrapOKCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int i, wrapAtWindow, margin, stat;
    WindowInfo *window = WrapDialogForWindow;
    
    /* get the values that the user entered and make sure they're ok */
    wrapAtWindow = XmToggleButtonGetState(WrapWindowToggle);
    if (wrapAtWindow)
    	margin = 0;
    else {
	stat = GetIntTextWarn(WrapText, &margin, "wrap Margin", True);
	if (stat != TEXT_READ_OK)
    	    return;
	if (margin <= 0 || margin >= 1000) {
    	    DialogF(DF_WARN, WrapText, 1, "Wrap margin out of range", "Dismiss");
    	    return;
	}
    }

#ifdef SGI_CUSTOM
    /* Ask the user about saving as a default preference */
    if (WrapDialogForWindow != NULL) {
	int setDefault;
	if (!shortPrefToDefault(window->shell, "Wrap Margin Settings",
	    	&setDefault)) {
	    DoneWithWrapDialog = True;
    	    return;
	}
	if (setDefault) {
    	    SetPrefWrapMargin(margin);
	    SaveNEditPrefs(window->shell, GetPrefShortMenus());
	}
    }
#endif

    /* Set the value in either the requested window or default preferences */
    if (WrapDialogForWindow == NULL)
    	SetPrefWrapMargin(margin);
    else {
	XtVaSetValues(window->textArea, textNwrapMargin, margin, NULL);
	for (i=0; i<window->nPanes; i++)
	    XtVaSetValues(window->textPanes[i], textNwrapMargin, margin, NULL);
    }
    DoneWithWrapDialog = True;
}

static void wrapCancelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    DoneWithWrapDialog = True;
}

static void wrapWindowCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int wrapAtWindow = XmToggleButtonGetState(w);
    
    XtSetSensitive(WrapTextLabel, !wrapAtWindow);
    XtSetSensitive(WrapText, !wrapAtWindow);
}

/*
** Present a dialog for editing language mode information
*/
void EditLanguageModes(Widget parent)
{
#define LIST_RIGHT 40
#define LEFT_MARGIN_POS 1
#define RIGHT_MARGIN_POS 99
#define H_MARGIN 5
    Widget form, nameLbl, topLbl, extLbl, recogLbl, delimitLbl;
    Widget okBtn, applyBtn, dismissBtn, stretchForm;
    Widget overrideLbl, overrideFrame, overrideForm, delimitForm;
    Widget tabForm, tabLbl, emTabLbl, indentBox, wrapBox;
    XmString s1;
    int i, ac;
    Arg args[20];

    /* if the dialog is already displayed, just pop it to the top and return */
    if (LMDialog.shell != NULL) {
    	RaiseShellWindow(LMDialog.shell);
    	return;
    }
    
    LMDialog.languageModeList = (languageModeRec **)XtMalloc(
    	    sizeof(languageModeRec *) * MAX_LANGUAGE_MODES);
    for (i=0; i<NLanguageModes; i++)
    	LMDialog.languageModeList[i] = copyLanguageModeRec(LanguageModes[i]);
    LMDialog.nLanguageModes = NLanguageModes;

    /* Create a form widget in an application shell */
    ac = 0;
    XtSetArg(args[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(args[ac], XmNiconName, "Language Modes"); ac++;
    XtSetArg(args[ac], XmNtitle, "Language Modes"); ac++;
    LMDialog.shell = XtAppCreateShell(APP_NAME, APP_CLASS,
	    applicationShellWidgetClass, TheDisplay, args, ac);
    AddSmallIcon(LMDialog.shell);
    form = XtVaCreateManagedWidget("editLanguageModes", xmFormWidgetClass,
	    LMDialog.shell, XmNautoUnmanage, False,
	    XmNresizePolicy, XmRESIZE_NONE, NULL);
    XtAddCallback(form, XmNdestroyCallback, lmDestroyCB, NULL);
    AddMotifCloseCallback(LMDialog.shell, lmDismissCB, NULL);
    
    topLbl = XtVaCreateManagedWidget("topLabel", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
"To modify the properties of an existing language mode, select the name from\n\
the list on the left.  To add a new language, select \"New\" from the list."),
	    XmNmnemonic, 'N',
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS, NULL);
    XmStringFree(s1);
    
    nameLbl = XtVaCreateManagedWidget("nameLbl", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("Name"),
	    XmNmnemonic, 'm',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopOffset, H_MARGIN,
	    XmNtopWidget, topLbl, NULL);
    XmStringFree(s1);
 
    LMDialog.nameW = XtVaCreateManagedWidget("name", xmTextWidgetClass, form,
    	    XmNcolumns, 15,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, nameLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, (RIGHT_MARGIN_POS + LIST_RIGHT)/2, NULL);
    RemapDeleteKey(LMDialog.nameW);
    XtVaSetValues(nameLbl, XmNuserData, LMDialog.nameW, NULL);
    
    extLbl = XtVaCreateManagedWidget("extLbl", xmLabelGadgetClass, form,
    	    XmNlabelString, 	    	
    	    	s1=XmStringCreateSimple("File extensions (separate w/ space)"),
    	    XmNmnemonic, 'F',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopOffset, H_MARGIN,
	    XmNtopWidget, LMDialog.nameW, NULL);
    XmStringFree(s1);
 
    LMDialog.extW = XtVaCreateManagedWidget("ext", xmTextWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, extLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS, NULL);
    RemapDeleteKey(LMDialog.extW);
    XtVaSetValues(extLbl, XmNuserData, LMDialog.extW, NULL);
    
    recogLbl = XtVaCreateManagedWidget("recogLbl", xmLabelGadgetClass, form,
    	    XmNlabelString, s1=MKSTRING(
"Recognition regular expression (applied to first 200\n\
characters of file to determine type from content)"),
    	    XmNalignment, XmALIGNMENT_BEGINNING,
    	    XmNmnemonic, 'R',
	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopOffset, H_MARGIN,
	    XmNtopWidget, LMDialog.extW, NULL);
    XmStringFree(s1);
 
    LMDialog.recogW = XtVaCreateManagedWidget("recog", xmTextWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_RIGHT,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, recogLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS, NULL);
    RemapDeleteKey(LMDialog.recogW);
    XtVaSetValues(recogLbl, XmNuserData, LMDialog.recogW, NULL);
	    
    okBtn = XtVaCreateManagedWidget("ok", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("OK"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 10,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 30,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, NULL);
    XtAddCallback(okBtn, XmNactivateCallback, lmOkCB, NULL);
    XmStringFree(s1);

    applyBtn = XtVaCreateManagedWidget("apply", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("Apply"),
    	    XmNmnemonic, 'A',
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 40,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 60,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, NULL);
    XtAddCallback(applyBtn, XmNactivateCallback, lmApplyCB, NULL);
    XmStringFree(s1);

    dismissBtn = XtVaCreateManagedWidget("dismiss",xmPushButtonWidgetClass,form,
    	    XmNlabelString, s1=XmStringCreateSimple("Dismiss"),
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, 70,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, 90,
    	    XmNbottomAttachment, XmATTACH_POSITION,
    	    XmNbottomPosition, 99, NULL);
    XtAddCallback(dismissBtn, XmNactivateCallback, lmDismissCB, NULL);
    XmStringFree(s1);

    overrideFrame = XtVaCreateManagedWidget("overrideFrame",
    	    xmFrameWidgetClass, form,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS,
	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNbottomWidget, dismissBtn,
	    XmNbottomOffset, H_MARGIN, NULL);
    overrideForm = XtVaCreateManagedWidget("overrideForm", xmFormWidgetClass,
	    overrideFrame, NULL);
#if XmVersion >= 1002
    overrideLbl = XtVaCreateManagedWidget("overrideLbl", xmLabelGadgetClass,
    	    overrideFrame,
    	    XmNlabelString, s1=XmStringCreateSimple("Override Defaults"),
	    XmNchildType, XmFRAME_TITLE_CHILD,
	    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER, NULL);
    XmStringFree(s1);
#endif
 
    delimitForm = XtVaCreateManagedWidget("overrideForm", xmFormWidgetClass,
	    overrideForm,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNtopOffset, H_MARGIN,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS, NULL);
    delimitLbl = XtVaCreateManagedWidget("delimitLbl", xmLabelGadgetClass,
    	    delimitForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Word delimiters"),
    	    XmNmnemonic, 'W',
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);
    LMDialog.delimitW = XtVaCreateManagedWidget("delimit", xmTextWidgetClass,
    	    delimitForm,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, delimitLbl,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    RemapDeleteKey(LMDialog.delimitW);
    XtVaSetValues(delimitLbl, XmNuserData, LMDialog.delimitW, NULL);

    tabForm = XtVaCreateManagedWidget("tabForm", xmFormWidgetClass,
	    overrideForm,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, delimitForm,
	    XmNtopOffset, H_MARGIN,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS, NULL);
    tabLbl = XtVaCreateManagedWidget("tabLbl", xmLabelGadgetClass, tabForm,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"Alternative hardware tab spacing"),
    	    XmNmnemonic, 't',
	    XmNleftAttachment, XmATTACH_FORM,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);
    LMDialog.tabW = XtVaCreateManagedWidget("delimit", xmTextWidgetClass,
    	    tabForm,
    	    XmNcolumns, 3,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, tabLbl,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    RemapDeleteKey(LMDialog.tabW);
    XtVaSetValues(tabLbl, XmNuserData, LMDialog.tabW, NULL);
    LMDialog.emTabW = XtVaCreateManagedWidget("delimit", xmTextWidgetClass,
    	    tabForm,
    	    XmNcolumns, 3,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNrightAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    RemapDeleteKey(LMDialog.emTabW);
    emTabLbl = XtVaCreateManagedWidget("emTabLbl", xmLabelGadgetClass, tabForm,
    	    XmNlabelString,
    	    s1=XmStringCreateSimple("Alternative emulated tab spacing"),
    	    XmNalignment, XmALIGNMENT_END, 
    	    XmNmnemonic, 'e',
	    XmNuserData, LMDialog.emTabW,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, LMDialog.tabW,
	    XmNrightAttachment, XmATTACH_WIDGET,
	    XmNrightWidget, LMDialog.emTabW,
	    XmNtopAttachment, XmATTACH_FORM,
	    XmNbottomAttachment, XmATTACH_FORM, NULL);
    XmStringFree(s1);

    indentBox = XtVaCreateManagedWidget("indentBox", xmRowColumnWidgetClass,
    	    overrideForm,
    	    XmNorientation, XmHORIZONTAL,
    	    XmNpacking, XmPACK_TIGHT,
    	    XmNradioBehavior, True,
	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, tabForm,
	    XmNtopOffset, H_MARGIN, NULL);
    LMDialog.defaultIndentW = XtVaCreateManagedWidget("defaultIndent", 
    	    xmToggleButtonWidgetClass, indentBox,
    	    XmNset, True,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Default indent style"),
    	    XmNmnemonic, 'D', NULL);
    XmStringFree(s1);
    LMDialog.noIndentW = XtVaCreateManagedWidget("noIndent", 
    	    xmToggleButtonWidgetClass, indentBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("No automatic indent"),
    	    XmNmnemonic, 'N', NULL);
    XmStringFree(s1);
    LMDialog.autoIndentW = XtVaCreateManagedWidget("autoIndent", 
    	    xmToggleButtonWidgetClass, indentBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Auto-indent"),
    	    XmNmnemonic, 'A', NULL);
    XmStringFree(s1);
    LMDialog.smartIndentW = XtVaCreateManagedWidget("smartIndent", 
    	    xmToggleButtonWidgetClass, indentBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Smart-indent"),
    	    XmNmnemonic, 'S', NULL);
    XmStringFree(s1);

    wrapBox = XtVaCreateManagedWidget("wrapBox", xmRowColumnWidgetClass,
    	    overrideForm,
    	    XmNorientation, XmHORIZONTAL,
    	    XmNpacking, XmPACK_TIGHT,
    	    XmNradioBehavior, True,
	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, LEFT_MARGIN_POS,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, indentBox,
	    XmNtopOffset, H_MARGIN,
	    XmNbottomAttachment, XmATTACH_FORM,
	    XmNbottomOffset, H_MARGIN, NULL);
    LMDialog.defaultWrapW = XtVaCreateManagedWidget("defaultWrap", 
    	    xmToggleButtonWidgetClass, wrapBox,
    	    XmNset, True,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Default wrap style"),
    	    XmNmnemonic, 'D', NULL);
    XmStringFree(s1);
    LMDialog.noWrapW = XtVaCreateManagedWidget("noWrap", 
    	    xmToggleButtonWidgetClass, wrapBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("No wrapping"),
    	    XmNmnemonic, 'N', NULL);
    XmStringFree(s1);
    LMDialog.newlineWrapW = XtVaCreateManagedWidget("newlineWrap", 
    	    xmToggleButtonWidgetClass, wrapBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Auto newline wrap"),
    	    XmNmnemonic, 'A', NULL);
    XmStringFree(s1);
    LMDialog.contWrapW = XtVaCreateManagedWidget("contWrap", 
    	    xmToggleButtonWidgetClass, wrapBox,
    	    XmNmarginHeight, 0,
    	    XmNlabelString, s1=XmStringCreateSimple("Continuous wrap"),
    	    XmNmnemonic, 'C', NULL);
    XmStringFree(s1);

    stretchForm = XtVaCreateManagedWidget("stretchForm", xmFormWidgetClass,
	    form,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, LMDialog.recogW,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, LIST_RIGHT,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, RIGHT_MARGIN_POS,
	    XmNbottomAttachment, XmATTACH_WIDGET,
	    XmNbottomWidget, overrideFrame,
	    XmNbottomOffset, H_MARGIN*2, NULL);
    
    ac = 0;
    XtSetArg(args[ac], XmNtopAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNtopOffset, H_MARGIN); ac++;
    XtSetArg(args[ac], XmNtopWidget, topLbl); ac++;
    XtSetArg(args[ac], XmNleftAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNleftPosition, LEFT_MARGIN_POS); ac++;
    XtSetArg(args[ac], XmNrightAttachment, XmATTACH_POSITION); ac++;
    XtSetArg(args[ac], XmNrightPosition, LIST_RIGHT-1); ac++;
    XtSetArg(args[ac], XmNbottomAttachment, XmATTACH_WIDGET); ac++;
    XtSetArg(args[ac], XmNbottomWidget, overrideForm); ac++;
    XtSetArg(args[ac], XmNbottomOffset, H_MARGIN*2); ac++;
    LMDialog.managedListW = CreateManagedList(form, "list", args, ac,
    	    (void **)LMDialog.languageModeList, &LMDialog.nLanguageModes,
    	    MAX_LANGUAGE_MODES, 15, lmGetDisplayedCB, NULL, lmSetDisplayedCB,
    	    NULL, lmFreeItemCB);
    AddDeleteConfirmCB(LMDialog.managedListW, lmDeleteConfirmCB, NULL);
    XtVaSetValues(topLbl, XmNuserData, LMDialog.managedListW, NULL);
    	
    /* Set initial default button */
    XtVaSetValues(form, XmNdefaultButton, okBtn, NULL);
    XtVaSetValues(form, XmNcancelButton, dismissBtn, NULL);
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);

    /* Realize all of the widgets in the new dialog */
    XtRealizeWidget(LMDialog.shell);
}

static void lmDestroyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int i;
    
    for (i=0; i<LMDialog.nLanguageModes; i++)
    	freeLanguageModeRec(LMDialog.languageModeList[i]);
    XtFree((char *)LMDialog.languageModeList);
}

static void lmOkCB(Widget w, XtPointer clientData, XtPointer callData)
{
    if (!updateLMList())
    	return;

    /* pop down and destroy the dialog */
    XtDestroyWidget(LMDialog.shell);
    LMDialog.shell = NULL;
}

static void lmApplyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    updateLMList();
}

static void lmDismissCB(Widget w, XtPointer clientData, XtPointer callData)
{
    /* pop down and destroy the dialog */
    XtDestroyWidget(LMDialog.shell);
    LMDialog.shell = NULL;
}

static int lmDeleteConfirmCB(int itemIndex, void *cbArg)
{
    int i;
    
    /* Allow duplicate names to be deleted regardless of dependencies */
    for (i=0; i<LMDialog.nLanguageModes; i++)
	if (i != itemIndex && !strcmp(LMDialog.languageModeList[i]->name,
		LMDialog.languageModeList[itemIndex]->name))
	    return True;
    
    /* don't allow deletion if data will be lost */
    if (LMHasHighlightPatterns(LMDialog.languageModeList[itemIndex]->name)) {
    	DialogF(DF_WARN, LMDialog.shell, 1,
"This language mode has syntax highlighting\n\
patterns defined.  Please delete the patterns\n\
first, in Preferences -> Default Settings ->\n\
Syntax Highlighting, before proceeding here.", "Dismiss");
	return False;
    }
    return True;
}

/*
** Apply the changes that the user has made in the language modes dialog to the
** stored language mode information for this NEdit session (the data array
** LanguageModes)
*/
static int updateLMList(void)
{
    WindowInfo *window;
    char *oldModeName, *newDelimiters;
    int i, j;
    
    /* Get the current contents of the dialog fields */
    if (!UpdateManagedList(LMDialog.managedListW, True))
    	return False;

    /* Fix up language mode indecies in all open windows (which may change
       if the currently selected mode is deleted or has changed position),
       and update word delimiters */
    for (window=WindowList; window!=NULL; window=window->next) {
	if (window->languageMode != PLAIN_LANGUAGE_MODE) {
    	    oldModeName = LanguageModes[window->languageMode]->name;
    	    window->languageMode = PLAIN_LANGUAGE_MODE;
    	    for (i=0; i<LMDialog.nLanguageModes; i++) {
    		if (!strcmp(oldModeName, LMDialog.languageModeList[i]->name)) {
    	    	    newDelimiters = LMDialog.languageModeList[i]->delimiters;
    	    	    if (newDelimiters == NULL)
    	    	    	newDelimiters = GetPrefDelimiters();
    	    	    XtVaSetValues(window->textArea, textNwordDelimiters,
    	    	    	    newDelimiters, NULL);
    	    	    for (j=0; j<window->nPanes; j++)
    	    	    	XtVaSetValues(window->textPanes[j],
    	    	    	    	textNwordDelimiters, newDelimiters, NULL);
    	    	    window->languageMode = i;
    	    	    break;
    		}
    	    }
	}
    }
    
    /* If there were any name changes, re-name dependent highlight patterns
       and fix up the weird rename-format names */
    for (i=0; i<LMDialog.nLanguageModes; i++) {
    	if (strchr(LMDialog.languageModeList[i]->name, ':') != NULL) {
    	    char *newName = strrchr(LMDialog.languageModeList[i]->name, ':')+1;
    	    *strchr(LMDialog.languageModeList[i]->name, ':') = '\0';
    	    RenameHighlightPattern(LMDialog.languageModeList[i]->name, newName);
    	    memmove(LMDialog.languageModeList[i]->name, newName,
    	    	    strlen(newName) + 1);
    	    ChangeManagedListData(LMDialog.managedListW);
    	}
    }
    
    /* Replace the old language mode list with the new one from the dialog */
    for (i=0; i<NLanguageModes; i++)
    	freeLanguageModeRec(LanguageModes[i]);
    for (i=0; i<LMDialog.nLanguageModes; i++)
    	LanguageModes[i] = copyLanguageModeRec(LMDialog.languageModeList[i]);
    NLanguageModes = LMDialog.nLanguageModes;
    
    /* Update the menus in the window menu bars */
    for (window=WindowList; window!=NULL; window=window->next)
    	updateLanguageModeSubmenu(window);
    
    /* If a syntax highlighting dialog is up, update its menu */
    UpdateLanguageModeMenu();
    
    /* Note that preferences have been changed */
    MarkPrefsChanged();

    return True;
}

static void *lmGetDisplayedCB(void *oldItem, int explicitRequest, int *abort,
    	void *cbArg)
{
    languageModeRec *lm, *oldLM = (languageModeRec *)oldItem;
    char *tempName;
    int i, nCopies, oldLen;
    
    /* If the dialog is currently displaying the "new" entry and the
       fields are empty, that's just fine */
    if (oldItem == NULL && lmDialogEmpty())
    	return NULL;
    
    /* Read the data the user has entered in the dialog fields */
    lm = readLMDialogFields(True);

    /* If there was a name change of a non-duplicate language mode, modify the
       name to the weird format of: ":old name:new name".  This signals that a
       name change is necessary in lm dependent data such as highlight
       patterns.  Duplicate language modes may be re-named at will, since no
       data will be lost due to the name change. */
    if (lm != NULL && oldLM != NULL && strcmp(oldLM->name, lm->name)) {
    	nCopies = 0;
	for (i=0; i<LMDialog.nLanguageModes; i++)
	    if (!strcmp(oldLM->name, LMDialog.languageModeList[i]->name))
		nCopies++;
	if (nCopies <= 1) {
    	    oldLen = strchr(oldLM->name, ':') == NULL ? strlen(oldLM->name) :
    	    	    strchr(oldLM->name, ':') - oldLM->name;
    	    tempName = XtMalloc(oldLen + strlen(lm->name) + 2);
    	    strncpy(tempName, oldLM->name, oldLen);
    	    sprintf(&tempName[oldLen], ":%s", lm->name);
    	    XtFree(lm->name);
    	    lm->name = tempName;
	}
    }
    
    /* If there are no problems reading the data, just return it */
    if (lm != NULL)
    	return (void *)lm;
    
    /* If there are problems, and the user didn't ask for the fields to be
       read, give more warning */
    if (!explicitRequest) {
	if (DialogF(DF_WARN, LMDialog.shell, 2,
    		"Discard incomplete entry\nfor current language mode?", "Keep",
    		"Discard") == 2) {
     	    return oldItem == NULL ? NULL :
     	    	    (void *)copyLanguageModeRec((languageModeRec *)oldItem);
	}
    }

    /* Do readLMDialogFields again without "silent" mode to display warning */
    lm = readLMDialogFields(False);
    *abort = True;
    return NULL;
}

static void lmSetDisplayedCB(void *item, void *cbArg)
{
    languageModeRec *lm = (languageModeRec *)item;
    char *extStr;

    if (item == NULL) {
    	XmTextSetString(LMDialog.nameW, "");
    	XmTextSetString(LMDialog.extW, "");
    	XmTextSetString(LMDialog.recogW, "");
    	XmTextSetString(LMDialog.delimitW, "");
    	XmTextSetString(LMDialog.tabW, "");
    	XmTextSetString(LMDialog.emTabW, "");
    	XmToggleButtonSetState(LMDialog.defaultIndentW, True, True);
    	XmToggleButtonSetState(LMDialog.defaultWrapW, True, True);
    } else {
    	XmTextSetString(LMDialog.nameW, strchr(lm->name, ':') == NULL ?
    	    	lm->name : strchr(lm->name, ':')+1);
    	extStr = createExtString(lm->extensions, lm->nExtensions);
    	XmTextSetString(LMDialog.extW, extStr);
    	XtFree(extStr);
    	XmTextSetString(LMDialog.recogW, lm->recognitionExpr);
    	XmTextSetString(LMDialog.delimitW, lm->delimiters);
    	if (lm->tabDist == DEFAULT_TAB_DIST)
    	    XmTextSetString(LMDialog.tabW, "");
    	else
    	    SetIntText(LMDialog.tabW, lm->tabDist);
    	if (lm->emTabDist == DEFAULT_EM_TAB_DIST)
    	    XmTextSetString(LMDialog.emTabW, "");
    	else
    	    SetIntText(LMDialog.emTabW, lm->emTabDist);
    	XmToggleButtonSetState(LMDialog.defaultIndentW,
    	    	lm->indentStyle == DEFAULT_INDENT, False);
    	XmToggleButtonSetState(LMDialog.noIndentW,
    	    	lm->indentStyle == NO_AUTO_INDENT, False);
    	XmToggleButtonSetState(LMDialog.autoIndentW,
    	    	lm->indentStyle == AUTO_INDENT, False);
    	XmToggleButtonSetState(LMDialog.smartIndentW,
    	    	lm->indentStyle == SMART_INDENT, False);
    	XmToggleButtonSetState(LMDialog.defaultWrapW,
    	    	lm->wrapStyle == DEFAULT_WRAP, False);
    	XmToggleButtonSetState(LMDialog.noWrapW,
    	    	lm->wrapStyle == NO_WRAP, False);
    	XmToggleButtonSetState(LMDialog.newlineWrapW,
    	    	lm->wrapStyle == NEWLINE_WRAP, False);
    	XmToggleButtonSetState(LMDialog.contWrapW,
    	    	lm->wrapStyle == CONTINUOUS_WRAP, False);
    }
}

static void lmFreeItemCB(void *item)
{
    freeLanguageModeRec((languageModeRec *)item);
}

static void freeLanguageModeRec(languageModeRec *lm)
{
    int i;
    
    XtFree(lm->name);
    if (lm->recognitionExpr != NULL)
    	XtFree(lm->recognitionExpr);
    if (lm->delimiters != NULL)
    	XtFree(lm->delimiters);
    for (i=0; i<lm->nExtensions; i++)
    	XtFree(lm->extensions[i]);
    XtFree((char *)lm->extensions);
    XtFree((char *)lm);
}

/*
** Copy a languageModeRec data structure and all of the allocated data it contains
*/
static languageModeRec *copyLanguageModeRec(languageModeRec *lm)
{
    languageModeRec *newLM;
    int i;
    
    newLM = (languageModeRec *)XtMalloc(sizeof(languageModeRec));
    newLM->name = XtMalloc(strlen(lm->name)+1);
    strcpy(newLM->name, lm->name);
    newLM->nExtensions = lm->nExtensions;
    newLM->extensions = (char **)XtMalloc(sizeof(char *) * lm->nExtensions);
    for (i=0; i<lm->nExtensions; i++) {
    	newLM->extensions[i] = XtMalloc(strlen(lm->extensions[i]) + 1);
    	strcpy(newLM->extensions[i], lm->extensions[i]);
    }
    if (lm->recognitionExpr == NULL)
    	newLM->recognitionExpr = NULL;
    else {
	newLM->recognitionExpr = XtMalloc(strlen(lm->recognitionExpr)+1);
	strcpy(newLM->recognitionExpr, lm->recognitionExpr);
    }
    if (lm->delimiters == NULL)
    	newLM->delimiters = NULL;
    else {
	newLM->delimiters = XtMalloc(strlen(lm->delimiters)+1);
	strcpy(newLM->delimiters, lm->delimiters);
    }
    newLM->wrapStyle = lm->wrapStyle;
    newLM->indentStyle = lm->indentStyle;
    newLM->tabDist = lm->tabDist;
    newLM->emTabDist = lm->emTabDist;
    return newLM;
}

/*
** Read the fields in the language modes dialog and create a languageModeRec data
** structure reflecting the current state of the selected language mode in the dialog.
** If any of the information is incorrect or missing, display a warning dialog and
** return NULL.  Passing "silent" as True, suppresses the warning dialogs.
*/
static languageModeRec *readLMDialogFields(int silent)
{
    languageModeRec *lm;
    regexp *compiledRE;
    char *compileMsg, *extStr, *extPtr;

    /* Allocate a language mode structure to return, set unread fields to
       empty so everything can be freed on errors by freeLanguageModeRec */
    lm = (languageModeRec *)XtMalloc(sizeof(languageModeRec));
    lm->nExtensions = 0;
    lm->recognitionExpr = NULL;
    lm->delimiters = NULL;

    /* read the name field */
    lm->name = ReadSymbolicFieldTextWidget(LMDialog.nameW,
    	    "language mode name", silent);
    if (lm->name == NULL) {
    	XtFree((char *)lm);
    	return NULL;
    }
    if (*lm->name == '\0') {
    	if (!silent) {
    	    DialogF(DF_WARN, LMDialog.shell, 1,
    		    "Please specify a name\nfor the language mode", "Dismiss");
    	    XmProcessTraversal(LMDialog.nameW, XmTRAVERSE_CURRENT);
    	}
    	freeLanguageModeRec(lm);
   	return NULL;
    }
    
    /* read the extension list field */
    extStr = extPtr = XmTextGetString(LMDialog.extW);
    lm->extensions = readExtensionList(&extPtr, &lm->nExtensions);
    XtFree(extStr);
    
    /* read recognition expression */
    lm->recognitionExpr = XmTextGetString(LMDialog.recogW);
    if (*lm->recognitionExpr == '\0') {
    	XtFree(lm->recognitionExpr);
    	lm->recognitionExpr = NULL;
    } else {
	compiledRE = CompileRE(lm->recognitionExpr, &compileMsg);
	if (compiledRE == NULL) {
   	    if (!silent) {
   		DialogF(DF_WARN, LMDialog.shell, 1, "Recognition expression:\n%s",
   	    		"Dismiss", compileMsg);
     		XmProcessTraversal(LMDialog.recogW, XmTRAVERSE_CURRENT);
	    }
 	    XtFree((char *)compiledRE);
 	    freeLanguageModeRec(lm);
 	    return NULL;    
	}
	XtFree((char *)compiledRE);
    }
    
    /* read tab spacing field */
    if (TextWidgetIsBlank(LMDialog.tabW))
    	lm->tabDist = DEFAULT_TAB_DIST;
    else {
    	if (GetIntTextWarn(LMDialog.tabW, &lm->tabDist, "tab spacing", False)
    	    	!= TEXT_READ_OK) {
   	    freeLanguageModeRec(lm);
    	    return NULL;
	}
	if (lm->tabDist <= 0 || lm->tabDist > 100) {
   	    if (!silent) {
		DialogF(DF_WARN, LMDialog.shell, 1, "Invalid tab spacing: %d",
	    		"Dismiss", lm->tabDist);
		XmProcessTraversal(LMDialog.tabW, XmTRAVERSE_CURRENT);
	    }
	    freeLanguageModeRec(lm);
    	    return NULL;
	}
    }
    
    /* read emulated tab field */
    if (TextWidgetIsBlank(LMDialog.emTabW))
    	lm->emTabDist = DEFAULT_EM_TAB_DIST;
    else {
    	if (GetIntTextWarn(LMDialog.emTabW, &lm->emTabDist,
    	    	"emulated tab spacing", False) != TEXT_READ_OK) {
    	    freeLanguageModeRec(lm);
    	    return NULL;
	}
	if (lm->emTabDist < 0 || lm->emTabDist > 100) {
   	    if (!silent) {
		DialogF(DF_WARN, LMDialog.shell, 1,
	    		"Invalid emulated tab spacing: %d", "Dismiss",
			lm->emTabDist);
		XmProcessTraversal(LMDialog.emTabW, XmTRAVERSE_CURRENT);
	    }
	    freeLanguageModeRec(lm);
    	    return NULL;
	}
    }
    
    /* read delimiters string */
    lm->delimiters = XmTextGetString(LMDialog.delimitW);
    if (*lm->delimiters == '\0') {
    	XtFree(lm->delimiters);
    	lm->delimiters = NULL;
    }
    
    /* read indent style */
    if (XmToggleButtonGetState(LMDialog.noIndentW))
    	 lm->indentStyle = NO_AUTO_INDENT;
    else if (XmToggleButtonGetState(LMDialog.autoIndentW))
    	 lm->indentStyle = AUTO_INDENT;
    else if (XmToggleButtonGetState(LMDialog.smartIndentW))
    	 lm->indentStyle = SMART_INDENT;
    else
    	 lm->indentStyle = DEFAULT_INDENT;
    
    /* read wrap style */
    if (XmToggleButtonGetState(LMDialog.noWrapW))
    	 lm->wrapStyle = NO_WRAP;
    else if (XmToggleButtonGetState(LMDialog.newlineWrapW))
    	 lm->wrapStyle = NEWLINE_WRAP;
    else if (XmToggleButtonGetState(LMDialog.contWrapW))
    	 lm->wrapStyle = CONTINUOUS_WRAP;
    else
    	 lm->wrapStyle = DEFAULT_WRAP;
    return lm;
}

/*
** Return True if the language mode dialog fields are blank (unchanged from the "New"
** language mode state).
*/
static int lmDialogEmpty(void)
{
    return TextWidgetIsBlank(LMDialog.nameW) &&
 	    TextWidgetIsBlank(LMDialog.extW) &&
	    TextWidgetIsBlank(LMDialog.recogW) &&
	    TextWidgetIsBlank(LMDialog.delimitW) &&
	    TextWidgetIsBlank(LMDialog.tabW) &&
	    TextWidgetIsBlank(LMDialog.emTabW) &&
	    XmToggleButtonGetState(LMDialog.defaultIndentW) &&
	    XmToggleButtonGetState(LMDialog.defaultWrapW);
}   	

/*
** Present a dialog for changing fonts (primary, and for highlighting).
*/
void ChooseFonts(WindowInfo *window, int forWindow)
{
#define MARGIN_SPACING 10
    Widget form, primaryLbl, primaryBtn, highlightLbl, italicLbl, italicBtn;
    Widget boldLbl, boldBtn, boldItalicLbl, boldItalicBtn;
    Widget primaryFrame, primaryForm, highlightFrame, highlightForm;
    Widget okBtn, applyBtn, dismissBtn;
    fontDialog *fd;
    XmString s1;
    int ac;
    Arg args[20];

    /* if the dialog is already displayed, just pop it to the top and return */
    if (window->fontDialog != NULL) {
    	RaiseShellWindow(((fontDialog *)window->fontDialog)->shell);
    	return;
    }
    
    /* Create a structure for keeping track of dialog state */
    fd = (fontDialog *)XtMalloc(sizeof(fontDialog));
    fd->window = window;
    fd->forWindow = forWindow;
    window->fontDialog = (void*)fd;
    
    /* Create a form widget in a dialog shell */
    ac = 0;
    XtSetArg(args[ac], XmNautoUnmanage, False); ac++;
    XtSetArg(args[ac], XmNresizePolicy, XmRESIZE_NONE); ac++;
    form = XmCreateFormDialog(window->shell, "choose Fonts", args, ac);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
    fd->shell = XtParent(form);
    XtVaSetValues(fd->shell, XmNtitle, "Fonts", NULL);
    AddMotifCloseCallback(XtParent(form), fontDismissCB, fd);
    XtAddCallback(form, XmNdestroyCallback, fontDestroyCB, fd);

    primaryFrame = XtVaCreateManagedWidget("primaryFrame", xmFrameWidgetClass,
    	    form,
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    primaryForm = XtVaCreateManagedWidget("primaryForm", xmFormWidgetClass,
	    primaryFrame, NULL);
#if XmVersion >= 1002
    primaryLbl = XtVaCreateManagedWidget("primaryFont", xmLabelGadgetClass,
    	    primaryFrame,
    	    XmNlabelString, s1=XmStringCreateSimple("Primary Font"),
    	    XmNmnemonic, 'P',
	    XmNchildType, XmFRAME_TITLE_CHILD,
	    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER, NULL);
    XmStringFree(s1);
#endif

    primaryBtn = XtVaCreateManagedWidget("primaryBtn",
    	    xmPushButtonWidgetClass, primaryForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Browse..."),
    	    XmNmnemonic, 'r',
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XtAddCallback(primaryBtn, XmNactivateCallback, primaryBrowseCB, fd);

    fd->primaryW = XtVaCreateManagedWidget("primary", xmTextWidgetClass,
    	    primaryForm,
    	    XmNcolumns, 70,
    	    XmNmaxLength, MAX_FONT_LEN,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, primaryBtn,
	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    RemapDeleteKey(fd->primaryW);
    XtAddCallback(fd->primaryW, XmNvalueChangedCallback,
    	    primaryModifiedCB, fd);
    XtVaSetValues(primaryLbl, XmNuserData, fd->primaryW, NULL);

    highlightFrame = XtVaCreateManagedWidget("highlightFrame",
    	    xmFrameWidgetClass, form,
	    XmNnavigationType, XmTAB_GROUP,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, primaryFrame,
	    XmNtopOffset, 20,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    highlightForm = XtVaCreateManagedWidget("highlightForm", xmFormWidgetClass,
    	    highlightFrame, NULL);
#if XmVersion >= 1002
    highlightLbl = XtVaCreateManagedWidget("highlightFonts", xmLabelGadgetClass,
    	    highlightFrame,
    	    XmNlabelString,
    	    	s1=XmStringCreateSimple("Fonts for Syntax Highlighting"),
	    XmNchildType, XmFRAME_TITLE_CHILD,
	    XmNchildHorizontalAlignment, XmALIGNMENT_CENTER, NULL);
    XmStringFree(s1);
#endif

    fd->fillW = XtVaCreateManagedWidget("fillBtn",
    	    xmPushButtonWidgetClass, highlightForm,
    	    XmNlabelString,
    	    	s1=XmStringCreateSimple("Fill Highlight Fonts from Primary"),
    	    XmNmnemonic, 'F',
    	    XmNtopAttachment, XmATTACH_POSITION,
	    XmNtopPosition, 2,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XtAddCallback(fd->fillW, XmNactivateCallback, fillFromPrimaryCB, fd);

    italicLbl = XtVaCreateManagedWidget("italicLbl", xmLabelGadgetClass,
    	    highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Italic Font"),
    	    XmNmnemonic, 'I',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, fd->fillW,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XmStringFree(s1);

    fd->italicErrW = XtVaCreateManagedWidget("italicErrLbl",
    	    xmLabelGadgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple(
    	    	"(vvv  spacing is inconsistent with primary font  vvv)"),
    	    XmNalignment, XmALIGNMENT_END,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, fd->fillW,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, italicLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    XmStringFree(s1);

    italicBtn = XtVaCreateManagedWidget("italicBtn",
    	    xmPushButtonWidgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Browse..."),
    	    XmNmnemonic, 'o',
    	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, italicLbl,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XtAddCallback(italicBtn, XmNactivateCallback, italicBrowseCB, fd);

    fd->italicW = XtVaCreateManagedWidget("italic", xmTextWidgetClass,
    	    highlightForm,
    	    XmNmaxLength, MAX_FONT_LEN,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, italicBtn,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, italicLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    RemapDeleteKey(fd->italicW);
    XtAddCallback(fd->italicW, XmNvalueChangedCallback,
    	    italicModifiedCB, fd);
    XtVaSetValues(italicLbl, XmNuserData, fd->italicW, NULL);

    boldLbl = XtVaCreateManagedWidget("boldLbl", xmLabelGadgetClass,
    	    highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Bold Font"),
    	    XmNmnemonic, 'B',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, italicBtn,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XmStringFree(s1);

    fd->boldErrW = XtVaCreateManagedWidget("boldErrLbl",
    	    xmLabelGadgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple(""),
    	    XmNalignment, XmALIGNMENT_END,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, italicBtn,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, boldLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    XmStringFree(s1);

    boldBtn = XtVaCreateManagedWidget("boldBtn",
    	    xmPushButtonWidgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Browse..."),
    	    XmNmnemonic, 'w',
    	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldLbl,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XtAddCallback(boldBtn, XmNactivateCallback, boldBrowseCB, fd);

    fd->boldW = XtVaCreateManagedWidget("bold", xmTextWidgetClass,
    	    highlightForm,
    	    XmNmaxLength, MAX_FONT_LEN,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, boldBtn,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    RemapDeleteKey(fd->boldW);
    XtAddCallback(fd->boldW, XmNvalueChangedCallback,
    	    boldModifiedCB, fd);
    XtVaSetValues(boldLbl, XmNuserData, fd->boldW, NULL);

    boldItalicLbl = XtVaCreateManagedWidget("boldItalicLbl", xmLabelGadgetClass,
    	    highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Bold Italic Font"),
    	    XmNmnemonic, 'l',
    	    XmNalignment, XmALIGNMENT_BEGINNING,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldBtn,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XmStringFree(s1);

    fd->boldItalicErrW = XtVaCreateManagedWidget("boldItalicErrLbl",
    	    xmLabelGadgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple(""),
    	    XmNalignment, XmALIGNMENT_END,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldBtn,
	    XmNtopOffset, MARGIN_SPACING,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, boldItalicLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    XmStringFree(s1);

    boldItalicBtn = XtVaCreateManagedWidget("boldItalicBtn",
    	    xmPushButtonWidgetClass, highlightForm,
    	    XmNlabelString, s1=XmStringCreateSimple("Browse..."),
    	    XmNmnemonic, 's',
    	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldItalicLbl,
	    XmNleftAttachment, XmATTACH_POSITION,
	    XmNleftPosition, 1, NULL);
    XtAddCallback(boldItalicBtn, XmNactivateCallback, boldItalicBrowseCB, fd);

    fd->boldItalicW = XtVaCreateManagedWidget("boldItalic",
    	    xmTextWidgetClass, highlightForm,
    	    XmNmaxLength, MAX_FONT_LEN,
	    XmNleftAttachment, XmATTACH_WIDGET,
	    XmNleftWidget, boldItalicBtn,
	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, boldItalicLbl,
	    XmNrightAttachment, XmATTACH_POSITION,
	    XmNrightPosition, 99, NULL);
    RemapDeleteKey(fd->boldItalicW);
    XtAddCallback(fd->boldItalicW, XmNvalueChangedCallback,
    	    boldItalicModifiedCB, fd);
    XtVaSetValues(boldItalicLbl, XmNuserData, fd->boldItalicW, NULL);

    okBtn = XtVaCreateManagedWidget("ok", xmPushButtonWidgetClass, form,
    	    XmNlabelString, s1=XmStringCreateSimple("OK"),
    	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, highlightFrame,
	    XmNtopOffset, MARGIN_SPACING,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, forWindow ? 13 : 26,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, forWindow ? 27 : 40, NULL);
    XtAddCallback(okBtn, XmNactivateCallback, fontOkCB, fd);
    XmStringFree(s1);

    if (forWindow) {
	applyBtn = XtVaCreateManagedWidget("apply",xmPushButtonWidgetClass,form,
    		XmNlabelString, s1=XmStringCreateSimple("Apply"),
    		XmNmnemonic, 'A',
    		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, highlightFrame,
		XmNtopOffset, MARGIN_SPACING,
    		XmNleftAttachment, XmATTACH_POSITION,
    		XmNleftPosition, 43,
    		XmNrightAttachment, XmATTACH_POSITION,
    		XmNrightPosition, 57, NULL);
	XtAddCallback(applyBtn, XmNactivateCallback, fontApplyCB, fd);
	XmStringFree(s1);
    }
    
    dismissBtn = XtVaCreateManagedWidget("dismiss",xmPushButtonWidgetClass,form,
    	    XmNlabelString, s1=XmStringCreateSimple("Dismiss"),
    	    XmNtopAttachment, XmATTACH_WIDGET,
	    XmNtopWidget, highlightFrame,
	    XmNtopOffset, MARGIN_SPACING,
    	    XmNleftAttachment, XmATTACH_POSITION,
    	    XmNleftPosition, forWindow ? 73 : 59,
    	    XmNrightAttachment, XmATTACH_POSITION,
    	    XmNrightPosition, forWindow ? 87 : 73, NULL);
    XtAddCallback(dismissBtn, XmNactivateCallback, fontDismissCB, fd);
    XmStringFree(s1);
 
    /* Set initial default button */
    XtVaSetValues(form, XmNdefaultButton, okBtn, NULL);
    XtVaSetValues(form, XmNcancelButton, dismissBtn, NULL);
    
    /* Set initial values */
    if (forWindow) {
	XmTextSetString(fd->primaryW, window->fontName);
	XmTextSetString(fd->boldW, window->boldFontName);
	XmTextSetString(fd->italicW, window->italicFontName);
	XmTextSetString(fd->boldItalicW, window->boldItalicFontName);
    } else {
    	XmTextSetString(fd->primaryW, GetPrefFontName());
	XmTextSetString(fd->boldW, GetPrefBoldFontName());
	XmTextSetString(fd->italicW, GetPrefItalicFontName());
	XmTextSetString(fd->boldItalicW, GetPrefBoldItalicFontName());
    }
    
    /* Handle mnemonic selection of buttons and focus to dialog */
    AddDialogMnemonicHandler(form);
    
    /* put up dialog */
    ManageDialogCenteredOnPointer(form);
}

static void fillFromPrimaryCB(Widget w, XtPointer clientData,
    	XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;
    char *primaryName, *errMsg, modifiedFontName[MAX_FONT_LEN];
    char *searchString = "(-[^-]*-[^-]*)-([^-]*)-([^-]*)-(.*)";
    char *italicReplaceString = "\\1-\\2-o-\\4";
    char *boldReplaceString = "\\1-bold-\\3-\\4";
    char *boldItalicReplaceString = "\\1-bold-o-\\4";
    regexp *compiledRE;

    /* Match the primary font agains RE pattern for font names.  If it
       doesn't match, we can't generate highlight font names, so return */
    compiledRE = CompileRE(searchString, &errMsg);
    primaryName = XmTextGetString(fd->primaryW);
    if (!ExecRE(compiledRE, primaryName, NULL, False, True, True, NULL)) {
    	XBell(XtDisplay(fd->shell), 0);
    	XtFree((char*)compiledRE);
    	XtFree(primaryName);
    	return;
    }
    
    /* Make up names for new fonts based on RE replace patterns */
    SubstituteRE(compiledRE, italicReplaceString, modifiedFontName,
    	    MAX_FONT_LEN);
    XmTextSetString(fd->italicW, modifiedFontName);
    SubstituteRE(compiledRE, boldReplaceString, modifiedFontName,
    	    MAX_FONT_LEN);
    XmTextSetString(fd->boldW, modifiedFontName);
    SubstituteRE(compiledRE, boldItalicReplaceString, modifiedFontName,
    	    MAX_FONT_LEN);
    XmTextSetString(fd->boldItalicW, modifiedFontName);
    XtFree(primaryName);
    XtFree((char*)compiledRE);
}

static void primaryModifiedCB(Widget w, XtPointer clientData,
	XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    showFontStatus(fd, fd->italicW, fd->italicErrW);
    showFontStatus(fd, fd->boldW, fd->boldErrW);
    showFontStatus(fd, fd->boldItalicW, fd->boldItalicErrW);
}
static void italicModifiedCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    showFontStatus(fd, fd->italicW, fd->italicErrW);
}
static void boldModifiedCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    showFontStatus(fd, fd->boldW, fd->boldErrW);
}
static void boldItalicModifiedCB(Widget w, XtPointer clientData,
    	XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    showFontStatus(fd, fd->boldItalicW, fd->boldItalicErrW);
}

static void primaryBrowseCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    browseFont(fd->shell, fd->primaryW, "Primary Font Selector");
}
static void italicBrowseCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    browseFont(fd->shell, fd->italicW, "Italic Font Selector");
}
static void boldBrowseCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    browseFont(fd->shell, fd->boldW, "Bold Font Selector");
}
static void boldItalicBrowseCB(Widget w, XtPointer clientData,
    	XtPointer callData)
{
   fontDialog *fd = (fontDialog *)clientData;

   browseFont(fd->shell, fd->boldItalicW, "Bold Italic Font Selector");
}

static void fontDestroyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;
    
    fd->window->fontDialog = NULL;
    XtFree((char *)fd);
}

static void fontOkCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    updateFonts(fd);

    /* pop down and destroy the dialog */
    XtDestroyWidget(fd->shell);
}

static void fontApplyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    updateFonts(fd);
}

static void fontDismissCB(Widget w, XtPointer clientData, XtPointer callData)
{
    fontDialog *fd = (fontDialog *)clientData;

    /* pop down and destroy the dialog */
    XtDestroyWidget(fd->shell);
}

/*
** Check over a font name in a text field to make sure it agrees with the
** primary font in height and spacing.
*/
static int checkFontStatus(fontDialog *fd, Widget fontTextFieldW)
{
    char *primaryName, *testName;
    XFontStruct *primaryFont, *testFont;
    Display *display = XtDisplay(fontTextFieldW);
    int primaryWidth, primaryHeight, testWidth, testHeight;
    
    /* Get width and height of the font to check.  Note the test for empty
       name: X11R6 clients freak out X11R5 servers if they ask them to load
       an empty font name, and kill the whole application! */
    testName = XmTextGetString(fontTextFieldW);
    if (testName[0] == '\0') {
    	XtFree(testName);
    	return BAD_FONT;
    }
    testFont = XLoadQueryFont(display, testName);
    if (testFont == NULL) {
    	XtFree(testName);
    	return BAD_FONT;
    }
    XtFree(testName);
    testWidth = testFont->min_bounds.width;
    testHeight = testFont->ascent + testFont->descent;
    XFreeFont(display, testFont);
    
    /* Get width and height of the primary font */
    primaryName = XmTextGetString(fd->primaryW);
    if (primaryName[0] == '\0') {
    	XtFree(primaryName);
    	return BAD_PRIMARY;
    }
    primaryFont = XLoadQueryFont(display, primaryName);
    if (primaryFont == NULL) {
    	XtFree(primaryName);
    	return BAD_PRIMARY;
    }
    XtFree(primaryName);
    primaryWidth = primaryFont->min_bounds.width;
    primaryHeight = primaryFont->ascent + primaryFont->descent;
    XFreeFont(display, primaryFont);
    
    /* Compare font information */
    if (testWidth != primaryWidth)
    	return BAD_SPACING;
    if (testHeight != primaryHeight)
    	return BAD_SIZE;
    return GOOD_FONT;
}

/*
** Update the error label for a font text field to reflect its validity and degree
** of agreement with the currently selected primary font
*/
static int showFontStatus(fontDialog *fd, Widget fontTextFieldW,
    	Widget errorLabelW)
{
    int status;
    XmString s;
    char *msg;
    
    status = checkFontStatus(fd, fontTextFieldW);
    if (status == BAD_PRIMARY)
    	msg = "(font below may not match primary font)";
    else if (status == BAD_FONT)
    	msg = "(xxx font below is invalid xxx)";
    else if (status == BAD_SIZE)
    	msg = "(height of font below does not match primary)";
    else if (status == BAD_SPACING)
    	msg = "(spacing of font below does not match primary)";
    else
    	msg = "";
    
    XtVaSetValues(errorLabelW, XmNlabelString, s=XmStringCreateSimple(msg), NULL);
    XmStringFree(s);
    return status;
}

/*
** Put up a font selector panel to set the font name in the text widget "fontTextW"
*/
static void browseFont(Widget parent, Widget fontTextW, char *title)
{
    char *origFontName, *newFontName;
    
    origFontName = XmTextGetString(fontTextW);
    newFontName = FontSel(parent, PREF_FIXED, origFontName, title);
    XtFree(origFontName);
    if (newFontName == NULL)
    	return;
    XmTextSetString(fontTextW, newFontName);
    XtFree(newFontName);
}

/*
** Accept the changes in the dialog and set the fonts regardless of errors
*/
static void updateFonts(fontDialog *fd)
{
    char *fontName, *italicName, *boldName, *boldItalicName;
    
    fontName = XmTextGetString(fd->primaryW);
    italicName = XmTextGetString(fd->italicW);
    boldName = XmTextGetString(fd->boldW);
    boldItalicName = XmTextGetString(fd->boldItalicW);
    
    if (fd->forWindow)
    	SetFonts(fd->window, fontName, italicName, boldName, boldItalicName);
    else {
    	SetPrefFont(fontName);
    	SetPrefItalicFont(italicName);
    	SetPrefBoldFont(boldName);
    	SetPrefBoldItalicFont(boldItalicName);
    }
    XtFree(fontName);
    XtFree(italicName);
    XtFree(boldName);
    XtFree(boldItalicName);
}

/*
** Change the language mode to the one indexed by "mode", reseting word
** delimiters, syntax highlighting and other mode specific parameters
*/
static void reapplyLanguageMode(WindowInfo *window, int mode, int forceDefaults)
{
    char *delimiters;
    int i, wrapMode, indentStyle, tabDist, emTabDist, highlight, oldEmTabDist;
    int wrapModeIsDef, tabDistIsDef, emTabDistIsDef, indentStyleIsDef;
    int highlightIsDef, haveHighlightPatterns, haveSmartIndentMacros;
    int oldMode = window->languageMode;
    
    /* If the mode is the same, and changes aren't being forced (as might
       happen with Save As...), don't mess with already correct settings */
    if (window->languageMode == mode && !forceDefaults)
	return;
    
    /* Change the mode name stored in the window */
    window->languageMode = mode;
    
    /* Set delimiters for all text widgets */
    if (mode == PLAIN_LANGUAGE_MODE || LanguageModes[mode]->delimiters == NULL)
    	delimiters = GetPrefDelimiters();
    else
    	delimiters = LanguageModes[mode]->delimiters;
    XtVaSetValues(window->textArea, textNwordDelimiters, delimiters, NULL);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNautoIndent, delimiters, NULL);
    
    /* Decide on desired values for language-specific parameters.  If a
       parameter was set to its default value, set it to the new default,
       otherwise, leave it alone */
    wrapModeIsDef = window->editorInfo->wrapMode == GetPrefWrap(oldMode);
    tabDistIsDef = BufGetTabDistance(window->editorInfo->buffer) == GetPrefTabDist(oldMode);
    XtVaGetValues(window->textArea, textNemulateTabs, &oldEmTabDist, NULL);
    emTabDistIsDef = oldEmTabDist == GetPrefEmTabDist(oldMode);
    indentStyleIsDef = window->editorInfo->indentStyle == GetPrefAutoIndent(oldMode) ||
	    (GetPrefAutoIndent(oldMode) == SMART_INDENT &&
	     window->editorInfo->indentStyle == AUTO_INDENT &&
	     !SmartIndentMacrosAvailable(LanguageModeName(oldMode)));
    highlightIsDef = window->highlightSyntax == GetPrefHighlightSyntax()
	    || (GetPrefHighlightSyntax() &&
		 FindPatternSet(LanguageModeName(oldMode)) == NULL);
    wrapMode = wrapModeIsDef || forceDefaults ?
    	    GetPrefWrap(mode) : window->editorInfo->wrapMode;
    tabDist = tabDistIsDef || forceDefaults ?
	    GetPrefTabDist(mode) : BufGetTabDistance(window->editorInfo->buffer);
    emTabDist = emTabDistIsDef || forceDefaults ?
	    GetPrefEmTabDist(mode) : oldEmTabDist;
    indentStyle = indentStyleIsDef || forceDefaults ?
    	    GetPrefAutoIndent(mode) : window->editorInfo->indentStyle;
    highlight = highlightIsDef || forceDefaults ? 
	    GetPrefHighlightSyntax() : window->highlightSyntax;
	     
    /* Dim/undim smart-indent and highlighting menu items depending on
       whether patterns/macros are available */
    haveHighlightPatterns = FindPatternSet(LanguageModeName(mode)) != NULL;
    haveSmartIndentMacros = SmartIndentMacrosAvailable(LanguageModeName(mode));
    XtSetSensitive(window->highlightItem, haveHighlightPatterns);
	/* Smart Indent is a buffer/file attribute and not a window 
	   specific attribute so we must update all of the cloned windows. */
    {
    WindowInfo *w;
    for(w = window->editorInfo->master; w; w = w->nextSlave) {
    	XtSetSensitive(w->smartIndentItem, haveSmartIndentMacros);
    }
    } /* var scope */
	
    /* Turn off requested options which are not available */
    highlight = haveHighlightPatterns && highlight;
    if (indentStyle == SMART_INDENT && !haveSmartIndentMacros)
	indentStyle = AUTO_INDENT;

    /* Change highlighting */
    window->highlightSyntax = highlight;
    StopHighlighting(window);
    if (highlight)
    	StartHighlighting(window, False);

    /* Force a change of smart indent macros (SetAutoIndent will re-start) */
    if (window->editorInfo->indentStyle == SMART_INDENT) {
	EndSmartIndent(window);
	window->editorInfo->indentStyle = AUTO_INDENT;
    }
    
    /* set requested wrap, indent, and tabs */
    SetAutoWrap(window, wrapMode);
    SetAutoIndent(window, indentStyle);
    setTabDist(window, tabDist);
    setEmTabDist(window, emTabDist);
    
    /* Add/remove language specific menu items */
#ifndef VMS
    UpdateShellMenu(window);
#endif
    UpdateMacroMenu(window);
    UpdateBGMenu(window);
}

/*
** Find and return the name of the appropriate languange mode for
** the file in "window".  Returns a pointer to a string, which will
** remain valid until a change is made to the language modes list.
*/
static int matchLanguageMode(WindowInfo *window)
{
    char *ext, *first200;
    int i, j, fileNameLen, extLen, beginPos, endPos, start;

    /*... look for an explicit mode statement first */
    
#if 0
    /* Do a regular expression search for the search pattern within
	** the lines specified. */
	for (i=0; i<nLanguageModes; i++) {
    	for (j=0; LanguageModes[i].languageSearchList[j].searchRE != NULL; j++) {
    		char *string;
    		int found, startPos, endPos;
    		string = BufGetLines(window->editorInfo->buffer,
    			LanguageModes[i].languageSearchList[j].startLine,
    			LanguageModes[i].languageSearchList[j].endLine);
  	    	found = SearchString(string, LanguageModes[i].languageSearchList[j].searchRE, 
  	    		SEARCH_FORWARD, SEARCH_REGEX, FALSE, 0, &startPos, &endPos);
  	    	XtFree(string);
  	    	if(found) {
  	    		fileMode = i;
  	    		break;
  	    	}
		}
		if(fileMode != -1)
			break;
	}
#endif

    /* Do a regular expression search on for recognition pattern */
    first200 = BufGetRange(window->editorInfo->buffer, 0, 200);
    for (i=0; i<NLanguageModes; i++) {
    	if (LanguageModes[i]->recognitionExpr != NULL) {
    	    if (SearchString(first200, LanguageModes[i]->recognitionExpr,
    	    	    SEARCH_FORWARD, SEARCH_REGEX, False, 0, &beginPos,
    	    	    &endPos, NULL))
    	    	break;
    	}
    }
    XtFree(first200);
	if(i<NLanguageModes)
		return i;
    
	/* Return PLAIN_LANGUAGE_MODE if the filename is not set */
	if(!window->editorInfo->filenameSet) 
    	return PLAIN_LANGUAGE_MODE;
	
    /* Look at file extension */
    fileNameLen = strlen(window->editorInfo->filename);
#ifdef VMS
    if (strchr(window->editorInfo->filename, ';') != NULL)
    	fileNameLen = strchr(window->editorInfo->filename, ';') - window->editorInfo->filename;
#else
	/* Ignore the clearcase version extended paths */
    if (strstr(window->editorInfo->filename, "@@/") != NULL)
    	fileNameLen = strstr(window->editorInfo->filename, "@@/") - window->editorInfo->filename;
#endif
    for (i=0; i<NLanguageModes; i++) {
    	for (j=0; j<LanguageModes[i]->nExtensions; j++) {
    	    ext = LanguageModes[i]->extensions[j];
    	    extLen = strlen(ext);
    	    start = fileNameLen - extLen;
    	    if (start >= 0 && !strncmp(&window->editorInfo->filename[start], ext, extLen))
    	    	return i;
    	}
    }

    /* no appropriate mode was found */
    return PLAIN_LANGUAGE_MODE;
}

static int loadLanguageModesString(char *inString)
{    
    char *errMsg, *styleName, *inPtr = inString;
    languageModeRec *lm;
    int i;

    for (;;) {
   	
	/* skip over blank space */
	inPtr += strspn(inPtr, " \t\n");
    	
	/* Allocate a language mode structure to return, set unread fields to
	   empty so everything can be freed on errors by freeLanguageModeRec */
	lm = (languageModeRec *)XtMalloc(sizeof(languageModeRec));
	lm->nExtensions = 0;
	lm->recognitionExpr = NULL;
	lm->delimiters = NULL;

	/* read language mode name */
	lm->name = ReadSymbolicField(&inPtr);
	if (lm->name == NULL) {
    	    XtFree((char *)lm);
    	    return modeError(NULL,inString,inPtr,"language mode name required");
	}
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
    	/* read list of extensions */
    	lm->extensions = readExtensionList(&inPtr,
    	    	&lm->nExtensions);
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
	/* read the recognition regular expression */
	if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
    	    lm->recognitionExpr = NULL;
    	else if (!ReadQuotedString(&inPtr, &errMsg, &lm->recognitionExpr))
    	    return modeError(lm, inString,inPtr, errMsg);
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
    	/* read the indent style */
    	styleName = ReadSymbolicField(&inPtr);
	if (styleName == NULL)
    	    lm->indentStyle = DEFAULT_INDENT;
    	else {
	    for (i=0; i<N_INDENT_STYLES; i++) {
	    	if (!strcmp(styleName, AutoIndentTypes[i])) {
	    	    lm->indentStyle = i;
	    	    break;
	    	}
	    }
	    XtFree(styleName);
	    if (i == N_INDENT_STYLES)
	    	return modeError(lm,inString,inPtr,"unrecognized indent style");
	}
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
    	/* read the wrap style */
    	styleName = ReadSymbolicField(&inPtr);
	if (styleName == NULL)
    	    lm->wrapStyle = DEFAULT_WRAP;
    	else {
	    for (i=0; i<N_WRAP_STYLES; i++) {
	    	if (!strcmp(styleName, AutoWrapTypes[i])) {
	    	    lm->wrapStyle = i;
	    	    break;
	    	}
	    }
	    XtFree(styleName);
	    if (i == N_WRAP_STYLES)
	    	return modeError(lm, inString, inPtr,"unrecognized wrap style");
	}
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
    	/* read the tab distance */
	if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
    	    lm->tabDist = DEFAULT_TAB_DIST;
    	else if (!ReadNumericField(&inPtr, &lm->tabDist))
    	    return modeError(lm, inString, inPtr, "bad tab spacing");
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
    	/* read emulated tab distance */
    	if (*inPtr == '\n' || *inPtr == '\0' || *inPtr == ':')
    	    lm->emTabDist = DEFAULT_EM_TAB_DIST;
    	else if (!ReadNumericField(&inPtr, &lm->emTabDist))
    	    return modeError(lm, inString, inPtr, "bad emulated tab spacing");
	if (!SkipDelimiter(&inPtr, &errMsg))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
	/* read the delimiters string */
	if (*inPtr == '\n' || *inPtr == '\0')
    	    lm->delimiters = NULL;
    	else if (!ReadQuotedString(&inPtr, &errMsg, &lm->delimiters))
    	    return modeError(lm, inString, inPtr, errMsg);
    	
   	/* pattern set was read correctly, add/replace it in the list */
   	for (i=0; i<NLanguageModes; i++) {
	    if (!strcmp(LanguageModes[i]->name, lm->name)) {
		freeLanguageModeRec(LanguageModes[i]);
		LanguageModes[i] = lm;
		break;
	    }
	}
	if (i == NLanguageModes) {
	    LanguageModes[NLanguageModes++] = lm;
   	    if (NLanguageModes > MAX_LANGUAGE_MODES)
   		return modeError(NULL, inString, inPtr,
   	    		"maximum allowable number of language modes exceeded");
	}
    	
    	/* if the string ends here, we're done */
   	inPtr += strspn(inPtr, " \t\n");
    	if (*inPtr == '\0')
    	    return True;
    }
}

static char *writeLanguageModesString(void)
{
    int i;
    char *outStr, *escapedStr, *str, numBuf[25];
    textBuffer *outBuf;
    
    outBuf = BufCreate();
    for (i=0; i<NLanguageModes; i++) {
    	BufInsert(outBuf, outBuf->length, "\t");
    	BufInsert(outBuf, outBuf->length, LanguageModes[i]->name);
    	BufInsert(outBuf, outBuf->length, ":");
    	BufInsert(outBuf, outBuf->length, str = createExtString(
    	    	LanguageModes[i]->extensions, LanguageModes[i]->nExtensions));
    	XtFree(str);
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->recognitionExpr != NULL) {
    	    BufInsert(outBuf, outBuf->length,
    	    	    str=MakeQuotedString(LanguageModes[i]->recognitionExpr));
    	    XtFree(str);
    	}
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->indentStyle != DEFAULT_INDENT)
    	    BufInsert(outBuf, outBuf->length,
    	    	    AutoIndentTypes[LanguageModes[i]->indentStyle]);
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->wrapStyle != DEFAULT_WRAP)
    	    BufInsert(outBuf, outBuf->length,
    	    	    AutoWrapTypes[LanguageModes[i]->wrapStyle]);
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->tabDist != DEFAULT_TAB_DIST) {
    	    sprintf(numBuf, "%d", LanguageModes[i]->tabDist);
    	    BufInsert(outBuf, outBuf->length, numBuf);
    	}
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->emTabDist != DEFAULT_EM_TAB_DIST) {
    	    sprintf(numBuf, "%d", LanguageModes[i]->emTabDist);
    	    BufInsert(outBuf, outBuf->length, numBuf);
    	}
    	BufInsert(outBuf, outBuf->length, ":");
    	if (LanguageModes[i]->delimiters != NULL) {
    	    BufInsert(outBuf, outBuf->length,
    	    	    str=MakeQuotedString(LanguageModes[i]->delimiters));
    	    XtFree(str);
    	}
    	BufInsert(outBuf, outBuf->length, "\n");
    }
    
    /* Get the output, and lop off the trailing newline */
    outStr = BufGetRange(outBuf, 0, outBuf->length - 1);
    BufFree(outBuf);
    escapedStr = EscapeSensitiveChars(outStr);
    XtFree(outStr);
    return escapedStr;
}

static char *createExtString(char **extensions, int nExtensions)
{
    int e, length = 1;
    char *outStr, *outPtr;

    for (e=0; e<nExtensions; e++)
    	length += strlen(extensions[e]) + 1;
    outStr = outPtr = XtMalloc(length);
    for (e=0; e<nExtensions; e++) {
    	strcpy(outPtr, extensions[e]);
    	outPtr += strlen(extensions[e]);
    	*outPtr++ = ' ';
    }
    if (nExtensions == 0)
    	*outPtr = '\0';
    else
    	*(outPtr-1) = '\0';
    return outStr;
}

static char **readExtensionList(char **inPtr, int *nExtensions)
{
    char *extensionList[MAX_FILE_EXTENSIONS];
    char **retList, *strStart;
    int i, len;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    for (i=0; i<MAX_FILE_EXTENSIONS && **inPtr!=':' && **inPtr!='\0'; i++) {
    	*inPtr += strspn(*inPtr, " \t");
	strStart = *inPtr;
	while (**inPtr!=' ' && **inPtr!='\t' && **inPtr!=':' && **inPtr!='\0')
	    (*inPtr)++;
    	len = *inPtr - strStart;
    	extensionList[i] = XtMalloc(len + 1);
    	strncpy(extensionList[i], strStart, len);
    	extensionList[i][len] = '\0';
    }
    *nExtensions = i;
    if (i == 0)
    	return NULL;
    retList = (char **)XtMalloc(sizeof(char *) * i);
    memcpy(retList, extensionList, sizeof(char *) * i);
    return retList;
}

int ReadNumericField(char **inPtr, int *value)
{
    int charsRead;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    if (sscanf(*inPtr, "%d%n", value, &charsRead) != 1)
    	return False;
    *inPtr += charsRead;
    return True;
}

/*
** Parse a symbolic field, skipping initial and trailing whitespace,
** stops on first invalid character or end of string.  Valid characters
** are letters, numbers, _, -, +, $, #, and internal whitespace.  Internal
** whitespace is compressed to single space characters.
*/
char *ReadSymbolicField(char **inPtr)
{
    char *outStr, *outPtr, *strStart, *strPtr;
    int len;
    
    /* skip over initial blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    /* Find the first invalid character or end of string to know how
       much memory to allocate for the returned string */
    strStart = *inPtr;
    while (isalnum(**inPtr) || **inPtr=='_' || **inPtr=='-' ||  **inPtr=='+' ||
    	    **inPtr=='$' || **inPtr=='#' || **inPtr==' ' || **inPtr=='\t')
    	(*inPtr)++;
    len = *inPtr - strStart;
    if (len == 0)
    	return NULL;
    outStr = outPtr = XtMalloc(len + 1);
    
    /* Copy the string, compressing internal whitespace to a single space */
    strPtr = strStart;
    while (strPtr - strStart < len) {
    	if (*strPtr == ' ' || *strPtr == '\t') {
    	    strPtr += strspn(strPtr, " \t");
    	    *outPtr++ = ' ';
    	} else
    	    *outPtr++ = *strPtr++;
    }
    
    /* If there's space on the end, take it back off */
    if (outPtr > outStr && *(outPtr-1) == ' ')
    	outPtr--;
    if (outPtr == outStr) {
    	XtFree(outStr);
    	return NULL;
    }
    *outPtr = '\0';
    return outStr;
}

/*
** parse an individual quoted string.  Anything between
** double quotes is acceptable, quote characters can be escaped by "".
** Returns allocated string "string" containing
** argument minus quotes.  If not successful, returns False with
** (statically allocated) message in "errMsg".
*/
int ReadQuotedString(char **inPtr, char **errMsg, char **string)
{
    char *outPtr, *c;
    
    /* skip over blank space */
    *inPtr += strspn(*inPtr, " \t");
    
    /* look for initial quote */
    if (**inPtr != '\"') {
    	*errMsg = "expecting quoted string";
    	return False;
    }
    (*inPtr)++;
    
    /* calculate max length and allocate returned string */
    for (c= *inPtr; ; c++) {
    	if (*c == '\0') {
    	    *errMsg = "string not terminated";
    	    return False;
    	} else if (*c == '\"') {
    	    if (*(c+1) == '\"')
    	    	c++;
    	    else
    	    	break;
    	}
    }
    
    /* copy string up to end quote, transforming escaped quotes into quotes */
    *string = XtMalloc(c - *inPtr + 1);
    outPtr = *string;
    while (True) {
    	if (**inPtr == '\"') {
    	    if (*(*inPtr+1) == '\"')
    	    	(*inPtr)++;
    	    else
    	    	break;
    	}
    	*outPtr++ = *(*inPtr)++;
    }
    *outPtr = '\0';

    /* skip end quote */
    (*inPtr)++;
    return True;
}

/*
** Replace characters which the X resource file reader considers control
** characters, such that a string will read back as it appears in "string".
** (So far, newline characters are replaced with with \n\<newline> and
** backslashes with \\.  This has not been tested exhaustively, and
** probably should be.  It would certainly be more asthetic if other
** control characters were replaced as well).
**
** Returns an allocated string which must be freed by the caller with XtFree.
*/
char *EscapeSensitiveChars(char *string)
{
    char *c, *outStr, *outPtr;
    int length = 0;

    /* calculate length and allocate returned string */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '\\')
    	    length++;
    	else if (*c == '\n')
    	    length += 3;
    	length++;
    }
    outStr = XtMalloc(length + 1);
    outPtr = outStr;
    
    /* add backslashes */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '\\')
    	    *outPtr++ = '\\';
    	else if (*c == '\n') {
    	    *outPtr++ = '\\';
    	    *outPtr++ = 'n';
    	    *outPtr++ = '\\';
    	}
    	*outPtr++ = *c;
    }
    *outPtr = '\0';
    return outStr;
}

/*
** Adds double quotes around a string and escape existing double quote
** characters with two double quotes.  Enables the string to be read back
** by ReadQuotedString.
*/
char *MakeQuotedString(char *string)
{
    char *c, *outStr, *outPtr;
    int length = 0;

    /* calculate length and allocate returned string */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '\"')
    	    length++;
    	length++;
    }
    outStr = XtMalloc(length + 3);
    outPtr = outStr;
    
    /* add starting quote */
    *outPtr++ = '\"';
    
    /* copy string, escaping quotes with "" */
    for (c=string; *c!='\0'; c++) {
    	if (*c == '\"')
    	    *outPtr++ = '\"';
    	*outPtr++ = *c;
    }
    
    /* add ending quote */
    *outPtr++ = '\"';

    /* terminate string and return */
    *outPtr = '\0';
    return outStr;
}

/*
** Read a dialog text field containing a symbolic name (language mode names,
** style names, highlight pattern names, colors, and fonts), clean the
** entered text of leading and trailing whitespace, compress all
** internal whitespace to one space character, and check it over for
** colons, which interfere with the preferences file reader/writer syntax.
** Returns NULL on error, and puts up a dialog if silent is False.  Returns
** an empty string if the text field is blank.
*/
char *ReadSymbolicFieldTextWidget(Widget textW, char *fieldName, int silent)
{
    char *string, *stringPtr, *parsedString;
    
    /* read from the text widget */
    string = stringPtr = XmTextGetString(textW);
    
    /* parse it with the same routine used to read symbolic fields from
       files.  If the string is not read entirely, there are invalid
       characters, so warn the user if not in silent mode. */
    parsedString = ReadSymbolicField(&stringPtr);
    if (*stringPtr != '\0') {
    	if (!silent) {
    	    *(stringPtr+1) = '\0';
    	    DialogF(DF_WARN, textW, 1,"Invalid character \"%s\" in %s",
    	    	    "Dismiss", stringPtr, fieldName);
    	    XmProcessTraversal(textW, XmTRAVERSE_CURRENT);
    	}
    	XtFree(string);
    	if (parsedString != NULL)
    	    XtFree(parsedString);
    	return NULL;
    }
    XtFree(string);
    if (parsedString == NULL) {
    	parsedString = XtMalloc(1);
    	*parsedString = '\0';
    }
    return parsedString;
}

/*
** Create a pulldown menu pane with the names of the current language modes.
** XmNuserData for each item contains the language mode name.
*/
Widget CreateLanguageModeMenu(Widget parent, XtCallbackProc cbProc, void *cbArg)
{
    Widget menu, btn;
    int i;
    XmString s1;

    menu = XmCreatePulldownMenu(parent, "languageModes", NULL, 0);
    for (i=0; i<NLanguageModes; i++) {
        btn = XtVaCreateManagedWidget("languageMode", xmPushButtonGadgetClass,
        	menu,
        	XmNlabelString, s1=XmStringCreateSimple(LanguageModes[i]->name),
		XmNmarginHeight, 0,
    		XmNuserData, (void *)LanguageModes[i]->name, NULL);
        XmStringFree(s1);
	XtAddCallback(btn, XmNactivateCallback, cbProc, cbArg);
    }
    return menu;
}

/*
** Set the language mode menu in option menu "optMenu" to
** show a particular language mode
*/
void SetLangModeMenu(Widget optMenu, char *modeName)
{
    int i;
    Cardinal nItems;
    WidgetList items;
    Widget pulldown, selectedItem;
    char *itemName;

    XtVaGetValues(optMenu, XmNsubMenuId, &pulldown, NULL);
    XtVaGetValues(pulldown, XmNchildren, &items, XmNnumChildren, &nItems, NULL);
    if (nItems == 0)
    	return;
    selectedItem = items[0];
    for (i=0; i<nItems; i++) {
    	XtVaGetValues(items[i], XmNuserData, &itemName, NULL);
    	if (!strcmp(itemName, modeName)) {
    	    selectedItem = items[i];
    	    break;
    	}
    }
    XtVaSetValues(optMenu, XmNmenuHistory, selectedItem,NULL);
}

/*
** Create a submenu for chosing language mode for the current window.
*/
Widget CreateLanguageModeSubMenu(WindowInfo *window, Widget parent, char *name,
    	char *label, char mnemonic)
{
    XmString s1;

    window->langModeCascade = XtVaCreateManagedWidget(name,
    	    xmCascadeButtonGadgetClass, parent, XmNlabelString,
    	    s1=XmStringCreateSimple(label), XmNmnemonic, mnemonic,
    	    XmNsubMenuId, NULL, NULL);
    XmStringFree(s1);
    updateLanguageModeSubmenu(window);
    return window->langModeCascade;
}

/*
** Re-build the language mode sub-menu using the current data stored
** in the master list: LanguageModes.
*/
static void updateLanguageModeSubmenu(WindowInfo *window)
{
    int i;
    XmString s1;
    Widget menu, btn;
    Arg args[1] = {{XmNradioBehavior, True}};
    
    /* Destroy and re-create the menu pane */
    XtVaGetValues(window->langModeCascade, XmNsubMenuId, &menu, NULL);
    if (menu != NULL)
    	XtDestroyWidget(menu);
    menu = XmCreatePulldownMenu(XtParent(window->langModeCascade),
    	    "languageModes", args, 1);
    btn = XtVaCreateManagedWidget("languageMode",
            xmToggleButtonGadgetClass, menu,
            XmNlabelString, s1=XmStringCreateSimple("Plain"),
    	    XmNuserData, (void *)PLAIN_LANGUAGE_MODE,
    	    XmNset, window->languageMode==PLAIN_LANGUAGE_MODE, NULL);
    XmStringFree(s1);
    XtAddCallback(btn, XmNvalueChangedCallback, setLangModeCB, window);
    for (i=0; i<NLanguageModes; i++) {
        btn = XtVaCreateManagedWidget("languageMode",
            	xmToggleButtonGadgetClass, menu,
            	XmNlabelString, s1=XmStringCreateSimple(LanguageModes[i]->name),
 	    	XmNmarginHeight, 0,
   		XmNuserData, (XtPointer)i,
    		XmNset, window->languageMode==i, NULL);
        XmStringFree(s1);
	XtAddCallback(btn, XmNvalueChangedCallback, setLangModeCB, window);
    }
    XtVaSetValues(window->langModeCascade, XmNsubMenuId, menu, NULL);
}

static void setLangModeCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    XtArgVal mode;
    
    if (!XmToggleButtonGetState(w))
    	return;
    	
    /* get name of language mode stored in userData field of menu item */
    XtVaGetValues(w, XmNuserData, &mode, NULL);
    
    /* If the mode didn't change, do nothing */
    if (window->languageMode == (int)mode)
    	return;
    
    /* redo syntax highlighting word delimiters, etc. */
    reapplyLanguageMode(window, (int)mode, False);
}

/*
** Skip a delimiter and it's surrounding whitespace
*/
int SkipDelimiter(char **inPtr, char **errMsg)
{
    *inPtr += strspn(*inPtr, " \t");
    if (**inPtr != ':') {
    	*errMsg = "syntax error";
    	return False;
    }
    (*inPtr)++;
    *inPtr += strspn(*inPtr, " \t");
    return True;
}

/*
** Short-hand error processing for language mode parsing errors, frees
** lm (if non-null), prints a formatted message explaining where the
** error is, and returns False;
*/
static int modeError(languageModeRec *lm, char *stringStart, char *stoppedAt,
	char *message)
{
    if (lm != NULL)
    	freeLanguageModeRec(lm);
    return ParseError(NULL, stringStart, stoppedAt,
    	    "language mode specification", message);
}

/*
** Report parsing errors in resource strings or macros, formatted nicely so
** the user can tell where things became botched.  Errors can be sent either
** to stderr, or displayed in a dialog.  For stderr, pass toDialog as NULL.
** For a dialog, pass the dialog parent in toDialog.
*/
int ParseError(Widget toDialog, char *stringStart, char *stoppedAt,
	char *errorIn, char *message)
{
    int len, nNonWhite = 0;
    char *c, *errorLine;
    
    for (c=stoppedAt; c>=stringStart; c--) {
    	if (c == stringStart)
    	    break;
    	else if (*c == '\n' && nNonWhite >= 5)
    	    break;
    	else if (*c != ' ' && *c != '\t')
    	    nNonWhite++;
    }
    len = stoppedAt - c + (*stoppedAt == '\0' ? 0 : 1);
    errorLine = XtMalloc(len+4);
    strncpy(errorLine, c, len);
    errorLine[len++] = '<';
    errorLine[len++] = '=';
    errorLine[len++] = '=';
    errorLine[len] = '\0';
    if (toDialog == NULL)
    	fprintf(stderr, "NEdit: %s in %s:\n%s\n", message, errorIn, errorLine);
    else
    	DialogF(DF_WARN, toDialog, 1, "%s in %s:\n%s", "Dismiss", message,
    	    	errorIn, errorLine);
    XtFree(errorLine);
    return False;
}

/*
** Make a new copy of a string, if NULL, return NULL
*/
char *CopyAllocatedString(char *string)
{
    char *newString;
    
    if (string == NULL)
    	return NULL;
    newString = XtMalloc(strlen(string)+1);
    strcpy(newString, string);
    return newString;
}

/*
** Compare two strings which may be NULL
*/
int AllocatedStringsDiffer(char *s1, char *s2)
{
    if (s1 == NULL && s2 == NULL)
    	return False;
    if (s1 == NULL || s2 == NULL)
    	return True;
    return strcmp(s1, s2);
}

#ifdef SGI_CUSTOM
/*
** Present the user a dialog for specifying whether or not a short
** menu mode preference should be applied toward the default setting.
** Return False (function value) if operation was canceled, return True
** in setDefault if requested to reset the default value.
*/
static int shortPrefToDefault(Widget parent, char *settingName, int *setDefault)
{
    char msg[100] = "";
    
    if (!GetPrefShortMenus()) {
    	*setDefault = False;
    	return True;
    }
    
    sprintf(msg, "%s\nSave as default for future windows as well?",settingName);
    switch(DialogF (DF_QUES, parent, 3, msg, "Yes", "No", "Cancel")) {
        case 1: /* yes */
            *setDefault = True;
            return True;
        case 2: /* no */
            *setDefault = False;
            return True;
        case 3: /* cancel */
            return False;
    }
    return False; /* not reached */
}
#endif
