/*******************************************************************************
*									       *
* nedit.c -- Nirvana Editor main program				       *
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
* May 10, 1991								       *
*									       *
* Written by Mark Edel							       *
*									       *
* Modifications:							       *
*									       *
*	8/18/93 - Mark Edel & Joy Kyriakopulos - Ported to VMS		       *
*									       *
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
# include <windows.h>
#endif
#ifdef HAVE_X11_XLOCALE_H
#include <X11/Xlocale.h>
#else
#include <locale.h>
#endif
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>
#if XmVersion >= 1002
#include <Xm/RepType.h>
#endif
#ifdef _WIN32
# include <X11/XLIBXTRA.h>
#endif
#ifdef VMS
#include <rmsdef.h>
#include "../util/VMSparam.h"
#include "../util/VMSUtils.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include "../util/misc.h"
#include "../util/printUtils.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "textBuf.h"
#include "nedit.h"
#include "file.h"
#include "preferences.h"
#include "regularExp.h"
#include "selection.h"
#include "tags.h"
#include "menu.h"
#include "macro.h"
#include "server_common.h"
#include "server.h"
#include "interpret.h"
#include "parse.h"

static void nextArg(int argc, char **argv, int *argIndex);
static int checkDoMacroArg(char *macro);

WindowInfo *WindowList = NULL;
Display *TheDisplay;
int isServer = FALSE;
#ifdef VMS
#if defined(ENABLE_OPEN_SELECTED_GLOBBING)
char BadFilenameChars[] = "\n \t(){}";
#else
char BadFilenameChars[] = "\n \t*?(){}";
#endif
#else
# ifdef _WIN32
#if defined(ENABLE_OPEN_SELECTED_GLOBBING)
char BadFilenameChars[] = "\n\r\t(){}";
#else
char BadFilenameChars[] = "\n\r\t*?()[]{}";
#endif
#else
#if defined(ENABLE_OPEN_SELECTED_GLOBBING)
char BadFilenameChars[] = "\n \t(){}";
#else
char BadFilenameChars[] = "\n \t*?()[]{}";
#endif
#endif
#endif /* VMS */

static char *fallbackResources[] = {
    "*menuBar.marginHeight: 1",
    "*pane.sashHeight: 11",
    "*pane.sashWidth: 11",
    "*text.selectionArrayCount: 3",
    "*fontList:-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*",
    "*XmList.fontList:-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*",
    "*XmText.fontList:-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*",
    "*XmTextField.fontList:-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*",
    "*background: #b3b3b3",
    "*foreground: black",
    "*mainForm.spacing: 0",
    "*mainForm.marginWidth: 0",
    "*mainForm.marginHeight: 0",
    "*statsLine.shadowThickness: 1",
    "*statsLine.marginHeight: 1",
    "*iSearchLabel.labelString: Search:",
    "*iSearchText.shadowThickness: 1",
    "*iSearchText.marginHeight: 1",
    "*iSearchText.translations: #override \
Shift<Key>Return: activate()\n",
    "*NEditText*background: #e5e5e5",
    "*NEditText*foreground: black",
    "*NEditText*highlightBackground: red",
    "*NEditText*highlightForeground: black",
    "*NEditText*font: -adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*",
    "*XmText*foreground: black",
    "*XmText*background: #cccccc",
    "*XmText.translations: #override \
Ctrl~Alt~Meta<KeyPress>v: paste-clipboard()\n\
Ctrl~Alt~Meta<KeyPress>c: copy-clipboard()\n\
Ctrl~Alt~Meta<KeyPress>x: cut-clipboard()\n",
    "*XmList*foreground: black",
    "*XmList*background: #cccccc",
    "*XmTextField*background: #cccccc",
    "*XmTextField*foreground: black",
	"*.multiClickTime: 350",
    "*fileMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*editMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*searchMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*preferencesMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*windowsMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*shellMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*macroMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*helpMenu.tearOffModel: XmTEAR_OFF_ENABLED",
    "*fileMenu.mnemonic: F",
    "*fileMenu.new.accelerator: Ctrl<Key>n",
    "*fileMenu.new.acceleratorText: Ctrl+N",
    "*fileMenu.open.accelerator: Ctrl<Key>o",
    "*fileMenu.open.acceleratorText: Ctrl+O",
    "*fileMenu.openSelected.accelerator: Ctrl<Key>y",
    "*fileMenu.openSelected.acceleratorText: Ctrl+Y",
    "*fileMenu.close.accelerator: Ctrl<Key>w",
    "*fileMenu.close.acceleratorText: Ctrl+W",
    "*fileMenu.save.accelerator: Ctrl<Key>s",
    "*fileMenu.save.acceleratorText: Ctrl+S",
    "*fileMenu.includeFile.accelerator: Ctrl<Key>i",
    "*fileMenu.includeFile.acceleratorText: Ctrl+I",
    "*fileMenu.print.accelerator: Ctrl<Key>p",
    "*fileMenu.print.acceleratorText: Ctrl+P",
    "*fileMenu.exit.accelerator: Ctrl<Key>q",
    "*fileMenu.exit.acceleratorText: Ctrl+Q",
    "*editMenu.mnemonic: E",
    "*editMenu.undo.accelerator: Ctrl<Key>z",
    "*editMenu.undo.acceleratorText: Ctrl+Z",
    "*editMenu.redo.accelerator: Shift Ctrl<Key>z",
    "*editMenu.redo.acceleratorText: Shift+Ctrl+Z",
    "*editMenu.cut.accelerator: Ctrl<Key>x",
    "*editMenu.cut.acceleratorText: Ctrl+X",
    "*editMenu.copy.accelerator: Ctrl<Key>c",
    "*editMenu.copy.acceleratorText: Ctrl+C",
    "*editMenu.paste.accelerator: Ctrl<Key>v",
    "*editMenu.paste.acceleratorText: Ctrl+V",
    "*editMenu.pasteColumn.accelerator: Shift Ctrl<Key>v",
    "*editMenu.pasteColumn.acceleratorText: Ctrl+Shift+V",
    "*editMenu.delete.acceleratorText: Del",
    "*editMenu.selectAll.accelerator: Ctrl<Key>a",
    "*editMenu.selectAll.acceleratorText: Ctrl+A",
    "*editMenu.shiftLeft.accelerator: Ctrl<Key>9",
    "*editMenu.shiftLeft.acceleratorText: [Shift]Ctrl+9",
    "*editMenu.shiftLeftShift.accelerator: Shift Ctrl<Key>9",
    "*editMenu.shiftRight.accelerator: Ctrl<Key>0",
    "*editMenu.shiftRight.acceleratorText: [Shift]Ctrl+0",
    "*editMenu.shiftRightShift.accelerator: Shift Ctrl<Key>0",
    "*editMenu.upperCase.accelerator: Ctrl<Key>6",
    "*editMenu.upperCase.acceleratorText: Ctrl+6",
    "*editMenu.lowerCase.accelerator: Shift Ctrl<Key>6",
    "*editMenu.lowerCase.acceleratorText: Shift+Ctrl+6",
    "*editMenu.fillParagraph.accelerator: Ctrl<Key>j",
    "*editMenu.fillParagraph.acceleratorText: Ctrl+J",
    "*editMenu.insertFormFeed.accelerator: Alt Ctrl<Key>l",
    "*editMenu.insertFormFeed.acceleratorText: Alt+Ctrl+L",
    "*editMenu.insControlCode.accelerator: Alt<Key>i",
    "*editMenu.insControlCode.acceleratorText: Alt+I",
    "*searchMenu.mnemonic: S",
    "*searchMenu.find.accelerator: Ctrl<Key>f",
    "*searchMenu.find.acceleratorText: [Shift]Ctrl+F",
    "*searchMenu.findShift.accelerator: Shift Ctrl<Key>f",
    "*searchMenu.findIncremental.accelerator: Ctrl<Key>t",
    "*searchMenu.findIncremental.acceleratorText: [Shift]Ctrl+T",
    "*searchMenu.findIncrementalShift.accelerator: Shift Ctrl<Key>t",
    "*searchMenu.findReplace.accelerator: Ctrl<Key>r",
    "*searchMenu.findReplace.acceleratorText: [Shift]Ctrl+R",
    "*searchMenu.findReplaceShift.accelerator: Shift Ctrl<Key>r",
    "*searchMenu.findAgain.accelerator: Ctrl<Key>g",
    "*searchMenu.findAgain.acceleratorText: [Shift]Ctrl+G",
    "*searchMenu.findAgainShift.accelerator: Shift Ctrl<Key>g",
    "*searchMenu.findSelection.accelerator: Ctrl<Key>h",
    "*searchMenu.findSelection.acceleratorText: [Shift]Ctrl+H",
    "*searchMenu.findSelectionShift.accelerator: Shift Ctrl<Key>h",
    "*searchMenu.replace.accelerator: Ctrl<Key>r",
    "*searchMenu.replace.acceleratorText: [Shift]Ctrl+R",
    "*searchMenu.replaceShift.accelerator: Shift Ctrl<Key>r",
    "*searchMenu.replaceAgain.accelerator: Alt<Key>t",
    "*searchMenu.replaceAgain.acceleratorText: [Shift]Alt+T",
    "*searchMenu.replaceAgainShift.accelerator: Shift Alt<Key>t",
    "*searchMenu.replaceFindAgain.accelerator: ",
    "*searchMenu.replaceFindAgain.acceleratorText: ",
    "*searchMenu.replaceFindAgainShift.accelerator: ",
    "*searchMenu.gotoLineNumber.accelerator: Ctrl<Key>l",
    "*searchMenu.gotoLineNumber.acceleratorText: Ctrl+L",
    "*searchMenu.gotoSelected.accelerator: Ctrl<Key>e",
    "*searchMenu.gotoSelected.acceleratorText: Ctrl+E",
    "*searchMenu.mark.accelerator: Alt<Key>m",
    "*searchMenu.mark.acceleratorText: Alt+M a-z",
    "*searchMenu.gotoMark.accelerator: Alt<Key>g",
    "*searchMenu.gotoMark.acceleratorText: [Shift]Alt+G a-z",
    "*searchMenu.gotoMarkShift.accelerator: Shift Alt<Key>g",
    "*searchMenu.match.accelerator: Shift Ctrl<Key>m",
    "*searchMenu.match.acceleratorText: Shift+Ctrl+M",
    "*searchMenu.gotoMatching.accelerator: Ctrl<Key>m",
    "*searchMenu.gotoMatching.acceleratorText: Ctrl+M",
    "*searchMenu.findDefinition.accelerator: Ctrl<Key>d",
    "*searchMenu.findDefinition.acceleratorText: Ctrl+D",
    "*preferencesMenu.mnemonic: P",
    "*preferencesMenu.statisticsLine.accelerator: Alt<Key>a",
    "*preferencesMenu.statisticsLine.acceleratorText: Alt+A",
    "*preferencesMenu.overtype.accelerator: Ctrl<Key>b",
    "*preferencesMenu.overtype.acceleratorText: Ctrl+B",
    "*shellMenu.mnemonic: l",
    "*shellMenu.filterSelection.accelerator: Alt<Key>r",
    "*shellMenu.filterSelection.acceleratorText: Alt+R",
    "*shellMenu.executeCommand.accelerator: Alt<Key>x",
    "*shellMenu.executeCommand.acceleratorText: Alt+X",
    "*shellMenu.executeCommandLine.accelerator: <Key>KP_Enter",
    "*shellMenu.executeCommandLine.acceleratorText: KP Enter",
    "*shellMenu.cancelShellCommand.accelerator: Ctrl<Key>period",
    "*shellMenu.cancelShellCommand.acceleratorText: Ctrl+.",
    "*macroMenu.mnemonic: c",
    "*macroMenu.learnKeystrokes.accelerator: Alt<Key>k",
    "*macroMenu.learnKeystrokes.acceleratorText: Alt+K",
    "*macroMenu.finishLearn.accelerator: Alt<Key>k",
    "*macroMenu.finishLearn.acceleratorText: Alt+K",
    "*macroMenu.cancelLearn.accelerator: Ctrl<Key>period",
    "*macroMenu.cancelLearn.acceleratorText: Ctrl+.",
    "*macroMenu.replayKeystrokes.accelerator: Ctrl<Key>k",
    "*macroMenu.replayKeystrokes.acceleratorText: Ctrl+K",
    "*macroMenu.repeat.accelerator: Ctrl<Key>comma",
    "*macroMenu.repeat.acceleratorText: Ctrl+,",
    "*windowsMenu.mnemonic: W",
    "*windowsMenu.splitWindow.accelerator: Ctrl<Key>2",
    "*windowsMenu.splitWindow.acceleratorText: Ctrl+2",
    "*windowsMenu.cloneWindow.accelerator: Ctrl<Key>3",
    "*windowsMenu.cloneWindow.acceleratorText: Ctrl+3",
    "*windowsMenu.closePane.accelerator: Ctrl<Key>1",
    "*windowsMenu.closePane.acceleratorText: Ctrl+1",
    "*helpMenu.mnemonic: H",
    "*iSearchCaseToggle.labelString: Case",
    "*iSearchRegExpToggle.labelString: RegEx",
    0
};

static char cmdLineHelp[] =
#ifndef VMS
"Usage:  nedit [-read] [-create] [-line n | +n] [-server] [-do command]\n\
	       [-tags file] [-tabs n] [-wrap] [-nowrap] [-autoindent]\n\
	       [-noautoindent] [-autosave] [-noautosave] [-rows n]\n\
	       [-columns n] [-font font] [-display [host]:server[.screen]\n\
	       [-geometry geometry] [-xrm resourcestring] [-svrname name]\n\
	       [-logmonitor][-import file] [file...]\n";
#else
"";
#endif /*VMS*/

int main(int argc, char **argv)
{
    int i, lineNum, nRead, fileSpecified = FALSE, editFlags = CREATE;
    int gotoLine = False, macroFileRead = False;
    char *toDoCommand = NULL;
    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
    XtAppContext context;
    XrmDatabase prefDB;
    WindowInfo *window = NULL;
	int logMonitor = FALSE;
	
    
#ifdef _WIN32
	HCLXmInit();
#endif

#ifdef HAVE_X11_XLOCALE_H
    /* Set local for C library and X, and Motif input functions */
    if (setlocale(LC_CTYPE, "") == NULL) {
	fprintf(stderr, "NEdit: Locale not supported by C library.\n");
	if(setlocale(LC_CTYPE, NULL)==NULL) {
            fprintf(stderr,"NEdit: cannot continue.\n");
            exit(0);
	} else
            fprintf(stderr,"NEdit: Using %s locale instead.\n",
            	    setlocale(LC_CTYPE, NULL));
    }
#ifdef HAVE_XSUPPORTSLOCALE
    if (!XSupportsLocale()) {
    	fprintf(stderr, "NEdit: Xlib: locale %s not supported\n.",
    	    	setlocale(LC_CTYPE, NULL));
    	exit(0);
    }
    if (XSetLocaleModifiers("") == NULL)
        fprintf(stderr,"NEdit: cannot set locale modifiers.\n");
#endif
#else
    /* Set locale for C library functions */
    setlocale(LC_CTYPE, "");
#endif
    
    /* Initialize toolkit and open display. */
    XtToolkitInitialize();
    context = XtCreateApplicationContext();
    
    /* Set up a warning handler to trap obnoxious Xt grab warnings */
    SuppressPassiveGrabWarnings();

    /* Set up default resources if no app-defaults file is found */
    XtAppSetFallbackResources(context, fallbackResources);
    
#if XmVersion >= 1002
    /* Allow users to change tear off menus with X resources */
    XmRepTypeInstallTearOffModelConverter();
#endif
    
#ifdef VMS
    /* Convert the command line to Unix style (This is not an ideal solution) */
    ConvertVMSCommandLine(&argc, &argv);
#endif /*VMS*/
    
    /* Read the preferences file and command line into a database */
    prefDB = CreateNEditPrefDB(&argc, argv);

    /* Open the display and read X database and remaining command line args */
    TheDisplay = XtOpenDisplay (context, NULL, APP_NAME, APP_CLASS, NULL,
    	    0, &argc, argv);
    if (!TheDisplay) {
	XtWarning ("NEdit: Can't open display\n");
	exit(0);
    }
    
    /* Initialize global symbols and subroutines used in the macro language */
    InitMacroGlobals();
    RegisterMacroSubroutines();

    /* Store preferences from the command line and .nedit file, 
       and set the appropriate preferences */
    RestoreNEditPrefs(prefDB, XtDatabase(TheDisplay));
    LoadPrintPreferences(XtDatabase(TheDisplay), APP_NAME, APP_CLASS, True);
    SetDeleteRemap(GetPrefMapDelete());
    SetPointerCenteredDialogs(GetPrefRepositionDialogs() == REPOSITION_DIALOGS_ON_POINTER);

    SetGetExistingFilenameTextFieldRemoval(!GetPrefStdOpenDialog());
    
    /* Set up action procedures for menu item commands */
    InstallMenuActions(context);
    
    /* Install word delimiters for regular expression matching */
    SetREDefaultWordDelimiters(GetPrefDelimiters());
    
    /* Read the nedit dynamic database of files for the Open Previous
       command (and eventually other information as well) */
    ReadNEditDB();
    
    /* Process -import command line argument before others which might
       open windows (loading preferences doesn't update menu settings,
       which would then be out of sync with the real preference settings) */
    for (i=1; i<argc; i++) {
    	if (!strcmp(argv[i], "-import")) {
    	    nextArg(argc, argv, &i);
    	    ImportPrefFile(argv[i]);
	}
    }

    /* Process any command line arguments (-tags, -do, -read, -create,
       +<line_number>, -line, -server, and files to edit) not already
       processed by RestoreNEditPrefs. */
    fileSpecified = FALSE;
    for (i=1; i<argc; i++) {
    	if (!strcmp(argv[i], "-tags")) {
    	    nextArg(argc, argv, &i);
    	    if (!LoadTagsFile(argv[i]))
    	    	fprintf(stderr, "NEdit: Unable to load tags file\n");
    	} else if (!strcmp(argv[i], "-do")) {
    	    nextArg(argc, argv, &i);
	    if (checkDoMacroArg(argv[i]))
    	    	toDoCommand = argv[i];
    	} else if (!strcmp(argv[i], "-read")) {
    	    editFlags |= FORCE_READ_ONLY;
    	} else if (!strcmp(argv[i], "-create")) {
    	    editFlags |= SUPPRESS_CREATE_WARN;
    	} else if (!strcmp(argv[i], "-line")) {
    	    nextArg(argc, argv, &i);
	    nRead = sscanf(argv[i], "%d", &lineNum);
	    if (nRead != 1)
    		fprintf(stderr, "NEdit: argument to line should be a number\n");
    	    else
    	    	gotoLine = True;
    	} else if (*argv[i] == '+') {
    	    nRead = sscanf((argv[i]+1), "%d", &lineNum);
	    if (nRead != 1)
    		fprintf(stderr, "NEdit: argument to + should be a number\n");
    	    else
    	    	gotoLine = True;
    	} else if (!strcmp(argv[i], "-server")) {
    	    isServer = True;
    	} else if (!strcmp(argv[i], "-logmonitor")) {
    	    logMonitor = True;
		} else if (!strcmp(argv[i], "-import")) {
			nextArg(argc, argv, &i); /* already processed, skip */
    	} else if (*argv[i] == '-') {
#ifdef VMS
	    *argv[i] = '/';
#endif /*VMS*/
    	    fprintf(stderr, "NEdit: Unrecognized option %s\n%s", argv[i],
    	    	    cmdLineHelp);
    	    exit(0);
    	} else {
#ifdef VMS
	    int numFiles, j;
	    char **nameList = NULL;
	    /* Use VMS's LIB$FILESCAN for filename in argv[i] to process */
	    /* wildcards and to obtain a full VMS file specification     */
	    numFiles = VMSFileScan(argv[i], &nameList, NULL, INCLUDE_FNF);
	    /* for each expanded file name do: */
	    for (j = 0; j < numFiles; ++j) {
			ParseFilename(nameList[j], filename, pathname);
			window = EditExistingFile(WindowList, filename, pathname, editFlags, False);
			if (!macroFileRead) {
		    	ReadMacroInitFile(window);
		    	macroFileRead = True;
			}
			if (toDoCommand != NULL)
				DoMacro(window, toDoCommand, "-do macro");
			if (gotoLine)
				SelectNumberedLine(window, lineNum);
			if (logMonitor)
				SetCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
			fileSpecified = TRUE;
			XtFree(nameList[j]);
	    }
	    if (nameList != NULL)
	    	XtFree(nameList);
#else
	    ParseFilename(argv[i], filename, pathname);
	    window = EditExistingFile(WindowList, filename, pathname, editFlags, False);
	    fileSpecified = TRUE;
	    if (!macroFileRead) {
		ReadMacroInitFile(WindowList);
		macroFileRead = True;
	    }
	    if (toDoCommand != NULL)
	    	DoMacro(window, toDoCommand, "-do macro");
	    if (gotoLine)
	    	SelectNumberedLine(window, lineNum);
		if (logMonitor)
			SetCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
#endif /*VMS*/
		toDoCommand = NULL;
	}
    }
#ifdef VMS
    VMSFileScanDone();
#endif /*VMS*/
    
    /* Load the default tags file (as long as -tags was not specified).
       Don't complain if it doesn't load, the tag file resource is
       intended to be set and forgotten.  Running nedit in a directory
       without a tags should not cause it to spew out errors. */
    if (!TagsFileLoaded() && *GetPrefTagFile() != '\0')
    	LoadTagsFile(GetPrefTagFile());
    
    /* If no file to edit was specified, open a window to edit "Untitled" */
    if (!fileSpecified) {
    	window = EditNewFile(NULL, False);
    	ReadMacroInitFile(window);
		if (toDoCommand != NULL)
		    DoMacro(window, toDoCommand, "-do macro");
    }
        
    /* Begin remembering last command invoked for "Repeat" menu item */
    AddLastCommandActionHook(context);

    /* Set up communication port and write ~/.nedit_server_process file */
    if (isServer) {
    	DaemonInit();
    	InitServerCommunication(GetPrefServerName());
    }

    /* Process events. */
   	ServerMainLoop(context);

    /* Not reached but this keeps some picky compilers happy */
    return 0;
}

static void nextArg(int argc, char **argv, int *argIndex)
{
    if (*argIndex + 1 >= argc) {
#ifdef VMS
	    *argv[*argIndex] = '/';
#endif /*VMS*/
    	fprintf(stderr, "NEdit: %s requires an argument\n%s", argv[*argIndex],
    	        cmdLineHelp);
    	exit(0);
    }
    (*argIndex)++;
}

/*
** Return True if -do macro is valid, otherwise write an error on stderr
*/
static int checkDoMacroArg(char *macro)
{
    Program *prog;
    char *errMsg, *stoppedAt, *tMacro;
    int macroLen;
    
    /* Add a terminating newline (which command line users are likely to omit
       since they are typically invoking a single routine) */
    macroLen = strlen(macro);
    tMacro = XtMalloc(strlen(macro+2));
    strncpy(tMacro, macro, macroLen);
    tMacro[macroLen] = '\n';
    tMacro[macroLen+1] = '\0';
    
    /* Do a test parse */
    prog = ParseMacro(tMacro, &errMsg, &stoppedAt);
    if (prog == NULL) {
    	ParseError(NULL, tMacro, stoppedAt, "argument to -do", errMsg);
	return False;
    }
    FreeProgram(prog);
    return True;
}
