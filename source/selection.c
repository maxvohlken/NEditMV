/*******************************************************************************
*									       *
* selection.c - (some) Nirvana Editor commands operating the primary selection *
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
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#ifdef HAVE_GLOB_H
# include <glob.h>
#endif
#include <Xm/Xm.h>
#include <X11/Xatom.h>
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "selection.h"
#include "file.h"
#include "window.h"
#include "menu.h"
#include "search.h"
#include "server.h"

typedef struct _SelectionInfo {
	int done;
	WindowInfo* window;
	char* selection;
} SelectionInfo;

static void normalizePathToWindow(WindowInfo *window, char *path);
static void getAnySelectionCB(Widget widget, char **result, Atom *sel,
	Atom *type, char *value, int *length, int *format);
static void processMarkEvent(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch, char *action, int extend);
static void markTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void markKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch);
static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch);
static void maintainSelection(selection *sel, int pos, int nInserted,
    	int nDeleted);
static void gotoMarkExtendKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch);
static void maintainPosition(int *position, int modPos, int nInserted,
    	int nDeleted);
static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *sel,
	Atom *type, char *value, int *length, int *format);

void GotoLineNumber(WindowInfo *window)
{
    char lineNumText[DF_MAX_PROMPT_LENGTH], *params[1];
    int lineNum, nRead, response;
    
    response = DialogF(DF_PROMPT, window->shell, 2, "Goto Line Number:",
    		       lineNumText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(lineNumText, "%d", &lineNum);
    if (nRead != 1) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = lineNumText;
    XtCallActionProc(window->lastFocus, "goto_line_number", NULL, params, 1);
}
    
void GotoSelectedLineNumber(WindowInfo *window, Time time)
{
    int lineNum;
    SelectionInfo selectionInfo;

    selectionInfo.done = 0;
    selectionInfo.window = window;
    selectionInfo.selection = 0;
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)getSelectionCB, &selectionInfo, time);
	while (selectionInfo.done == 0) {
	    XEvent nextEvent;
	    XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
	    ServerDispatchEvent(&nextEvent);
	}
    if(selectionInfo.selection == 0) {
        return;
    }
    if (sscanf(selectionInfo.selection, "%d", &lineNum) != 1) {
    	XBell(TheDisplay, 0);
    	XtFree(selectionInfo.selection);
		return;
    }
    XtFree(selectionInfo.selection);
    SelectNumberedLine(window, lineNum);
}

void OpenSelectedFile(WindowInfo *window, Time time)
{
    char *line, *term, *term2;
    int lineNum;
    char buf1[MAXPATHLEN], buf2[MAXPATHLEN];
    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
    WindowInfo *windowToSearch;
    SelectionInfo selectionInfo;

    selectionInfo.done = 0;
    selectionInfo.window = window;
    selectionInfo.selection = 0;
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)getSelectionCB, &selectionInfo, time);
	while (selectionInfo.done == 0) {
	    XEvent nextEvent;
	    XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
	    ServerDispatchEvent(&nextEvent);
	}
    if(selectionInfo.selection == 0) {
        return;
    }
        
    /* for each line in the selection */
    for(line = selectionInfo.selection; ; line = term + 1) {
		char searchString[SEARCHMAX+1];
		term = strpbrk(line, "\n\r\f");
		if(term) 
	    	*term = 0;
		/* extract name from #include syntax */
		if (sscanf(line, "# include \"%[^\"]\"", buf1) == 1) {
			normalizePathToWindow(window, buf1);
	    	ParseFilename(buf1, filename, pathname);
	    	EditExistingFile(WindowList, filename, pathname, CREATE, True);
		}
		else if (sscanf(line, "# include <%[^<>]>", buf1) == 1) {
			strcpy(buf2, buf1);
			FindIncludeFile(buf2);
			normalizePathToWindow(window, buf2);
	    	ParseFilename(buf2, filename, pathname);
	    	EditExistingFile(WindowList, filename, pathname, CREATE, True);
		}
		/* goto file/line from compiler output */
		else if (sscanf(line, "%[^:\n \t]: %d", buf1, &lineNum) == 2
/* .b: cc output: any selection including <"file.c", line nn:> */
                  || sscanf(line, " \"%[^\"]\", %*s %d", buf1, &lineNum) == 2
                  || sscanf(line, "%*s \"%[^\"]\", %*s %d", buf1, &lineNum) == 2
/* .e */
		  || sscanf(line, "\"%[^\"]\", line %d", buf1, &lineNum) == 2) {
			normalizePathToWindow(window, buf1);
			ParseFilename(buf1, filename, pathname);
			windowToSearch = EditExistingFile(WindowList, filename, pathname, CREATE, True);
			if (windowToSearch != NULL) {
				SelectNumberedLine(windowToSearch, lineNum);
	    	}
		}
		/* goto file/line from grep output */
		else if (sscanf(line, "%[^:\n \t]: %[^\n]", buf1, searchString) == 2
#ifdef _WIN32
			&& strlen(buf1) > 1
#endif
		) {
			normalizePathToWindow(window, buf1);
	    	ParseFilename(buf1, filename, pathname);
	    	windowToSearch = EditExistingFile(WindowList, filename, pathname, CREATE, True);
	    	if (windowToSearch != NULL) {
				/* search for it in the window */
				SearchAndSelect(windowToSearch, SEARCH_FORWARD, searchString, SEARCH_CASE_SENSE);
	    	}
		} 
		else {
	    	char *path;
	    	/* edit all of the files on the line using whitespace 
	    	 * and other special characters as a delimiters
	    	 */
	    	for(path = line; ; path = term2 + 1) {
				/* skip leading non file characters */
				path += strspn(path, BadFilenameChars);

				/* stop if empty string */
				if(*path == 0)
					break;

				/* find the end of the filename */
				term2 = strpbrk(path, BadFilenameChars);
				if(term2) {
					*term2 = 0;
				}

				if(strlen(path) == 0) {
					break;
				}

				/* Open the file */
				strcpy(buf1, path);
#ifndef VMS
    			/* Process ~ characters in name */
    			ExpandTilde(buf1);
#endif
				normalizePathToWindow(window, buf1);

    			/* Expand wildcards in file name.
    			   Some older systems don't have the glob subroutine for expanding file
    			   names, in these cases, either don't expand names, or try to use the
    			   Motif internal parsing routine _XmOSGetDirEntries, which is not
    			   guranteed to be available, but in practice is there and does work. */
#if defined(ENABLE_OPEN_SELECTED_GLOBBING) && defined(HAVE_GLOB_H)
				{	glob_t globbuf; int i;
					glob(buf1, GLOB_NOCHECK, NULL, &globbuf);
					for (i=0; i<globbuf.gl_pathc; i++) {
						ParseFilename(globbuf.gl_pathv[i], filename, pathname);
						if(!EditExistingFile(WindowList, filename, pathname, CREATE, True)) {
							term2 = 0; /* We are done with this line if cancelled by the user */
							break;
						}
					}
					globfree(&globbuf);
				}
#elif defined(ENABLE_OPEN_SELECTED_GLOBBING) && defined(HAVE__XMOSGETDIRENTRIES)
				{	char **nameList = NULL; int i, nFiles = 0, maxFiles = 30;
					ParseFilename(buf1, filename, pathname);
					_XmOSGetDirEntries(pathname, filename, XmFILE_ANY_TYPE, False, True,
						&nameList, &nFiles, &maxFiles);
					for (i=0; i<nFiles; i++) {
						ParseFilename(nameList[i], filename, pathname);
						if(!EditExistingFile(WindowList, filename, pathname, CREATE, True)) {
							term2 = 0; /* We are done with this line if cancelled by the user */
							break;
						}
					}
					for (i=0; i<nFiles; i++)
						XtFree(nameList[i]);
					XtFree((char *)nameList);
				}
#else
				/* Open the file */
				ParseFilename(buf1, filename, pathname);
				if(!EditExistingFile(WindowList, filename, pathname, CREATE, True)) {
					break;
				}
#endif

				if(term2 == 0) {
					break;
				}
			}
      	}

		if(term == 0) {
			break;
		}
    }
	
	XtFree(selectionInfo.selection);
	CheckCloseDim();
}

/*
** If path is a relative path then make it an absolute path relative
** to the current window. If it is already absolute then it is not modified.
*/
static void normalizePathToWindow(WindowInfo *window, char *path)
{
    char buf1[MAXPATHLEN];
    int len;
	if(window == 0 || window->editorInfo->path[0] == 0 || path == 0
#ifdef _WIN32
		/* absolute if <drive letter>:\<path> */
		|| (isalpha(path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\'))
		/* or a UNC path */
		|| ((path[0] == '/' || path[0] == '\\') && (path[1] == '/' || path[1] == '\\'))
#else
		|| *path == '/'
#endif
	)
		return;
	strcpy(buf1, window->editorInfo->path);
#ifdef _WIN32
	/* We only need the drive letter from the window. The path is absolute otherwise. */
	if(path[0] == '/' || path[0] == '\\') {
		/* remove everything after <drive_letter>:\ */
		buf1[3] = 0;
	} else {
		/* See if we need to add a backslash */
		len = strlen(buf1);
		if(len > 0 && buf1[len-1] != '/'
			&& buf1[len-1] != '\\'
		) {
			strcat(buf1, "\\");
		}
	}
#else
	/* see if we need to add a slash */
	len = strlen(buf1);
	if(len > 0 && buf1[len-1] != '/') {
		strcat(buf1, "/");
	}
#endif
	strcat(buf1, path);
	strcpy(path, buf1);
}

/*
** Getting the current selection by making the request, and then blocking
** (processing events) while waiting for a reply.  On failure (timeout or
** bad format) returns NULL, otherwise returns the contents of the selection.
*/
char *GetAnySelection(WindowInfo *window)
{
    char waitingMarker[1] = "";
    char *selText = waitingMarker;
    XEvent nextEvent;	 
    
    /* If the selection is in the window's own buffer get it from there,
       but substitute null characters as if it were an external selection */
    if (window->editorInfo->buffer->primary.selected) {
	selText = BufGetSelectionText(window->editorInfo->buffer);
	BufUnsubstituteNullChars(selText, window->editorInfo->buffer);
	return selText;
    }
    
    /* Request the selection value to be delivered to getAnySelectionCB */
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
	    (XtSelectionCallbackProc)getAnySelectionCB, &selText, 
	    XtLastTimestampProcessed(XtDisplay(window->textArea)));

    /* Wait for the value to appear */
    while (selText == waitingMarker) {
	XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), 
		&nextEvent);
	ServerDispatchEvent(&nextEvent);
    }
    return selText;
}

static void getAnySelectionCB(Widget widget, char **result, Atom *sel,
	Atom *type, char *value, int *length, int *format)
{
    /* Confirm that the returned value is of the correct type */
    if (*type != XA_STRING || *format != 8) {
	XBell(TheDisplay, 0);
	if (value != NULL)
	    XtFree((char *)value);
	*result = NULL;
	return;
    }

    /* Append a null, and return the string */
    *result = XtMalloc(*length + 1);
    strncpy(*result, value, *length);
    XtFree(value);
    (*result)[*length] = '\0';
    XtFree(value);
}

void SelectNumberedLine(WindowInfo *window, int lineNum)
{
    int i, lineStart = 0, lineEnd;

    /* count lines to find the start and end positions for the selection */
    if (lineNum < 1)
    	lineNum = 1;
    lineEnd = -1;
    for (i=1; i<=lineNum && lineEnd<window->editorInfo->buffer->length; i++) {
    	lineStart = lineEnd + 1;
    	lineEnd = BufEndOfLine(window->editorInfo->buffer, lineStart);
    }

/* .b:  Warn if file has less lines than numbered */
    if (lineNum > i-1) {
        char msg[40];

        XBell(TheDisplay, 0);
        sprintf(msg, "Last line number: %d", i-1);
        DialogF(DF_WARN, window->shell, 1, msg, "OK");
        return;
    }
/* .e */
    
    /* highlight the line */
    BufSelect(window->editorInfo->buffer, lineStart, lineEnd+1, CHAR_SELECT);
    MakeSelectionVisible(window, window->lastFocus);
    TextSetCursorPos(window->lastFocus, lineStart);
}

void MarkDialog(WindowInfo *window)
{
    char letterText[DF_MAX_PROMPT_LENGTH], *params[1];
    int response;
    
    response = DialogF(DF_PROMPT, window->shell, 2,
"Enter a single letter label to use for recalling\n\
the current selection and cursor position.\n\
\n\
(To skip this dialog, use the accelerator key,\n\
followed immediately by a letter key (a-z))",
    		       letterText, "OK", "Cancel");
    if (response == 2)
    	return;
    if (strlen(letterText) != 1 || !isalpha(letterText[0])) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = letterText;
    XtCallActionProc(window->lastFocus, "mark", NULL, params, 1);
}

void GotoMarkDialog(WindowInfo *window, int extend)
{
    char letterText[DF_MAX_PROMPT_LENGTH], *params[2];
    int response;
    
    response = DialogF(DF_PROMPT, window->shell, 2,
"Enter the single letter label used to mark\n\
the selection and/or cursor position.\n\
\n\
(To skip this dialog, use the accelerator\n\
key, followed immediately by the letter)",
    		       letterText, "OK", "Cancel");
    if (response == 2)
    	return;
    if (strlen(letterText) != 1 || !isalpha(letterText[0])) {
    	XBell(TheDisplay, 0);
	return;
    }
    params[0] = letterText;
    params[1] = "extend";
    XtCallActionProc(window->lastFocus, "goto_mark", NULL, params,
	    extend ? 2 : 1);
}

/*
** Process a command to mark a selection.  Expects the user to continue
** the command by typing a label character.  Handles both correct user
** behavior (type a character a-z) or bad behavior (do nothing or type
** something else).
*/
void BeginMarkCommand(WindowInfo *window)
{
    XtInsertEventHandler(window->lastFocus, KeyPressMask, False,
    	    markKeyCB, window, XtListHead);
    window->markTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 4000,
    	    markTimeoutProc, window->lastFocus);
}

/*
** Process a command to go to a marked selection.  Expects the user to
** continue the command by typing a label character.  Handles both correct
** user behavior (type a character a-z) or bad behavior (do nothing or type
** something else).
*/
void BeginGotoMarkCommand(WindowInfo *window, int extend)
{
    XtInsertEventHandler(window->lastFocus, KeyPressMask, False,
    	    extend ? gotoMarkExtendKeyCB : gotoMarkKeyCB, window, XtListHead);
    window->markTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell), 4000,
    	    markTimeoutProc, window->lastFocus);
}

/*
** Xt timer procedure for removing event handler if user failed to type a
** mark character withing the allowed time
*/
static void markTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    Widget w = (Widget)clientData;
    WindowInfo *window = WidgetToWindow(w);
    
    XtRemoveEventHandler(w, KeyPressMask, False, markKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkExtendKeyCB, window);
    window->markTimeoutID = 0;
}

/*
** Temporary event handlers for keys pressed after the mark or goto-mark
** commands, If the key is valid, grab the key event and call the action
** procedure to mark (or go to) the selection, otherwise, remove the handler
** and give up.
*/
static void processMarkEvent(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch, char *action, int extend)
{
    XKeyEvent *e = (XKeyEvent *)event;
    WindowInfo *window = WidgetToWindow(w);
    Modifiers modifiers;
    KeySym keysym;
    char *params[2], string[2];

    XtTranslateKeycode(TheDisplay, e->keycode, e->state, &modifiers,
    	    &keysym);
    if ((keysym >= 'A' && keysym <= 'Z') || (keysym >= 'a' && keysym <= 'z')) {
    	string[0] = toupper(keysym);
    	string[1] = '\0';
    	params[0] = string;
	params[1] = "extend";
    	XtCallActionProc(window->lastFocus, action, event, params,
		extend ? 2 : 1);
    	*continueDispatch = False;
    }
    XtRemoveEventHandler(w, KeyPressMask, False, markKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkKeyCB, window);
    XtRemoveEventHandler(w, KeyPressMask, False, gotoMarkExtendKeyCB, window);
    XtRemoveTimeOut(window->markTimeoutID);
}
static void markKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch)
{
    processMarkEvent(w, clientData, event, continueDispatch, "mark", False);
}
static void gotoMarkKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch)
{
    processMarkEvent(w, clientData, event, continueDispatch, "goto_mark",False);
}
static void gotoMarkExtendKeyCB(Widget w, XtPointer clientData, XEvent *event,
    	Boolean *continueDispatch)
{
    processMarkEvent(w, clientData, event, continueDispatch, "goto_mark", True);
}

void AddMark(WindowInfo *window, Widget widget, char label)
{
    int index;
    
    /* look for a matching mark to re-use, or advance
       nMarks to create a new one */
    label = toupper(label);
    for (index=0; index<window->nMarks; index++) {
    	if (window->markTable[index].label == label)
   	    break;
    }
    if (index >= MAX_MARKS) {
    	DialogF(DF_WARN, window->shell, 1, "No more marks allowed.", "OK"); /* shouldn't happen */
    	return;
    }
    if (index == window->nMarks)
    	window->nMarks++;
    
    /* store the cursor location and selection position in the table */
    window->markTable[index].label = label;
    memcpy(&window->markTable[index].sel, &window->editorInfo->buffer->primary,
    	    sizeof(selection));
    window->markTable[index].cursorPos = TextGetCursorPos(widget);
}

void GotoMark(WindowInfo *window, Widget w, char label, int extendSel)
{
    int index, oldStart, newStart, oldEnd, newEnd, cursorPos;
    selection *sel, *oldSel;
    
    /* look up the mark in the mark table */
    label = toupper(label);
    for (index=0; index<window->nMarks; index++) {
    	if (window->markTable[index].label == label)
   	    break;
    }
    if (index == window->nMarks) {
    	XBell(TheDisplay, 0);
    	return;
    }
    
    /* reselect marked the selection, and move the cursor to the marked pos */
    sel = &window->markTable[index].sel;
    oldSel = &window->editorInfo->buffer->primary;
    cursorPos = window->markTable[index].cursorPos;
    if (extendSel) {
	oldStart = oldSel->selected ? oldSel->start : TextGetCursorPos(w);
	oldEnd = oldSel->selected ? oldSel->end : TextGetCursorPos(w);
	newStart = sel->selected ? sel->start : cursorPos;
	newEnd = sel->selected ? sel->end : cursorPos;
	BufSelect(window->editorInfo->buffer, oldStart < newStart ? oldStart : newStart,
		oldEnd > newEnd ? oldEnd : newEnd, CHAR_SELECT);
    } else {
	if (sel->selected) {
    	    if (sel->rectangular)
    		BufRectSelect(window->editorInfo->buffer, sel->start, sel->end,
			sel->rectStart, sel->rectEnd);
    	    else
    		BufSelect(window->editorInfo->buffer, sel->start, sel->end, sel->type);
	} else
    	    BufUnselect(window->editorInfo->buffer);
    }
    
    /* Move the window into a pleasing position relative to the selection
       or cursor.   MakeSelectionVisible is not great with multi-line
       selections, and here we will sometimes give it one.  And to set the
       cursor position without first using the less pleasing capability
       of the widget itself for bringing the cursor in to view, you have to
       first turn it off, set the position, then turn it back on. */
    XtVaSetValues(w, textNautoShowInsertPos, False, NULL);
    TextSetCursorPos(w, cursorPos);
    MakeSelectionVisible(window, window->lastFocus);
    XtVaSetValues(w, textNautoShowInsertPos, True, NULL);
}

/*
** Keep the marks in the windows book-mark table up to date across
** changes to the underlying buffer
*/
void UpdateMarkTable(WindowInfo *window, int pos, int nInserted,
    	int nDeleted)
{
    int i;
    
    for (i=0; i<window->nMarks; i++) {
    	maintainSelection(&window->markTable[i].sel, pos, nInserted,
    	    	nDeleted);
    	maintainPosition(&window->markTable[i].cursorPos, pos, nInserted,
    	    	nDeleted);
    }
}

/*
** Update a selection across buffer modifications specified by
** "pos", "nDeleted", and "nInserted".
*/
static void maintainSelection(selection *sel, int pos, int nInserted,
	int nDeleted)
{
    if (!sel->selected || pos > sel->end)
    	return;
    maintainPosition(&sel->start, pos, nInserted, nDeleted);
    maintainPosition(&sel->end, pos, nInserted, nDeleted);
    if (sel->end <= sel->start)
	sel->selected = False;
}

/*
** Update a position across buffer modifications specified by
** "modPos", "nDeleted", and "nInserted".
*/
static void maintainPosition(int *position, int modPos, int nInserted,
    	int nDeleted)
{
    if (modPos > *position)
    	return;
    if (modPos+nDeleted <= *position)
    	*position += nInserted - nDeleted;
    else
    	*position = modPos;
}

static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *sel,
	Atom *type, char *value, int *length, int *format)
{
	WindowInfo *window = selectionInfo->window;
	
	/* skip if we can't get the selection data or it's too long */
	if (*type == XT_CONVERT_FAIL || *type != XA_STRING || value == NULL || *length == 0) {
		XBell(TheDisplay, 0);
		XtFree(value);
		selectionInfo->selection = 0;
		selectionInfo->done = 1;
		return;
	}
	/* should be of type text??? */
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
