#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "search.h"
#include "selection.h"
#include "preferences.h"
#include "tags.h"
#include "server.h"

#define MAXLINE 512
#define MAX_TAG_LEN 80

enum searchDirection {FORWARD, BACKWARD};

typedef struct {
    char *name;
    char *file;
    char *searchString;
} tag;

typedef struct _SelectionInfo {
	int done;
	WindowInfo* window;
	char* selection;
} SelectionInfo;

static void setTag(tag *t, char *name, char *file, char *searchString);
static void freeTagList(tag *tags);
static int fakeRegExSearch(WindowInfo *window, char *searchString, 
	int *start, int *end);
static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
	Atom *type, char *value, int *length, int *format);

/* Parsed list of tags read by LoadTagsFile.  List is terminated by a tag
   structure with the name field == NULL */
static tag *Tags = NULL;
static char TagPath[MAXPATHLEN];

int LoadTagsFile(char *filename)
{
    FILE *fp = NULL;
    char line[MAXLINE], name[MAXLINE], file[MAXLINE], searchString[MAXLINE];
    char unused[MAXPATHLEN];
    char *charErr;
    tag *tags;
    int i, nTags, nRead;

    /* Open the file */
    if ((fp = fopen(filename, "r")) == NULL)
    	return FALSE;
	
    /* Read it once to see how many lines there are */
    for (nTags=0; TRUE; nTags++) {
    	charErr = fgets(line, MAXLINE, fp);
    	if (charErr == NULL) {
    	    if (feof(fp))
    	    	break;
    	    else
    	    	return FALSE;
    	}
    }
    
    /* Allocate zeroed memory for list so that it is automatically terminated
       and can be freed by freeTagList at any stage in its construction*/
    tags = (tag *)calloc(nTags + 1, sizeof(tag));
    
    /* Read the file and store its contents */
    rewind(fp);
    for (i=0; i<nTags; i++) {
    	charErr = fgets(line, MAXLINE, fp);
    	if (charErr == NULL) {
    	    if (feof(fp))
    	    	break;
    	    else {
    	    	freeTagList(tags);
    	    	fclose(fp);
    	    	return FALSE;
    	    }
    	}
    	nRead = sscanf(line, "%s\t%s\t%[^\n]", name, file, searchString);
    	if (nRead != 3) {
    	    freeTagList(tags);
    	    fclose(fp);
    	    return FALSE;
    	}
	setTag(&tags[i], name, file, searchString);
    }
    fclose(fp);
    
    /* Make sure everything was read */
    if (i != nTags) {
    	freeTagList(tags);
    	return FALSE;
    }
    
    /* Replace current tags data and path for retrieving files */
    if (Tags != NULL)
    	freeTagList(Tags);
    Tags = tags;
    ParseFilename(filename, unused, TagPath);
    
    return TRUE;
}

int TagsFileLoaded(void)
{
    return Tags != NULL;
}

/*
** Given a name, lookup a file, search string.  Returned strings are pointers
** to internal storage which are valid until the next LoadTagsFile call.
*/
int LookupTag(char *name, char **file, char **searchString)
{
    tag *t;
    
    if(Tags) {
		for (t=Tags; t->name!=NULL; t++) {
			if (!strcmp(t->name, name)) {
				*file = t->file;
				*searchString = t->searchString;
				return TRUE;
			}
		}
    }
    return FALSE;
}

void FindDefinition(WindowInfo *window, Time time)
{
    int startPos, endPos, found, lineNum, rows;
    char *fileToSearch, *searchString, *eptr;
    WindowInfo *windowToSearch;
    char filename[MAXPATHLEN], pathname[MAXPATHLEN], temp[MAXPATHLEN];
    SelectionInfo selectionInfo;
    
    /* Warn if the tags file has not been loaded */
    if(!TagsFileLoaded()) {
        DialogF(DF_WARN, window->shell, 1, "Tags file has not been loaded.", "OK");
    	return;
    }

    /* get the name to match from the primary selection */
    selectionInfo.window = window;
    selectionInfo.selection = 0;
    selectionInfo.done = 0;
    XtGetSelectionValue(window->textArea, XA_PRIMARY, XA_STRING,
    	    (XtSelectionCallbackProc)getSelectionCB, &selectionInfo, CurrentTime);
	while (selectionInfo.done == 0) {
	    XEvent nextEvent;
	    XtAppNextEvent(XtWidgetToApplicationContext(window->textArea), &nextEvent);
	    ServerDispatchEvent(&nextEvent);
	}
    if(selectionInfo.selection == 0) {
        return;
    }
    
    /* lookup the name in the tags file */
    found = LookupTag(selectionInfo.selection, &fileToSearch, &searchString);
    if (!found) {
    	DialogF(DF_WARN, window->shell, 1, "%s not found in tags file", "OK",
    		selectionInfo.selection);
    	XtFree(selectionInfo.selection);
    	return;
    }
    
    /* if the path is not absolute, qualify file path with directory
       from which tags file was loaded */
    if (fileToSearch[0] == '/'
#ifdef WIN32
	|| fileToSearch[0] == '\\'
#endif
	)
    	strcpy(temp, fileToSearch);
    else {
    	strcpy(temp, TagPath);
    	strcat(temp, fileToSearch);
    }
    ParseFilename(temp, filename, pathname);
    
    /* open the file containing the definition */
    windowToSearch = EditExistingFile(WindowList, filename, pathname, 0, True);
    if (windowToSearch == NULL) {
    	DialogF(DF_WARN, window->shell, 1, "File %s not found", 
    		"OK", fileToSearch);
    	XtFree(selectionInfo.selection);
    	return;
    }
    
    /* if the search string is a number, select the numbered line */
    lineNum = strtol(searchString, &eptr, 10);
    if (eptr != searchString) {
    	SelectNumberedLine(windowToSearch, lineNum);
    	XtFree(selectionInfo.selection);
    	return;
    }
    
    /* search for the tags file search string in the newly opened file */
    found = fakeRegExSearch(windowToSearch, searchString ,&startPos, &endPos);
    if (!found) {
    	DialogF(DF_WARN, windowToSearch->shell, 1,"Definition for %s\nnot found in %s", 
    		"OK", selectionInfo.selection, fileToSearch);
    	XtFree(selectionInfo.selection);
    	return;
    }
    XtFree(selectionInfo.selection);

    /* select the matched string */
    BufSelect(windowToSearch->editorInfo->buffer, startPos, endPos, CHAR_SELECT);
    
    /* Position it nicely in the window, about 1/4 of the way down from the
       top */
    lineNum = BufCountLines(windowToSearch->editorInfo->buffer, 0, startPos);
    XtVaGetValues(windowToSearch->lastFocus, textNrows, &rows, 0);
    TextSetScroll(windowToSearch->lastFocus, lineNum - rows/4, 0);
    TextSetCursorPos(windowToSearch->lastFocus, endPos);
    return;
}

static void setTag(tag *t, char *name, char *file, char *searchString)
{
    t->name = (char *)XtMalloc(sizeof(char) * strlen(name) + 1);
    strcpy(t->name, name);
    t->file = (char *)XtMalloc(sizeof(char) * strlen(file) + 1);
    strcpy(t->file, file);
    t->searchString = (char *)XtMalloc(sizeof(char) * strlen(searchString) + 1);
    strcpy(t->searchString, searchString);
}

static void freeTagList(tag *tags)
{
    int i;
    tag *t;
    
    for (i=0, t=tags; t->name!=NULL; i++, t++) {
    	XtFree(t->name);
    	XtFree(t->file);
    	XtFree(t->searchString);
    }
    XtFree((char*)tags);
}

/*
** regex searching is not available on all platforms.  To use built in
** case sensitive searching, this routine fakes enough to handle the
** search characters presented in ctags files
*/
static int fakeRegExSearch(WindowInfo *window, char *searchString, 
	int *start, int *end)
{
    int startPos, endPos, found=FALSE, hasBOL, hasEOL, fileLen, searchLen, dir;
    char *fileString, searchSubs[MAXLINE];
/* .b: char pointers for copying */
    char *inChar, *outChar;
/* .e */
    
    /* get the entire (sigh) text buffer from the text area widget */
    fileString = BufGetAll(window->editorInfo->buffer);
    fileLen = window->editorInfo->buffer->length;

    /* remove / .. / or ? .. ? and substitute ^ and $ with \n */
    searchLen = strlen(searchString);
    if (searchString[0] == '/')
    	dir = FORWARD;
    else if (searchString[0] == '?')
    	dir = BACKWARD;
    else {
    	DialogF(DF_WARN, window->shell, 1, "Error parsing search string from tag file.", "OK");
    	return FALSE;
    }
    searchLen -= 2;

/* .b: Remove backslashes and copy search string. */
    inChar = &searchString[1];
    outChar = searchSubs;
    while (*inChar != '\0') {
        if (*inChar == '\\') {
            inChar++;
            searchLen--;
        }
        *outChar = *inChar;
        inChar++;
        outChar++;
    }
/* .e */
    searchSubs[searchLen] = '\0';
    hasBOL = searchSubs[0] == '^';
    hasEOL = searchSubs[searchLen-1] == '$';
    if (hasBOL) searchSubs[0] = '\n';
    if (hasEOL) searchSubs[searchLen-1] = '\n';

    /* search for newline-substituted string in the file */
    if (dir==FORWARD)
    	found = SearchString(fileString, searchSubs, SEARCH_FORWARD,
    		SEARCH_CASE_SENSE, False, 0, &startPos, &endPos, NULL);
    else
    	found = SearchString(fileString, searchSubs, SEARCH_BACKWARD,
    		SEARCH_CASE_SENSE, False, fileLen, &startPos, &endPos, NULL);
    if (found) {
    	if (hasBOL) startPos++;
    	if (hasEOL) endPos--;
    }
    
    /* if not found: ^ may match beginning of file, $ may match end */
    if (!found && hasBOL) {
    	found = strncmp(&searchSubs[1], fileString, searchLen-1);
    	if (found) {
    	    startPos = 0;
    	    endPos = searchLen - 2;
    	}
    }
    if (!found && hasEOL) {	    
    	found = strncmp(searchSubs, fileString+fileLen-searchLen+1,
    		 searchLen-1);
    	if (found) {
    	    startPos = fileLen-searchLen+2;
    	    endPos = fileLen;
    	}
    }

    /* XtFree the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
    
    /* return the result */
    if (!found) {
    	XBell(TheDisplay, 0);
	return FALSE;
    }

    /* save the search string in the history minus the added newlines */
    if(hasEOL) searchSubs[searchLen-1] = 0;
    SaveSearchHistory(hasBOL ? searchSubs+1 : searchSubs, NULL, SEARCH_CASE_SENSE, False);
    
    *start = startPos;
    *end = endPos;
    return TRUE;
}

static void getSelectionCB(Widget w, SelectionInfo *selectionInfo, Atom *selection,
	Atom *type, char *value, int *length, int *format)
{
	WindowInfo *window = selectionInfo->window;
	
	/* skip if we can't get the selection data or it's too long */
	if (*type == XT_CONVERT_FAIL || *type != XA_STRING || value == NULL) {
		if (GetPrefSearchDlogs())
			DialogF(DF_WARN, window->shell, 1,
				"Selection not appropriate for searching", "OK");
		else
			XBell(TheDisplay, 0);
		XtFree(value);
		selectionInfo->selection = 0;
		selectionInfo->done = 1;
		return;
	}
	if (*length > SEARCHMAX) {
		if (GetPrefSearchDlogs())
			DialogF(DF_WARN, window->shell, 1, "Selection too long", "OK");
		else
			XBell(TheDisplay, 0);
		XtFree(value);
		selectionInfo->selection = 0;
		selectionInfo->done = 1;
		return;
	}
	if (*length == 0) {
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
