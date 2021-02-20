/*******************************************************************************
*									       *
* window.c -- Nirvana Editor window creation/deletion			       *
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
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef VMS
#include "../util/VMSparam.h"
#include <stat.h>
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/stat.h>
#endif /*VMS*/
#include <errno.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/MainW.h>
#include <Xm/PanedW.h>
#include <Xm/PanedWP.h>
#include <Xm/Form.h>
#include <Xm/RowColumnP.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <Xm/Label.h>
#include <Xm/ScrolledW.h>
#include <Xm/ScrollBar.h>
#include <Xm/PrimitiveP.h>
#include <Xm/PushB.h>
#ifdef HAVE_X11_XMU_EDITRES_H
# include <X11/Xmu/Editres.h>
#else
# ifdef HAVE__XEDITRESCHECKMESSAGES
extern XtEventHandler _XEditResCheckMessages;
# endif
#endif
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "textSel.h"
#include "text.h"
#include "textDisp.h"
#include "textP.h"
#include "nedit.h"
#include "window.h"
#include "menu.h"
#include "file.h"
#include "search.h"
#include "undo.h"
#include "preferences.h"
#include "selection.h"
#include "shell.h"
#include "macro.h"
#include "highlight.h"
#include "smartIndent.h"
#include "userCmds.h"
#include "nedit.bm"
#include "n.bm"
#include "clearcase.h"
#include "server.h"

#define MAIN_WINDOW_NAME "mainWindow"

/* Initial minimum height of a pane.  Just a fallback in case setPaneMinHeight
   (which may break in a future release) is not available */
#define PANE_MIN_HEIGHT 39

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols, int emTabDist, char *delimiters, int wrapMargin);
static void addToWindowList(WindowInfo *window);
static void removeFromWindowList(WindowInfo *window);
static void focusCB(Widget w, WindowInfo *window, XtPointer callData);
static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
	char *deletedText, void *cbArg);
static void movedCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragStartCB(Widget w, WindowInfo *window, XtPointer callData);
static void dragEndCB(Widget w, WindowInfo *window, dragEndCBStruct *callData);
static void closeCB(Widget w, WindowInfo *window, XtPointer callData);
static void setPaneDesiredHeight(Widget w, int height);
static void setPaneMinHeight(Widget w, int min);
static void addWindowIcon(Widget shell);
static void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id);
#ifdef ROWCOLPATCH
static void patchRowCol(Widget w);
static void patchedRemoveChild(Widget child);
#endif
static void iSearchToggleCB(Widget w, WindowInfo *window, XtPointer callData);
static void iSearchHideCB(Widget w, WindowInfo *window, XtPointer callData);

/*
** Create a new editor window
*/
WindowInfo *NEditCreateWindow(char *name, EditorInfo *editorInfo)
{
    Widget appShell, main, menuBar, pane, text, stats;
    Widget iSearchForm, iSearchLabel, iSearchText;
    Widget iSearchRadioBox, iSearchCaseToggle, iSearchRegExpToggle;
    WindowInfo *window;
    Arg al[20];
    int ac;
#ifdef SGI_CUSTOM
    char sgi_title[MAXPATHLEN + 14 + SGI_WINDOW_TITLE_LEN] = SGI_WINDOW_TITLE; 
#endif

    /* Allocate some memory for the new window data structure */
    window = (WindowInfo *)XtMalloc(sizeof(WindowInfo));
    
    /* initialize window structure */
    if(editorInfo) {
    	window->editorInfo = editorInfo;
    }
    else {
    	window->editorInfo = (EditorInfo *)XtMalloc(sizeof(EditorInfo));
    	window->editorInfo->path[0] = 0;
    	window->editorInfo->st__mtime = 0;
    	window->editorInfo->st_mode = 0;
    	window->editorInfo->st_size = 0;
    	window->editorInfo->checkingMode = CHECKING_MODE_DISABLED;
    	window->editorInfo->tailMinusFTimeoutID = 0;
    	window->editorInfo->undo = NULL;
    	window->editorInfo->redo = NULL;
    	window->editorInfo->autoSaveCharCount = 0;
    	window->editorInfo->autoSaveOpCount = 0;
    	window->editorInfo->undoOpCount = 0;
    	window->editorInfo->undoMemUsed = 0;
    	window->editorInfo->flashTimeoutID = 0;
    	window->editorInfo->flashPos = 0;
    	window->editorInfo->filenameSet = FALSE;
    	strcpy(window->editorInfo->filename, name);
    	window->editorInfo->fileChanged = FALSE;
    	window->editorInfo->readOnly = FALSE;
    	window->editorInfo->lockWrite = FALSE;
    	window->editorInfo->autoSave = GetPrefAutoSave();
    	window->editorInfo->saveOldVersion = GetPrefSaveOldVersion();
    	window->editorInfo->overstrike = False;
    	window->editorInfo->showMatching = GetPrefShowMatching();
    	window->editorInfo->ignoreModify = FALSE;
    	window->editorInfo->indentStyle = GetPrefAutoIndent(PLAIN_LANGUAGE_MODE);
    	window->editorInfo->wrapMode = GetPrefWrap(PLAIN_LANGUAGE_MODE);
    	window->editorInfo->smartIndentData = NULL;
    	window->editorInfo->fileOpenAtom = None;
    	window->editorInfo->master = window;
    }

    window->nextSlave = NULL;
    window->replaceDlog = NULL;
    window->replaceText = NULL;
    window->replaceWithText = NULL;
    window->replaceLiteralBtn = NULL;
    window->replaceCaseBtn = NULL;
    window->replaceRegExpBtn = NULL;
    window->replaceBtns = NULL;
    window->replaceBtn = NULL;
    window->replaceFindBtn = NULL;
    window->replaceInSelBtn = NULL;
    window->replaceSearchTypeBox = NULL;
    window->findBtn = NULL;
    window->windowMenuValid = FALSE;
    window->prevOpenMenuValid = FALSE;
    window->rHistIndex = 0;
    window->wasSelected = FALSE;
    window->showStats = GetPrefStatsLine();
    window->highlightSyntax = GetPrefHighlightSyntax();
    window->showISearchLine = GetPrefShowISearchLine();
    window->modeMessageDisplayed = FALSE;
    window->nPanes = 0;
    strcpy(window->fontName, GetPrefFontName());
    strcpy(window->italicFontName, GetPrefItalicFontName());
    strcpy(window->boldFontName, GetPrefBoldFontName());
    strcpy(window->boldItalicFontName, GetPrefBoldItalicFontName());
    window->fontList = GetPrefFontList();
    window->italicFontStruct = GetPrefItalicFont();
    window->boldFontStruct = GetPrefBoldFont();
    window->boldItalicFontStruct = GetPrefBoldItalicFont();
    window->fontDialog = NULL;
    window->nMarks = 0;
    window->markTimeoutID = 0;
    window->highlightData = NULL;
    window->shellCmdData = NULL;
    window->macroCmdData = NULL;
    window->languageMode = PLAIN_LANGUAGE_MODE;
    window->iSearchDirection = SEARCH_FORWARD;
    window->iSearchLastSearchString[0] = 0;
    window->iSearchLastSearchType = GetPrefSearch();
    window->iSearchForm = NULL;
    window->iSearchText = NULL;
    window->iSearchHistIndex = 0;
    
	/* Interface elements that we test for existance */
    window->bgMenuUndoItem = NULL;
    window->bgMenuRedoItem = NULL;
    
    /* Create a new toplevel shell to hold the window */
    ac = 0;
#ifdef SGI_CUSTOM
    strcat(sgi_title, name);
    XtSetArg(al[ac], XmNtitle, sgi_title); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    if (strncmp(name, "Untitled", 8) == 0) { 
    	XtSetArg(al[ac], XmNiconName, APP_NAME); ac++;
    } else {
    	XtSetArg(al[ac], XmNiconName, name); ac++;
    }
#else
    XtSetArg(al[ac], XmNtitle, name); ac++;
    XtSetArg(al[ac], XmNdeleteResponse, XmDO_NOTHING); ac++;
    XtSetArg(al[ac], XmNiconName, name); ac++;
#endif
    appShell = XtAppCreateShell("nedit", "NEdit",
		applicationShellWidgetClass, TheDisplay, al, ac);
    window->shell = appShell;

#ifdef HAVE__XEDITRESCHECKMESSAGES
    XtAddEventHandler (appShell, (EventMask)0, True,
	    (XtEventHandler)_XEditResCheckMessages, NULL);
#endif /* EDITRES */

#ifndef SGI_CUSTOM
    addWindowIcon(appShell);
#endif

    /* Create a MainWindow to manage the menubar and text area, set the
       userData resource to be used by WidgetToWindow to recover the
       window pointer from the widget id of any of the window's widgets */
    XtSetArg(al[ac], XmNuserData, window); ac++;
    main = XmCreateMainWindow(appShell, MAIN_WINDOW_NAME, al, ac);
    XtManageChild(main);

    iSearchForm = XtVaCreateWidget("iSearchForm", 
       	    xmFormWidgetClass, main, NULL);
    window->iSearchForm = iSearchForm;
    iSearchLabel = XtVaCreateManagedWidget("iSearchLabel",
      	    xmPushButtonWidgetClass, iSearchForm,
			XmNmarginHeight, 1,
			XmNshadowThickness, 0,
      	    NULL);
    XtAddCallback(iSearchLabel, XmNactivateCallback, (XtCallbackProc)iSearchHideCB, window);
    iSearchText = XtVaCreateManagedWidget("iSearchText",
      	    xmTextFieldWidgetClass, iSearchForm,
    	    XmNfontList, window->fontList,
    		XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
    	    NULL);
    window->iSearchText = iSearchText;
    iSearchRadioBox = XtVaCreateManagedWidget("iSearchRadioBox",
    		xmRowColumnWidgetClass, iSearchForm,
    		XmNorientation, XmHORIZONTAL,
    		XmNpacking, XmPACK_TIGHT,
    		NULL);
    iSearchCaseToggle = XtVaCreateManagedWidget("iSearchCaseToggle",
    		xmToggleButtonWidgetClass, iSearchRadioBox,
    		XmNtraversalOn, False,
    		XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
    		XmNhighlightThickness, 2,
			XmNmarginHeight, 0,
    		XmNset, GetPrefSearch() == SEARCH_CASE_SENSE || GetPrefSearch() == SEARCH_REGEX,
    		NULL);
    XtAddCallback(iSearchCaseToggle, XmNvalueChangedCallback, (XtCallbackProc)iSearchToggleCB, window);
    window->iSearchCaseToggle = iSearchCaseToggle;
    iSearchRegExpToggle = XtVaCreateManagedWidget("iSearchRegExpToggle",
    		xmToggleButtonWidgetClass, iSearchRadioBox,
    		XmNtraversalOn, False,
    		XmNnavigationType, XmEXCLUSIVE_TAB_GROUP,
    		XmNhighlightThickness, 2,
			XmNmarginHeight, 0,
    		XmNset, GetPrefSearch() == SEARCH_REGEX,
    		NULL);
    XtAddCallback(iSearchRegExpToggle, XmNvalueChangedCallback, (XtCallbackProc)iSearchToggleCB, window);
    window->iSearchRegExpToggle = iSearchRegExpToggle;
    XtManageChild(iSearchRadioBox);
    XtVaSetValues(iSearchLabel,
       	    XmNtopAttachment, XmATTACH_FORM,
       	    XmNbottomAttachment, XmATTACH_FORM,
      	    XmNleftAttachment, XmATTACH_FORM,
      	    XmNrightAttachment, XmATTACH_NONE,
    		NULL);
    XtVaSetValues(iSearchText,
       	    XmNtopAttachment, XmATTACH_NONE,
       	    XmNtopOffset, 2,
       	    XmNbottomAttachment, XmATTACH_FORM,
       	    XmNbottomOffset, 2,
      	    XmNleftAttachment, XmATTACH_WIDGET,
      	    XmNleftWidget, iSearchLabel,
      	    XmNrightAttachment, XmATTACH_WIDGET,
      	    XmNrightWidget, iSearchRadioBox,
    		NULL);
    XtVaSetValues(iSearchRadioBox,
       	    XmNtopAttachment, XmATTACH_NONE,
       	    XmNbottomAttachment, XmATTACH_FORM,
      	    XmNleftAttachment, XmATTACH_NONE,
      	    XmNrightAttachment, XmATTACH_FORM,
    		NULL);

    SetISearchTextCallbacks(window);
    if(window->showISearchLine) {
	    XtManageChild(iSearchForm);
	}

    /* Create the menu bar */
    menuBar = CreateMenuBar(main, window);
    window->menuBar = menuBar;
    XtManageChild(menuBar);
        
    /* Create paned window to manage split window behavior */
    pane = XtVaCreateManagedWidget("pane", xmPanedWindowWidgetClass,  main,
    	    XmNmarginWidth, 0, XmNmarginHeight, 0, XmNseparatorOn, False,
    	    XmNspacing, 3, XmNsashIndent, -2,
			NULL);
    window->splitPane = pane;

    /* Create the first, and most permanent text area (other panes may
       be added & removed, but this one will never be removed */
	text = createTextArea(pane, window, GetPrefRows(), GetPrefCols(),
    	    GetPrefEmTabDist(PLAIN_LANGUAGE_MODE), GetPrefDelimiters(),
    	    GetPrefWrapMargin());
    XtManageChild(text);
    window->textArea = text;
    window->lastFocus = text;

    /* Create file statistics display area.  Using a text widget rather than
       a label solves a layout problem with the main window, which messes up
       if the label is too long (we would need a resize callback to control
       the length when the window changed size).*/
    stats = XtVaCreateWidget("statsLine", xmTextFieldWidgetClass,  pane,
    	    XmNscrollHorizontal, False,
    	    XmNeditMode, XmSINGLE_LINE_EDIT,
    	    XmNeditable, False,
    	    XmNtraversalOn, False,
    	    XmNcursorPositionVisible, False,
#if 0
    	    XmNfontList, window->fontList,
#endif
			XmNskipAdjust, True,
			NULL);
    window->statsLine = stats;
   
    /* If the fontList was NULL, use the magical default provided by Motif,
       since it must have worked if we've gotten this far */
    if (window->fontList == NULL)
    	XtVaGetValues(stats, XmNfontList, &window->fontList, NULL);

    if (window->showStats) {
		XtManageChild(stats);
	}
	
	XtManageChild(pane);
	
    XmMainWindowSetAreas(main, menuBar, iSearchForm, NULL, NULL, pane);
    
    /* Create the right button popup menu (note: order is important here,
       since the translation for popping up this menu was probably already
       added in createTextArea, but CreateBGMenu requires window->textArea
       to be set so it can attach the menu to it (because menu shells are
       finicky about the kinds of widgets they are attached to)) */
    window->bgMenuPane = CreateBGMenu(window);
    
    /* Create the text buffer rather than using the one created automatically
       with the text area widget.  This is done so the syntax highlighting
       modify callback can be called to synchronize the style buffer BEFORE
       the text display's callback is called upon to display a modification */
    if(!editorInfo) {
    	window->editorInfo->buffer = BufCreate();
    }
    BufAddModifyCB(window->editorInfo->buffer, SyntaxHighlightModifyCB, window);
    
    /* Attach the buffer to the text widget, and add callbacks for modify */
    TextSetBuffer(text, window->editorInfo->buffer);
    BufAddModifyCB(window->editorInfo->buffer, modifiedCB, window);
    
    /* Set the requested hardware tab distance and useTabs in the text buffer */
    if(!editorInfo) {
    	BufSetTabDistance(window->editorInfo->buffer, GetPrefTabDist(PLAIN_LANGUAGE_MODE));
    	window->editorInfo->buffer->useTabs = GetPrefInsertTabs();
    }

    /* add the window to the global window list, update the Windows menus */
    addToWindowList(window);
    InvalidateWindowMenus();
    
    /* realize all of the widgets in the new window */
    XtRealizeWidget(appShell);

    /* Make close command in window menu gracefully prompt for close */
    AddMotifCloseCallback(appShell, (XtCallbackProc)closeCB, window);
    
    /* Make window resizing work in nice character heights */
    UpdateWMSizeHints(window);
    
    /* Set the minimum pane height for the initial text pane */
    UpdateMinPaneHeights(window);
    
    /* make sure the window has been drawn before returning */
	UpdateDisplay();
	
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
    return window;
}

/*
** Close an editor window
*/
void NEditCloseWindow(WindowInfo *window)
{
	EditorInfo *editorInfo = window->editorInfo;
	int keepWindow;
	char name[MAXPATHLEN];

	/* Free smart indent macro programs */
	EndSmartIndent(window);

	/* Clean up macro references to the doomed window.  If a macro is
	   executing, stop it.  If macro is calling this (closing its own
	   window), leave the window alive until the macro completes */
	keepWindow = !MacroWindowCloseActions(window);

	/* Kill shell sub-process and free related memory */
	keepWindow = !AbortShellCommand(window);

	/* if this is the last window, or must be kept alive temporarily because
	   it's running the macro calling us, don't close it, make it Untitled */
	if (keepWindow || (WindowList == window && window->next == NULL)) {
		/* Destroy the file open property for this file */
		DestroyFileOpenProperty(window);
		editorInfo->filename[0] = '\0';
		UniqueUntitledName(name);
		editorInfo->readOnly = FALSE;
		editorInfo->lockWrite = FALSE;
		strcpy(editorInfo->filename, name);
		/*strcpy(editorInfo->path, "");*//* leave the path set to its last value. */
		editorInfo->ignoreModify = TRUE;
		BufSetAll(editorInfo->buffer, "");
		editorInfo->ignoreModify = FALSE;
		editorInfo->filenameSet = FALSE;
		editorInfo->fileChanged = FALSE;
		StopHighlighting(window);
		EndSmartIndent(window);
		SetCheckingMode(window, CHECKING_MODE_DISABLED);
		UpdateWindowReadOnly(window);
		UpdateWindowTitle(window);
		XtSetSensitive(window->closeItem, FALSE);
		XtSetSensitive(window->readOnlyItem, TRUE);
		XmToggleButtonSetState(window->readOnlyItem, FALSE, FALSE);
    	ClearUndoList(window);
    	ClearRedoList(window);
    	XmTextFieldSetString(window->statsLine, ""); /* resets scroll pos of stats
    	    	    	    	    	           line from long file names */
		UpdateStatsLine(window);
		DetermineLanguageMode(window, True);
		return;
    }
    
    /* make sure the window has been removed before continuing */
    XtUnmapWidget(window->shell);
	UpdateDisplay();

    /* Free syntax highlighting patterns, if any. w/o redisplaying */
    FreeHighlightingData(window);
    	
    /* remove the buffer modification callbacks so the buffer will be
       deallocated when the last text widget is destroyed */
    BufRemoveModifyCB(editorInfo->buffer, modifiedCB, window, True);
    BufRemoveModifyCB(editorInfo->buffer, SyntaxHighlightModifyCB, window, True);
    
#ifdef ROWCOLPATCH
    patchRowCol(window->menuBar);
#endif
    
    /* remove and deallocate all of the widgets associated with window */
    XtDestroyWidget(window->shell);
    
    /* remove the window from the global window list, update window menus */
    removeFromWindowList(window);
    InvalidateWindowMenus();
    
    /* Update the slave window linked list */
    if(window == editorInfo->master) {
    	/* If we are the master then make the next slave the master. */
   		editorInfo->master = editorInfo->master->nextSlave;
    }
    else {
    	/* If we are a slave then remove ourself from the slave list. */
    	WindowInfo *w;
    	for(w = editorInfo->master; w->nextSlave; w = w->nextSlave) {
			if(w->nextSlave == window) {
				w->nextSlave = window->nextSlave;
				break;
			}
		}
    }
    
    /* If there are still slave windows then make sure selections
       are still attached and the title is updated else free
       the editorInfo stuff. */
    if(editorInfo->master != NULL) {
    	TextHandleXSelections(editorInfo->master->textArea);
    	UpdateWindowTitle(editorInfo->master);
    }
    else {
		/* Remove the file checking timeout. */
		if(editorInfo->tailMinusFTimeoutID) {
			XtRemoveTimeOut(editorInfo->tailMinusFTimeoutID);
		}
		editorInfo->tailMinusFTimeoutID = 0;

		/* Remove the flash timeout. */
		if(editorInfo->flashTimeoutID) {
			XtRemoveTimeOut(editorInfo->flashTimeoutID);
		}
		editorInfo->flashTimeoutID = 0;

    	/* Destroy the file open property for this file */
  		DestroyFileOpenProperty(window);

    	/* free the undo and redo lists */
    	ClearUndoList(window);
    	ClearRedoList(window);

    	XtFree((char *)editorInfo);
    }
    
    /* deallocate the window data structure */
    XtFree((char *)window);
}

/*
** Create a slave editor window
*/
WindowInfo *CreateSlaveWindow(WindowInfo *window)
{
    WindowInfo *newWindow;
    int i, topLine, horizOffset;
    int emTabDist, wrapMargin;
    char *delimiters;
    WindowInfo *w;
    
    /* create a new window and transfer values from the old one */
    newWindow = NEditCreateWindow("", window->editorInfo);
    
    /* Add window to end of slave list */
    for(w = newWindow->editorInfo->master; w->nextSlave != NULL; w = w->nextSlave)
    	;
    w->nextSlave = newWindow;
    newWindow->nextSlave = NULL;

    SetFonts(newWindow,
    		window->fontName, 
    	    window->italicFontName,
    	    window->boldFontName,
    	    window->boldItalicFontName
    );
    XtVaGetValues(window->textArea,
    		textNemulateTabs, &emTabDist,
    		textNwrapMargin, &wrapMargin, 
    		textNwordDelimiters, &delimiters, 
    		NULL);
    XtVaSetValues(newWindow->textArea, 
    		textNemulateTabs, emTabDist,
    	    textNwrapMargin, wrapMargin,
    		textNwordDelimiters, delimiters, 
    	    NULL);
    XtSetSensitive(newWindow->undoItem, XtIsSensitive(window->undoItem));
    XtSetSensitive(newWindow->redoItem, XtIsSensitive(window->redoItem));
    TextSetCursorPos(newWindow->textArea, TextGetCursorPos(window->textArea));
    TextGetScroll(window->textArea, &topLine, &horizOffset);
    TextSetScroll(newWindow->textArea, topLine, horizOffset);
    for (i=0; i<window->nMarks; i++)
    	newWindow->markTable[i] = window->markTable[i];
    newWindow->nMarks = window->nMarks;
    newWindow->iSearchDirection = window->iSearchDirection;
    strcpy(newWindow->iSearchLastSearchString, window->iSearchLastSearchString);
    newWindow->iSearchLastSearchType = window->iSearchLastSearchType;

    /* Retain the current language mode and syntax highlighting. */
    newWindow->languageMode = window->languageMode;
    newWindow->highlightSyntax = window->highlightSyntax;
    if (newWindow->highlightSyntax) {
    	StartHighlighting(newWindow, False);
    }

    /* Update the checking mode toggle buttons */
    SetCheckingMode(newWindow, window->editorInfo->checkingMode);
    UpdateWindowReadOnly(newWindow);
    UpdateWindowTitle(newWindow);

    /* Add/remove language specific menu items */
    UpdateShellMenu(newWindow);
    UpdateMacroMenu(newWindow);
    UpdateBGMenu(newWindow);

   	/* Make sure the window is not created iconfied */
	RaiseShellWindow(newWindow->shell);
    
	return newWindow;
}


/*
** Check if there is already a window open for a given file
*/
WindowInfo *FindWindowWithFile(char *name, char *path)
{
    WindowInfo *w;

    for (w=WindowList; w!=NULL; w=w->next) {
    	if (!FILECOMPARE(w->editorInfo->filename, name) && !FILECOMPARE(w->editorInfo->path, path)) {
	    return w;
	}
    }
    return NULL;
}

/*
** Add another independently scrollable window pane to the current window,
** splitting the pane which currently has keyboard focus.
*/
void SplitWindow(WindowInfo *window)
{
    Dimension paneHeights[MAX_PANES+1];
    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
    int horizOffsets[MAX_PANES+1];
    int i, focusPane, emTabDist, wrapMargin, totalHeight=0;
    char *delimiters;
    Widget text;
    
    /* Don't create new panes if we're already at the limit */
    if (window->nPanes >= MAX_PANES)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, keyboard focus */
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], NULL);
    	totalHeight += paneHeights[i];
    	TextGetScroll(text, &topLines[i], &horizOffsets[i]);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Create a text widget to add to the pane and set its buffer and
       highlight data to be the same as the other panes in the window */
    XtVaGetValues(window->textArea, textNemulateTabs, &emTabDist,
    	    textNwordDelimiters, &delimiters, textNwrapMargin, &wrapMargin, NULL);
    text = createTextArea(window->splitPane, window, 1, 1, emTabDist,
    	    delimiters, wrapMargin);
    TextSetBuffer(text, window->editorInfo->buffer);
    if (window->highlightData != NULL)
    	AttachHighlightToWidget(text, window);
	/* Set the position in the paned window where we want new text
	   area added. We can't use the default of adding to the end
	   because the last widget in the pane is actually the status bar. */
	XtVaSetValues(XtParent(text),
		XmNpositionIndex, window->nPanes+1,
		NULL);
    XtManageChild(text);
    window->textPanes[window->nPanes++] = text;
    
    /* Set the minimum pane height in the new pane */
    UpdateMinPaneHeights(window);

    /* adjust the heights, scroll positions, etc., to split the focus pane */
    for (i=window->nPanes; i>focusPane; i--) {
    	insertPositions[i] = insertPositions[i-1];
    	paneHeights[i] = paneHeights[i-1];
    	topLines[i] = topLines[i-1];
    	horizOffsets[i] = horizOffsets[i-1];
    }
    paneHeights[focusPane] = paneHeights[focusPane]/2;
    paneHeights[focusPane+1] = paneHeights[focusPane];
    
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	setPaneDesiredHeight(XtParent(text), paneHeights[i]);
    }

    /* Re-manage panedWindow to recalculate pane heights & reset selection */
    XtManageChild(window->splitPane);
    
    /* Reset all of the heights, scroll positions, etc. */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], horizOffsets[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);
    
    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
    	    wmSizeUpdateProc, window);
}

/*
** Close the window pane that last had the keyboard focus.  (Actually, close
** the bottom pane and make it look like pane which had focus was closed)
*/
void ClosePane(WindowInfo *window)
{
    Dimension paneHeights[MAX_PANES+1];
    int insertPositions[MAX_PANES+1], topLines[MAX_PANES+1];
    int horizOffsets[MAX_PANES+1];
    int i, focusPane,totalHeight=0;
    Widget text;
    
    /* Don't delete the last pane */
    if (window->nPanes <= 0)
    	return;
    
    /* Record the current heights, scroll positions, and insert positions
       of the existing panes, and the keyboard focus */
    focusPane = 0;
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	XtVaGetValues(XtParent(text), XmNheight, &paneHeights[i], NULL);
    	totalHeight += paneHeights[i];
    	TextGetScroll(text, &topLines[i], &horizOffsets[i]);
    	if (text == window->lastFocus)
    	    focusPane = i;
    }
    
    /* Unmanage & remanage the panedWindow so it recalculates pane heights */
    XtUnmanageChild(window->splitPane);
    
    /* Destroy last pane, and make sure lastFocus points to an existing pane */
    XtDestroyWidget(XtParent(window->textPanes[--window->nPanes]));
    if (window->nPanes == 0)
	window->lastFocus = window->textArea;
    else if (focusPane > window->nPanes)
	window->lastFocus = window->textPanes[window->nPanes-1];
    
    /* adjust the heights, scroll positions, etc., to make it look
       like the pane with the input focus was closed */
    for (i=window->nPanes; i>=focusPane; i--) {
    	insertPositions[i] = insertPositions[i+1];
    	paneHeights[i] = paneHeights[i+1];
    	topLines[i] = topLines[i+1];
    	horizOffsets[i] = horizOffsets[i+1];
    }
    
    /* set the desired heights and re-manage the paned window so it will
       recalculate pane heights */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	setPaneDesiredHeight(XtParent(text), paneHeights[i]);
    }
    XtManageChild(window->splitPane);
    
    /* Reset all of the scroll positions, insert positions, etc. */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], horizOffsets[i]);
    	setPaneDesiredHeight(XtParent(text), totalHeight/(window->nPanes+1));
    }
    XmProcessTraversal(window->lastFocus, XmTRAVERSE_CURRENT);

    /* Make sure selections are handled. Transfer the selection 
	   ownership to the new last focus window */
	HandleXSelections(window->lastFocus);
	
    /* Update the window manager size hints after the sizes of the panes have
       been set (the widget heights are not yet readable here, but they will
       be by the time the event loop gets around to running this timer proc) */
    XtAppAddTimeOut(XtWidgetToApplicationContext(window->shell), 0,
    	    wmSizeUpdateProc, window);
}

/*
** Turn on and off the display of the statistics line
*/
void ShowStatsLine(WindowInfo *window, int state)
{
    /* Return if the if the new state matches the current state. */
    if(window->showStats == state)
    	return;
    
    window->showStats = state;
    
    if (state) {
    	XtManageChild(window->statsLine);
	   	UpdateStatsLine(window);
    } else {
     	XtUnmanageChild(window->statsLine);
    }
    
   	/* Set the menu item to the current state. */
   	XmToggleButtonSetState(window->statisticsLineItem, state, FALSE);

    /* Tell WM that the non-expandable part of the window has changed size */
    UpdateWMSizeHints(window);
}

/*
** Turn on and off the display of the incremental search line
*/
void ShowISearchLine(WindowInfo *window, int state)
{
    Widget mainW = XtParent(window->iSearchForm);
    
    /* Return if the iSearchForm was not created or if the new state
       matches the current state. */
    if(window->iSearchForm == NULL || window->showISearchLine == state)
    	return;
    
    window->showISearchLine = state;
    
    /* The very silly use of XmNcommandWindowLocation and XmNshowSeparator
       below are to kick the main window widget to position and remove the
       status line when it is managed and unmanaged */
   	XtVaSetValues(mainW, XmNshowSeparator, True, NULL);
    if (state) {
    	XtManageChild(window->iSearchForm);
    } else {
     	XtUnmanageChild(window->iSearchForm);
    }
   	XtVaSetValues(mainW, XmNshowSeparator, False, NULL);
   	
   	/* Set the menu item to the current state. */
   	XmToggleButtonSetState(window->iSearchLineItem, state, FALSE);
    
    /* Tell WM that the non-expandable part of the window has changed size */
    UpdateWMSizeHints(window);
}

/*
** Display a special message in the stats line (show the stats line if it
** is not currently shown).
*/
void SetModeMessage(WindowInfo *window, char *message)
{
	/* Save the state of the status line before displaying the message */
	if(!window->modeMessageDisplayed) {
		window->modeMessageShowStats = window->showStats;
	}
    window->modeMessageDisplayed = True;
    XmTextFieldSetString(window->statsLine, message);
    ShowStatsLine(window, True);
}

/*
** Clear special statistics line message set in SetModeMessage, returns
** the statistics line to its original state as set in window->showStats
*/
void ClearModeMessage(WindowInfo *window)
{
    window->modeMessageDisplayed = False;
    ShowStatsLine(window, window->modeMessageShowStats);
    UpdateStatsLine(window);
}

/*
** Count the windows
*/
int NWindows(void)
{
    WindowInfo *win;
    int n;
    
    for (win=WindowList, n=0; win!=NULL; win=win->next, n++);
    return n;
}

/*
** Set autoindent state to one of  NO_AUTO_INDENT, AUTO_INDENT, or SMART_INDENT.
*/
void SetAutoIndent(WindowInfo *window, int state)
{
    int autoIndent = state == AUTO_INDENT, smartIndent = state == SMART_INDENT;
    int i;
    
    if (window->editorInfo->indentStyle == SMART_INDENT && !smartIndent)
    	EndSmartIndent(window);
    else if (smartIndent && window->editorInfo->indentStyle != SMART_INDENT)
    	BeginSmartIndent(window, True);
    window->editorInfo->indentStyle = state;
    XtVaSetValues(window->textArea, textNautoIndent, autoIndent,
    	    textNsmartIndent, smartIndent, NULL);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNautoIndent, autoIndent,
    	    	textNsmartIndent, smartIndent, NULL);
    XmToggleButtonSetState(window->smartIndentItem, smartIndent, False);
    XmToggleButtonSetState(window->autoIndentItem, autoIndent, False);
    XmToggleButtonSetState(window->autoIndentOffItem, state == NO_AUTO_INDENT,
    	    False);
}

/*
** Set the fonts for "window" from a font name, and updates the display.
** Also updates window->fontList which is used for statistics line.
**
** Note that this leaks memory and server resources.  In previous NEdit
** versions, fontLists were carefully tracked and freed, but X and Motif
** have some kind of timing problem when widgets are distroyed, such that
** fonts may not be freed immediately after widget destruction with 100%
** safety.  Rather than kludge around this with timerProcs, I have chosen
** to create new fontLists only when the user explicitly changes the font
** (which shouldn't happen much in normal NEdit operation), and skip the
** futile effort of freeing them.
*/
void SetFonts(WindowInfo *window, char *fontName, char *italicName,
	char *boldName, char *boldItalicName)
{
    XFontStruct *font, *oldFont;
    int i, oldFontWidth, oldFontHeight, fontWidth, fontHeight;
    int marginHeight, marginWidth;
    Dimension borderWidth, borderHeight;
    int primaryChanged, highlightChanged = False;
    Dimension oldWindowWidth, oldWindowHeight, oldTextWidth, oldTextHeight;
    Dimension textWidth, textHeight, newWindowWidth, newWindowHeight;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    
    /* Check which fonts have changed */
    primaryChanged = strcmp(fontName, window->fontName);
    if (strcmp(italicName, window->italicFontName)) highlightChanged = True;
    if (strcmp(boldName, window->boldFontName)) highlightChanged = True;
    if (strcmp(boldItalicName, window->boldItalicFontName))
    	highlightChanged = True;
    if (!primaryChanged && !highlightChanged)
    	return;
    	
    /* Get information about the current window sizing, to be used to
       determine the correct window size after the font is changed */
    XtVaGetValues(window->shell, XmNwidth, &oldWindowWidth, XmNheight,
    	    &oldWindowHeight, NULL);
    XtVaGetValues(window->textArea, XmNheight, &textHeight, XmNwidth,
    	    &textWidth, textNmarginHeight, &marginHeight, textNmarginWidth,
    	    &marginWidth, textNfont, &oldFont, NULL);
    oldTextWidth = textWidth - 2*marginWidth;
    oldTextHeight = textHeight - 2*marginHeight;
    for (i=0; i<window->nPanes; i++) {
    	XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, NULL);
    	oldTextHeight += textHeight - 2*marginHeight;
    }
    borderWidth = oldWindowWidth - oldTextWidth;
    borderHeight = oldWindowHeight - oldTextHeight;
    oldFontWidth = oldFont->max_bounds.width;
    oldFontHeight = textD->ascent + textD->descent;
    
    	
    /* Change the fonts in the window data structure.  If the primary font
       didn't work, use Motif's fallback mechanism by stealing it from the
       statistics line.  Highlight fonts are allowed to be NULL, which
       is interpreted as "use the primary font" */
    if (primaryChanged) {
		strcpy(window->fontName, fontName);
		font = XLoadQueryFont(TheDisplay, fontName);
		if (font == NULL)
    	    XtVaGetValues(window->statsLine, XmNfontList, &window->fontList, NULL);
    	else
	    	window->fontList = XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
    }
    if (highlightChanged) {
	strcpy(window->italicFontName, italicName);
	window->italicFontStruct = XLoadQueryFont(TheDisplay, italicName);
	strcpy(window->boldFontName, boldName);
	window->boldFontStruct = XLoadQueryFont(TheDisplay, boldName);
	strcpy(window->boldItalicFontName, boldItalicName);
	window->boldItalicFontStruct = XLoadQueryFont(TheDisplay, boldItalicName);
    }

    /* Change the primary font in all the widgets */
    if (primaryChanged) {
	font = GetDefaultFontStruct(window->fontList);
	XtVaSetValues(window->textArea, textNfont, font, NULL);
	for (i=0; i<window->nPanes; i++)
    	    XtVaSetValues(window->textPanes[i], textNfont, font, NULL);
    }
    
    /* Change the highlight fonts, even if they didn't change, because
       primary font is read through the style table for syntax highlighting */
    if (window->highlightData != NULL)
    	UpdateHighlightStyles(window);
        
    /* Use the information from the old window to re-size the window to a
       size appropriate for the new font */
    fontWidth = GetDefaultFontStruct(window->fontList)->max_bounds.width;
    fontHeight = textD->ascent + textD->descent;
    newWindowWidth = (oldTextWidth*fontWidth) / oldFontWidth + borderWidth;
    newWindowHeight = (oldTextHeight*fontHeight) / oldFontHeight + borderHeight;
    XtVaSetValues(window->shell, XmNwidth, newWindowWidth, XmNheight,
    	    newWindowHeight, NULL);
    
    /* Change the window manager size hints and the minimum pane height */
    UpdateWMSizeHints(window);
    UpdateMinPaneHeights(window);
}

/*
** Set insert/overstrike mode
*/
void SetOverstrike(WindowInfo *window, int overstrike)
{
    int i;
    WindowInfo *win;

    window->editorInfo->overstrike = overstrike;
    for(win = window->editorInfo->master; win; win = win->nextSlave) {
    	XtVaSetValues(win->textArea, textNoverstrike, overstrike, NULL);
    	for (i=0; i<win->nPanes; i++)
    		XtVaSetValues(win->textPanes[i], textNoverstrike, overstrike, NULL);
    	XmToggleButtonSetState(win->overstrikeItem, overstrike, FALSE);
    }
}

/*
** Select auto-wrap mode, one of NO_WRAP, NEWLINE_WRAP, or CONTINUOUS_WRAP
*/
void SetAutoWrap(WindowInfo *window, int state)
{
    int i;
    int autoWrap = state == NEWLINE_WRAP, contWrap = state == CONTINUOUS_WRAP;
    
    XtVaSetValues(window->textArea, textNautoWrap, autoWrap,
    	    textNcontinuousWrap, contWrap, NULL);
    for (i=0; i<window->nPanes; i++)
    	XtVaSetValues(window->textPanes[i], textNautoWrap, autoWrap,
    	    	textNcontinuousWrap, contWrap, NULL);
    window->editorInfo->wrapMode = state;

    XmToggleButtonSetState(window->newlineWrapItem, autoWrap, False);
    XmToggleButtonSetState(window->continuousWrapItem, contWrap, False);
    XmToggleButtonSetState(window->noWrapItem, state == NO_WRAP, False);
}

/*
** Set the wrap margin (0 == wrap at right edge of window)
*/
void SetWrapMargin(WindowInfo *window, int margin)
{
    int i;
    WindowInfo *w;

    for(w = window->editorInfo->master; w; w = w->nextSlave) {
    	XtVaSetValues(w->textArea, textNwrapMargin, margin, NULL);
    	for (i=0; i<w->nPanes; i++)
    		XtVaSetValues(w->textPanes[i], textNwrapMargin, margin, NULL);
    }
}

/*
** Set auto save mode.
*/
void SetAutoSave(WindowInfo *window, int state)
{
    WindowInfo *win;
    window->editorInfo->autoSave = state;
    for(win = window->editorInfo->master; win; win = win->nextSlave) {
    	XmToggleButtonSetState(win->autoSaveItem, state, False);
    }
}

/*
** Set save old version mode.
*/
void SetSaveOldVersion(WindowInfo *window, int state)
{
    WindowInfo *win;
    window->editorInfo->saveOldVersion = state;
    for(win = window->editorInfo->master; win; win = win->nextSlave) {
    	XmToggleButtonSetState(win->saveLastItem, state, False);
    }
}

/*
** Set show matching mode.
*/
void SetShowMatching(WindowInfo *window, int state)
{
    WindowInfo *win;
    window->editorInfo->showMatching = state;
    for(win = window->editorInfo->master; win; win = win->nextSlave) {
    	XmToggleButtonSetState(win->showMatchingItem, state, False);
    }
}

/*
** Recover the window pointer from any widget in the window, by searching
** up the widget hierarcy for the top level container widget where the window
** pointer is stored in the userData field.
*/
WindowInfo *WidgetToWindow(Widget w)
{
    WindowInfo *window;

	for(; w; w = XtParent(w)) {
		if(strcmp(XtName(w), MAIN_WINDOW_NAME) == 0) {
			XtVaGetValues(w, XmNuserData, &window, NULL);
			return window;
		}
	}
    return NULL;
}

/*
** Change the window appearance and the window data structure to show
** that the file it contains has been modified
*/
void SetWindowModified(WindowInfo *window, int modified)
{

    if (window->editorInfo->fileChanged == FALSE && modified == TRUE) {
		WindowInfo *w;
		for(w = window->editorInfo->master; w; w = w->nextSlave) {
    		XtSetSensitive(w->closeItem, TRUE);
    	}
    	window->editorInfo->fileChanged = TRUE;
    	UpdateWindowTitle(window);
    } else if (window->editorInfo->fileChanged == TRUE && modified == FALSE) {
    	window->editorInfo->fileChanged = FALSE;
    	UpdateWindowTitle(window);
    }
}

/*
** Format the title of a window. Returns a pointer to static storage that
** will be overwritten on the next call.
*/
char *FormatWindowTitle(WindowInfo *window)
{
	static char title[MAXPATHLEN + 35 + 1];    /* strlen(":1 (read only)(locked)(modified)" */
	char slaveNumber[10];

	/* Figure out which slave we are. */
	slaveNumber[0] = 0;
	if(window->editorInfo->master->nextSlave != NULL) {
		WindowInfo *w;
		int num;
		num = 0;
		for(w = window->editorInfo->master; w; w = w->nextSlave) {
			num++;
			if(w == window) {
				break;
			}
		}
		sprintf(slaveNumber, ":%d", num);
	}
	
	/* Set the window title, adding annotations for "modified" or "read-only" */
	sprintf(title, "%s%s%s%s%s%s", 
		window->editorInfo->filename,
		slaveNumber,
		(window->editorInfo->readOnly || window->editorInfo->lockWrite || window->editorInfo->fileChanged) ? " " : "",
		window->editorInfo->readOnly ? "(read only)" : "",
		window->editorInfo->lockWrite ? "(locked)" : "",
		window->editorInfo->fileChanged ? "(modified)" : ""
	);
	return title;
}

/*
** Update the window title to reflect the filename, read-only, and modified
** status of the window data structure
*/
void UpdateWindowTitle(WindowInfo *window)
{
    char title[MAXPATHLEN * 4 + 1] = "";
    char icontitle[MAXPATHLEN + 1 + 1] = ""; /* strlen(">") */
	char server[200 + 3] = "";
    char *ccaseViewTag;
    WindowInfo *w;

	/* Generate title.
	*/
    ccaseViewTag = GetClearCaseViewTag();
    for(w = window->editorInfo->master; w; w = w->nextSlave) {
    	if(isServer) {
    		sprintf(server, "[%s]", GetPrefServerName());
    	}
    	if(ccaseViewTag && ccaseViewTag[0] != 0) {
   			sprintf(server+strlen(server), "{%s}", ccaseViewTag);
    	}
		/* Kludge to more sensibly format the title bar and icons.
		** The first attempt is use the X server vendor string to assume that
		** the window manager uses a Windows like start bar to switch between
		** windows.
		** FIXME!! This should be fixed to interrogate the window manager instead.
		*/
		if(strstr(ServerVendor(TheDisplay), "Hummingbird Communications") != NULL
		|| strstr(ServerVendor(TheDisplay), "Hummingbird Ltd.") != NULL
		|| strstr(ServerVendor(TheDisplay), "XFree86 Project") != NULL
		) {
			/* On Windows they haven't thought of sorting the start menu */
			/* by the title. So having NEditMV at the start serves no purpose. It also makes it */
			/* hard to locate the filename in the start bar if the window is open. */
			/* filename (status) [server]{view} - path */
			sprintf(title, "%s%s%s%s%s", 
				FormatWindowTitle(w),
				strlen(server) != 0 ? " " : "",
				server,
				window->editorInfo->filenameSet ? 
					(strlen(server) != 0 ? " " : " - ") : "",
				window->editorInfo->filenameSet ? window->editorInfo->path : ""
			);
    		SetWindowTitle(w, title, title);
		} else {
			/* NEditMV[server_name]{view}: file_info (status) - path */
			sprintf(title, "%s%s%s%s%s%s", 
				PACKAGE,
				server,
				strlen(server) != 0 ? ": " : ": ",
				FormatWindowTitle(w),
				window->editorInfo->filenameSet ? " - " : "",
				window->editorInfo->filenameSet ? window->editorInfo->path : ""
			);

    		strcpy(icontitle, "");
    		if (window->editorInfo->fileChanged)
    			strcat(icontitle, ">");
    		strcat(icontitle, window->editorInfo->filename);
    		SetWindowTitle(w, title, icontitle);
		}

    	/* If there's a find or replace dialog up in "Keep Up" mode, with a
    	   file name in the title, update it too */
    	if (w->replaceDlog) {
    		sprintf(title, PACKAGE " - Find/Replace (%s)", window->editorInfo->filename);
    		XtVaSetValues(XtParent(w->replaceDlog), XmNtitle, title, NULL);
    	}
    }
}

/*
** Update the read-only state of the text area(s) in the window, and
** the ReadOnly toggle button in the File menu to agree the readOnly
** and lockWrite state in the window data structure.
*/
void UpdateWindowReadOnly(WindowInfo *window)
{
    int i, state;
    WindowInfo *w;

    state = (!GetPrefAllowReadOnlyEdits() && window->editorInfo->readOnly) || window->editorInfo->lockWrite;
    for(w = window->editorInfo->master; w; w = w->nextSlave) {
   		XtVaSetValues(w->textArea, textNreadOnly, state, NULL);
   		for (i=0; i<w->nPanes; i++)
    		XtVaSetValues(w->textPanes[i], textNreadOnly, state, NULL);
    	XmToggleButtonSetState(w->readOnlyItem, state, FALSE);
    	XtSetSensitive(w->readOnlyItem, (GetPrefAllowReadOnlyEdits() || !w->editorInfo->readOnly));
    }
    UpdateWindowTitle(window);
}

void SetWindowTitle(WindowInfo *window, char *title, char *icontitle)
{
    /* Set both the window title and the icon title */
    XtVaSetValues(window->shell, XmNtitle, title, XmNiconName, icontitle, NULL);

    /* Update the Windows menus with the new name */
    InvalidateWindowMenus();
}

/*
** Get the start and end of the current selection.  This routine is obsolete
** because it ignores rectangular selections, and reads from the widget
** instead of the buffer.  Use BufGetSelectionPos.
*/
int GetSelection(Widget widget, int *left, int *right)
{
    return GetSimpleSelection(TextGetBuffer(widget), left, right);
}

/*
** Find the start and end of a single line selection.  Hides rectangular
** selection issues for older routines which use selections that won't
** span lines.
*/
int GetSimpleSelection(textBuffer *buf, int *left, int *right)
{
    int selStart, selEnd, isRect, rectStart, rectEnd, lineStart;

    /* get the character to match and its position from the selection, or
       the character before the insert point if nothing is selected.
       Give up if too many characters are selected */
    if (!BufGetSelectionPos(buf, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
        return False;
    if (isRect) {
    	lineStart = BufStartOfLine(buf, selStart);
    	selStart = BufCountForwardDispChars(buf, lineStart, rectStart);
    	selEnd = BufCountForwardDispChars(buf, lineStart, rectEnd);
    }
    *left = selStart;
    *right = selEnd;
    return True;
}

/*
** Returns a range of text from a text widget (this routine is obsolete,
** get text from the buffer instead).  Memory is allocated with
** XtMalloc and caller should free it.
*/
char *GetTextRange(Widget widget, int left, int right)
{
    return BufGetRange(TextGetBuffer(widget), left, right);
}

/*
** If the selection (or cursor position if there's no selection) is not
** fully shown, scroll to bring it in to view.  Note that as written,
** this won't work well with multi-line selections.  Modest re-write
** of the horizontal scrolling part would be quite easy to make it work
** well with rectangular selections.
*/
void MakeSelectionVisible(WindowInfo *window, Widget textPane)
{
    int left, right, isRect, rectStart, rectEnd, horizOffset;
    int scrollOffset, leftX, rightX, y, rows, margin;
    int topLineNum, lastLineNum, rightLineNum, leftLineNum, linesToScroll;
    textBuffer *buf = window->editorInfo->buffer;
    int topChar = TextFirstVisiblePos(textPane);
    int lastChar = TextLastVisiblePos(textPane);
    Dimension width;
    
    /* find out where the selection is */
    if (!BufGetSelectionPos(window->editorInfo->buffer, &left, &right, &isRect,
    	    &rectStart, &rectEnd)) {
    	left = right = TextGetCursorPos(textPane);
    	isRect = False;
    }
    	
    /* Check vertical positioning unless the selection is already shown or
       already covers the display.  If the end of the selection is below
       bottom, scroll it in to view until the end selection is scrollOffset
       lines from the bottom of the display or the start of the selection
       scrollOffset lines from the top.  Calculate a pleasing distance from the
       top or bottom of the window, to scroll the selection to (if scrolling is
       necessary), around 1/3 of the height of the window */
    if (!((left >= topChar && right <= lastChar) ||
    	    (left < topChar && right > lastChar))) {
	XtVaGetValues(textPane, textNrows, &rows, NULL);
	scrollOffset = rows/3;
	TextGetScroll(textPane, &topLineNum, &horizOffset);
	lastLineNum = topLineNum + rows;
	if (right > lastChar) {
            if (left <= topChar)
        	return;
            leftLineNum = topLineNum + BufCountLines(buf, topChar, left);
            if (leftLineNum < topLineNum + scrollOffset)
        	return;
            linesToScroll = BufCountLines(buf, lastChar, right) + scrollOffset;
            if (leftLineNum - linesToScroll < topLineNum + scrollOffset)
        	linesToScroll = leftLineNum - (topLineNum + scrollOffset);
    	    TextSetScroll(textPane, topLineNum+linesToScroll, horizOffset);
	} else if (left < topChar) {
            if (right >= lastChar)
        	return;
            rightLineNum = lastLineNum - BufCountLines(buf, right, lastChar);
            if (rightLineNum > lastLineNum - scrollOffset)
        	return;
            linesToScroll = BufCountLines(buf, left, topChar) + scrollOffset;
            if (rightLineNum + linesToScroll > lastLineNum - scrollOffset)
        	linesToScroll = (lastLineNum - scrollOffset) - rightLineNum;
    	    TextSetScroll(textPane, topLineNum-linesToScroll, horizOffset);
	}
    }
    
    /* If either end of the selection off screen horizontally, try to bring it
       in view, by making sure both end-points are visible.  Using only end
       points of a multi-line selection is not a great idea, and disaster for
       rectangular selections, so this part of the routine should be re-written
       if it is to be used much with either.  Note also that this is a second
       scrolling operation, causing the display to jump twice.  It's done after
       vertical scrolling to take advantage of TextPosToXY which requires it's
       reqested position to be vertically on screen) */
    if (    TextPosToXY(textPane, left, &leftX, &y) &&
    	    TextPosToXY(textPane, right, &rightX, &y) && leftX <= rightX) {
    	TextGetScroll(textPane, &topLineNum, &horizOffset);
    	XtVaGetValues(textPane, XmNwidth, &width, textNmarginWidth, &margin, NULL);
    	if (leftX < margin)
    	    horizOffset -= margin - leftX;
    	else if (rightX > width - margin)
    	    horizOffset += rightX - (width - margin);
    	TextSetScroll(textPane, topLineNum, horizOffset);
    }
     
    /* make sure that the statistics line is up to date */
    UpdateStatsLine(window);
}

/*
** Move the selection to the center of the visible area.
*/
void MoveSelectionToMiddle(WindowInfo *window, Widget textPane)
{
    int left, right, isRect, rectStart, rectEnd, horizOffset;
    int newTopLine, leftX, rightX, y, rows, margin;
    int leftLineNum, selLines;
	int topLineNum;
    textBuffer *buf = window->editorInfo->buffer;
    int topChar = TextFirstVisiblePos(textPane);
    int lastChar = TextLastVisiblePos(textPane);
    Dimension width;
    
    /* find out where the selection is */
    if (!BufGetSelectionPos(window->editorInfo->buffer, &left, &right, &isRect,
    	    &rectStart, &rectEnd)) {
    	left = right = TextGetCursorPos(textPane);
    	isRect = False;
    }
    
	/* Get the line number of the first visible character from the scroll bar. */
	TextGetScroll(textPane, &topLineNum, &horizOffset);
	
	/* Determine the line number of the selection by counting the number
	   of line from the first visible character. This should be more efficient
	   than always counting from the beginning of the file. */
	if(left >= topChar) {
		leftLineNum = topLineNum+BufCountLines(buf, topChar, left);
	} else {
		leftLineNum = topLineNum-BufCountLines(buf, left, topChar);
	}
	selLines = BufCountLines(buf, left, right);

	XtVaGetValues(textPane, textNrows, &rows, NULL);

	/* If the selection is larger than the visible are then move the
	   start of the selection to the top of the visible area. */
	if(selLines > rows) {
		newTopLine = leftLineNum;
	} else {
		/* Center the selection. */
		newTopLine = leftLineNum+selLines/2-rows/2;
	}
	
	TextSetScroll(textPane, newTopLine, horizOffset);
	
    /* If either end of the selection off screen horizontally, try to bring it
       in view, by making sure both end-points are visible.  Using only end
       points of a multi-line selection is not a great idea, and disaster for
       rectangular selections, so this part of the routine should be re-written
       if it is to be used much with either.  Note also that this is a second
       scrolling operation, causing the display to jump twice.  It's done after
       vertical scrolling to take advantage of TextPosToXY which requires it's
       reqested position to be vertically on screen) */
    if (    TextPosToXY(textPane, left, &leftX, &y) &&
    	    TextPosToXY(textPane, right, &rightX, &y) && leftX <= rightX) {
    	TextGetScroll(textPane, &topLineNum, &horizOffset);
    	XtVaGetValues(textPane, XmNwidth, &width, textNmarginWidth, &margin, NULL);
    	if (leftX < margin)
    	    horizOffset -= margin - leftX;
    	else if (rightX > width - margin)
    	    horizOffset += rightX - (width - margin);
    	TextSetScroll(textPane, topLineNum, horizOffset);
    }
     
    /* make sure that the statistics line is up to date */
    UpdateStatsLine(window);
}

static Widget createTextArea(Widget parent, WindowInfo *window, int rows,
	int cols, int emTabDist, char *delimiters, int wrapMargin)
{
    Widget text, sw, hScrollBar;
    XFontStruct *fs;
    int marginHeight;
    Dimension hsbHeight, swMarginHeight;
        
    /* Create a text widget inside of a scrolled window widget */
    sw = XtVaCreateManagedWidget("scrolledW", xmScrolledWindowWidgetClass,
    	    parent, XmNspacing, 0, XmNpaneMaximum, SHRT_MAX,
    	    XmNpaneMinimum, PANE_MIN_HEIGHT, XmNhighlightThickness, 0, NULL); 
    text = XtVaCreateManagedWidget("text", textWidgetClass, sw,
    	    textNrows, rows, textNcolumns, cols,
    	    textNemulateTabs, emTabDist,
    	    textNfont, GetDefaultFontStruct(window->fontList),
    	    textNreadOnly, (!GetPrefAllowReadOnlyEdits() && window->editorInfo->readOnly) || window->editorInfo->lockWrite,
    	    textNwordDelimiters, delimiters,
    	    textNwrapMargin, wrapMargin,
    	    textNautoIndent, window->editorInfo->indentStyle == AUTO_INDENT,
    	    textNsmartIndent, window->editorInfo->indentStyle == SMART_INDENT,
    	    textNautoWrap, window->editorInfo->wrapMode == NEWLINE_WRAP,
    	    textNcontinuousWrap, window->editorInfo->wrapMode == CONTINUOUS_WRAP,
    	    textNoverstrike, window->editorInfo->overstrike, NULL);
    
    /* add focus, drag, cursor tracking, and smart indent callbacks */
    XtAddCallback(text, textNfocusCallback, (XtCallbackProc)focusCB, window);
    XtAddCallback(text, textNcursorMovementCallback, (XtCallbackProc)movedCB,
    	    window);
    XtAddCallback(text, textNdragStartCallback, (XtCallbackProc)dragStartCB,
    	    window);
    XtAddCallback(text, textNdragEndCallback, (XtCallbackProc)dragEndCB,
    	    window);
    XtAddCallback(text, textNsmartIndentCallback, SmartIndentCB, window);
    	    
    /* This makes sure the text area initially has a the insert point shown
       ... (check if still true with the nedit text widget, probably not) */
    XmAddTabGroup(XtParent(text));

    /* set the minimum size of a pane */
    XtVaGetValues(sw, XmNhorizontalScrollBar, &hScrollBar, 
    		XmNscrolledWindowMarginHeight, &swMarginHeight, NULL);
    XtVaGetValues(text, textNmarginHeight, &marginHeight, textNfont, &fs, NULL);
    XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, NULL);
    setPaneMinHeight(sw, fs->ascent + fs->descent + marginHeight*2 +
    	    swMarginHeight*2 + hsbHeight);
    
    /* compensate for Motif delete/backspace problem */
    RemapDeleteKey(text);

    /* Augment translation table for right button popup menu */
    AddBGMenuAction(text);
   
    return text;
}

static void movedCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    if (window->editorInfo->ignoreModify)
    	return;

    /* update line and column nubers in statistics line */
    UpdateStatsLine(window);
    
    /* Check the character before the cursor for matchable characters */
    FlashMatching(window, w);
}

static void modifiedCB(int pos, int nInserted, int nDeleted, int nRestyled,
	char *deletedText, void *cbArg) 
{
    WindowInfo *window = (WindowInfo *)cbArg;
    int selected = window->editorInfo->buffer->primary.selected;
    
    /* update the table of bookmarks */
    UpdateMarkTable(window, pos, nInserted, nDeleted);
    
    /* Check and dim/undim selection related menu items */
    if ((window->wasSelected && !selected) || (!window->wasSelected && selected)) {
    	window->wasSelected = selected;
    	XtSetSensitive(window->printSelItem, selected);
    	XtSetSensitive(window->cutItem, selected);
    	XtSetSensitive(window->copyItem, selected);
#ifndef VMS
    	XtSetSensitive(window->filterItem, selected);
#endif

    	DimSelectionDepUserMenuItems(window, selected);
    	if (window->replaceDlog != NULL)
    	    XtSetSensitive(window->replaceInSelBtn, selected);
    }
    
    /* When the program needs to make a change to a text area without without
       recording it for undo or marking file as changed it sets ignoreModify */
    if (window->editorInfo->ignoreModify || (nDeleted == 0 && nInserted == 0))
    	return;
    
    /* Only the master keeps track of the undo info */
    if(window == window->editorInfo->master) {
    	/* Save information for undoing this operation (this call also counts
    	   characters and editing operations for triggering autosave */
    	SaveUndoInformation(window, pos, nInserted, nDeleted, deletedText);

    	/* Trigger automatic backup if operation or character limits reached */
    	if (window->editorInfo->autoSave &&
    	    	(window->editorInfo->autoSaveCharCount > AUTOSAVE_CHAR_LIMIT ||
    	    	 window->editorInfo->autoSaveOpCount > AUTOSAVE_OP_LIMIT)) {
    		WriteBackupFile(window);
    		window->editorInfo->autoSaveCharCount = 0;
    		window->editorInfo->autoSaveOpCount = 0;
    	}
    }
    
    /* Indicate that the window has now been modified */ 
    SetWindowModified(window, TRUE);

    /* Update # of bytes, and line and col statistics */
    UpdateStatsLine(window);
}

static void focusCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    /* record which window pane last had the keyboard focus */
    window->lastFocus = w;
    
    /* update line number statistic to reflect current focus pane */
    UpdateStatsLine(window);
    
    CheckForChangesInFile(window);
}

static void dragStartCB(Widget w, WindowInfo *window, XtPointer callData) 
{
    /* don't record all of the intermediate drag steps for undo */
    window->editorInfo->ignoreModify = True;
}

static void dragEndCB(Widget w, WindowInfo *window, dragEndCBStruct *callData) 
{
    /* restore recording of undo information */
    window->editorInfo->ignoreModify = False;
    
    /* Do nothing if drag operation was canceled */
    if (callData->nCharsInserted == 0)
    	return;
    	
    /* Save information for undoing this operation not saved while
       undo recording was off */
    modifiedCB(callData->startPos, callData->nCharsInserted,
    	    callData->nCharsDeleted, 0, callData->deletedText, window);
}

static void closeCB(Widget w, WindowInfo *window, XtPointer callData) 
{
	/* Don't close the window if a shell command is in progress for
	** this window. */
	if(IsShellCommandInProgress(window)) {
    	XBell(TheDisplay, 0);
		return;
	}
	
    if (WindowList->next == NULL) {
    	if (!CheckPrefsChangesSaved(window->shell))
    	    return;
    	if (!WindowList->editorInfo->fileChanged)
     	    exit(0);
     	if (CloseFileAndWindow(window))
     	    exit(0);
    } else
    	CloseFileAndWindow(window);
    	
}

/*
** Add a window to the the window list.
*/
static void addToWindowList(WindowInfo *window) 
{
    WindowInfo *temp;

    temp = WindowList;
    WindowList = window;
    window->next = temp;
}

/*
** Remove a window from the list of windows
*/
static void removeFromWindowList(WindowInfo *window)
{
    WindowInfo *temp;

    if (WindowList == window)
	WindowList = window->next;
    else {
	for (temp = WindowList; temp != NULL; temp = temp->next) {
	    if (temp->next == window) {
		temp->next = window->next;
		break;
	    }
	}
    }
}

/*
** Update the optional statistics line.  
*/
void UpdateStatsLine(WindowInfo *window)
{
    int line, pos, colNum;
    char *string;
    Widget statW = window->statsLine;
    char *path;
#ifdef SGI_CUSTOM
    char *sleft, *smid, *sright;
#endif
    
    /* This routine is called for each character typed, so its performance
       affects overall editor perfomance.  Only update if the line is on
       and not displaying a special mode message */ 
    if (!window->showStats || window->modeMessageDisplayed)
    	return;
    
    /* Compose the string to display. If line # isn't available, leave it off */
    pos = TextGetCursorPos(window->lastFocus);
    /* Display the path only if filenameSet is set. path can be set for use
       as the current directory for shell command and for the initial 
       directory of the open dialog but it shouldn't be displayed as the
       path of the file if filenameSet is not set. This is normally the case 
       with the Untitled window. */
    path = window->editorInfo->filenameSet ? window->editorInfo->path : "";
    string = XtMalloc(strlen(window->editorInfo->filename) + strlen(path) + 45);
    if (!TextPosToLineAndCol(window->lastFocus, pos, &line, &colNum))
    	sprintf(string, "%s%s %d bytes", path, window->editorInfo->filename,
    		window->editorInfo->buffer->length);
    else {
    	sprintf(string, "%s%s line %d, col %d, %d bytes", path, 
    		window->editorInfo->filename, line, colNum, window->editorInfo->buffer->length);
    }
    
    /* Change the text in the stats line */
#ifdef SGI_CUSTOM
    /* don't show full pathname, just dir and filename (+ line/col/byte info) */
#ifdef WIN32
	smid = strpbrk(string, "/\\");
#else
	smid = strchr(string, '/'); 
#endif
    if ( smid != NULL ) {
	sleft = smid;
#ifdef WIN32
		{
		char *s1, *s2;
		s1 = strrchr(string, '/');
		s2 = strrchr(string, '\\');
		sright = max(s1, s2));
		}
#else
		sright = strrchr(string, '/'); 
#endif
        while (strcmp(smid, sright)) {
		sleft = smid;
#ifdef WIN32
		smid = strpbrk(sleft + 1, "/\\");
#else
		smid = strchr(sleft + 1, '/');
#endif
	}
    	XmTextFieldSetString(statW, sleft + 1);
    } else
    	XmTextFieldSetString(statW, string);
#else
    XmTextFieldSetString(statW, string);
#endif
    XtFree(string);
}

/*
** Paned windows are impossible to adjust after they are created, which makes
** them nearly useless for NEdit (or any application which needs to dynamically
** adjust the panes) unless you tweek some private data to overwrite the
** desired and minimum pane heights which were set at creation time.  These
** will probably break in a future release of Motif because of dependence on
** private data.
*/
static void setPaneDesiredHeight(Widget w, int height)
{
    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.dheight = height;
}
static void setPaneMinHeight(Widget w, int min)
{
    ((XmPanedWindowConstraintPtr)w->core.constraints)->panedw.min = min;
}

/*
** Update the window manager's size hints.  These tell it the increments in
** which it is allowed to resize the window.  While this isn't particularly
** important for NEdit (since it can tolerate any window size), setting these
** hints also makes the resize indicator show the window size in characters
** rather than pixels, which is very helpful to users.
*/
void UpdateWMSizeHints(WindowInfo *window)
{
    Dimension shellWidth=0, shellHeight=0, textHeight=0, textWidth=0, hScrollBarHeight=0;
    int marginHeight=0, marginWidth=0, totalHeight=0;
    XFontStruct *fs;
    int i, baseWidth=0, baseHeight=0, fontHeight=0, fontWidth=0;
    Widget hScrollBar;
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;

    /* Find the base (non-expandable) width and height of the editor window */
    XtVaGetValues(window->textArea, XmNheight, &textHeight, XmNwidth,
    	    &textWidth, textNmarginHeight, &marginHeight, textNmarginWidth,
    	    &marginWidth, NULL);
    totalHeight = textHeight - 2*marginHeight;
    for (i=0; i<window->nPanes; i++) {
    	XtVaGetValues(window->textPanes[i], XmNheight, &textHeight, 
    		textNhScrollBar, &hScrollBar, NULL);
    	totalHeight += textHeight - 2*marginHeight;
    	if (hScrollBar != NULL && !XtIsManaged(hScrollBar)) {
    	    XtVaGetValues(hScrollBar, XmNheight, &hScrollBarHeight, NULL);
    	    totalHeight -= hScrollBarHeight;
    	}
    }

    /* Find the dimensions of a single character of the text font */
    XtVaGetValues(window->textArea, textNfont, &fs, NULL);
    fontHeight = textD->ascent + textD->descent;
    fontWidth = fs->max_bounds.width;
    
    XtVaGetValues(window->shell, XmNwidth, &shellWidth,
    	    XmNheight, &shellHeight, NULL);
    baseWidth = shellWidth - (textWidth - 2*marginWidth);
    baseHeight = shellHeight - totalHeight;
    
    /* Set the size hints in the shell widget */
    XtVaSetValues(window->shell, XmNwidthInc, fs->max_bounds.width,
    	    XmNheightInc, fontHeight,
    	    XmNbaseWidth, baseWidth, 
			XmNbaseHeight, baseHeight,
    	    XmNminWidth, baseWidth + fontWidth,
    	    XmNminHeight, baseHeight + (1+window->nPanes) * fontHeight, NULL);
}

/*
** Update the minimum allowable height for a split window pane after a change
** to font or margin height.
*/
void UpdateMinPaneHeights(WindowInfo *window)
{
    textDisp *textD = ((TextWidget)window->textArea)->text.textD;
    Dimension hsbHeight, swMarginHeight, minPaneHeight;
    int i, marginHeight;
    Widget hScrollBar;

    /* find the minimum allowable size for a pane */
    XtVaGetValues(window->textArea, textNhScrollBar, &hScrollBar, NULL);
    XtVaGetValues(XtParent(window->textArea), XmNscrolledWindowMarginHeight,
    	    &swMarginHeight, NULL);
    XtVaGetValues(window->textArea, textNmarginHeight, &marginHeight, NULL);
    XtVaGetValues(hScrollBar, XmNheight, &hsbHeight, NULL);
    minPaneHeight = textD->ascent + textD->descent + marginHeight*2 +
    	    swMarginHeight*2 + hsbHeight;
    
    /* Set it in all of the widgets in the window */
    setPaneMinHeight(XtParent(window->textArea), minPaneHeight);
    for (i=0; i<window->nPanes; i++)
    	setPaneMinHeight(XtParent(window->textPanes[i]), minPaneHeight);
	
	{
	XmFontList fl;
	XFontStruct *fs;
	Dimension marginHeight = 0;
	Dimension borderWidth = 0;
	Dimension shadowThickness = 0;
	Dimension highlightThickness = 0;
	Dimension paneHeight;
    XtVaGetValues(window->statsLine, 
		XmNmarginHeight, &marginHeight,
		XmNborderWidth, &borderWidth,
		XmNshadowThickness, &shadowThickness,
		XmNhighlightThickness, &highlightThickness,
		XmNfontList, &fl,
		NULL);
	fs = GetDefaultFontStruct(fl);
    paneHeight = fs->ascent + fs->descent + marginHeight*2 +
		borderWidth*2 + shadowThickness*2 + highlightThickness*2;
	XtVaSetValues(window->statsLine,
		XmNpaneMinimum, paneHeight,
		XmNpaneMaximum, paneHeight,
		NULL);
	}
}

/* Add an icon to an applicaction shell widget.  addWindowIcon adds a large
** (primary window) icon, AddSmallIcon adds a small (secondary window) icon.
**
** Note: I would prefer that these were not hardwired, but yhere is something
** weird about the  XmNiconPixmap resource that prevents it from being set
** from the defaults in the application resource database.
*/
static void addWindowIcon(Widget shell)
{ 
    static Pixmap iconPixmap = 0, maskPixmap = 0;

    if (iconPixmap == 0) {
    	iconPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen(shell)), (char *)iconBits,
    		iconBitmapWidth, iconBitmapHeight);
    	maskPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen(shell)), (char *)maskBits,
    		iconBitmapWidth, iconBitmapHeight);
    }
    XtVaSetValues(shell, XmNiconPixmap, iconPixmap, XmNiconMask, maskPixmap, NULL);
}
void AddSmallIcon(Widget shell)
{ 
    static Pixmap iconPixmap = 0, maskPixmap = 0;

    if (iconPixmap == 0) {
    	iconPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen(shell)), (char *)n_bits,
    		n_width, n_height);
    	maskPixmap = XCreateBitmapFromData(TheDisplay,
    		RootWindowOfScreen(XtScreen(shell)), (char *)n_mask,
    		n_width, n_height);
    }
    XtVaSetValues(shell, XmNiconPixmap, iconPixmap,
    	    XmNiconMask, maskPixmap, NULL);
}

/*
** Xt timer procedure for updating size hints.  The new sizes of objects in
** the window are not ready immediately after adding or removing panes.  This
** is a timer routine to be invoked with a timeout of 0 to give the event
** loop a chance to finish processing the size changes before reading them
** out for setting the window manager size hints.
*/
static void wmSizeUpdateProc(XtPointer clientData, XtIntervalId *id)
{
    UpdateWMSizeHints((WindowInfo *)clientData);
}

#ifdef ROWCOLPATCH
/*
** There is a bad memory reference in the delete_child method of the
** RowColumn widget in some Motif versions (so far, just Solaris with Motif
** 1.2.3) which appears durring the phase 2 destroy of the widget. This
** patch replaces the method with a call to the Composite widget's
** delete_child method.  The composite delete_child method handles part,
** but not all of what would have been done by the original method, meaning
** that this is dangerous and should be used sparingly.  Note that
** patchRowCol is called only in CloseWindow, before the widget is about to
** be destroyed, and only on systems where the bug has been observed
*/
static void patchRowCol(Widget w)
{
    ((XmRowColumnClassRec *)XtClass(w))->composite_class.delete_child =
    	    patchedRemoveChild;
}
static void patchedRemoveChild(Widget child)
{
    /* Call composite class method instead of broken row col delete_child
       method */
    (*((CompositeWidgetClass)compositeWidgetClass)->composite_class.
                delete_child) (child);
}
#endif /* ROWCOLPATCH */

static void iSearchToggleCB(Widget w, WindowInfo *window, XtPointer callData) 
{
	XmToggleButtonCallbackStruct *cbs = (XmToggleButtonCallbackStruct*)callData;
	if(w == window->iSearchCaseToggle) {
		/* disable the RegEx toggle anytime the case toggle is toggled */
		XmToggleButtonSetState(window->iSearchRegExpToggle, False, False);
	} else if(w == window->iSearchRegExpToggle) {
		/* Set the case toggle whenever the RegEx button is set to indicate that
		** RegEx searches are always case sensitive */
		if(cbs->set) {
			XmToggleButtonSetState(window->iSearchCaseToggle, True, False);
		}
	}
}

static void iSearchHideCB(Widget w, WindowInfo *window, XtPointer callData)
{
	ShowISearchLine(window, False);
}

/*
** Block until all of the expose events are processed.
** This is a copy of the code in XmUpdateDisplay with the
** display value hardcoded and the XSync() commented out.
*/
void UpdateDisplay(void)
{
	XEvent event;
#if 0
	XSync (TheDisplay, 0);
#endif
	while (XCheckMaskEvent(TheDisplay, ExposureMask, &event))
		ServerDispatchEvent(&event);
}
