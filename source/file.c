/*******************************************************************************
*									       *
* file.c -- Nirvana Editor file i/o					       *
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
#include <errno.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#include <string.h>
#include <stdlib.h>
#ifdef VMS
#include "../util/VMSparam.h"
#include <types.h>
#include <stat.h>
#include <unixio.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UTIME_H
# include <utime.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#endif /*VMS*/
#ifdef _WIN32
# include <io.h>
#endif
#include <Xm/Xm.h>
#include <Xm/ToggleB.h>
#include <Xm/FileSB.h>
#include "../util/misc.h"
#include "../util/DialogF.h"
#include "../util/fileUtils.h"
#include "../util/getfiles.h"
#include "../util/printUtils.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "undo.h"
#include "menu.h"
#include "file.h"
#include "menu.h"
#include "server.h"
#include "highlight.h"
#include "shell.h"
#include "clearcase.h"

static int doSave(WindowInfo *window);
static int doOpen(WindowInfo *window, char *name, char *path, int flags, long start, long end);
static void backupFileName(WindowInfo *window, char *name);
static int writeBckVersion(WindowInfo *window);
static int bckError(WindowInfo *window, char *errString, char *file);
static char *errorString(void);
static void addWrapNewlines(WindowInfo *window);
static void addWrapCB(Widget w, XtPointer clientData, XtPointer callData);
#ifdef VMS
void removeVersionNumber(char *fileName);
#endif /*VMS*/

WindowInfo* EditNewFile(WindowInfo *parentWindow, Boolean forceRaise)
{
    char name[MAXPATHLEN];
    WindowInfo *window;
    
    /*... test for creatability? */
    
    /* Find a (relatively) unique name for the new file */
    UniqueUntitledName(name);

    /* create the window */
    window = NEditCreateWindow(name, NULL);
    strcpy(window->editorInfo->filename, name);
    
    /* Use the path of the window were we were invoked from so that the
       open dialog, etc will be relative to that window. */
    strcpy(window->editorInfo->path, parentWindow ? parentWindow->editorInfo->path : "");
    window->editorInfo->filenameSet = FALSE;
    SetWindowModified(window, FALSE);
    window->editorInfo->readOnly = FALSE;
    window->editorInfo->lockWrite = FALSE;
    DetermineLanguageMode(window, True);
    UpdateWindowReadOnly(window);
    UpdateStatsLine(window);
    UpdateWindowTitle(window);
    CheckCloseDim();

   	if(forceRaise) {
   		RaiseShellWindow(window->shell);
   	}
	
    return window;
}   

/*
** Open an existing file specified by name and path.  Use the window inWindow
** unless inWindow is NULL or points to a window which is already in use
** (displays a file other than Untitled, or is Untitled but modified).  Flags
** can be any of:
**
**	CREATE: 		If file is not found, (optionally) prompt the
**				user whether to create
**	SUPPRESS_CREATE_WARN	When creating a file, don't ask the user
**	FORCE_READ_ONLY		Make the file read-only regardless
**
*/
WindowInfo* EditExistingFile(WindowInfo *inWindow, char *name, char *path, int flags, Boolean forceRaise)
{
    WindowInfo *window;
    char fullname[MAXPATHLEN];
    
    /* first look to see if file is already displayed in a window */
    window = FindWindowWithFile(name, path);
    if (window != NULL) {
    	if(forceRaise) {
    		RaiseShellWindow(window->shell);
    	}
    	/* Refresh the file open property to indicate that we have
    	** been reopened .*/
    	CreateFileOpenProperty(window);
    	return window;
    }
    
    /* If an existing window isn't specified, or the window is already
       in use (not Untitled or Untitled and modified), create the window */
    if (inWindow == NULL 
	 || inWindow->editorInfo->filenameSet 
	 || inWindow->editorInfo->fileChanged 
	 || IsShellCommandInProgress(inWindow)
	 || inWindow->editorInfo->buffer->length > 0
	) {
    	window = NEditCreateWindow(name, NULL);
    } else {
    	window = inWindow;
    }
    
   	/* Make sure the window is displayed to the user */
   	if(forceRaise) {
   		RaiseShellWindow(window->shell);
   	}
    	
    /* Open the file */
    if (!doOpen(window, name, path, flags, 0, -1)) {
    	NEditCloseWindow(window);
    	return NULL;
    }
    
    /* Bring the title bar and statistics line up to date, doOpen does
       not necessarily set the window title or read-only status */
   	UpdateWindowReadOnly(window);
   	UpdateWindowTitle(window);
    UpdateStatsLine(window);
    CheckCloseDim();
    
    /* Add the name to the convenience menu of previously opened files */
    strcpy(fullname, path);
    strcat(fullname, name);
    AddToPrevOpenMenu(fullname);
    
    /* Create the property used to keep track of the open state of this window */
    CreateFileOpenProperty(window);
    
    /* Set the checking mode back to the default */
    SetCheckingMode(window, GetPrefCheckingMode());
    
    /* Decide what language mode to use, trigger language specific actions */
    DetermineLanguageMode(window, True);

    return window;
}

void RevertToSaved(WindowInfo *window, int silent)
{
    char name[MAXPATHLEN], path[MAXPATHLEN];
    int i;
    int insertPositions[MAX_PANES], topLines[MAX_PANES];
    int horizOffsets[MAX_PANES];
    Widget text;
    
    /* Can't revert untitled windows */
    if (!window->editorInfo->filenameSet) {
    	DialogF(DF_WARN, window->shell, 1,
    		"Window was never saved, can't re-read", "Dismiss");
    	return;
    }
    
    /* re-reading file is irreversible, prompt the user first */
    if (!silent) {
		if (window->editorInfo->fileChanged) {
    		int b;
	    	b = DialogF(DF_QUES, window->shell, 2, "Discard changes to\n%s%s?",
    		    "Discard Changes", "Cancel", window->editorInfo->path, window->editorInfo->filename);
			if (b != 1)
    	    	return;
		}
    }
    
    /* save insert & scroll positions of all of the panes to restore later */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	TextGetScroll(text, &topLines[i], &horizOffsets[i]);
    }

    /* re-read the file, update the window title if new file is different */
    strcpy(name, window->editorInfo->filename);
    strcpy(path, window->editorInfo->path);
    RemoveBackupFile(window);
    ClearUndoList(window);

	{
	Boolean highlight = (window->highlightSyntax && window->highlightData);
	StopHighlighting(window);
    if (!doOpen(window, name, path, 0, 0, -1)) {
    	NEditCloseWindow(window);
    	return;
    }
	if(highlight) {
		StartHighlighting(window, False);
	}
	} /* local vars */
    UpdateWindowReadOnly(window);
    UpdateWindowTitle(window);
    
    /* restore the insert and scroll positions of each pane */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], horizOffsets[i]);
    }
    CheckCloseDim();
}

static int doOpen(WindowInfo *window, char *name, char *path, int flags, long start, long end)
{
    char fullname[MAXPATHLEN];
    struct stat statbuf;
    int fileLen, readLen;
    char *fileString, *c;
    FILE *fp = NULL;
    int fd;
    int readOnly = FALSE;
    int resp;
    
    /* Update the window data structure */
    strcpy(window->editorInfo->filename, name);
    strcpy(window->editorInfo->path, path);
    window->editorInfo->filenameSet = TRUE;

    /* Get the full name of the file */
    strcpy(fullname, path);
    strcat(fullname, name);
    
    /* Open the file */
#ifdef USE_ACCESS
# ifdef WIN32_BINARY_FILE_MODE
    fp = fopen(fullname, "rb");
# else
    fp = fopen(fullname, "r");
# endif
#else
	/* Try to open read/write */
# ifdef WIN32_BINARY_FILE_MODE
    fp = fopen(fullname, "rb+");
# else
    fp = fopen(fullname, "r+");
# endif
	/* If that failed then try read-only */
    if (fp == NULL) {
		readOnly = TRUE;
# ifdef WIN32_BINARY_FILE_MODE
		fp = fopen(fullname, "rb");
# else
		fp = fopen(fullname, "r");
# endif
   }
#endif
    if (fp == NULL) {
		/* Error opening the file. If the CREATE flag is set
		   and the error was because the file didn't exist then
		   try to create it. */
		if (flags & CREATE && errno == ENOENT) {
			/* Give option to create (or to exit if this is the only window) */
			if (!(flags & SUPPRESS_CREATE_WARN)) {
				if (WindowList == window && window->next == NULL)
					resp = DialogF(DF_WARN, window->shell, 3,
						"Can't open %s:\n%s", "Create", "Cancel",
						"Exit NEdit", fullname, errorString());
				else
					resp = DialogF(DF_WARN, window->shell, 2,
						"Can't open %s:\n%s", "Create", "Cancel",
						fullname, errorString());
				if (resp == 2)
					return FALSE;
				else if (resp == 3)
					exit(0);
			}
			/* Test if new file can be created */
			if ((fd = creat(fullname, 0666)) == -1) {
				DialogF(DF_ERR, window->shell, 1, "Can't create %s:\n%s",
					"Dismiss", fullname, errorString());
				return FALSE;
			} else {
#ifdef VMS
				/* get correct version number and close before removing */
				getname(fd, fullname);
#endif
				close(fd);
				remove(fullname);
			}
			SetWindowModified(window, FALSE);
			window->editorInfo->readOnly = FALSE;
			window->editorInfo->lockWrite = flags & FORCE_READ_ONLY;
			UpdateWindowReadOnly(window);
			return TRUE;
		} else {
			/* A true error */
			DialogF(DF_ERR, window->shell, 1, "Could not open %s%s:\n%s",
				"Dismiss", path, name, errorString());
			return FALSE;
		}
    }
    
    /* See if the file is writable */
#ifdef USE_ACCESS
    if(access(fullname, W_OK) != 0) {
		/* File is read only */
		readOnly = TRUE;
	}
#endif
	
    /* Get the length of the file and the protection mode */
    if (fstat(fileno(fp), &statbuf) != 0) {
	DialogF(DF_ERR, window->shell, 1, "Error opening %s", "Dismiss", name);
	fclose(fp);
	return FALSE;
    }
    
    /* make sure start is within the range of the file */
    start = start < 0 ? 0 : start;
    start = start > statbuf.st_size ? statbuf.st_size : start;
    
    /* make sure end is within the range of the file */
    /* if end is negative then use the end of the file */
    end = end < 0 ? statbuf.st_size : end;
    end = end > statbuf.st_size ? statbuf.st_size : end;
    
    /* make sure end is greater than start */
	if(start > end) {
		long tmp;
		tmp = end;
		end = start;
		start = tmp;
	}
    fileLen = end - start;
    window->editorInfo->st__mtime = statbuf.st_mtime;
    window->editorInfo->st_mode = statbuf.st_mode;
    window->editorInfo->st_size = statbuf.st_size;
    
    /* Allocate space for the whole contents of the file (unfortunately) */
    fileString = (char *)XtMalloc(fileLen+1);  /* +1 = space for null */
    if (fileString == NULL) {
	DialogF(DF_ERR, window->shell, 1, "File is too large to edit",
	    	"Dismiss");
	fclose(fp);
	return FALSE;
    }

    /* Read the file into fileString and terminate with a null */
    if(start > 0 && fseek(fp, start, SEEK_SET) != 0) {
		DialogF(DF_ERR, window->shell, 1, "Error reading %s", "Dismiss", name);
		fclose(fp);
		return FALSE;
    }
    readLen = fread(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "Error reading %s:\n%s", "Dismiss",
    		name, errorString());
		XtFree(fileString);
	fclose(fp);
	return FALSE;
    }
    fileString[readLen] = 0;
 
    /* Close the file */
    if (fclose(fp) != 0) {
    	/* unlikely error */
	DialogF(DF_WARN, window->shell, 1, "Unable to close file", "Dismiss");
	/* we read it successfully, so continue */
    }
    
    /* Check for nulls in the data read. If the strlen != readLen then were
	   probably nuls in the file.
       Substitute them with another character.  If that is impossible, warn
       the user, make the file read-only, and force a substitution */
    if (strlen(fileString) != readLen) {
    	if (!BufSubstituteNullChars(fileString, readLen, window->editorInfo->buffer)) {
	    resp = DialogF(DF_ERR, window->shell, 2,
"Too much binary data in file.  You may view\n\
it, but not modify or re-save its contents.", "View", "Cancel");
	    if (resp == 2)
		return FALSE;
	    readOnly = TRUE;
	    for (c=fileString; c<&fileString[readLen]; c++)
    		if (*c == '\0')
    		    *c = 0xfe;
	    window->editorInfo->buffer->nullSubsChar = 0xfe;
    }
    }

    /* Display the file contents in the text widget */
    window->editorInfo->ignoreModify = True;
    if(start == 0 && end == statbuf.st_size) {
    	BufSetAll(window->editorInfo->buffer, fileString);
    } else {
    	BufReplace(window->editorInfo->buffer, start, end, fileString);
    }
    window->editorInfo->ignoreModify = False;
    
    /* Release the memory that holds fileString */
    XtFree(fileString);

    /* Set window title and file changed flag */
    window->editorInfo->lockWrite = flags & FORCE_READ_ONLY;
    if (readOnly) {
	window->editorInfo->readOnly = TRUE;
	window->editorInfo->fileChanged = FALSE;
	UpdateWindowTitle(window);
    } else {
	window->editorInfo->readOnly = FALSE;
	SetWindowModified(window, FALSE);
	if (window->editorInfo->lockWrite)
	    UpdateWindowTitle(window);
    }
    UpdateWindowReadOnly(window);
    
    return TRUE;
}   

int IncludeFile(WindowInfo *window, char *name)
{
    struct stat statbuf;
    int fileLen, readLen;
    char *fileString;
    FILE *fp = NULL;

    /* Open the file */
#ifdef WIN32_BINARY_FILE_MODE
    fp = fopen(name, "rb");
#else
    fp = fopen(name, "r");
#endif
    if (fp == NULL) {
	DialogF(DF_ERR, window->shell, 1, "Could not open %s:\n%s",
	    	"Dismiss", name, errorString());
	return FALSE;
    }
    
    /* Get the length of the file */
    if (fstat(fileno(fp), &statbuf) != 0) {
	DialogF(DF_ERR, window->shell, 1, "Error openinig %s", "Dismiss", name);
	return FALSE;
    }
    fileLen = statbuf.st_size;
 
    /* allocate space for the whole contents of the file */
    fileString = (char *)XtMalloc(fileLen+1);  /* +1 = space for null */
    if (fileString == NULL) {
	DialogF(DF_ERR, window->shell, 1, "File is too large to include",
	    	"Dismiss");
	return FALSE;
    }

    /* read the file into fileString and terminate with a null */
    readLen = fread(fileString, sizeof(char), fileLen, fp);
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "Error reading %s:\n%s", "Dismiss",
    		name, errorString());
	fclose(fp);
	XtFree(fileString);
	return FALSE;
    }
    fileString[readLen] = 0;
    
    /* If the file contained ascii nulls, re-map them */
    if (!BufSubstituteNullChars(fileString, readLen, window->editorInfo->buffer))
	DialogF(DF_ERR, window->shell, 1, "Too much binary data in file",
	    	"Dismiss");
 
    /* close the file */
    if (fclose(fp) != 0) {
    	/* unlikely error */
	DialogF(DF_WARN, window->shell, 1, "Unable to close file", "Dismiss");
	/* we read it successfully, so continue */
    }
    
    /* insert the contents of the file in the selection or at the insert
       position in the window if no selection exists */
    if (window->editorInfo->buffer->primary.selected)
    	BufReplaceSelected(window->editorInfo->buffer, fileString, False);
    else
    	BufInsert(window->editorInfo->buffer, TextGetCursorPos(window->lastFocus),
    		fileString);

    /* release the memory that holds fileString */
    XtFree(fileString);

    return TRUE;
}

/*
** Close all files and windows, leaving one untitled window
*/
int CloseAllFilesAndWindows(void)
{
    while (WindowList->next != NULL || 
    		WindowList->editorInfo->filenameSet || WindowList->editorInfo->fileChanged) {
    	if (!CloseFileAndWindow(WindowList))
    	    return FALSE;
    }
    return TRUE;
}

int CloseFileAndWindow(WindowInfo *window)
{ 
    int response, stat;
    
	/* Don't close the window if a shell command is in progress for
	** this window. */
	if(IsShellCommandInProgress(window)) {
    	XBell(TheDisplay, 0);
		return FALSE;
	}

    /* if window is modified, ask about saving, otherwise just close */
    if (!window->editorInfo->fileChanged ||
    	window->editorInfo->master->nextSlave != 0) {
       NEditCloseWindow(window);
       /* up-to-date windows don't have outstanding backup files to close */
    } else {
      /* Make sure that the window is not in iconified state */
      RaiseShellWindow(window->shell);

	response = DialogF(DF_WARN, window->shell, 3,
		"Save %s before closing?", "Yes", "No", "Cancel",
		window->editorInfo->filename);
	if (response == 1) {
	    /* Save */
	    stat = SaveWindow(window);
	    if (stat)
	    	NEditCloseWindow(window);
	} else if (response == 2) {
	    /* Don't Save */
	    RemoveBackupFile(window);
	    NEditCloseWindow(window);
	} else /* 3 == Cancel */
	    return FALSE;
    }
    return TRUE;
}

int SaveWindow(WindowInfo *window)
{
    int stat;
    
    if (!window->editorInfo->fileChanged || window->editorInfo->lockWrite)
    	return TRUE;
    if (!window->editorInfo->filenameSet)
    	return SaveWindowAs(window, NULL, False);
#ifdef VMS
    RemoveBackupFile(window);
    stat = doSave(window);
#else
    if (writeBckVersion(window))
    	return FALSE;
    stat = doSave(window);
    RemoveBackupFile(window);
#endif /*VMS*/
    return stat;
}
    
int SaveWindowAs(WindowInfo *window, char *newName, int addWrap)
{
    int response, retVal;
    char fullname[MAXPATHLEN], filename[MAXPATHLEN], pathname[MAXPATHLEN];
    WindowInfo *otherWindow;
    
/* .b: Gives complete name to PromptForNewFile ("Save As" mode) */
	strcpy(fullname, window->editorInfo->path);
	if(window->editorInfo->filenameSet)
		strcat(fullname, window->editorInfo->filename);
/* .e */

    /* Get the new name for the file */
    if (newName == NULL) {
	response = PromptForNewFile(window, "Save File As:", fullname,&addWrap);
	if (response != GFN_OK)
    	    return FALSE;
    } else
    	strcpy(fullname, newName);
    
    /* Format conversion: Add newlines if requested */
    if (addWrap)
    	addWrapNewlines(window);
    
    /* If the requested file is this file, just save it and return */
    ParseFilename(fullname, filename, pathname);
    if (!FILECOMPARE(window->editorInfo->filename, filename) &&
    	    !FILECOMPARE(window->editorInfo->path, pathname)) {
	if (writeBckVersion(window))
    	    return FALSE;
	return doSave(window);
    }
    
    /* If the file is open in another window, make user close it.  Note that
       it is possible for user to close the window by hand while the dialog
       is still up, because the dialog is not application modal, so after
       doing the dialog, check again whether the window still exists. */
    otherWindow = FindWindowWithFile(filename, pathname);
    if (otherWindow != NULL) {
	response = DialogF(DF_WARN, window->shell, 2,
		"%s is open in another NEdit window", "Cancel",
		"Close Other Window", filename);
	if (response == 1)
	    return FALSE;
	if (otherWindow == FindWindowWithFile(filename, pathname))
	    if (!CloseFileAndWindow(otherWindow))
	    	return FALSE;
    }
    
    /* Destroy the file open property for the original file */
   	DestroyFileOpenProperty(window);
    
    /* Change the name of the file and save it under the new name */
    RemoveBackupFile(window);
    strcpy(window->editorInfo->filename, filename);
    strcpy(window->editorInfo->path, pathname);
    window->editorInfo->filenameSet = TRUE;
    window->editorInfo->readOnly = FALSE;
    window->editorInfo->st__mtime = 0;
    window->editorInfo->st_mode = 0;
    window->editorInfo->st_size = 0;
    window->editorInfo->lockWrite = FALSE;
    retVal = doSave(window);
    UpdateWindowReadOnly(window);
    UpdateWindowTitle(window);
    
    /* Add the name to the convenience menu of previously opened files */
    AddToPrevOpenMenu(fullname);
    CreateFileOpenProperty(window);
    
    /* If name has changed, language mode may have changed as well */
    DetermineLanguageMode(window, False);

    return retVal;
}

static void fileCheckTimerProc(WindowInfo *window, XtIntervalId *id)
{
    window->editorInfo->tailMinusFTimeoutID = 0;
    CheckForChangesInFile(window);
}

void SetCheckingMode(WindowInfo *window, CheckingMode checkingMode)
{
    WindowInfo *win;
    window->editorInfo->checkingMode = checkingMode;
    
    /* Update Preferences menu */
    for(win = window->editorInfo->master; win; win = win->nextSlave) {
    	XmToggleButtonSetState(win->checkingModePromptToReloadItem, 
    		checkingMode == CHECKING_MODE_PROMPT_TO_RELOAD, FALSE);
    	XmToggleButtonSetState(win->checkingModeDisabledItem, 
    		checkingMode == CHECKING_MODE_DISABLED, FALSE);
    	XmToggleButtonSetState(win->checkingModeTailMinusFItem, 
    		checkingMode == CHECKING_MODE_TAIL_MINUS_F, FALSE);
    }

	/* If tail -f mode then lets go and see if anything has already been 
	** added to the file */
	if(checkingMode == CHECKING_MODE_TAIL_MINUS_F) {
		CheckForChangesInFile(window);
	}
	/* If not tail -f mode then make sure a previous tail -f timer is
	** removed. */
	else {
		if(window->editorInfo->tailMinusFTimeoutID) {
			XtRemoveTimeOut(window->editorInfo->tailMinusFTimeoutID);
			window->editorInfo->tailMinusFTimeoutID = 0;
		}
	}
}

/*
** Check if the file in the window was changed by an external source.
*/
void CheckForChangesInFile(WindowInfo *window)
{
    char fullname[MAXPATHLEN];
	int fd;
    struct stat statbuf;

	/* Don't do anything if file checking is disabled. */
	if(window->editorInfo->checkingMode == CHECKING_MODE_DISABLED)
		return;
		
	
	/* No file to check, so return. */	
	if(!window->editorInfo->filenameSet)
		return;
		
	/* Don't check for changes if a shell command is running and
	** the file will be reloaded after the shell command completes. */
	if(IsShellCommandInProgress(window) && IsReloadAfterShellCommand(window))
		return;

    /* Get the full name of the file */
    strcpy(fullname, window->editorInfo->path);
    strcat(fullname, window->editorInfo->filename);

    /* 
	** To work around a ClearCase bug where stat returns stale information
	** we first open the file which forces an update of the stat information.
	*/
	if((fd = open(fullname, O_RDONLY)) < 0) {
		return;
	} else {
		close(fd);
	}
	
	/* get the current mtime and st_mode */
    if (stat(fullname, &statbuf) != 0)
    	return;
    
	/* See if the mode has changed. */
    if(window->editorInfo->st_mode != statbuf.st_mode) {
#ifndef USE_ACCESS
		FILE *fp;
#endif
	    /* if the mode has changed, update the read only status */
    	/* if it doesn't exist we should still be able to create it by
    	   doing a save, so it shouldn't be considered read-only. */
#ifdef USE_ACCESS
    	if(access(fullname, W_OK) == 0) {
#else
		/* This needs to be tested with clearcase to verify that it doesn't
		   update the mtime value. */
#ifdef WIN32_BINARY_FILE_MODE
		if((fp = fopen(fullname, "rb+")) != NULL) {
#else
		if((fp = fopen(fullname, "r+")) != NULL) {
#endif
			fclose(fp);
#endif
			window->editorInfo->readOnly = FALSE;
    	}
    	else if(errno == ENOENT) {
    		window->editorInfo->readOnly = FALSE;
		}
    	else {
	   		window->editorInfo->readOnly = TRUE;
    	}
    	UpdateWindowReadOnly(window);
	   	window->editorInfo->st_mode = statbuf.st_mode;
    }
    
    /* see if the file was modified by an external source */
    if(window->editorInfo->st__mtime != statbuf.st_mtime
		|| window->editorInfo->st_size != statbuf.st_size
	) {
	   	
   		/* if it was then prompt the user if he wants to revert to the 
   		   saved version */
    	if(window->editorInfo->checkingMode == CHECKING_MODE_PROMPT_TO_RELOAD) {
    		int b;
    		static dialog_popped_up = 0;
    		
    		/* ignore multiple calls to ourself from inside the application loop
    		   in DialogF() */
    		if(!dialog_popped_up) {
    			dialog_popped_up = 1;
    			/* raise window to the top before displaying the dialog. */
				RaiseShellWindow(window->shell);
	    		b = DialogF(DF_QUES, window->shell, 3, "%s%s\nwas modified by an external source.\nDo you want to re-read it?",
    		    		"Re-read", "Don't Re-read", "Switch Modes...",
    		    		window->editorInfo->path, window->editorInfo->filename);
    			/* Re-read File */
    			if(b == 1) {
    		    	/* RevertToSaved() will update the mtime. */
	    			RevertToSaved(window, !window->editorInfo->fileChanged);
	    		}
	    		if(b == 2) {
    		    	/* Update the mtime so we don't prompt the user again
    		    	** since he didn't want to reread it at this time. */
	   				window->editorInfo->st__mtime = statbuf.st_mtime;
	   				window->editorInfo->st_size = statbuf.st_size;
	    		}
	    		/* Switch Checking Mode */
    			else if(b == 3) {
	    			b = DialogF(DF_QUES, window->shell, 3, "New Checking Mode?",
    		    			"Prompt To Reload", "Log File Monitor", "Disabled");
    		    	if(b == 1) {
    		    		/* Don't update the mtime. The user still wants 
    		    		** to be prompted. No need to call SetCheckingMode()
    		    		** we are already in prompt mode. We should just 
    		    		** return. */
    		    	} else if(b == 2) {
    		    		/* Don't update the mtime. It will be updated in
    		    		** SetCheckingMode(). */
    		    		SetCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
    		    	} else if(b == 3) {
    		    		/* Don't update the mtime. It is not needed since 
    		    		** checking is disabled. */
    		    		SetCheckingMode(window, CHECKING_MODE_DISABLED);
    		    	}
    				/* Return since the user wanted to change the mode. */
    				dialog_popped_up = 0;
					return;
	    		}
    			dialog_popped_up = 0;
    		}
    	}
    	else if(window->editorInfo->checkingMode == CHECKING_MODE_TAIL_MINUS_F) {
			int file_length = window->editorInfo->buffer->length;

#if defined(_WIN32) && !defined(WIN32_BINARY_FILE_MODE)
			/* Adjust the buffer length for the fact that the carriage returns */
			/* have been removed. So add the number of lines in the file. */
			/* TODO - This needs to be done conditionally if we detect that the
			 * file being read contains carriage returns. If a file on Windows 
			 * doesn't contain carriage returns then this calculation is incorrect
			 * and we will read from the wrong location in the file.
			 */
			file_length += BufCountLines(window->editorInfo->buffer, 0, window->editorInfo->buffer->length);
#endif
    		/* reload the file if it has gotten smaller */
    		if(statbuf.st_size < file_length) {
	    		RevertToSaved(window, !window->editorInfo->fileChanged);
	    		
	    		/* move the cursor to the end of the file, which will also scroll
	    		** the window to the end of the file.
	    		*/
	    		TextSetCursorPos(window->textArea, window->editorInfo->buffer->length);
    		} 
    		/* Else only read what has been added at the end */
    		else {
    			int eofWasVisible, lineNum, column;
    			
    			/* See if the end of file is visible. If it is not then we can
    			** assume that the user is looking at something in the file
    			** and we shouldn't scroll the window out from under him when
    			** we add the text to the window that was added to the file.
    			*/
    			eofWasVisible = TextPosToLineAndCol(window->textArea, 
    				window->editorInfo->buffer->length, &lineNum, &column);
	    		
	    		doOpen(window, window->editorInfo->filename, window->editorInfo->path,
	    			FORCE_READ_ONLY, file_length, -1);
	    		
	    		/* If the end of file was visible before adding to the end then
	    		** move the cursor to the end of the file, which will also scroll
	    		** the window to the end of the file.
	    		*/
	    		if(eofWasVisible) {
	    			TextSetCursorPos(window->textArea, window->editorInfo->buffer->length);
	    		}
	    	}
	    	
	    	/* Make all of the text areas read-only */
	    	if(window->editorInfo->lockWrite == FALSE) {
	    		window->editorInfo->lockWrite = TRUE;
    			UpdateWindowReadOnly(window);
	    		UpdateWindowTitle(window);
	    	}
    	}
    }

    /* If tail -f mode then add a timer to continually check if the
    ** file has changed */
    if(window->editorInfo->checkingMode == CHECKING_MODE_TAIL_MINUS_F) {
    	/* Only register another timer if one isn't already registered */
    	if(window->editorInfo->tailMinusFTimeoutID == 0) {
    		window->editorInfo->tailMinusFTimeoutID = XtAppAddTimeOut(
				XtWidgetToApplicationContext(window->shell),
    			GetPrefTailMinusFInterval()/*milliseconds*/, 
    			(XtTimerCallbackProc)fileCheckTimerProc, window);
    	}
    } 
	/* If not tail -f mode then make sure a previous tail -f timer is
	** removed. */
	else {
		if(window->editorInfo->tailMinusFTimeoutID) {
			XtRemoveTimeOut(window->editorInfo->tailMinusFTimeoutID);
			window->editorInfo->tailMinusFTimeoutID = 0;
		}
	}
}

static int doSave(WindowInfo *window)
{
    char *fileString = NULL;
    char fullname[MAXPATHLEN];
    FILE *fp;
    int fileLen;
    struct stat statbuf;
    int mode = -1;
    
    /* Get the full name of the file */
    strcpy(fullname, window->editorInfo->path);
    strcat(fullname, window->editorInfo->filename);

#ifdef VMS
    /* strip the version number from the file so VMS will begin a new one */
    removeVersionNumber(fullname);
#endif

    /* get the current mode bits */
    if (stat(fullname, &statbuf) == 0) {
    	mode = statbuf.st_mode;
    }
    
    /* open the file */
#ifdef VMS
    if ((fp = fopen(fullname, "w", "rfm = stmlf")) == NULL) {
#else
# ifdef WIN32_BINARY_FILE_MODE
    if ((fp = fopen(fullname, "wb")) == NULL) {
# else
    if ((fp = fopen(fullname, "w")) == NULL) {
# endif
#endif /* VMS */
    	DialogF(DF_WARN, window->shell, 1, "Unable to save %s:\n%s", "Dismiss",
		window->editorInfo->filename, errorString());
		window->editorInfo->readOnly = TRUE;
        return FALSE;
    }

#ifdef VMS
    /* get the complete name of the file including the new version number */
    fgetname(fp, fullname);
        
#else /* Unix */
    
#endif /*VMS/Unix*/

    /* get the text buffer contents and its length */
    fileString = BufGetAll(window->editorInfo->buffer);
    fileLen = window->editorInfo->buffer->length;
    
    /* If null characters are substituted for, put them back */
    BufUnsubstituteNullChars(fileString, window->editorInfo->buffer);

    /* add a terminating newline if the file doesn't already have one */
    if (fileLen == 0 || fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* write to the file */
#ifdef IBM_FWRITE_BUG
    write(fileno(fp), fileString, fileLen);
#else
    fwrite(fileString, sizeof(char), fileLen, fp);
#endif
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1, "%s not saved:\n%s", "Dismiss", 
		window->editorInfo->filename, errorString());
	fclose(fp);
	remove(fullname);
        XtFree(fileString);
	return FALSE;
    }
    
    /* close the file */
    if (fclose(fp) != 0) {
    	DialogF(DF_ERR, window->shell,1,"Error closing file:\n%s", "Dismiss",
		errorString());
        XtFree(fileString);
	return FALSE;
    }

    /* XtFree the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
    
#ifdef VMS
    /* reflect the fact that NEdit is now editing a new version of the file */
    ParseFilename(fullname, window->editorInfo->filename, window->editorInfo->path);
#endif /*VMS*/

    /* success, file was written */
    SetWindowModified(window, FALSE);
    
	/* The file must have been writable. */
	window->editorInfo->readOnly = FALSE;
	UpdateWindowReadOnly(window);
	
    /* set the mode bits back to what they were before we opened the file.
    ** For some reason the set-uid bit is being turned off. */
    if(mode != -1) {
    	chmod(fullname, mode);
    }
    
    /* get the current st_mtime and st_mode */
    if (stat(fullname, &statbuf) == 0) {
    	window->editorInfo->st__mtime = statbuf.st_mtime;
    	window->editorInfo->st_mode = statbuf.st_mode;
    	window->editorInfo->st_size = statbuf.st_size;
    }

    return TRUE;
}

/*
** Create a backup file for the current window.  The name for the backup file
** is generated using the name and path stored in the window and adding a
** tilde (~) on UNIX to the end of the name or an underscore (_) on VMS to
** the beginning of the name.  
*/
int WriteBackupFile(WindowInfo *window)
{
    char *fileString = NULL;
    char name[MAXPATHLEN];
    FILE *fp;
    int fileLen;
    WindowInfo *w;
    
    /* Generate a name for the autoSave file */
    backupFileName(window, name);

#ifdef VMS
    /* remove the old backup file because we reuse the same version number */
    remove(name);
#endif /*VMS*/
    
    /* open the file */
#ifdef VMS
    if ((fp = fopen(name, "w", "rfm = stmlf")) == NULL) {
#else
# ifdef WIN32_BINARY_FILE_MODE
    if ((fp = fopen(name, "wb")) == NULL) {
# else
    if ((fp = fopen(name, "w")) == NULL) {
# endif
#endif /* VMS */
    	DialogF(DF_WARN, window->shell, 1,
    	       "Unable to save backup for %s:\n%s\nAutomatic backup is now off",
    	       "Dismiss", window->editorInfo->filename, errorString());
        window->editorInfo->autoSave = FALSE;
        for(w = window->editorInfo->master; w; w = w->nextSlave) {
        	XmToggleButtonSetState(w->autoSaveItem, FALSE, FALSE);
        }
        return FALSE;
    }

    /* get the text buffer contents and its length */
    fileString = BufGetAll(window->editorInfo->buffer);
    fileLen = window->editorInfo->buffer->length;
    
    /* If null characters are substituted for, put them back */
    BufUnsubstituteNullChars(fileString, window->editorInfo->buffer);
    
    /* add a terminating newline if the file doesn't already have one */
    if (fileLen == 0 || fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* write out the file */
#ifdef IBM_FWRITE_BUG
    write(fileno(fp), fileString, fileLen);
#else
    fwrite(fileString, sizeof(char), fileLen, fp);
#endif
    if (ferror(fp)) {
    	DialogF(DF_ERR, window->shell, 1,
    	   "Error while saving backup for %s:\n%s\nAutomatic backup is now off",
    	   "Dismiss", window->editorInfo->filename, errorString());
	fclose(fp);
	remove(name);
        XtFree(fileString);
        window->editorInfo->autoSave = FALSE;
	return FALSE;
    }
    
    /* close the backup file */
    if (fclose(fp) != 0) {
	XtFree(fileString);
	return FALSE;
    }

    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);

    return TRUE;
}

/*
** Remove the backup file associated with this window
*/
void RemoveBackupFile(WindowInfo *window)
{
    char name[MAXPATHLEN];
    
    backupFileName(window, name);
    remove(name);
}

/*
** Generate the name of the backup file for this window from the filename
** and path in the window data structure & write into name
*/
static void backupFileName(WindowInfo *window, char *name)
{
#ifdef VMS
    if (window->editorInfo->filenameSet)
    	sprintf(name, "%s_%s", window->editorInfo->path, window->editorInfo->filename);
    else
    	sprintf(name, "%s_%s", "SYS$LOGIN:", window->editorInfo->filename);
#else
    if (window->editorInfo->filenameSet)
    	sprintf(name, "%s%s~", window->editorInfo->path, window->editorInfo->filename);
    else
    	sprintf(name, "%s/%s~", getenv("HOME"), window->editorInfo->filename);
#endif /*VMS*/
}

/*
** If saveOldVersion is on, copies the existing version of the file to
** <filename><backupSuffix> in anticipation of a new version being saved.
** <backupSuffix> comes from the backupSuffix preference resource.
** Returns True if backup fails and user requests that the new file not be
** written.
*/
static int writeBckVersion(WindowInfo *window)
{
#ifndef VMS
    char fullname[MAXPATHLEN+1], bckname[MAXPATHLEN+1];
    struct stat statbuf;
    FILE *inFP, *outFP;
    int fileLen;
    char *fileString;
#ifdef HAVE_UTIME_H
	struct utimbuf file_time[1];
#endif
	char *backupSuffix = GetPrefBackupSuffix();

    /* Do only if version backups are turned on */
    if (!window->editorInfo->saveOldVersion)
    	return False;
    
    /* Get the full name of the file */
    strcpy(fullname, window->editorInfo->path);
    strcat(fullname, window->editorInfo->filename);
    
    /* Generate name for old version */
    if (strlen(fullname) + strlen(backupSuffix) > MAXPATHLEN)
    	return bckError(window, "file name too long", window->editorInfo->filename);
    sprintf(bckname, "%s%s", fullname, backupSuffix);

    /* open the file being edited.  If there are problems with the
       old file, don't bother the user, just skip the backup */
	/* Always use binary mode to make the copy. */
    if ((inFP = fopen(fullname, "rb")) == NULL)
    	return FALSE;
    
    /* find the length of the file */
    if (fstat(fileno(inFP), &statbuf) != 0)
	return FALSE;
    fileLen = statbuf.st_size;
    
    /* Always make the backup file writable */
    chmod(bckname, statbuf.st_mode | 0222);
    
    /* open the file to receive a copy of the old version */
	/* Always use binary mode to make the copy. */
    if ((outFP = fopen(bckname, "wb")) == NULL) {
    	fclose(inFP);
    	return bckError(window, errorString(), bckname);
    }
    
    /* Allocate space for the whole contents of the file */
    fileString = (char *)XtMalloc(fileLen);
    if (fileString == NULL) {
    	fclose(inFP);
    	fclose(outFP);
	return bckError(window, "out of memory", bckname);
    }
    
    /* read the file into fileString */
    fread(fileString, sizeof(char), fileLen, inFP);
    if (ferror(inFP)) {
    	fclose(inFP);
    	fclose(outFP);
    	XtFree(fileString);
    	return FALSE;
    }
 
    /* close the input file, ignore any errors */
    fclose(inFP);
    
     /* write to the file */
#ifdef IBM_FWRITE_BUG
    write(fileno(outFP), fileString, fileLen);
#else
    fwrite(fileString, sizeof(char), fileLen, outFP);
#endif
    if (ferror(outFP)) {
	fclose(outFP);
	remove(bckname);
        XtFree(fileString);
	return bckError(window, errorString(), bckname);
    }
    XtFree(fileString);
    
    /* close the file */
    if (fclose(outFP) != 0)
	return bckError(window, errorString(), bckname);

    /* set the protection and times for the backup file */
	chmod(bckname, statbuf.st_mode);
#ifdef HAVE_UTIME_H
	file_time->actime = statbuf.st_atime;
	file_time->modtime = statbuf.st_mtime;
	utime(bckname, file_time);
#endif
#endif /* VMS */
	
    return FALSE;
}

/*
** Error processing for writeBckVersion, gives the user option to cancel
** the subsequent save, or continue and optionally turn off versioning
*/
static int bckError(WindowInfo *window, char *errString, char *file)
{
    int resp;

    resp = DialogF(DF_ERR, window->shell, 3,
    	    "Couldn't write backup file of last version.\n%s: %s",
    	    "Cancel Save", "Turn off Backups", "Continue", file, errString);
    if (resp == 1)
    	return TRUE;
    if (resp == 2) {
    	WindowInfo *w;
    	window->editorInfo->saveOldVersion = FALSE;
        for(w = window->editorInfo->master; w; w = w->nextSlave) {
    		XmToggleButtonSetState(w->saveLastItem, FALSE, FALSE);
    	}
    }
    return FALSE;
}

void NEditPrintWindow(WindowInfo *window, int selectedOnly)
{
    textBuffer *buf = window->editorInfo->buffer;
    selection *sel = &buf->primary;
    char *fileString = NULL;
    int fileLen;

    /* get the contents of the text buffer from the text area widget.  Add
       wrapping newlines if necessary to make it match the displayed text */
    if (selectedOnly) {
    	if (!sel->selected) {
    	    XBell(TheDisplay, 0);
	    return;
	}
	if (sel->rectangular) {
    	    fileString = BufGetSelectionText(buf);
    	    fileLen = strlen(fileString);
    	} else
    	    fileString = TextGetWrapped(window->textArea, sel->start, sel->end,
    	    	    &fileLen);
    } else
    	fileString = TextGetWrapped(window->textArea, 0, buf->length, &fileLen);
    
    /* If null characters are substituted for, put them back */
    BufUnsubstituteNullChars(fileString, buf);

        /* add a terminating newline if the file doesn't already have one */
    if (fileLen != 0 && fileString[fileLen-1] != '\n')
    	fileString[fileLen++] = '\n'; 	 /* null terminator no longer needed */
    
    /* Print the string */
    PrintString(fileString, fileLen, window->shell, window->editorInfo->filename);

    /* Free the text buffer copy returned from XmTextGetString */
    XtFree(fileString);
}

/*
** Print a string (length is required).  parent is the dialog parent, for
** error dialogs, and jobName is the print title.
*/
void PrintString(char *string, int length, Widget parent, char *jobName)
{
    char tmpFileName[L_tmpnam];    /* L_tmpnam defined in stdio.h */
    FILE *fp;

    /* Generate a temporary file name */
    tmpnam(tmpFileName);

    /* open the temporary file */
#ifdef VMS
    if ((fp = fopen(tmpFileName, "w", "rfm = stmlf")) == NULL) {
#else
# ifdef WIN32_BINARY_FILE_MODE
    if ((fp = fopen(tmpFileName, "wb")) == NULL) {
# else
    if ((fp = fopen(tmpFileName, "w")) == NULL) {
# endif
#endif /* VMS */
    	DialogF(DF_WARN, parent, 1, "Unable to write file for printing:\n%s",
		"Dismiss", errorString());
        return;
    }
    
    /* write to the file */
#ifdef IBM_FWRITE_BUG
    write(fileno(fp), string, length);
#else
    fwrite(string, sizeof(char), length, fp);
#endif
    if (ferror(fp)) {
    	DialogF(DF_ERR, parent, 1, "%s not printed:\n%s", "Dismiss", 
		jobName, errorString());
	fclose(fp);
    	remove(tmpFileName);
	return;
    }
    
    /* close the temporary file */
    if (fclose(fp) != 0) {
    	DialogF(DF_ERR, parent, 1, "Error closing temp. print file:\n%s",
		"Dismiss", errorString());
    	remove(tmpFileName);
	return;
    }

    /* Print the temporary file, then delete it and return success */
#ifdef VMS
    strcat(tmpFileName, ".");
    PrintFile(parent, tmpFileName, jobName, True);
#else
    PrintFile(parent, tmpFileName, jobName);
    remove(tmpFileName);
#endif /*VMS*/
    return;
}

/*
** Wrapper for GetExistingFilename which uses the current window's path
** (if set) as the default directory.
*/
int PromptForExistingFile(WindowInfo *window, char *prompt, char *fullname, char *title)
{
    char *savedDefaultDir;
    int retVal;
    
    /* Temporarily set default directory to window->editorInfo->path, prompt for file,
       then, if the call was unsuccessful, restore the original default
       directory */
    savedDefaultDir = GetFileDialogDefaultDirectory();
    if (*window->editorInfo->path != '\0')
    	SetFileDialogDefaultDirectory(window->editorInfo->path);
    retVal = GetExistingFilename(window->shell, prompt, fullname, title);
    if (retVal != GFN_OK)
    	SetFileDialogDefaultDirectory(savedDefaultDir);
    if (savedDefaultDir != NULL)
    	XtFree(savedDefaultDir);
    return retVal;
}

/*
** Find a name for an untitled file, unique in the name space of in the opened
** files in this session, i.e. Untitled or Untitled_nn, and write it into
** the string "name".
*/
void UniqueUntitledName(char *name)
{
    WindowInfo *w;
    int i;

   for (i=0; i<INT_MAX; i++) {
    	if (i == 0)
    	    sprintf(name, "Untitled");
    	else
    	    sprintf(name, "Untitled_%d", i);
		for (w=WindowList; w!=NULL; w=w->next)
			if (!FILECOMPARE(w->editorInfo->filename, name))
				break;
    	if (w == NULL)
    	    break;
    }
}

/*
** Wrapper for GetNewFilename which uses the current window's path
** (if set) as the default directory, and asks about embedding newlines
** to make wrapping permanent.
*/
int PromptForNewFile(WindowInfo *window, char *prompt, char *fullname,
    	int *addWrap)
{
    int n, retVal;
    Arg args[20];
    XmString s1, s2;
    Widget fileSB, wrapToggle;
    char *savedDefaultDir;
    
    /* Temporarily set default directory to window->path, prompt for file,
       then, if the call was unsuccessful, restore the original default
       directory */
    savedDefaultDir = GetFileDialogDefaultDirectory();
    if (*window->editorInfo->path != '\0')
    	SetFileDialogDefaultDirectory(window->editorInfo->path);
    
    /* Present a file selection dialog with an added field for requesting
       long line wrapping to become permanent via inserted newlines */
    n = 0;
    XtSetArg(args[n], XmNselectionLabelString, 
    	    s1=XmStringCreateSimple(prompt)); n++;     
    XtSetArg(args[n], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL); n++;
    XtSetArg(args[n], XmNdialogTitle, s2=XmStringCreateSimple(" ")); n++;
    XtSetArg(args[n], XmNresizePolicy, XmRESIZE_GROW); n++;
    fileSB = XmCreateFileSelectionDialog(window->shell,"FileSelect",args,n);
    XmStringFree(s1);
    XmStringFree(s2);
    if (window->editorInfo->wrapMode == CONTINUOUS_WRAP) {
	wrapToggle = XtVaCreateManagedWidget("addWrap",
	    	xmToggleButtonWidgetClass, fileSB, XmNlabelString,
	    	s1=XmStringCreateSimple("Add line breaks where wrapped"),
    		XmNmarginHeight, 0, XmNalignment, XmALIGNMENT_BEGINNING, NULL);
	XtAddCallback(wrapToggle, XmNvalueChangedCallback, addWrapCB,
    	    	addWrap);
	XmStringFree(s1);
    }
    *addWrap = False;
    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB,
    	    XmDIALOG_FILTER_LABEL), XmNmnemonic, 'l', XmNuserData,
    	    XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT), NULL);
    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB,
    	    XmDIALOG_DIR_LIST_LABEL), XmNmnemonic, 'D', XmNuserData,
    	    XmFileSelectionBoxGetChild(fileSB, XmDIALOG_DIR_LIST), NULL);
    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB,
    	    XmDIALOG_LIST_LABEL), XmNmnemonic, 'F', XmNuserData,
    	    XmFileSelectionBoxGetChild(fileSB, XmDIALOG_LIST), NULL);
    XtVaSetValues(XmFileSelectionBoxGetChild(fileSB,
    	    XmDIALOG_SELECTION_LABEL), XmNmnemonic,
    	    prompt[strspn(prompt, "lFD")], XmNuserData,
    	    XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT), NULL);
    AddDialogMnemonicHandler(fileSB);
    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_FILTER_TEXT));
    RemapDeleteKey(XmFileSelectionBoxGetChild(fileSB, XmDIALOG_TEXT));
    retVal = HandleCustomNewFileSB(fileSB, fullname);

    if (retVal != GFN_OK)
    	SetFileDialogDefaultDirectory(savedDefaultDir);
    if (savedDefaultDir != NULL)
    	XtFree(savedDefaultDir);
    return retVal;
}

/*
** Check the read-only or locked status of the window and beep and return
** false if the window should not be written in.
*/
int CheckReadOnly(WindowInfo *window)
{
    if ((!GetPrefAllowReadOnlyEdits() && window->editorInfo->readOnly) || window->editorInfo->lockWrite) {
    	XBell(TheDisplay, 0);
	return True;
    }
    return False;
}

/*
** Wrapper for strerror so all the calls don't have to be ifdef'd for VMS.
*/
static char *errorString(void)
{
#ifdef VMS
    return strerror(errno, vaxc$errno);
#else
    return strerror(errno);
#endif
}

#ifdef VMS
/*
** Removing the VMS version number from a file name (if has one).
*/
void removeVersionNumber(char *fileName)
{
    char *versionStart;
    
    versionStart = strrchr(fileName, ';');
    if (versionStart != NULL)
    	*versionStart = '\0';
}
#endif /*VMS*/

/*
** Callback procedure for toggle button requesting newlines to be inserted
** to emulate continuous wrapping.
*/
static void addWrapCB(Widget w, XtPointer clientData, XtPointer callData)
{
    int resp;
    int *addWrap = (int *)clientData;
    
    if (XmToggleButtonGetState(w)) {
    	resp = DialogF(DF_WARN, w, 2,
"This operation adds permanent line breaks to\n\
match the automatic wrapping done by the\n\
Continuous Wrap mode Preferences Option.\n\
\n\
      *** This Option is Irreversable ***\n\
\n\
Once newlines are inserted, continuous wrapping\n\
will no longer work automatically on these lines", "OK", "Cancel");
    	if (resp == 2) {
    	    XmToggleButtonSetState(w, False, False);
    	    *addWrap = False;
    	} else
    	    *addWrap = True;
    } else
    	*addWrap = False;
}

/*
** Change a window created in NEdit's continuous wrap mode to the more
** conventional Unix format of embedded newlines.  Indicate to the user
** by turning off Continuous Wrap mode.
*/
static void addWrapNewlines(WindowInfo *window)
{
    int fileLen, i, insertPositions[MAX_PANES], topLines[MAX_PANES];
    int horizOffset;
    Widget text;
    char *fileString;
	
    /* save the insert and scroll positions of each pane */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
    	insertPositions[i] = TextGetCursorPos(text);
    	TextGetScroll(text, &topLines[i], &horizOffset);
    }

    /* Modify the buffer to add wrapping */
    fileString = TextGetWrapped(window->textArea, 0,
    	    window->editorInfo->buffer->length, &fileLen);
    BufSetAll(window->editorInfo->buffer, fileString);
    XtFree(fileString);

    /* restore the insert and scroll positions of each pane */
    for (i=0; i<=window->nPanes; i++) {
    	text = i==0 ? window->textArea : window->textPanes[i-1];
	TextSetCursorPos(text, insertPositions[i]);
	TextSetScroll(text, topLines[i], 0);
    }

    /* Show the user that something has happened by turning off
       Continuous Wrap mode */
    XmToggleButtonSetState(window->continuousWrapItem, False, True);
}

void FindIncludeFile(char *path)
{
    char newpath[MAXPATHLEN];
#ifdef VMS
# ifndef __DECC
    strcpy(newpath, "sys$library:");
	strcat(newpath, path);
	strcpy(path, newpath);
# else
	strcpy(newpath, "decc$library_include:");
	strcat(newpath, path);
	strcpy(path, newpath);
# endif
#else
# ifdef _WIN32
#if 1
	_searchenv(path, "INCLUDE", newpath);
	if(newpath[0]) {
		strcpy(path, newpath);
	}
#else

	char *include;
	struct stat statbuf[1];

	include = getenv("include");
	if(include) {
		char buf[4096];
		char *token;
		static char *seps = ";";

		/* local copy we can modify */
		strcpy(buf, include);
		
		token = strtok(buf, seps);
		while(token != NULL) {
			int len;

			/* Assemble the path to the include file. */
			strcpy(newpath, token);
			len = strlen(newpath);
			if(newpath[len] != '\\' && newpath[len] != '/') {
				strcat(newpath, "\\");
			}
			strcat(newpath, path);

			/* See if the include file exist at the path */
			if(stat(newpath, statbuf) == 0) {
				strcpy(path, newpath);
				break;
			}

			/* Get next token: */
			token = strtok( NULL, seps );
		}
	}
#endif
# else
	strcpy(newpath, "/usr/include/");
	strcat(newpath, path);
	strcpy(path, newpath);
# endif
#endif /* VMS */

}

