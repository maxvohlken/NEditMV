/*******************************************************************************
*									       *
* search.c -- Nirvana Editor search and replace functions		       *
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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include <Xm/Xm.h>
#include <X11/Shell.h>
#include <Xm/XmP.h>
#include <Xm/Form.h>
#include <Xm/Label.h>
#include <Xm/PushB.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/ToggleB.h>
#include <X11/Xatom.h>		/* for getting selection */
#include <X11/keysym.h>
#ifdef MOTIF10
#include <X11/Selection.h>	/* " " */
#endif
#include <X11/X.h>		/* " " */
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "regularExp.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "search.h"
#include "window.h" 
#include "preferences.h"
#include "server.h"

typedef struct _SelectionInfo {
	int done;
	WindowInfo* window;
	char* selection;
} SelectionInfo;

/* History mechanism for search and replace strings */
static char *SearchHistory[MAX_SEARCH_HISTORY];
static char *ReplaceHistory[MAX_SEARCH_HISTORY];
static int SearchTypeHistory[MAX_SEARCH_HISTORY];
static int SearchIsIncrementalHistory[MAX_SEARCH_HISTORY];
static int HistStart = 0;
static int NHist = 0;

static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static void createReplaceDlog(Widget parent, WindowInfo *window);
static void rFocusCB(Widget w, WindowInfo *window, caddr_t *callData);
static void rKeepCB(Widget w, WindowInfo *window, caddr_t *callData);
static void replaceCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData); 
static void replaceAllCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void rInSelCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData); 
static void rCancelCB(Widget w, WindowInfo *window, caddr_t callData);
static void rFindCB(Widget w,WindowInfo *window, XmAnyCallbackStruct *callData);
static void iSearchTextActivateCB(Widget w,WindowInfo *window, XmAnyCallbackStruct *callData);
static void iSearchTextValueChangedCB(Widget w,WindowInfo *window, XmAnyCallbackStruct *callData);
static void iSearchTextArrowKeyEH(Widget w, WindowInfo *window, XKeyEvent *event);
static void replaceFindCB(Widget w, WindowInfo *window,
	XmAnyCallbackStruct *callData);
static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void replaceArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event);
static void flashTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void eraseFlash(WindowInfo *window);
static int getReplaceDlogInfo(WindowInfo *window, int *direction,
	char *searchString, char *replaceString, int *searchType);
static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
	Atom *type, char *value, int *length, int *format);
static int searchLiteral(char *string, char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos);
static int searchRegex(char *string, char *searchString, int direction,
	int wrap, int beginPos, int *startPos, int *endPos, char *delimiters);
static int forwardRegexSearch(char *string, char *searchString,
	int wrap, int beginPos, int *startPos, int *endPos, char *delimiters);
static int backwardRegexSearch(char *string, char *searchString,
	int wrap, int beginPos, int *startPos, int *endPos, char *delimiters);
static void upCaseString(char *outString, char *inString);
static void downCaseString(char *outString, char *inString);
static void resetReplaceTabGroup(WindowInfo *window);
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right);
static int findMatchingChar(textBuffer *buf, int charPos,int startLimit, 
	int endLimit, int *matchBeginPos, int *matchPos);
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen, char *delimiters);
static int historyIndex(int nCycles);
static char *searchTypeArg(int searchType);
static char *directionArg(int direction);
static int isStartOfLine(char *string, int beginPos);
static int isStartOfWord(char *string, int beginPos);

typedef struct _charMatchTable {
    char begin;
    char end;
} charMatchTable;

static charMatchTable MatchingChars[] = {
    {'{', '}'},
    {'(', ')'},
    {'[', ']'},
    {'<', '>'},
    {'"', '"'},
    {'\'', '\''},
    {'`', '`'},
#if 0
    {'/', '/'},
    {'\\', '\\'},
#endif
    {0, 0}
};
    
void DoFindReplaceDlog(WindowInfo *window, int direction, FindReplaceDlogDefaultButton defaultButton, Time time)
{
    FindReplaceDlogDefaultButton oldDefaultButton;
    Widget newDefaultButtonWidget;
    XEvent nextEvent;
    char *primary_selection = 0;

    /* Create the dialog if it doesn't already exist */
    if (window->replaceDlog == NULL)
    	createReplaceDlog(window->shell, window);
    
    if(GetPrefFindReplaceUsesSelection()) {
    	SelectionInfo selectionInfo;

    	selectionInfo.done = 0;
    	selectionInfo.window = window;
    	selectionInfo.selection = 0;
    	XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    	(XtSelectionCallbackProc)getSelectionCB, &selectionInfo, time);
		while (selectionInfo.done == 0) {
	    	XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
	    	ServerDispatchEvent(&nextEvent);
		}
		primary_selection = selectionInfo.selection;
	}
	if(primary_selection == 0) {
		primary_selection = XtNewString("");
	}

	/* Update the String to find field */
   	XmTextSetString(window->replaceText, primary_selection);

    XtVaGetValues(window->findBtn, XmNuserData, &oldDefaultButton, NULL);

    /* Use the userData field of the find button to keep track of the
       current default button */
    SET_ONE_RSRC(window->findBtn, XmNuserData, defaultButton);
    
   	newDefaultButtonWidget = defaultButton == FIND_REPLACE_FIND_BUTTON_DEFAULT ? window->findBtn : 
   	                         defaultButton == FIND_REPLACE_REPLACE_BUTTON_DEFAULT ? window->replaceFindBtn :
   	                         window->findBtn;
    /* if we are setting a new default button then update the 
       XmNshowAsDefault value on the old default button */
    if(oldDefaultButton != FIND_REPLACE_NO_BUTTON_DEFAULT && oldDefaultButton != defaultButton) {
   		Widget old = defaultButton == FIND_REPLACE_FIND_BUTTON_DEFAULT ? window->findBtn : 
	   	             defaultButton == FIND_REPLACE_REPLACE_BUTTON_DEFAULT ? window->replaceFindBtn :
 	  	             window->findBtn;

    	SET_ONE_RSRC(old, XmNshowAsDefault, 0);
    }
    /* if we are setting a new default button then update the 
       XmNshowAsDefault value on the new default button */
    if(oldDefaultButton == FIND_REPLACE_NO_BUTTON_DEFAULT || oldDefaultButton != defaultButton) {
    	SET_ONE_RSRC(newDefaultButtonWidget, XmNshowAsDefault, 1);
	}
	
   	SET_ONE_RSRC(window->replaceDlog, XmNdefaultButton, newDefaultButtonWidget);

    /* If the window is already up, just pop it to the top */
    if (XtIsManaged(window->replaceDlog)) {
    	RaiseShellWindow(XtParent(window->replaceDlog));
    	XtFree(primary_selection);
    	return;
    }
    	
    /* Blank the Replace with field */
   	XmTextSetString(window->replaceWithText, "");

    /* Set the initial search type */
    switch (GetPrefSearch()) {
      case SEARCH_LITERAL:
    	XmToggleButtonSetState(window->replaceLiteralBtn, True, True);
	break;
      case SEARCH_CASE_SENSE:
    	XmToggleButtonSetState(window->replaceCaseBtn, True, True);
	break;
      case SEARCH_REGEX:
    	XmToggleButtonSetState(window->replaceRegExpBtn, True, True);
	break;
    }
    
    /* Set the initial direction based on the direction argument */
    XmToggleButtonSetState(direction == SEARCH_FORWARD ?
	    window->replaceFwdBtn : window->replaceRevBtn, True, True);
    
    /* Set the state of the Keep Dialog Up button */
    XmToggleButtonSetState(window->replaceKeepBtn, GetPrefKeepSearchDlogs(),
    	    True);
    
    /* Start the search history mechanism at the current history item */
    window->rHistIndex = 0;
    
   	XtFree(primary_selection);

    /* Display the dialog */
    ManageDialogJustOutsideWindow(window->replaceDlog, window->shell);
}

static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
	WindowInfo *window = selectionInfo->window;

    /* return an empty string if we can't get the selection data */
    if (*type == XT_CONVERT_FAIL || *type != XA_STRING || value == NULL || *length == 0) {
    	XtFree(value);
    	selectionInfo->selection = 0;
		selectionInfo->done = 1;
    	return;
    }
    /* return an empty string if the data is not of the correct format. */
    if (*format != 8) {
		DialogF(DF_WARN, window->shell, 1, "NEdit can't handle non 8-bit text", "OK");
    	XtFree(value);
    	selectionInfo->selection = 0;
		selectionInfo->done = 1;
    	return;
    }
    selectionInfo->selection = XtMalloc(*length+1);
    memcpy(selectionInfo->selection, value, *length);
    selectionInfo->selection[*length] = 0;
    XtFree(value);
	selectionInfo->done = 1;
}

void SetISearchTextCallbacks (WindowInfo *window)
{
    XtAddCallback(window->iSearchText, XmNactivateCallback, 
      (XtCallbackProc) iSearchTextActivateCB, window);
    XtAddCallback(window->iSearchText, XmNvalueChangedCallback, 
      (XtCallbackProc) iSearchTextValueChangedCB, window);
    XtAddEventHandler(window->iSearchText, KeyPressMask, False,
      (XtEventHandler)iSearchTextArrowKeyEH, window);
}

static void createReplaceDlog(Widget parent, WindowInfo *window)
{
    Arg    	args[50];
    int    	argcnt;
    XmString	st1;
    Widget	form, btnForm;
    Widget	searchTypeBox, literalBtn, caseBtn, regExpBtn;
    Widget	label2, label1, label, replaceText, findText;
    Widget	findBtn, replaceAllBtn, rInSelBtn, cancelBtn, replaceBtn;
    Widget    	replaceFindBtn;
    Widget	searchDirBox, forwardBtn, reverseBtn, keepBtn;
    char 	title[MAXPATHLEN + 14];
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNautoUnmanage, False); argcnt++;
    form = XmCreateFormDialog(parent, "replaceDialog", args, argcnt);
    XtVaSetValues(form, XmNshadowThickness, 0, NULL);
   	sprintf(title, PACKAGE " - Find/Replace (%s)", window->editorInfo->filename);
   	XtVaSetValues(XtParent(form), XmNtitle, title, NULL);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("String to Find:"));
    	    argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 't'); argcnt++;
    label1 = XmCreateLabel(form, "label1", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label1);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_END); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING(
    	   "(use up arrow key to recall previous)")); argcnt++;
    label2 = XmCreateLabel(form, "label2", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label2);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label1); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNmaxLength, SEARCHMAX); argcnt++;
    findText = XmCreateText(form, "replaceString", args, argcnt);
    XtAddCallback(findText, XmNfocusCallback, (XtCallbackProc)rFocusCB, window);
    XtAddEventHandler(findText, KeyPressMask, False,
    	    (XtEventHandler)rFindArrowKeyCB, window);
    RemapDeleteKey(findText);
    XtManageChild(findText);
    XmAddTabGroup(findText);
    XtVaSetValues(label1, XmNuserData, findText, NULL); /* mnemonic processing */
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, findText); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNalignment, XmALIGNMENT_BEGINNING); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Replace With:")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'W'); argcnt++;
    label = XmCreateLabel(form, "label", args, argcnt);
    XmStringFree(st1);
    XtManageChild(label);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, label); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNmaxLength, SEARCHMAX); argcnt++;
    replaceText = XmCreateText(form, "replaceWithString", args, argcnt);
    XtAddEventHandler(replaceText, KeyPressMask, False,
    	    (XtEventHandler)replaceArrowKeyCB, window);
    RemapDeleteKey(replaceText);
    XtManageChild(replaceText);
    XmAddTabGroup(replaceText);
    XtVaSetValues(label, XmNuserData, replaceText, NULL); /* mnemonic processing */

    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, replaceText); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchTypeBox = XmCreateRowColumn(form, "searchTypeBox", args, argcnt);
    XtManageChild(searchTypeBox);
    XmAddTabGroup(searchTypeBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'L'); argcnt++;
    literalBtn = XmCreateToggleButton(searchTypeBox, "literal", args, argcnt);
    XmStringFree(st1);
    XtManageChild(literalBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Case Sensitive Literal")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'C'); argcnt++;
    caseBtn = XmCreateToggleButton(searchTypeBox, "caseSenseLiteral", args, argcnt);
    XmStringFree(st1);
    XtManageChild(caseBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Regular Expression")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'x'); argcnt++;
    regExpBtn = XmCreateToggleButton(searchTypeBox, "regExp", args, argcnt);
    XmStringFree(st1);
    XtManageChild(regExpBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
    XtSetArg(args[argcnt], XmNpacking, XmPACK_TIGHT); argcnt++;
    XtSetArg(args[argcnt], XmNmarginHeight, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 2); argcnt++;
    XtSetArg(args[argcnt], XmNradioBehavior, True); argcnt++;
    XtSetArg(args[argcnt], XmNradioAlwaysOne, True); argcnt++;
    searchDirBox = XmCreateRowColumn(form, "searchDirBox", args, argcnt);
    XtManageChild(searchDirBox);
    XmAddTabGroup(searchDirBox);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Forward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'o'); argcnt++;
    forwardBtn = XmCreateToggleButton(searchDirBox, "forward", args, argcnt);
    XmStringFree(st1);
    XtManageChild(forwardBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Search Backward")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'B'); argcnt++;
    reverseBtn = XmCreateToggleButton(searchDirBox, "reverse", args, argcnt);
    XmStringFree(st1);
    XtManageChild(reverseBtn);
    
    argcnt = 0;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Keep Dialog")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'K'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 0); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchTypeBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 4); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, searchDirBox); argcnt++;
    keepBtn = XmCreateToggleButton(form, "keep", args, argcnt);
    XtAddCallback(keepBtn, XmNvalueChangedCallback,
    	    (XtCallbackProc)rKeepCB, window);
    XmStringFree(st1);
    XtManageChild(keepBtn);
    XmAddTabGroup(keepBtn);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopWidget, searchDirBox); argcnt++;
    XtSetArg(args[argcnt], XmNleftOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNrightOffset, 6); argcnt++;
    btnForm = XmCreateForm(form, "buttons", args, argcnt);
    XtManageChild(btnForm);
    XmAddTabGroup(btnForm);

    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING(" Find ")); argcnt++;
    XtSetArg(args[argcnt], XmNdefaultButtonShadowThickness, 1); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'F'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNuserData, FIND_REPLACE_NO_BUTTON_DEFAULT); argcnt++;
    findBtn = XmCreatePushButton(btnForm, "find", args, argcnt);
    XtAddCallback(findBtn, XmNactivateCallback, (XtCallbackProc)rFindCB,window);
    XmStringFree(st1);
    XtManageChild(findBtn);
	
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace\n& Find")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'd'); argcnt++;
    XtSetArg(args[argcnt], XmNdefaultButtonShadowThickness, 1); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, findBtn); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    replaceFindBtn = XmCreatePushButton(btnForm, "replacefind", args, argcnt);
    XtAddCallback(replaceFindBtn, XmNactivateCallback, (XtCallbackProc)replaceFindCB,window);
    XmStringFree(st1);
    XtManageChild(replaceFindBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Replace")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'R'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, replaceFindBtn); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    replaceBtn = XmCreatePushButton(btnForm, "replace", args, argcnt);
    XtAddCallback(replaceBtn, XmNactivateCallback, (XtCallbackProc)replaceCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(replaceBtn);
  
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Replace\nAll")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'A'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, replaceBtn); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    replaceAllBtn = XmCreatePushButton(btnForm, "all", args, argcnt);
    XtAddCallback(replaceAllBtn, XmNactivateCallback,
    	    (XtCallbackProc)replaceAllCB, window);
    XmStringFree(st1);
    XtManageChild(replaceAllBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString,
    	     st1=MKSTRING("Replace\nIn Selection")); argcnt++;
    XtSetArg(args[argcnt], XmNmnemonic, 'S'); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, replaceAllBtn); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    rInSelBtn = XmCreatePushButton(btnForm, "inSel", args, argcnt);
    XtAddCallback(rInSelBtn, XmNactivateCallback, (XtCallbackProc)rInSelCB,
    	    window);
    XmStringFree(st1);
    XtManageChild(rInSelBtn);
 
    argcnt = 0;
    XtSetArg(args[argcnt], XmNtraversalOn, True); argcnt++;
    XtSetArg(args[argcnt], XmNhighlightThickness, 2); argcnt++;
    XtSetArg(args[argcnt], XmNlabelString, st1=MKSTRING("Cancel")); argcnt++;
    XtSetArg(args[argcnt], XmNtopAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNbottomAttachment, XmATTACH_FORM); argcnt++;
    XtSetArg(args[argcnt], XmNleftAttachment, XmATTACH_WIDGET); argcnt++;
    XtSetArg(args[argcnt], XmNleftWidget, rInSelBtn); argcnt++;
    XtSetArg(args[argcnt], XmNrightAttachment, XmATTACH_NONE); argcnt++;
    XtSetArg(args[argcnt], XmNtopOffset, 6); argcnt++;
    XtSetArg(args[argcnt], XmNbottomOffset, 6); argcnt++;
    cancelBtn = XmCreatePushButton(btnForm, "cancel", args, argcnt);
    XmStringFree(st1);
    XtAddCallback(cancelBtn, XmNactivateCallback, (XtCallbackProc)rCancelCB,
    	    window);
    XtManageChild(cancelBtn);


    XtVaSetValues(form, XmNcancelButton, cancelBtn, NULL);
    AddDialogMnemonicHandler(form);
    
    window->replaceDlog = form;
    window->replaceText = findText;
    window->replaceWithText = replaceText;
    window->replaceLiteralBtn = literalBtn;
    window->replaceCaseBtn = caseBtn;
    window->replaceRegExpBtn = regExpBtn;
    window->replaceFwdBtn = forwardBtn;
    window->replaceRevBtn = reverseBtn;
    window->replaceKeepBtn = keepBtn;
    window->replaceBtns = btnForm;
    window->replaceBtn = replaceBtn;
    window->replaceFindBtn = replaceFindBtn;
    window->findBtn = findBtn;
    window->replaceInSelBtn = rInSelBtn;
    window->replaceSearchTypeBox = searchTypeBox;
}


/*
** These callbacks fix a Motif 1.1 problem that the default button gets the
** keyboard focus when a dialog is created.  We want the first text field
** to get the focus, so we don't set the default button until the text field
** has the focus for sure.  I have tried many other ways and this is by far
** the least nasty.
*/
static void rFocusCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    FindReplaceDlogDefaultButton defaultButton = FIND_REPLACE_NO_BUTTON_DEFAULT;
    XtVaGetValues(window->findBtn, XmNuserData, &defaultButton, NULL);
    if(defaultButton != FIND_REPLACE_NO_BUTTON_DEFAULT) {
   		Widget new = defaultButton == FIND_REPLACE_FIND_BUTTON_DEFAULT ? window->findBtn : 
	   	             defaultButton == FIND_REPLACE_REPLACE_BUTTON_DEFAULT ? window->replaceFindBtn :
	   	             window->findBtn;
	    SET_ONE_RSRC(window->replaceDlog, XmNdefaultButton, new);
	}
}

/* when keeping a window up, clue the user what window it's associated with */
static void rKeepCB(Widget w, WindowInfo *window, caddr_t *callData) 
{
    char title[MAXPATHLEN + 14];

   	sprintf(title, PACKAGE " - Find/Replace (%s)", window->editorInfo->filename);
   	XtVaSetValues(XtParent(window->replaceDlog), XmNtitle, title, NULL);
}

static void replaceCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[4];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    /* Find the text and replace it */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = directionArg(direction);
    params[3] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace", callData->event, params, 4);
    
    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void replaceAllCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[3];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace_all", callData->event,
    	    params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void rInSelCB(Widget w, WindowInfo *window,
			 XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[3];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);

    /* do replacement */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace_in_selection",
    	    callData->event, params, 3);
    
    /* pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void rCancelCB(Widget w, WindowInfo *window, caddr_t callData) 
{
    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);

    /* pop down the dialog */
    XtUnmanageChild(window->replaceDlog);
}

static void rFindCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[3];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string	*/
    resetReplaceTabGroup(window);
    
    /* Toggle the search direction if the Ctrl or Shift key was pressed */
    if(callData->event->xbutton.state & (ShiftMask | ControlMask)) {
		direction = direction == SEARCH_FORWARD ? SEARCH_BACKWARD : SEARCH_FORWARD;
	}

    /* Find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(direction);
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 3);
    
    /* Doctor the search history generated by the action to include the
       replace string (if any), so the replace string can be used on
       subsequent replaces, even though no actual replacement was done. */
    if (!strcmp(SearchHistory[historyIndex(1)], searchString)) {
    	XtFree(ReplaceHistory[historyIndex(1)]);
    	ReplaceHistory[historyIndex(1)] = XtNewString(replaceString);
    }

    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

/*
 * The direction of the search is toggled if the Ctrl key or the Shift key is
 * pressed when the text field is activated.
 */
static void iSearchTextActivateCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData) 
{
    char *params[3];
    
    char searchString[SEARCHMAX+1];
    char *text;
    int searchDirection = window->iSearchDirection;
    int searchType;
   
    text = XmTextFieldGetString(window->iSearchText);
    strcpy(searchString, text);
    XtFree(text);
    
    /* Toggle the search direction if the Ctrl or Shift key was pressed */
    if(callData->event->xbutton.state & (ShiftMask | ControlMask)) {
		searchDirection = searchDirection == SEARCH_FORWARD ? SEARCH_BACKWARD : SEARCH_FORWARD;
	}
	
	searchType = XmToggleButtonGetState(window->iSearchCaseToggle) ? SEARCH_CASE_SENSE : SEARCH_LITERAL;
	searchType = XmToggleButtonGetState(window->iSearchRegExpToggle) ? SEARCH_REGEX : searchType;
	
	/* .b */
	if (searchType == SEARCH_REGEX) {
    	regexp *compiledRE = NULL;
    	char *compileMsg;

    	/* If the search type is a regular expression, test compile it */
    	compiledRE = CompileRE(searchString, &compileMsg);
    	if (compiledRE == NULL){
		return;
    	} else 
    		XtFree((char *)compiledRE);
	}
	/* .e */

	/* find the text and mark it */
    params[0] = searchString;
    params[1] = directionArg(searchDirection);
    params[2] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "find", callData->event, params, 3);
}

static void iSearchTextValueChangedCB(Widget w, WindowInfo *window, XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1];
    char *text;
    int searchType;
   
    text = XmTextFieldGetString(window->iSearchText);
    strcpy(searchString, text);
    XtFree(text);
    
	searchType = XmToggleButtonGetState(window->iSearchCaseToggle) ? SEARCH_CASE_SENSE : SEARCH_LITERAL;
	searchType = XmToggleButtonGetState(window->iSearchRegExpToggle) ? SEARCH_REGEX : searchType;

	/* .b */
	if (searchType == SEARCH_REGEX) {
    	regexp *compiledRE = NULL;
    	char *compileMsg;

    	/* If the search type is a regular expression, test compile it */
    	compiledRE = CompileRE(searchString, &compileMsg);
    	if (compiledRE == NULL){
			return;
    	} else 
    		XtFree((char *)compiledRE);
	}
	/* .e */

    /* find the text and mark it */
    SearchAndSelectIncremental(window, window->iSearchDirection, searchString, searchType);    
}

static void iSearchTextArrowKeyEH(Widget w, WindowInfo *window, XKeyEvent *event)
{
	KeySym keysym = XLookupKeysym(event, 0);
	int index = window->iSearchHistIndex;
	char *searchStr;
	int searchType;

	/* only process up and down arrow keys */
	if (keysym != XK_Up && keysym != XK_Down)
		return;

	/* increment or decrement the index depending on which arrow was pressed */
	index += (keysym == XK_Up) ? 1 : -1;

	/* if the index is out of range, beep and return */
	if (index != 0 && historyIndex(index) == -1) {
		XBell(TheDisplay, 100);
		return;
	}

	/* determine the strings and button settings to use */
	if (index == 0) {
		searchStr = "";
		searchType = GetPrefSearch();
	} else {
		searchStr = SearchHistory[historyIndex(index)];
		searchType = SearchTypeHistory[historyIndex(index)];
	}

	/* Set the info used in the value changed callback before calling
	  XmTextFieldSetString(). */
	window->iSearchHistIndex = index;
	XmToggleButtonSetState(window->iSearchCaseToggle, (searchType != SEARCH_LITERAL), False);
	XmToggleButtonSetState(window->iSearchRegExpToggle, (searchType == SEARCH_REGEX), False);
	/* Beware the value changed callback is processed as part of this call */
	XmTextFieldSetString(window->iSearchText, searchStr);
	XmTextFieldSetInsertionPosition(window->iSearchText, 
		XmTextFieldGetLastPosition(window->iSearchText));
}

static void replaceFindCB(Widget w, WindowInfo *window,
		      XmAnyCallbackStruct *callData) 
{
    char searchString[SEARCHMAX+1], replaceString[SEARCHMAX+1];
    int direction, searchType;
    char *params[4];
    
    /* Validate and fetch the find and replace strings from the dialog */
    if (!getReplaceDlogInfo(window, &direction, searchString, replaceString,
    	    &searchType))
    	return;

    /* Set the initial focus of the dialog back to the search string */
    resetReplaceTabGroup(window);
    
    /* Find the text and replace it */
    params[0] = searchString;
    params[1] = replaceString;
    params[2] = directionArg(direction);
    params[3] = searchTypeArg(searchType);
    XtCallActionProc(window->lastFocus, "replace-find", callData->event, params, 4);
    
    /* Pop down the dialog */
    if (!XmToggleButtonGetState(window->replaceKeepBtn))
    	XtUnmanageChild(window->replaceDlog);
}

static void rFindArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->rHistIndex;
    char *searchStr, *replaceStr;
    int searchType;
    
    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down)
    	return;
    
    /* increment or decrement the index depending on which arrow was pressed */
    index += (keysym == XK_Up) ? 1 : -1;

    /* if the index is out of range, beep and return */
    if (index != 0 && historyIndex(index) == -1) {
    	XBell(TheDisplay, 100);
    	return;
    }
    
    /* determine the strings and button settings to use */
    if (index == 0) {
    	searchStr = "";
    	replaceStr = "";
    	searchType = GetPrefSearch();
    } else {
	searchStr = SearchHistory[historyIndex(index)];
	replaceStr = ReplaceHistory[historyIndex(index)];
	searchType = SearchTypeHistory[historyIndex(index)];
    }
    
    /* Set the buttons and fields with the selected search type */
    switch (searchType) {
      case SEARCH_LITERAL:
      	XmToggleButtonSetState(window->replaceLiteralBtn, True, True);
	break;
      case SEARCH_CASE_SENSE:
      	XmToggleButtonSetState(window->replaceCaseBtn, True, True);
	break;
      case SEARCH_REGEX:
      	XmToggleButtonSetState(window->replaceRegExpBtn, True, True);
	break;
    }
    XmTextSetString(window->replaceText, searchStr);
    XmTextSetString(window->replaceWithText, replaceStr);
    window->rHistIndex = index;
}

static void replaceArrowKeyCB(Widget w, WindowInfo *window, XKeyEvent *event)
{
    KeySym keysym = XLookupKeysym(event, 0);
    int index = window->rHistIndex;
    
    /* only process up and down arrow keys */
    if (keysym != XK_Up && keysym != XK_Down)
    	return;
    
    /* increment or decrement the index depending on which arrow was pressed */
    index += (keysym == XK_Up) ? 1 : -1;

    /* if the index is out of range, beep and return */
    if (index != 0 && historyIndex(index) == -1) {
    	XBell(TheDisplay, 100);
    	return;
    }
    
    /* change only the replace field information */
    if (index == 0)
    	XmTextSetString(window->replaceWithText, "");
    else
    	XmTextSetString(window->replaceWithText,
    		ReplaceHistory[historyIndex(index)]);
    window->rHistIndex = index;
}

/*
** Fetch and verify (particularly regular expression) search and replace
** strings and search type from the Replace dialog.  If the strings are ok,
** save a copy in the search history, copy them in to "searchString",
** "replaceString', which are assumed to be at least SEARCHMAX in length,
** return search type in "searchType", and return TRUE as the function
** value.  Otherwise, return FALSE.
*/
static int getReplaceDlogInfo(WindowInfo *window, int *direction,
	char *searchString, char *replaceString, int *searchType)
{
    char *replaceText, *replaceWithText;
    regexp *compiledRE = NULL;
    char *compileMsg;
    int type;
    
    /* Get the search and replace strings, search type, and direction
       from the dialog */
    replaceText = XmTextGetString(window->replaceText);
    replaceWithText = XmTextGetString(window->replaceWithText);
    if (XmToggleButtonGetState(window->replaceLiteralBtn))
    	type = SEARCH_LITERAL;
    else if (XmToggleButtonGetState(window->replaceCaseBtn))
    	type = SEARCH_CASE_SENSE;
    else
    	type = SEARCH_REGEX;
    *direction = XmToggleButtonGetState(window->replaceFwdBtn) ? SEARCH_FORWARD :
    	    SEARCH_BACKWARD;
    *searchType = type;
    
    /* If the search type is a regular expression, test compile it immediately
       and present error messages */
    if (type == SEARCH_REGEX) {
	compiledRE = CompileRE(replaceText, &compileMsg);
	if (compiledRE == NULL) {
   	    DialogF(DF_WARN, XtParent(window->replaceDlog), 1,
   	    	   "Please respecify the search string:\n%s", "OK", compileMsg);
 	    return FALSE;
 	}
    }
    
    /* Return strings */
    strcpy(searchString, replaceText);
    strcpy(replaceString, replaceWithText);
    XtFree(replaceText);
    XtFree(replaceWithText);
	XtFree((char *)compiledRE);
    return TRUE;
}

int SearchAndSelectSame(WindowInfo *window, int direction)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 100);
    	return FALSE;
    }
    
    return SearchAndSelect(window, direction, SearchHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)]);
}

/*
** Search for "searchString" in "window", and select the matching text in
** the window when found (or beep or put up a dialog if not found).  Also
** adds the search string to the global search history.
*/
int SearchAndSelect(WindowInfo *window, int direction, char *searchString,
	int searchType)
{
    int startPos, endPos;
    int beginPos, cursorPos, selStart, selEnd;
    
    /* Save a copy of searchString in the search history */
    SaveSearchHistory(searchString, NULL, searchType, False);
        
    /* set the position to start the search so we don't find the same
       string that was found on the last search	*/
    if (searchMatchesSelection(window, searchString, searchType,
    	    &selStart, &selEnd)) {
    	/* selection matches search string, start before or after sel.	*/
	if (direction == SEARCH_BACKWARD) {
	    beginPos = selStart-1;
	} else {
	    beginPos = selEnd;
	}
    } else {
    	selStart = -1; selEnd = -1;
    	/* no selection, or no match, search relative cursor */
    	cursorPos = TextGetCursorPos(window->lastFocus);
	if (direction == SEARCH_BACKWARD) {
	    /* use the insert position - 1 for backward searches */
	    beginPos = cursorPos-1;
	} else {
	    /* use the insert position for forward searches */
	    beginPos = cursorPos;
	}
    }

    /* do the search.  SearchWindow does appropriate dialogs and beeps */
    if (!SearchWindow(window, direction, searchString, searchType,
    	    beginPos, &startPos, &endPos))
    	return FALSE;
    	
    /* if the search matched an empty string (possible with regular exps)
       beginning at the start of the search, go to the next occurrence,
       otherwise repeated finds will get "stuck" at zero-length matches */
    if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
    	if (!SearchWindow(window, direction, searchString, searchType,
    		beginPos+1, &startPos, &endPos))
    	    return FALSE;
    
    /* if matched text is already selected, just beep */
    if (selStart==startPos && selEnd==endPos) {
    	XBell(TheDisplay, 100);
    	return FALSE;
    }

    /* select the text found string */
    BufSelect(window->editorInfo->buffer, startPos, endPos, CHAR_SELECT);
    MoveSelectionToMiddle(window, window->lastFocus);
    TextSetCursorPos(window->lastFocus, endPos);
    
    return TRUE;
}

int SearchAndSelectIncremental(WindowInfo *window, int direction, char *searchString,
	int searchType)
{
	int startPos, endPos;
	int beginPos, cursorPos, selStart, selEnd;

	/* set the position to start the search so we include the same
	   string that was found on the last search	*/
	if (searchMatchesSelection(window, window->iSearchLastSearchString, window->iSearchLastSearchType, 
		&selStart, &selEnd)) {
	
		if (direction == SEARCH_FORWARD) {
			beginPos = selStart;
		} else {
			beginPos = selEnd;
		}
	} else {
		selStart = -1; selEnd = -1;
		/* no selection, or no match, search relative cursor */
		cursorPos = TextGetCursorPos(window->lastFocus);
		if (direction == SEARCH_BACKWARD) {
			/* use the insert position - 1 for backward searches */
			beginPos = cursorPos-1;
		} else {
			/* use the insert position for forward searches */
			beginPos = cursorPos;
		}
	}

	/* Ignore this change in value if we were called due to cycling
	   thru the search history. That is when the incremental search history
	   index is zero and the search string is empty or when the index is 
	   greater than 1 and the searchString doesn't match the search string
	   in the history at that index. */
	if((window->iSearchHistIndex == 0 && searchString[0] == 0)
	 || (window->iSearchHistIndex > 1 && strcmp(searchString, 
			SearchHistory[historyIndex(window->iSearchHistIndex)]) == 0)
	) {
		/* Don't change the search history in this case. */
	}
	else {
   		SaveSearchHistory(searchString, NULL, searchType, True);
		/* Reset the incremental search history pointer to the beginning */
		window->iSearchHistIndex = 1;
	}
		
	/* if the searchString is empty then clear the selection
	   and set the cursor to what would be the beginning of the search 
	   and return. */
	if(searchString[0] == 0) {
		BufUnselect(window->editorInfo->buffer);
		TextSetCursorPos(window->lastFocus, beginPos);
		return TRUE;
	}
	
	/* do the search.  SearchWindow does appropriate dialogs and beeps */
	if (!SearchWindow(window, direction, searchString, searchType,
		beginPos, &startPos, &endPos))
		return FALSE;

	/* remember this search if there was a match. */
	strncpy(window->iSearchLastSearchString, searchString, 
		sizeof(window->iSearchLastSearchString));
	window->iSearchLastSearchString[sizeof(window->iSearchLastSearchString)] = 0;
	window->iSearchLastSearchType = searchType;
	
	/* if the search matched an empty string (possible with regular exps)
	   beginning at the start of the search, go to the next occurrence,
	   otherwise repeated finds will get "stuck" at zero-length matches */
	if (direction==SEARCH_FORWARD && beginPos==startPos && beginPos==endPos)
		if (!SearchWindow(window, direction, searchString, searchType,
			beginPos+1, &startPos, &endPos))
			return FALSE;

	/* select the text found string */
	BufSelect(window->editorInfo->buffer, startPos, endPos, CHAR_SELECT);
	MoveSelectionToMiddle(window, window->lastFocus);
	TextSetCursorPos(window->lastFocus, endPos);

	return TRUE;
}

void SearchForSelected(WindowInfo *window, int direction, Time time)
{
   XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)selectedSearchCB, (XtPointer)direction,
    	    time);
}

static void selectedSearchCB(Widget w, XtPointer callData, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
    WindowInfo *window = WidgetToWindow(w);
    int direction = (int)callData;
    int searchType;
    char searchString[SEARCHMAX+1];
    
    /* skip if we can't get the selection data or it's too long */
    if (*type == XT_CONVERT_FAIL || value == NULL) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1,
   	    	    "Selection not appropriate for searching", "OK");
    	else
    	    XBell(TheDisplay, 100);
	return;
    }
    if (*length > SEARCHMAX) {
    	if (GetPrefSearchDlogs())
   	    DialogF(DF_WARN, window->shell, 1, "Selection too long", "OK");
    	else
    	    XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    if (*length == 0) {
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* should be of type text??? */
    if (*format != 8) {
    	fprintf(stderr, "NEdit: can't handle non 8-bit text\n");
    	XBell(TheDisplay, 100);
	XtFree(value);
	return;
    }
    /* make the selection the current search string */
    strncpy(searchString, value, *length);
    searchString[*length] = '\0';
    XtFree(value);
    
    /* Use the default method for searching, unless it is regex, since this
       kind of search is by definition a literal search */
    searchType = GetPrefSearch();
    if (searchType == SEARCH_REGEX)
    	searchType = SEARCH_LITERAL;

    /* search for it in the window */
    SearchAndSelect(window, direction, searchString, searchType);
}

/*
** Check the character before the insertion cursor of textW and flash
** matching parenthesis, brackets, or braces, by temporarily highlighting
** the matching character (a timer procedure is scheduled for removing the
** highlights)
*/
void FlashMatching(WindowInfo *window, Widget textW)
{
    int startPos, endPos, matchBeginPos, matchPos;
    
    /* if a marker is already drawn, erase it and cancel the timeout */
    if (window->editorInfo->flashTimeoutID != 0) {
    	eraseFlash(window);
    	XtRemoveTimeOut(window->editorInfo->flashTimeoutID);
    	window->editorInfo->flashTimeoutID = 0;
    }
    
    /* don't do anything if showMatching isn't on */
    if (!window->editorInfo->showMatching)
    	return;

    /* don't flash matching characters if there's a selection */
    if (window->editorInfo->buffer->primary.selected)
   	return;

    /* Constrain the search to visible text (unless we're in split-window mode,
       then search the whole buffer), and get the string to search */
   	startPos = window->nPanes == 0 ? TextFirstVisiblePos(textW) : 0;
   	endPos = window->nPanes == 0 ? TextLastVisiblePos(textW) :
    	    	window->editorInfo->buffer->length;
    
    /* do the search */
    if (!findMatchingChar(window->editorInfo->buffer, TextGetCursorPos(textW), 
    		startPos, endPos, &matchBeginPos, &matchPos))
    	return;

    /* highlight the matched character */
    BufHighlight(window->editorInfo->buffer, matchPos, matchPos+1);
      
    /* Set up a timer to erase the box after 1.5 seconds */
    window->editorInfo->flashTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 1500,
    	    flashTimeoutProc, window);
    window->editorInfo->flashPos = matchPos;
}

void MatchSelectedCharacter(WindowInfo *window)
{
	int selStart, selEnd;
	int startPos, endPos, matchBeginPos, matchPos;
	textBuffer *buf = window->editorInfo->buffer;

	/* get the character to match and its position from the selection, or
	   the character before the insert point if nothing is selected.
	   Give up if too many characters are selected */
	if (GetSimpleSelection(buf, &selStart, &selEnd)) {
		if ((selEnd - selStart) != 1) {
			XBell(TheDisplay, 100);
			return;
		}
	}
	else {
		selEnd = TextGetCursorPos(window->lastFocus);
		selStart = selEnd - 1;
	}
    
	/* Search for it in the buffer */
	if (!findMatchingChar(buf, selEnd, 0, buf->length, &matchBeginPos, &matchPos)) {
		XBell(TheDisplay, 100);
		return;
	}
	startPos = (matchPos > matchBeginPos) ? matchBeginPos : matchPos;
	endPos = (matchPos > matchBeginPos) ? matchPos : matchBeginPos;

	/* select the text between the matching characters */
	BufSelect(buf, startPos, endPos+1, CHAR_SELECT);
}

void GotoMatchingCharacter(WindowInfo *window)
{
	int pos, matchBeginPos, matchPos;
	textBuffer *buf = window->editorInfo->buffer;

	pos = TextGetCursorPos(window->lastFocus);
    
	/* Search for it in the buffer */
	if (!findMatchingChar(buf, pos, 0, buf->length, &matchBeginPos, &matchPos)) {
		XBell(TheDisplay, 100);
		return;
	}
	/* Move the cursor inside the beginning match character. */
	if(matchPos < matchBeginPos) {
		matchPos++;
	}
	/* Move the cursor to the matching character. */
	TextSetCursorPos(window->lastFocus, matchPos);
}

static int findMatchingChar(textBuffer *buf, int charPos,int startLimit, 
	int endLimit, int *matchBeginPos, int *matchPos)
{
	int beforePos, afterPos, beginPos, pos;
	char beforeChar, beginChar, endChar;
    int nestDepth, matchIndex, direction;
    char c;
    
    /* Check for the beginning character before the cursor and for
       the ending character after the cursor. */
    afterPos = charPos;
    /* Nothing to match if the cursor is at the beginning of the buffer. */
    if (afterPos <= 0)
    	return FALSE;

    beginPos = -1;
    beforePos = (afterPos - 1);
    beforeChar = BufGetCharacter(buf, beforePos);

    /* Search forward for the ending character when the
    ** the character left of the cursor is one of the beginning
    ** match character.
    */
    for (matchIndex = 0; MatchingChars[matchIndex].begin != 0; matchIndex++) {
        if (MatchingChars[matchIndex].begin == beforeChar) {
        	direction = SEARCH_FORWARD;
        	beginPos = beforePos + 1;
        	break;
        }
    }
    /* Else search backwards for the beginning character for the
    ** the character left of the cursor only if the begin and
    ** end match characters are unique.
    ** {({({()})})}
    */
    if (beginPos == -1) {
    	for (matchIndex = 0; MatchingChars[matchIndex].begin != 0; matchIndex++) {
        	if (MatchingChars[matchIndex].end != MatchingChars[matchIndex].begin &&
        		MatchingChars[matchIndex].end == beforeChar) {
        		direction = SEARCH_BACKWARD;
        		beginPos = beforePos - 1;
        		break;
        	}
        }
    }
    /* Else search backwards for the beginning character if the
    ** character right of the cursor is one of the ending
    ** match character. Also make sure the character right of the cursor
    ** is not the end of the buffer.
    ** {(({({()})}))}
    */
    if (beginPos == -1 && afterPos < buf->length) {
		char afterChar;
    	afterChar = BufGetCharacter(buf, afterPos);
    	for (matchIndex = 0; MatchingChars[matchIndex].begin != 0; matchIndex++) {
        	if (MatchingChars[matchIndex].end == afterChar) {
        		direction = SEARCH_BACKWARD;
        		beginPos = afterPos - 1;
        		break;
        	}
        }
    }
    if (beginPos == -1)
		return FALSE;
	
    beginChar = MatchingChars[matchIndex].begin;
    endChar = MatchingChars[matchIndex].end;

    /* find it in the buffer */
    nestDepth = 1;
    if (direction == SEARCH_FORWARD) {
    	for (pos=beginPos; pos<endLimit; pos++) {
	    c=BufGetCharacter(buf, pos);
	    if (c == endChar) {
	    	nestDepth--;
		if (nestDepth == 0) {
		    *matchPos = pos;
			*matchBeginPos = beginPos-1;
		    return TRUE;
		}
	    } else if (c == beginChar)
	    	nestDepth++;
	}
    } else { /* SEARCH_BACKWARD */
	for (pos=beginPos; pos>=startLimit; pos--) {
	    c=BufGetCharacter(buf, pos);
	    if (c == beginChar) {
	    	nestDepth--;
		if (nestDepth == 0) {
			*matchPos = pos;
		    *matchBeginPos = beginPos+1;
		    return TRUE;
		}
	    } else if (c == endChar)
	    	nestDepth++;
	}
    }
    return FALSE;
}

/*
** Xt timer procedure for erasing the matching parenthesis marker.
*/
static void flashTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    eraseFlash((WindowInfo *)clientData);
    ((WindowInfo *)clientData)->editorInfo->flashTimeoutID = 0;
}

/*
** Erase the marker drawn on a matching parenthesis bracket or brace
** character.
*/
static void eraseFlash(WindowInfo *window)
{
    BufUnhighlight(window->editorInfo->buffer);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceSame(WindowInfo *window, int direction)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 100);
    	return FALSE;
    }

    return SearchAndReplace(window, direction, SearchHistory[historyIndex(1)],
    	    ReplaceHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)]);
}

/*
** Search and replace using previously entered search strings (from dialog
** or selection).
*/
int ReplaceFindSame(WindowInfo *window, int direction)
{
    if (NHist < 1) {
    	XBell(TheDisplay, 100);
    	return FALSE;
    }

    return ReplaceAndSearch(window, direction, SearchHistory[historyIndex(1)],
    	    ReplaceHistory[historyIndex(1)],
    	    SearchTypeHistory[historyIndex(1)]);
}

/*
** Search for string "searchString" in window "window", using algorithm
** "searchType" and direction "direction", and replace it with "replaceString"
** Also adds the search and replace strings to the global search history.
*/
int SearchAndReplace(WindowInfo *window, int direction, char *searchString,
	char *replaceString, int searchType)
{
	int startPos, endPos, replaceLen;
	int found;
	int beginPos, cursorPos;

    /* Save a copy of search and replace strings in the search history */
    SaveSearchHistory(searchString, replaceString, searchType, False);
    
	/* If the text selected in the window matches the search string, 	*/
	/* the user is probably using search then replace method, so	*/
	/* replace the selected text regardless of where the cursor is.	*/
	/* Otherwise, search for the string.				*/
	if (!searchMatchesSelection(window, searchString, searchType,
		&startPos, &endPos)) {
		/* get the position to start the search */
		cursorPos = TextGetCursorPos(window->lastFocus);
		if (direction == SEARCH_BACKWARD) {
			/* use the insert position - 1 for backward searches */
			beginPos = cursorPos-1;
		} else {
			/* use the insert position for forward searches */
			beginPos = cursorPos;
		}
		/* do the search */
		found = SearchWindow(window, direction, searchString, searchType,
			beginPos, &startPos, &endPos);
		if (!found)
			return FALSE;
	}

	/* replace the text */
	if (searchType == SEARCH_REGEX) {
		char replaceResult[SEARCHMAX+1], *foundString;
		foundString = BufGetRange(window->editorInfo->buffer, startPos, endPos);
		replaceUsingRE(searchString, replaceString, foundString,
			replaceResult, SEARCHMAX, GetWindowDelimiters(window));
		XtFree(foundString);
		BufReplace(window->editorInfo->buffer, startPos, endPos, replaceResult);
		replaceLen = strlen(replaceResult);
	} else {
		BufReplace(window->editorInfo->buffer, startPos, endPos, replaceString);
		replaceLen = strlen(replaceString);
	}

	/* after successfully completing a replace, selected text attracts
	   attention away from the area of the replacement, particularly
	   when the selection represents a previous search. so deselect */
	BufUnselect(window->editorInfo->buffer);

	/* temporarily shut off autoShowInsertPos before setting the cursor
	   position so MoveSelectionToMiddle gets a chance to place the replaced
	   string at a pleasing position on the screen (otherwise, the cursor would
	   be automatically scrolled on screen and MoveSelectionToMiddle would do
	   nothing) */
	XtVaSetValues(window->lastFocus, textNautoShowInsertPos, False, NULL);
	TextSetCursorPos(window->lastFocus, startPos +
		((direction == SEARCH_FORWARD) ? replaceLen : 0));
	MoveSelectionToMiddle(window, window->lastFocus);
	XtVaSetValues(window->lastFocus, textNautoShowInsertPos, True, NULL);

	return TRUE;
}

/*
** Replace selection with "replaceString" and search for string "searchString" in window "window", 
** using algorithm "searchType" and direction "direction"
*/
int ReplaceAndSearch(WindowInfo *window, int direction, char *searchString,
	char *replaceString, int searchType)
{
	int startPos = 0, endPos = 0, replaceLen = 0;
	int found, replaced;

    /* Save a copy of search and replace strings in the search history */
    SaveSearchHistory(searchString, replaceString, searchType, False);
    
	replaced = 0;

	/* Replace the selected text only if it matches the search string */
	if (searchMatchesSelection(window, searchString, searchType,
		&startPos, &endPos)) {
		/* replace the text */
		if (searchType == SEARCH_REGEX) {
			char replaceResult[SEARCHMAX+1], *foundString;
			foundString = BufGetRange(window->editorInfo->buffer, startPos, endPos);
			replaceUsingRE(searchString, replaceString, foundString,
		replaceResult, SEARCHMAX, GetWindowDelimiters(window));
			XtFree(foundString);
			BufReplace(window->editorInfo->buffer, startPos, endPos, replaceResult);
			replaceLen = strlen(replaceResult);
		} else {
			BufReplace(window->editorInfo->buffer, startPos, endPos, replaceString);
			replaceLen = strlen(replaceString);
		}
		
		/* Position the cursor so the next search will work correctly based */
		/* on the direction of the search */
		TextSetCursorPos(window->lastFocus, startPos +
			((direction == SEARCH_FORWARD) ? replaceLen : 0));

		replaced = 1;
	}

	/* do the search */
	found = SearchAndSelect(window, direction, searchString, searchType);

	if (!found)
		XBell(TheDisplay, 100);	    

	return replaced;
}    	    


/*
** Replace all occurences of "searchString" in "window" with "replaceString"
** within the current primary selection in "window". Also adds the search and
** replace strings to the global search history.
*/
int ReplaceInSelection(WindowInfo *window, char *searchString,
	char *replaceString, int searchType)
{
    int selStart, selEnd, beginPos, startPos, endPos, realOffset, replaceLen;
    int found, anyFound, isRect, rectStart, rectEnd, lineStart, cursorPos;
    char *fileString;
    textBuffer *tempBuf;
    
    /* save a copy of search and replace strings in the search history */
    SaveSearchHistory(searchString, replaceString, searchType, False);
    
    /* find out where the selection is */
    if (!BufGetSelectionPos(window->editorInfo->buffer, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
    	return FALSE;
	
    /* get the selected text */
    if (isRect) {
    	selStart = BufStartOfLine(window->editorInfo->buffer, selStart);
    	selEnd = BufEndOfLine(window->editorInfo->buffer, selEnd);
    	fileString = BufGetRange(window->editorInfo->buffer, selStart, selEnd);
    } else
    	fileString = BufGetSelectionText(window->editorInfo->buffer);
    
    /* create a temporary buffer in which to do the replacements to hide the
       intermediate steps from the display routines, and so everything can
       be undone in a single operation */
    tempBuf = BufCreate();
    BufSetAll(tempBuf, fileString);
    
    /* search the string and do the replacements in the temporary buffer */
    replaceLen = strlen(replaceString);
    found = TRUE;
    anyFound = FALSE;
    beginPos = 0;
    cursorPos = 0;
    realOffset = 0;
	while (found) {
		found = SearchString(fileString, searchString, SEARCH_FORWARD,
		searchType, FALSE, beginPos, &startPos, &endPos,
		GetWindowDelimiters(window));
		if (!found)
			break;
		/* if the selection is rectangular, verify that the found
		   string is in the rectangle */
		if (isRect) {
			lineStart = BufStartOfLine(tempBuf, startPos+realOffset);
			if (BufCountDispChars(tempBuf, lineStart, startPos+realOffset) <
				rectStart || BufCountDispChars(tempBuf, lineStart,
				endPos+realOffset) > rectEnd) {
				beginPos = (startPos == endPos) ? endPos+1 : endPos;
				continue;
			}
		}
		/* Make sure the match did not start past the end (regular expressions
		   can consider the artificial end of the range as the end of a line,
		   and match a fictional whole line beginning there) */
		if (startPos == selEnd - selStart) {
			found = False;
			break;
		}
		/* replace the string and compensate for length change */
		if (searchType == SEARCH_REGEX) {
			char replaceResult[SEARCHMAX+1], *foundString;
			foundString = BufGetRange(tempBuf, startPos+realOffset,
				endPos+realOffset);
			replaceUsingRE(searchString, replaceString, foundString,
		    replaceResult, SEARCHMAX, GetWindowDelimiters(window));
			XtFree(foundString);
			BufReplace(tempBuf, startPos+realOffset, endPos+realOffset,
				replaceResult);
			replaceLen = strlen(replaceResult);
		} else
			BufReplace(tempBuf, startPos+realOffset, endPos+realOffset,
				replaceString);
		realOffset += replaceLen - (endPos - startPos);
		/* start again after match unless match was empty, then endPos+1 */
		beginPos = (startPos == endPos) ? endPos+1 : endPos;
		cursorPos = endPos;
		anyFound = TRUE;
	}
    XtFree(fileString);
    
    /* if nothing was found, tell user and return */
    if (!anyFound) {
    	if (GetPrefSearchDlogs()) {
    	    /* Avoid bug in Motif 1.1 by putting away search dialog
    	       before DialogF */
    	    if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	    !XmToggleButtonGetState(window->replaceKeepBtn))
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 100);
 	BufFree(tempBuf);
 	return FALSE;
    }
    
    /* replace the selected range in the real buffer */
    fileString = BufGetAll(tempBuf);
    BufFree(tempBuf);
    BufReplace(window->editorInfo->buffer, selStart, selEnd, fileString);
    XtFree(fileString);
    
    /* set the insert point at the end of the last replacement */
    TextSetCursorPos(window->lastFocus, selStart + cursorPos + realOffset);
    
    /* leave non-rectangular selections selected (rect. ones after replacement
       are less useful since left/right positions are randomly adjusted) */
    if (!isRect)
    	BufSelect(window->editorInfo->buffer, selStart, selEnd + realOffset, CHAR_SELECT);

    return TRUE;
}

/*
** Replace all occurences of "searchString" in "window" with "replaceString".
** Also adds the search and replace strings to the global search history.
*/
int ReplaceAll(WindowInfo *window, char *searchString, char *replaceString,
	int searchType)
{
    char *fileString, *newFileString;
    int copyStart, copyEnd, replacementLen;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
    
    /* save a copy of search and replace strings in the search history */
    SaveSearchHistory(searchString, replaceString, searchType, False);
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->editorInfo->buffer);
    
    newFileString = ReplaceAllInString(fileString, searchString, replaceString,
	    searchType, &copyStart, &copyEnd, &replacementLen,
	    GetWindowDelimiters(window));
    if (newFileString == NULL) {
    	if (GetPrefSearchDlogs()) {
    	    if (window->replaceDlog && XtIsManaged(window->replaceDlog) &&
    	    	    !XmToggleButtonGetState(window->replaceKeepBtn))
    		XtUnmanageChild(window->replaceDlog);
   	    DialogF(DF_INF, window->shell, 1, "String was not found", "OK");
    	} else
    	    XBell(TheDisplay, 0);
	return FALSE;
    }
    XtFree(fileString);
    
    /* replace the contents of the text widget with the substituted text */
    BufReplace(window->editorInfo->buffer, copyStart, copyEnd, newFileString);
    
    /* Move the cursor to the end of the last replacement */
    TextSetCursorPos(window->lastFocus, copyStart + replacementLen);

    XtFree(newFileString);
    return TRUE;	
}    

/*
** Replace all occurences of "searchString" in "inString" with "replaceString"
** and return an allocated string covering the range between the start of the
** first replacement (returned in "copyStart", and the end of the last
** replacement (returned in "copyEnd")
*/
char *ReplaceAllInString(char *inString, char *searchString,
	char *replaceString, int searchType, int *copyStart,
	int *copyEnd, int *replacementLength, char *delimiters)
{
    int beginPos, startPos, endPos, lastEndPos;
    int found, nFound, removeLen, replaceLen, copyLen, addLen;
    char *outString, *fillPtr;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return NULL;
    
    /* rehearse the search first to determine the size of the buffer needed
       to hold the substituted text.  No substitution done here yet */
    replaceLen = strlen(replaceString);
    found = TRUE;
    nFound = 0;
    removeLen = 0;
    addLen = 0;
    beginPos = 0;
    *copyStart = -1;
    while (found) {
    	found = SearchString(inString, searchString, SEARCH_FORWARD,
    		searchType, FALSE, beginPos, &startPos, &endPos, delimiters);
	if (found) {
	    if (*copyStart < 0)
	    	*copyStart = startPos;
    	    *copyEnd = endPos;
    	    /* start next after match unless match was empty, then endPos+1 */
    	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	    nFound++;
	    removeLen += endPos - startPos;
	    if (searchType == SEARCH_REGEX) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString, &inString[startPos],
    			replaceResult, SEARCHMAX, delimiters);
    		addLen += strlen(replaceResult);
    	    } else
    	    	addLen += replaceLen;
	}
    }
    
    if (nFound == 0)
	return NULL;
    
    /* Allocate a new buffer to hold all of the new text between the first
       and last substitutions */
    copyLen = *copyEnd - *copyStart;
    outString = XtMalloc(copyLen - removeLen + addLen + 1);
    
    /* Scan through the text buffer again, substituting the replace string
       and copying the part between replaced text to the new buffer  */
    found = TRUE;
    beginPos = 0;
    lastEndPos = 0;
    fillPtr = outString;
    while (found) {
    	found = SearchString(inString, searchString, SEARCH_FORWARD,
    		searchType, FALSE, beginPos, &startPos, &endPos, delimiters);
	if (found) {
	    if (beginPos != 0) {
		memcpy(fillPtr, &inString[lastEndPos], startPos - lastEndPos);
		fillPtr += startPos - lastEndPos;
	    }
	    if (searchType == SEARCH_REGEX) {
    		char replaceResult[SEARCHMAX];
    		replaceUsingRE(searchString, replaceString, &inString[startPos],
    			replaceResult, SEARCHMAX, delimiters);
    		replaceLen = strlen(replaceResult);
    		memcpy(fillPtr, replaceResult, replaceLen);
	    } else {
		memcpy(fillPtr, replaceString, replaceLen);
	    }
	    fillPtr += replaceLen;
	    lastEndPos = endPos;
	    /* start next after match unless match was empty, then endPos+1 */
	    beginPos = (startPos == endPos) ? endPos+1 : endPos;
	}
    }
    *fillPtr = '\0';
    *replacementLength = fillPtr - outString;
    return outString;
}

/*
** Search the text in "window", attempting to match "searchString"
*/
int SearchWindow(WindowInfo *window, int direction, char *searchString,
	int searchType, int beginPos, int *startPos, int *endPos)
{
    char *fileString;
    int found, resp, fileEnd;
    
    /* reject empty string */
    if (*searchString == '\0')
    	return FALSE;
	
    /* get the entire text buffer from the text area widget */
    fileString = BufGetAll(window->editorInfo->buffer);
    
    /* search the string copied from the text area widget, and present
       dialogs, or just beep */
    found = SearchString(fileString, searchString, direction, searchType,
       	FALSE, beginPos, startPos, endPos, GetWindowDelimiters(window));
	/* Avoid Motif 1.1 bug by putting away search dialog before DialogF */
	if (GetPrefSearchDlogs() && window->replaceDlog && 
		XtIsManaged(window->replaceDlog) && 
		!XmToggleButtonGetState(window->replaceKeepBtn))
		XtUnmanageChild(window->replaceDlog);
	if (!found) {
		fileEnd = window->editorInfo->buffer->length - 1;
		if (direction == SEARCH_FORWARD && beginPos != 0) {
			if (GetPrefSearchDlogs()) {
				resp = DialogF(DF_QUES, window->shell, 2,
					"Continue search from\nbeginning of file?", "Continue",
					"Cancel");
				if (resp == 2) {
					XtFree(fileString);
					return False;
				}
			} else {
				/* Ring the bell to indicate that we have wrapped */
				XBell(TheDisplay, 100);
			}
			found = SearchString(fileString, searchString, direction,
    			searchType, FALSE, 0, startPos, endPos,
    			GetWindowDelimiters(window));
		} else if (direction == SEARCH_BACKWARD && beginPos != fileEnd) {
			if (GetPrefSearchDlogs()) {
				resp = DialogF(DF_QUES, window->shell, 2,
					"Continue search\nfrom end of file?", "Continue",
					"Cancel");
				if (resp == 2) {
					XtFree(fileString);
					return False;
				}
			} else {
				/* Ring the bell to indicate that we have wrapped */
				XBell(TheDisplay, 100);
			}
			found = SearchString(fileString, searchString, direction,
				searchType, FALSE, fileEnd, startPos, endPos, 
				GetWindowDelimiters(window));
		}
		if (!found) {
			if (GetPrefSearchDlogs()) {
				DialogF(DF_INF, window->shell,1,"String was not found","OK");
			} else {
				/* Ring the bell to indicate that we have wrapped */
				XBell(TheDisplay, 100);
			}
		}
	}
    
    /* Free the text buffer copy returned from BufGetAll */
    XtFree(fileString);

    return found;
}

/*
** Search the null terminated string "string" for "searchString", beginning at
** "beginPos".  Returns the boundaries of the match in "startPos" and "endPos".
** "delimiters" may be used to provide an alternative set of word delimiters
** for regular expression "<" and ">" characters, or simply passed as null
** for the default delimiter set.
*/
int SearchString(char *string, char *searchString, int direction,
	   int searchType, int wrap, int beginPos, int *startPos,
	   int *endPos, char *delimiters)
{
    switch (searchType) {
      case SEARCH_CASE_SENSE:
      	 return searchLiteral(string, searchString, TRUE, direction, wrap,
	 		       beginPos, startPos, endPos);
      case SEARCH_LITERAL:
      	 return  searchLiteral(string, searchString, FALSE, direction, wrap,
	 	beginPos, startPos, endPos);
      case SEARCH_REGEX:
      	 return  searchRegex(string, searchString, direction, wrap,
      	 	beginPos, startPos, endPos, delimiters);
    }
    return FALSE; /* never reached, just makes compilers happy */
}

static int searchLiteral(char *string, char *searchString, int caseSense, 
	int direction, int wrap, int beginPos, int *startPos, int *endPos)
{
/* This is critical code for the speed of searches.			    */
/* For efficiency, we define the macro DOSEARCH with the guts of the search */
/* routine and repeat it, changing the parameters of the outer loop for the */
/* searching, forwards, backwards, and before and after the begin point	    */
#define DOSEARCH() \
    if (*filePtr == *ucString || *filePtr == *lcString) { \
	/* matched first character */ \
	ucPtr = ucString; \
	lcPtr = lcString; \
	tempPtr = filePtr; \
	while (*tempPtr == *ucPtr || *tempPtr == *lcPtr) { \
	    tempPtr++; ucPtr++; lcPtr++; \
	    if (*ucPtr == 0) { \
		/* matched whole string */ \
		*startPos = filePtr - string; \
		*endPos = tempPtr - string; \
		return TRUE; \
	    } \
	} \
    } \

    register char *filePtr, *tempPtr, *ucPtr, *lcPtr;
    char lcString[SEARCHMAX+1], ucString[SEARCHMAX+1];

    if (caseSense) {
        strcpy(ucString, searchString);
        strcpy(lcString, searchString);
    } else {
    	upCaseString(ucString, searchString);
    	downCaseString(lcString, searchString);
    }

    if (direction == SEARCH_FORWARD) {
	/* search from beginPos to end of string */
	for (filePtr=string+beginPos; *filePtr!=0; filePtr++) {
	    DOSEARCH()
	}
	if (!wrap)
	    return FALSE;
	/* search from start of file to beginPos	*/
	for (filePtr=string; filePtr<=string+beginPos; filePtr++) {
	    DOSEARCH()
	}
	return FALSE;
    } else {
    	/* SEARCH_BACKWARD */
		int searchLen = strlen(searchString);
	/* search from beginPos to start of file.  A negative begin pos	*/
	/* says begin searching from the far end of the file		*/
	if (beginPos >= 0) {
	    for (filePtr=string+beginPos-searchLen+1; filePtr>=string; filePtr--) {
		DOSEARCH()
	    }
	}
	if (!wrap)
	    return FALSE;
	/* search from end of file to beginPos */
	/*... this strlen call is extreme inefficiency, but it's not obvious */
	/* how to get the text string length from the text widget (under 1.1)*/
	for (filePtr=string+strlen(string)-searchLen+1;
		filePtr>=string+beginPos; filePtr--) {
	    DOSEARCH()
	}
	return FALSE;
    }
}

static int searchRegex(char *string, char *searchString, int direction,
	int wrap, int beginPos, int *startPos, int *endPos, char *delimiters)
{
    if (direction == SEARCH_FORWARD)
	return forwardRegexSearch(string, searchString, wrap, 
		beginPos, startPos, endPos, delimiters);
    else
    	return backwardRegexSearch(string, searchString, wrap, 
		beginPos, startPos, endPos, delimiters);
}

static int forwardRegexSearch(char *string, char *searchString, int wrap,
	int beginPos, int *startPos, int *endPos, char *delimiters)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    
    /* compile the search string for searching with ExecRE.  Note that
       this does not process errors from compiling the expression.  It
       assumes that the expression was checked earlier. */
    compiledRE = CompileRE(searchString, &compileMsg);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to end of string */
    if (ExecRE(compiledRE, string + beginPos, NULL, FALSE,
    	    isStartOfLine(string, beginPos), isStartOfWord(string, beginPos),
    	    delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }
    
    /* if wrap turned off, we're done */
    if (!wrap) {
    	XtFree((char *)compiledRE);
	return FALSE;
    }
	
    /* search from the beginning of the string to beginPos */
    if (ExecRE(compiledRE, string, string + beginPos, FALSE, TRUE, TRUE,
    	    delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }

    XtFree((char *)compiledRE);
    return FALSE;
}

static int backwardRegexSearch(char *string, char *searchString,
	int wrap, int beginPos, int *startPos, int *endPos, char *delimiters)
{
    regexp *compiledRE = NULL;
    char *compileMsg;
    int length;

    /* compile the search string for searching with ExecRE */
    compiledRE = CompileRE(searchString, &compileMsg);
    if (compiledRE == NULL)
	return FALSE;

    /* search from beginPos to start of file.  A negative begin pos	*/
    /* says begin searching from the far end of the file.		*/
    if (beginPos >= 0) {
	if (ExecRE(compiledRE, string, string + beginPos + 1, TRUE, TRUE, TRUE,
		delimiters)) {
	    *startPos = compiledRE->startp[0] - string;
	    *endPos = compiledRE->endp[0] - string;
	    XtFree((char *)compiledRE);
	    return TRUE;
	}
    }
    
    /* if wrap turned off, we're done */
    if (!wrap && beginPos >= 0) {
    	XtFree((char *)compiledRE);
    	return FALSE;
    }
    
    /* search from the end of the string to beginPos */
    if (beginPos < 0)
    	beginPos = 0;
    length = strlen(string); /* sadly, this means scanning entire string */
    if (ExecRE(compiledRE, string + beginPos + 1, string + length, TRUE,
    	    isStartOfLine(string, beginPos), isStartOfWord(string, beginPos),
    	    delimiters)) {
	*startPos = compiledRE->startp[0] - string;
	*endPos = compiledRE->endp[0] - string;
	XtFree((char *)compiledRE);
	return TRUE;
    }
    XtFree((char *)compiledRE);
    return FALSE;
}

static void upCaseString(char *outString, char *inString)
{
    char *outPtr, *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = toupper(*inPtr);
    }
    *outPtr = 0;
}

static void downCaseString(char *outString, char *inString)
{
    char *outPtr, *inPtr;
    
    for (outPtr=outString, inPtr=inString; *inPtr!=0; inPtr++, outPtr++) {
    	*outPtr = tolower(*inPtr);
    }
    *outPtr = 0;
}

/*
** resetReplaceTabGroup are really gruesome kludges to
** set the keyboard traversal.  XmProcessTraversal does not work at
** all on these dialogs.  ...It seems to have started working around
** Motif 1.1.2
*/
static void resetReplaceTabGroup(WindowInfo *window)
{
#ifdef MOTIF10
    XmRemoveTabGroup(window->replaceText);
    XmRemoveTabGroup(window->replaceWithText);
    XmRemoveTabGroup(window->replaceSearchTypeBox);
    XmRemoveTabGroup(window->replaceBtns);
    XmAddTabGroup(window->replaceText);
    XmAddTabGroup(window->replaceWithText);
    XmAddTabGroup(window->replaceSearchTypeBox);
    XmAddTabGroup(window->replaceBtns);
#endif
    XmProcessTraversal(window->replaceText, XmTRAVERSE_CURRENT);
}

/*
** Return TRUE if "searchString" exactly matches the text in the window's
** current primary selection using search algorithm "searchType".  If true,
** also return the position of the selection in "left" and "right".
*/
static int searchMatchesSelection(WindowInfo *window, char *searchString,
	int searchType, int *left, int *right)
{
    int selLen, selStart, selEnd, startPos, endPos;
    char *string;
    int found, isRect, rectStart, rectEnd, lineStart;
    
    /* find length of selection, give up on no selection or too long */
    if (!BufGetSelectionPos(window->editorInfo->buffer, &selStart, &selEnd, &isRect,
    	    &rectStart, &rectEnd))
	return FALSE;
    if (selEnd - selStart > SEARCHMAX)
	return FALSE;
    
    /* if the selection is rectangular, don't match if it spans lines */
    if (isRect) {
    	lineStart = BufStartOfLine(window->editorInfo->buffer, selStart);
    	if (lineStart != BufStartOfLine(window->editorInfo->buffer, selEnd))
    	    return FALSE;
    }
    
    /* get the selected text */
    string = BufGetSelectionText(window->editorInfo->buffer);
    if (*string == '\0') {
    	XtFree(string);
    	return FALSE;
    }
    selLen = strlen(string);
    
    /* search for the string in the selection (we are only interested 	*/
    /* in an exact match, but the procedure SearchString does important */
    /* stuff like applying the correct matching algorithm)		*/
    found = SearchString(string, searchString, SEARCH_FORWARD, searchType,
    	    FALSE, 0, &startPos, &endPos, GetWindowDelimiters(window));
    XtFree(string);
    
    /* decide if it is an exact match */
    if (!found)
    	return FALSE;
    if (startPos != 0 || endPos != selLen)
    	return FALSE;
    
    /* return the start and end of the selection */
    if (isRect)
    	GetSimpleSelection(window->editorInfo->buffer, left, right);
    else {
    	*left = selStart;
    	*right = selEnd;
    }
    return TRUE;
}

/*
** Substitutes a replace string for a string that was matched using a
** regular expression.  This was added later and is very inneficient
** because instead of using the compiled regular expression that was used
** to make the match in the first place, it re-compiles the expression
** and redoes the search on the already-matched string.  This allows the
** code to continue using strings to represent the search and replace
** items.
*/  
static void replaceUsingRE(char *searchStr, char *replaceStr, char *sourceStr,
	char *destStr, int maxDestLen, char *delimiters)
{
    regexp *compiledRE;
    char *compileMsg;
    
    compiledRE = CompileRE(searchStr, &compileMsg);
    ExecRE(compiledRE, sourceStr, NULL, False, True, True, delimiters);
    SubstituteRE(compiledRE, replaceStr, destStr, maxDestLen);
    XtFree((char *)compiledRE);
}

/*
** Store the search and replace strings, and search type for later recall.
** If replaceString is NULL, duplicate the last replaceString used.
*/
void SaveSearchHistory(char *searchString, char *replaceString,
	int searchType, Boolean isIncremental)
{
    char *sStr, *rStr;
    
    /* If replaceString is NULL, duplicate the last one (if any) */
    if (replaceString == NULL)
    	replaceString = NHist >= 1 ? ReplaceHistory[historyIndex(1)] : "";
    
    /* If an incremental save then just update the last item */
	if (isIncremental && NHist >=1 
	 && SearchIsIncrementalHistory[historyIndex(1)]
	 && searchType == SearchTypeHistory[historyIndex(1)]
	) {
    	XtFree(SearchHistory[historyIndex(1)]);
    	SearchHistory[historyIndex(1)] = XtNewString(searchString);
		SearchTypeHistory[historyIndex(1)] = searchType;
		return;
	}
	
	/* Compare the current search and replace strings against the saved ones.
       If they are identical, don't bother saving */
    if (NHist >= 1 && searchType == SearchTypeHistory[historyIndex(1)] &&
    	    !strcmp(SearchHistory[historyIndex(1)], searchString) &&
    	    !strcmp(ReplaceHistory[historyIndex(1)], replaceString)
		) {
		/* Remember to update the save inIncremental flag in this case. */
		SearchIsIncrementalHistory[historyIndex(1)] = isIncremental;
    	return;
    }
    
    /* If there are more than MAX_SEARCH_HISTORY strings saved, recycle
       some space, XtFree the entry that's about to be overwritten */
    if (NHist == MAX_SEARCH_HISTORY) {
    	XtFree(SearchHistory[HistStart]);
    	XtFree(ReplaceHistory[HistStart]);
    } else
    	NHist++;
    
    /* Allocate and copy the search and replace strings and add them to the
       circular buffers at HistStart, bump the buffer pointer to next pos. */
    sStr = XtMalloc(strlen(searchString) + 1);
    rStr = XtMalloc(strlen(replaceString) + 1);
    strcpy(sStr, searchString);
    strcpy(rStr, replaceString);
    SearchHistory[HistStart] = sStr;
    ReplaceHistory[HistStart] = rStr;
    SearchTypeHistory[HistStart] = searchType;
    SearchIsIncrementalHistory[HistStart] = isIncremental;
    HistStart++;
    if (HistStart >= MAX_SEARCH_HISTORY)
    	HistStart = 0;
}

/*
** return an index into the circular buffer arrays of history information
** for search strings, given the number of SaveSearchHistory cycles back from
** the current time.
*/

static int historyIndex(int nCycles)
{
    int index;
    
    if (nCycles > NHist || nCycles <= 0)
    	return -1;
    index = HistStart - nCycles;
    if (index < 0)
    	index = MAX_SEARCH_HISTORY + index;
    return index;
}

/*
** Return a pointer to the string describing search type for search action
** routine parameters (see menu.c for processing of action routines)
*/
static char *searchTypeArg(int searchType)
{
    if (searchType == SEARCH_LITERAL)
    	return "literal";
    else if (searchType == SEARCH_CASE_SENSE)
    	return "case";
    return "regex";
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters (see menu.c for processing of action routines)
*/
static char *directionArg(int direction)
{
    if (direction == SEARCH_BACKWARD)
    	return "backward";
    return "forward";
}

/*
** Determine if beginPos begins a line or word in string.  Note that we should
** be asking the caller to provide this information about the string as a
** whole, but it probably isn't worth all the extra work
*/
static int isStartOfLine(char *string, int beginPos)
{
    return beginPos == 0 ? TRUE : string[beginPos-1] == '\n';
}
static int isStartOfWord(char *string, int beginPos)
{
    char *d, prevChar;
    
    if (beginPos == 0)
    	return TRUE;
    prevChar = string[beginPos-1];
    if (prevChar == ' ' || prevChar == '\t' || prevChar == '\n')
    	return TRUE;
    for (d=GetPrefDelimiters(); *d!='\0'; d++)
	if (string[beginPos-1] == *d)
	    return TRUE;
    return FALSE;
}
