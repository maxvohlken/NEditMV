/*******************************************************************************
*									       *
* shell.c -- Nirvana Editor shell command execution			       *
*									       *
* Copyright (c) 1991-1997 Universities Research Association, Inc.	       *
* All rights reserved.							       *
* 									       *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retaFins a paid-up,     *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warranty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                        				       *
* Fermilab Nirvana GUI Library						       *
* December, 1993							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif
#ifdef HAVE_SYS_SELECT_H
# include <sys/select.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#include <ctype.h>
#include <errno.h>
#include <signal.h>
#ifdef _WIN32
# include <io.h>
# include <process.h>
# include <direct.h>
# include <windows.h>
#endif
#include <Xm/Xm.h>
#include <Xm/MessageB.h>
#include <Xm/Text.h>
#include <Xm/Form.h>
#include <Xm/PushBG.h>
#include <Xm/ScrolledW.h>
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "window.h"
#include "preferences.h"
#include "file.h"
#include "shell.h"
#include "macro.h"
#include "interpret.h"
#include "server.h"

/* Tuning parameters */
#define IO_BUF_SIZE 4096	/* size of buffers for collecting cmd output */
#define MAX_SHELL_CMD_LEN 2048	/* max length of a shell command (should be
				   eliminated, but substitutePercent needs) */
#define MAX_OUT_DIALOG_ROWS 30	/* max height of dialog for command output */
#define MAX_OUT_DIALOG_COLS 80	/* max width of dialog for command output */
#define OUTPUT_FLUSH_FREQ 1000	/* how often (msec) to flush output buffers
    	    	    	    	   when process is taking too long */
#define BANNER_WAIT_TIME 6000	/* how long to wait (msec) before putting up
    	    	    	    	   Shell Command Executing... banner */

/* flags for issueCommand */
#define ACCUMULATE 1
#define ERROR_DIALOGS 2
#define REPLACE_SELECTION 4
#define RELOAD_FILE_AFTER 8
#define OUTPUT_TO_DIALOG 16
#define OUTPUT_TO_STRING 32
#define OUTPUT_TO_NEW_WINDOW 64

#define MYCLOSEFD(fd) \
	if((fd) != -1) {\
		close(fd);\
		(fd) = -1;\
	}

#define MYCLOSEHANDLE(handle) \
	if((handle) != INVALID_HANDLE_VALUE) {\
		CloseHandle(handle);\
		(handle) = INVALID_HANDLE_VALUE;\
	}

/* element of a buffer list for collecting output from shell processes */
typedef struct bufElem {
    struct bufElem *next;
    int length;
    char contents[IO_BUF_SIZE];
} buffer;

static buffer *createBuffer(void) {
	buffer *buf;
	buf = (buffer *)XtMalloc(sizeof(buffer));
	return buf;
}

static void destroyBuffer(buffer *buf) {
	XtFree((char*)buf);
}

/* data attached to window during shell command execution with
   information for controling and communicating with the process */
typedef struct {
    int flags;
    buffer *outBufs, *errBufs;
    char *input;
    char *inPtr;
    Widget textW;
    int leftPos, rightPos;
    int inLength;
    XtIntervalId bannerTimeoutID, flushTimeoutID;
    char bannerIsUp;
    char fromMacro;
#ifdef _WIN32
    HANDLE stdinH, stdoutH, stderrH, childHandle;
	DWORD childPid;
	HANDLE stdinThreadHandle;
	HANDLE stdinThreadExitEventHandle;
	HANDLE stdoutThreadHandle;
	HANDLE stdoutThreadExitEventHandle;
	HANDLE stderrThreadHandle;
	HANDLE stderrThreadExitEventHandle;
	BOOLEAN terminatedOnError;
	char *terminationErrorMessage;
	CRITICAL_SECTION lock;
#else
    int stdinFD, stdoutFD, stderrFD;
    pid_t childPid;
	XtInputId stdinInputID;
	XtInputId stdoutInputID;
	XtInputId stderrInputID;
#endif
} shellCmdInfo;

static shellCmdInfo *createShellCmdInfo(void) {
	shellCmdInfo *buf;
	buf = (shellCmdInfo *)XtMalloc(sizeof(shellCmdInfo));
#ifdef _WIN32
	InitializeCriticalSection(&buf->lock);
#endif
	return buf;
}

static void destroyShellCmdInfo(shellCmdInfo *buf) {
#ifdef _WIN32
	DeleteCriticalSection(&buf->lock);
#endif
	XtFree((char*)buf);
}

static void issueCommand(WindowInfo *window, char *command, char *input,
	int inputLen, int flags, Widget textW, int replaceLeft,
	int replaceRight, int fromMacro);
static void finishCmdExecution(WindowInfo *window, int terminatedOnError);
#ifdef _WIN32
static void forkCommand(Widget parent, char *command, char *cmdDir,
	HANDLE *stdinHP, HANDLE *stdoutHP, HANDLE *stderrHP, HANDLE *childHP, DWORD *childPidDWP);
static BOOL WINAPI TerminateChildGracefully(DWORD dwPID, DWORD dwTimeout);
#else
static pid_t forkCommand(Widget parent, char *command, char *cmdDir,
	int *stdinFD, int *stdoutFD, int *stderrFD);
static void stdoutReadProc(XtPointer clientData, int *source, XtInputId *id);
static void stderrReadProc(XtPointer clientData, int *source, XtInputId *id);
static void stdinWriteProc(XtPointer clientData, int *source, XtInputId *id);
#endif
static void addOutput(buffer **bufList, buffer *buf);
static char *coalesceOutput(buffer **bufList, int *length);
static void freeBufList(buffer **bufList);
static void removeTrailingNewlines(char *string);
static Widget createOutputDialog(WindowInfo *window, char *text, 
	int dialogStyle);
static void destroyOutDialogCB(Widget w, XtPointer callback, XtPointer closure);
static void measureText(char *text, int wrapWidth, int *rows, int *cols);
static void truncateString(char *string, unsigned int length);
static int substitutePercent(WindowInfo *window, char *outStr, char *inStr,
        int outLen, char *selection);
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void flushTimeoutProc(XtPointer clientData, XtIntervalId *id);
static void abortShellCommand(shellCmdInfo *cmdData);

/*
** Filter the current selection through shell command "command".  The selection
** is removed, and replaced by the output from the command execution.  Failed
** command status and output to stderr are presented in dialog form.
*/
void FilterSelection(WindowInfo *window, char *command, int fromMacro)
{
    int left, right, textLen;
    char *text;
    char subsCommand[MAX_SHELL_CMD_LEN];

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* Get the selection and the range in character positions that it
       occupies.  Beep and return if no selection */
    text = BufGetSelectionText(window->editorInfo->buffer);
    if (*text == '\0') {
	XtFree(text);
	XBell(TheDisplay, 0);
	return;
    }
    
/* .b: Dialog message is now inside function */
    if (!substitutePercent(window, subsCommand, command, 
		MAX_SHELL_CMD_LEN, text))
        return;
/* .e */
    	
    textLen = strlen(text);
    BufUnsubstituteNullChars(text, window->editorInfo->buffer);
    left = window->editorInfo->buffer->primary.start;
    right = window->editorInfo->buffer->primary.end;
    
    /* Issue the command and collect its output */
    issueCommand(window, subsCommand, text, textLen, ACCUMULATE | ERROR_DIALOGS |
	    REPLACE_SELECTION, window->lastFocus, left, right, fromMacro);
}

/*
** Execute shell command "command", depositing the result at the current
** insert position or in the current selection if if the window has a
** selection.
*/
void ExecShellCommand(WindowInfo *window, char *command, int fromMacro)
{
    int left, right, flags = 0;
    char subsCommand[MAX_SHELL_CMD_LEN];
	
    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }
    
    /* get the selection or the insert position */
    if (GetSimpleSelection(window->editorInfo->buffer, &left, &right))
    	flags = ACCUMULATE | REPLACE_SELECTION;
    else
    	left = right = TextGetCursorPos(window->lastFocus);
    
    if (!substitutePercent(window, subsCommand, command,
            MAX_SHELL_CMD_LEN, NULL))
        return;
    	
    /* issue the command */
    issueCommand(window, subsCommand, NULL, 0, flags, window->lastFocus, left,
	    right, fromMacro);
}

/*
** Execute shell command "command", on input string "input", depositing the
** in a macro string (via a call back to ReturnShellCommandOutput).
*/
void ShellCmdToMacroString(WindowInfo *window, char *command, char *input)
{
    char *inputCopy;
    char subsCommand[MAX_SHELL_CMD_LEN];
    
    /* Make a copy of the input string for issueCommand to hold and free
       upon completion */
    inputCopy = *input == '\0' ? NULL : XtNewString(input);
    
    if (!substitutePercent(window, subsCommand, command,
            MAX_SHELL_CMD_LEN, inputCopy))
        return;

    /* fork the command and begin processing input/output */
    issueCommand(window, subsCommand, inputCopy, strlen(input),
	    ACCUMULATE | OUTPUT_TO_STRING, NULL, 0, 0, True);
}

/*
** Execute the line of text where the the insertion cursor is positioned
** as a shell command.
*/
void ExecCursorLine(WindowInfo *window, int fromMacro)
{
    char *cmdText;
    int left, right, insertPos;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* get all of the text on the line with the insert position */
    if (!GetSimpleSelection(window->editorInfo->buffer, &left, &right)) {
	left = right = TextGetCursorPos(window->lastFocus);
	left = BufStartOfLine(window->editorInfo->buffer, left);
	right = BufEndOfLine(window->editorInfo->buffer, right);
	insertPos = right;
    } else
    	insertPos = BufEndOfLine(window->editorInfo->buffer, right);
    cmdText = BufGetRange(window->editorInfo->buffer, left, right);
    BufUnsubstituteNullChars(cmdText, window->editorInfo->buffer);
    
    /* insert a newline after the entire line */
    BufInsert(window->editorInfo->buffer, insertPos, "\n");

    /* issue the command */
    issueCommand(window, cmdText, NULL, 0, 0, window->lastFocus, insertPos+1,
	    insertPos+1, fromMacro);

    XtFree(cmdText);
}

/*
** Do a shell command, with the options allowed to users (input source,
** output destination, save first and load after) in the shell commands
** menu.
*/
void DoShellMenuCmd(WindowInfo *window, char *command, int input, int output,
	int outputReplacesInput, int saveFirst, int loadAfter, int fromMacro) 
{
    int flags = 0;
    char *text;
    char subsCommand[MAX_SHELL_CMD_LEN];
    int left, right, textLen;
    WindowInfo *inWindow = window;
    Widget outWidget;

    /* Can't do two shell commands at once in the same window */
    if (window->shellCmdData != NULL) {
    	XBell(TheDisplay, 0);
    	return;
    }

    /* Get the command input as a text string.  If there is input, errors
      shouldn't be mixed in with output, so set flags to ERROR_DIALOGS */
    if (input == FROM_SELECTION) {
	text = BufGetSelectionText(window->editorInfo->buffer);
	if (*text == '\0') {
    	    XtFree(text);
    	    XBell(TheDisplay, 0);
    	    return;
    	}
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else if (input == FROM_WINDOW) {
	text = BufGetAll(window->editorInfo->buffer);
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else if (input == FROM_EITHER) {
	text = BufGetSelectionText(window->editorInfo->buffer);
	if (*text == '\0') {
	    XtFree(text);
	    text = BufGetAll(window->editorInfo->buffer);
    	}
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    } else /* FROM_NONE */
    	text = NULL;
    
    /* If the buffer was substituting another character for ascii-nuls,
       put the nuls back in before exporting the text */
    if (text != NULL) {
    	textLen = strlen(text);
    	BufUnsubstituteNullChars(text, window->editorInfo->buffer);
    } else
    	textLen = 0;
    
    /* Assign the output destination.  If output is to a new window,
       create it, and run the command from it instead of the current
       one, to free the current one from waiting for lengthy execution */
    if (output == TO_DIALOG) {
    	outWidget = NULL;
    	flags |= OUTPUT_TO_DIALOG;
    	left = right = 0;
    } else if (output == TO_NEW_WINDOW) {
    	inWindow = EditNewFile(window, True);
    	outWidget = inWindow->textArea;
    	flags |= OUTPUT_TO_NEW_WINDOW;
    	left = right = 0;
    } else { /* TO_SAME_WINDOW */
    	outWidget = window->lastFocus;
    	if (outputReplacesInput && input != FROM_NONE) {
    	    if (input == FROM_WINDOW) {
    		left = 0;
    		right = window->editorInfo->buffer->length;
    	    } else if (input == FROM_SELECTION) {
    	    	GetSimpleSelection(window->editorInfo->buffer, &left, &right);
	        flags |= ACCUMULATE | REPLACE_SELECTION;
    	    } else if (input == FROM_EITHER) {
    	    	if (GetSimpleSelection(window->editorInfo->buffer, &left, &right))
	            flags |= ACCUMULATE | REPLACE_SELECTION;
	        else {
	            left = 0;
	            right = window->editorInfo->buffer->length;
	        }
	    }
    	} else {
	    if (GetSimpleSelection(window->editorInfo->buffer, &left, &right))
	        flags |= ACCUMULATE | REPLACE_SELECTION;
	    else
    		left = right = TextGetCursorPos(window->lastFocus);
    	}
    }
    
    /* Catch errors in the command if the user wants the file loaded
    ** after the command. */
    if (loadAfter) {
    	flags |= ACCUMULATE | ERROR_DIALOGS;
    }
    
    /* If the command requires the file be saved first, save it */
    if (saveFirst) {
    	if (!SaveWindow(window)) {
    	    if (input != FROM_NONE)
    		XtFree(text);
    	    return;
	}
    }
    
/* .b: Dialog message is now inside function */
    if (!substitutePercent(window, subsCommand, command,
            MAX_SHELL_CMD_LEN, text))
        return;
/* .e */
    	
    /* If the command requires the file to be reloaded after execution, set
       a flag for issueCommand to deal with it when execution is complete */
    if (loadAfter)
    	flags |= RELOAD_FILE_AFTER;
    	
    /* issue the command */
    issueCommand(inWindow, subsCommand, text, textLen, flags, outWidget, left,
	    right, fromMacro);
}

static void abortShellCommand(shellCmdInfo *cmdData)
{
#ifdef _WIN32
	EnterCriticalSection(&cmdData->lock);
	if(!TerminateChildGracefully(cmdData->childPid, 15)) {
		if(!TerminateProcess(cmdData->childHandle, 0)) {
			PERRORWIN32("Unable to terminate child process");
		}
	}
	/* signal the threads to finish. */
	if(cmdData->stdinThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stdinThreadExitEventHandle);
	}
	if(cmdData->stdoutThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stdoutThreadExitEventHandle);
	}
	if(cmdData->stderrThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stderrThreadExitEventHandle);
	}
	LeaveCriticalSection(&cmdData->lock);
#else
	kill(- cmdData->childPid, SIGTERM);
#endif
}

/*
** Cancel the shell command in progress. Returns TRUE if successfully aborted.
*/
Boolean AbortShellCommand(WindowInfo *window)
{
    shellCmdInfo *cmdData = window->shellCmdData;

    if (cmdData == NULL)
    	return TRUE;

#ifdef _WIN32
	if(!TerminateChildGracefully(cmdData->childPid, 15)) {
    	int resp = DialogF(DF_WARN, window->shell, 2, 
			"The shell command did not terminate.\nDo you want to forcibly kill it?",
    		"KILL", "Cancel");
		if(resp == 2) {
			return FALSE;
		}
		if(!TerminateProcess(cmdData->childHandle, 0)) {
			PERRORWIN32("Unable to terminate child process.");
		}
	}
	EnterCriticalSection(&cmdData->lock);
	/* signal the threads to finish. */
	if(cmdData->stdinThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stdinThreadExitEventHandle);
	}
	if(cmdData->stdoutThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stdoutThreadExitEventHandle);
	}
	if(cmdData->stderrThreadExitEventHandle != INVALID_HANDLE_VALUE) {
		SetEvent(cmdData->stderrThreadExitEventHandle);
	}
	LeaveCriticalSection(&cmdData->lock);
#else
	abortShellCommand(cmdData);
	finishCmdExecution(window, True);
#endif
	
	return TRUE;
}

/*
** Function to check if there is a shell command active for window.
*/
Boolean IsShellCommandInProgress(WindowInfo *window)
{
	return (window->shellCmdData != NULL);
}

/*
** Function to check if the file will be reloaded 
** after the command completes.
*/
Boolean IsReloadAfterShellCommand(WindowInfo *window)
{
	return (window->shellCmdData != NULL 
	     && ((shellCmdInfo *)window->shellCmdData)->flags & RELOAD_FILE_AFTER);
}

#ifdef _WIN32

/* Returns the last error message as a string value.
** The return value must be freed with LocalFree().
*/
char *GetLastErrorString(void)
{
	static char message[256];
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM,
		0,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
		(LPTSTR) message,
		sizeof(message),
		NULL
	);
	return message;
}

/* TerminateAppEnum Callbacks */

static BOOL CALLBACK TerminateChildEnum( HWND hwnd, LPARAM lParam )
{
	DWORD dwID ;

	GetWindowThreadProcessId(hwnd, &dwID) ;

	if(dwID == (DWORD)lParam) {
		PostMessage(hwnd, WM_CLOSE, 0, 0) ;
	}

	return TRUE ;
}

static BOOL WINAPI TerminateChildGracefully(DWORD dwPID, DWORD dwTimeout)
{
	HANDLE   hProc ;
	BOOL   dwRet ;

	/* If we can't open the process with PROCESS_TERMINATE rights,
	   then we give up immediately. */
	hProc = OpenProcess(SYNCHRONIZE|PROCESS_TERMINATE, FALSE, dwPID);

	if(hProc == NULL) {
		return FALSE ;
	}

	/* TerminateAppEnum() posts WM_CLOSE to all windows whose PID
	   matches your process's. */
	EnumWindows((WNDENUMPROC)TerminateChildEnum, (LPARAM) dwPID) ;

	/* Wait on the handle. If it signals, great. If it times out,
	   then you kill it. */
	if(WaitForSingleObject(hProc, dwTimeout)!=WAIT_OBJECT_0) {
		dwRet = FALSE;
	} else {
		dwRet = TRUE;
	}

	return dwRet ;
}

static BOOLEAN isUNCPath(char *path)
{
	if(path == NULL) {
		return False;
	}
	return (strncmp(path, "//", 2) == 0 || strncmp(path, "\\\\", 2) == 0);		
}

/* Thread to write stdin. If it blocks who cares. */
static DWORD WINAPI stdinWriteThread(LPVOID lpParam) {
    WindowInfo *window = (WindowInfo *)lpParam;
    shellCmdInfo *cmdData = window->shellCmdData;
	HANDLE eventHandle;
	HANDLE stdinH;
	char *inPtr;
	int inLength;
		
	EnterCriticalSection(&cmdData->lock);
	eventHandle = cmdData->stdinThreadExitEventHandle;
	stdinH = cmdData->stdinH;
	inPtr = cmdData->inPtr;
	inLength = cmdData->inLength;
	LeaveCriticalSection(&cmdData->lock);
 
	/* No file handle to write on so just return; */
	if(stdinH == INVALID_HANDLE_VALUE) {
		return 0;
	}

	while(1) {
		int nWritten;

		/* We have been signaled to terminate */
		if(WaitForSingleObject(eventHandle, 0/*milliseconds*/) 
			== WAIT_OBJECT_0) {
			EnterCriticalSection(&cmdData->lock);
			cmdData->terminatedOnError = FALSE;
			MYCLOSEHANDLE(cmdData->stdinH);
			LeaveCriticalSection(&cmdData->lock);
			break;
		}
	
		
		if(!WriteFile(stdinH, inPtr, inLength, &nWritten, NULL)) {
			EnterCriticalSection(&cmdData->lock);
			cmdData->terminatedOnError = TRUE;
			cmdData->terminationErrorMessage = GetLastErrorString();
			abortShellCommand(cmdData);
			LeaveCriticalSection(&cmdData->lock);
			break;
		} else {
			EnterCriticalSection(&cmdData->lock);
			cmdData->inPtr += nWritten;
			cmdData->inLength -= nWritten;
			if (cmdData->inLength <= 0) {
				MYCLOSEHANDLE(cmdData->stdinH);
    			cmdData->inPtr = NULL;
				LeaveCriticalSection(&cmdData->lock);
				break;
    		}
			LeaveCriticalSection(&cmdData->lock);
		}
	}
	return 0;
}

/* Remove all <CR>s from the string in buf. */
/* Returns the new length of the string */
static int removeCarriageReturns(char *buf, int len)
{
	char *s, *d, *end;
	s = d = buf;
	end = s + len;
	for(; s < end; s++) {
		if(*s == '\r' && (s+1) < end && *(s+1) == '\n') {
			continue;
		}
		*d = *s;
		d++;
	}
	return d - buf;
}


/* Thread to read stdout. If it blocks who cares. */
static DWORD WINAPI stdoutReadThread(LPVOID lpParam) {
    WindowInfo *window = (WindowInfo *)lpParam;
    shellCmdInfo *cmdData = window->shellCmdData;
	HANDLE eventHandle;
	HANDLE stdoutH;
	
	EnterCriticalSection(&cmdData->lock);
	eventHandle = cmdData->stdoutThreadExitEventHandle;
	stdoutH = cmdData->stdoutH;
	LeaveCriticalSection(&cmdData->lock);
 
	while(1) {
		buffer *buf;
		int nRead;
		    
		/* See if we have been signaled to terminate */
		if(WaitForSingleObject(eventHandle, 0/*milliseconds*/) 
			== WAIT_OBJECT_0) {
			EnterCriticalSection(&cmdData->lock);
			cmdData->terminatedOnError = FALSE;
			MYCLOSEHANDLE(cmdData->stdoutH);
			LeaveCriticalSection(&cmdData->lock);
			break;
		}

		buf = createBuffer();

		/* read from the process' stdout stream */
	    if(!ReadFile(stdoutH, buf->contents, IO_BUF_SIZE, &nRead, NULL)) {
			if(GetLastError() == ERROR_BROKEN_PIPE) {
				nRead = 0;
			} else {
				EnterCriticalSection(&cmdData->lock);
				cmdData->terminatedOnError = TRUE;
				cmdData->terminationErrorMessage = GetLastErrorString();
				abortShellCommand(cmdData);
				LeaveCriticalSection(&cmdData->lock);
				destroyBuffer(buf);
				break;
			}
		}
    
		/* end of data.  If the stdout stream is done too, execution of the
		   shell process is complete, and we can display the results */
		if (nRead == 0) {
			destroyBuffer(buf);
	    	break;
	    }
    
		/* characters were read successfully, add buf to linked list of buffers */
		buf->length = removeCarriageReturns(buf->contents, nRead);
		EnterCriticalSection(&cmdData->lock);
		addOutput(&cmdData->outBufs, buf);
		LeaveCriticalSection(&cmdData->lock);
	}
	
	return 0;
}

/* Thread to read stdout. If it blocks who cares. */
static DWORD WINAPI stderrReadThread(LPVOID lpParam) {
    WindowInfo *window = (WindowInfo *)lpParam;
    shellCmdInfo *cmdData = window->shellCmdData;
	HANDLE eventHandle;
	HANDLE stderrH;
		
	EnterCriticalSection(&cmdData->lock);
	eventHandle = cmdData->stderrThreadExitEventHandle;
	stderrH = cmdData->stderrH;
	LeaveCriticalSection(&cmdData->lock);

	while(1) {
	    buffer *buf;
	    int nRead;
    
		/* See if we have been signaled to terminate */
		if(WaitForSingleObject(eventHandle, 0/*milliseconds*/) 
			== WAIT_OBJECT_0) {
			EnterCriticalSection(&cmdData->lock);
			cmdData->terminatedOnError = FALSE;
			MYCLOSEHANDLE(cmdData->stderrH);
			LeaveCriticalSection(&cmdData->lock);
			break;
		}

		buf = createBuffer();

		/* read from the process' stderr stream */
	    if(!ReadFile(stderrH, buf->contents, IO_BUF_SIZE, &nRead, NULL)) {
			if(GetLastError() == ERROR_BROKEN_PIPE) {
				nRead = 0;
			} else {
				EnterCriticalSection(&cmdData->lock);
				cmdData->terminationErrorMessage = GetLastErrorString();
				cmdData->terminatedOnError = TRUE;
				abortShellCommand(cmdData);
				LeaveCriticalSection(&cmdData->lock);
				destroyBuffer(buf);
				break;
			}
		}
    
		/* end of data.  If the stdout stream is done too, execution of the
		   shell process is complete, and we can display the results */
		if (nRead == 0) {
			destroyBuffer(buf);
	    	break;
	    }
    
		/* characters were read successfully, add buf to linked list of buffers */
		buf->length = removeCarriageReturns(buf->contents, nRead);
		EnterCriticalSection(&cmdData->lock);
		addOutput(&cmdData->errBufs, buf);
		LeaveCriticalSection(&cmdData->lock);
	}
	
	return 0;
}

static void createStdinWriteThread(WindowInfo *window) {
	unsigned dwThreadId;
	shellCmdInfo *cmdData = window->shellCmdData;

	cmdData->stdinThreadHandle = (HANDLE)_beginthreadex(
		NULL,                        /* no security attributes  */
	    0,                           /* use default stack size   */
		stdinWriteThread,            /* thread function  */
		window,                      /* argument to thread function  */
		CREATE_SUSPENDED,            /* The thread is suspended until we start the */
			                         /* child process */
		&dwThreadId                  /* returns the thread identifier */
	);
	if(cmdData->stdinThreadHandle == 0) {
		PERRORWIN32("Error creating stdin write thread.");
	}

	/* Create an event to use to signal the thread to terminate. */
	cmdData->stdinThreadExitEventHandle = CreateEvent(
		NULL,                    /* pointer to security attributes  */
		FALSE,                   /* flag for manual-reset event  */
		FALSE,                   /* flag for initial state  */
		NULL                     /* pointer to event-object name (unnamed) */
	);
}

static void createStdoutReadThread(WindowInfo *window) {
	DWORD dwThreadId;
	shellCmdInfo *cmdData = window->shellCmdData;

	cmdData->stdoutThreadHandle = (HANDLE)_beginthreadex(
		NULL,                        /* no security attributes  */
	    0,                           /* use default stack size   */
		stdoutReadThread,            /* thread function  */
		window,                      /* argument to thread function  */
		CREATE_SUSPENDED,            /* The thread is suspended until we start the */
			                         /* child process */
		&dwThreadId                  /* returns the thread identifier */
	);
	if(cmdData->stdoutThreadHandle == 0) {
		PERRORWIN32("Error creating stdout read thread.");
	}

	/* Create an event to use to signal the thread to terminate. */
	cmdData->stdoutThreadExitEventHandle = CreateEvent(
		NULL,                    /* pointer to security attributes  */
		FALSE,                   /* flag for manual-reset event  */
		FALSE,                   /* flag for initial state  */
		NULL                     /* pointer to event-object name (unnamed) */
	);
}

static void createStderrReadThread(WindowInfo *window) {
	unsigned dwThreadId;
	shellCmdInfo *cmdData = window->shellCmdData;

	cmdData->stderrThreadHandle = (HANDLE)_beginthreadex(
		NULL,                        /* no security attributes  */
	    0,                           /* use default stack size   */
		stderrReadThread,            /* thread function  */
		window,                      /* argument to thread function  */
		CREATE_SUSPENDED,            /* The thread is suspended until we start the */
			                         /* child process */
		&dwThreadId                  /* returns the thread identifier */
	);
	if(cmdData->stderrThreadHandle == 0) {
		PERRORWIN32("Error creating stderr read thread.");
	}
		
	/* Create an event to use to signal the thread to terminate. */
	cmdData->stderrThreadExitEventHandle = CreateEvent(
		NULL,                    /* pointer to security attributes  */
		FALSE,                   /* flag for manual-reset event  */
		FALSE,                   /* flag for initial state  */
		NULL                     /* pointer to event-object name (unnamed) */
	);
}

typedef struct _threadCompletionTimerProcStruct {
	Boolean done;
	shellCmdInfo *cmdData;
	WindowInfo *window;
} threadCompletionTimerProcStruct;

static void threadCompletionTimerProc(threadCompletionTimerProcStruct *TimerProcInfo, XtIntervalId *id)
{
	HANDLE handlelist[4];
	INT n = 0;
	DWORD rc;
	shellCmdInfo *cmdData = TimerProcInfo->cmdData;
	WindowInfo *window = TimerProcInfo->window;
	EnterCriticalSection(&cmdData->lock);
	if(cmdData->stdinThreadHandle != INVALID_HANDLE_VALUE) {
		handlelist[n] = cmdData->stdinThreadHandle;
		n++;
	}
	if(cmdData->stdoutThreadHandle != INVALID_HANDLE_VALUE) {
		handlelist[n] = cmdData->stdoutThreadHandle;
		n++;
	}
	if(cmdData->stderrThreadHandle != INVALID_HANDLE_VALUE) {
		handlelist[n] = cmdData->stderrThreadHandle;
		n++;
	}
	if(cmdData->childHandle != INVALID_HANDLE_VALUE) {
		handlelist[n] = cmdData->childHandle;
		n++;
	}
	LeaveCriticalSection(&cmdData->lock);

	rc = WaitForMultipleObjects(n, handlelist, TRUE, 0);
	if(rc != WAIT_TIMEOUT) {
		XEvent event[1];
		memset(event, 0, sizeof(XEvent));
		TimerProcInfo->done = True;
		/* Send ClientMessage event to get us out of EventLoop. */
		event->xclient.type = ClientMessage;
		event->xclient.send_event = True;
		event->xclient.display = XtDisplay(window->textArea);
		event->xclient.window = XtWindow(window->textArea);
		event->xclient.format = 8;
		XSendEvent(
			XtDisplay(window->textArea),
			XtWindow(window->textArea),
			False,
			0,
			event
		);
		return;
	}
	/* reregister ourselves */
	XtAppAddTimeOut(
		XtWidgetToApplicationContext(window->shell),
		250,
		(XtTimerCallbackProc)threadCompletionTimerProc,
		TimerProcInfo
	);
}

#else /* !_WIN32 */
/*
** Called when the shell sub-process stdout stream has data.  Reads data into
** the "outBufs" buffer chain in the window->shellCommandData data structure.
*/
static void stdoutReadProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    buffer *buf;
    int nRead;

    /* read from the process' stdout stream */
    buf = (buffer *)XtMalloc(sizeof(buffer));
    
	nRead = read(cmdData->stdoutFD, buf->contents, IO_BUF_SIZE);
    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
			PERROR("Error reading shell command output");
			destroyBuffer(buf);
			finishCmdExecution(window, True);
		}
		return;
    }
    
    /* end of data.  If the stderr stream is done too, execution of the
       shell process is complete, and we can display the results */
    if (nRead == 0) {
		destroyBuffer(buf);
		if(cmdData->stdoutInputID) {
    		XtRemoveInput(cmdData->stdoutInputID);
    		cmdData->stdoutInputID = 0;
		}
    	if (cmdData->stderrInputID == 0)
    	    finishCmdExecution(window, False);
    	return;
    }
    
    /* characters were read successfully, add buf to linked list of buffers */
    buf->length = nRead;
    addOutput(&cmdData->outBufs, buf);
}

/*
** Called when the shell sub-process stderr stream has data.  Reads data into
** the "errBufs" buffer chain in the window->shellCommandData data structure.
*/
static void stderrReadProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    buffer *buf;
    int nRead;
    
    /* read from the process' stderr stream */
    buf = (buffer *)XtMalloc(sizeof(buffer));
    nRead = read(cmdData->stderrFD, buf->contents, IO_BUF_SIZE);
    if (nRead == -1) { /* error */
		if (errno != EWOULDBLOCK && errno != EAGAIN) {
			PERROR("Error reading shell command error stream");
			destroyBuffer(buf);
			finishCmdExecution(window, True);
		}
		return;
    }
    
    /* end of data.  If the stdout stream is done too, execution of the
       shell process is complete, and we can display the results */
    if (nRead == 0) {
		destroyBuffer(buf);
		if(cmdData->stderrInputID) {
    		XtRemoveInput(cmdData->stderrInputID);
    		cmdData->stderrInputID = 0;
		}
    	if (cmdData->stdoutInputID == 0)
    	    finishCmdExecution(window, False);
    	return;
    }
    
    /* characters were read successfully, add buf to linked list of buffers */
    buf->length = nRead;
    addOutput(&cmdData->errBufs, buf);
}

/*
** Called when the shell sub-process stdin stream is ready for input.  Writes
** data from the "input" text string passed to issueCommand.
*/
static void stdinWriteProc(XtPointer clientData, int *source, XtInputId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    int nWritten;

	if(cmdData->stdinFD == -1) {
		return;
	}

    nWritten = write(cmdData->stdinFD, cmdData->inPtr, cmdData->inLength);
    if (nWritten == -1) {
    	if (errno != EWOULDBLOCK && errno != EAGAIN) {
    	    PERROR("Write to shell command failed");
    	    finishCmdExecution(window, True);
    	}
    } else {
	cmdData->inPtr += nWritten;
	cmdData->inLength -= nWritten;
	if (cmdData->inLength <= 0) {
    	    if(cmdData->stdinFD != -1) {
				close(cmdData->stdinFD);
				cmdData->stdinFD = -1;
			}
    	    cmdData->inPtr = NULL;
			if(cmdData->stdinInputID) {
				XtRemoveInput(cmdData->stdinInputID);
				cmdData->stdinInputID = 0;
			}
    	}
    }
}
#endif /* !_WIN32 */

/*
** Issue a shell command and feed it the string "input".  Output can be
** directed either to text widget "textW" where it replaces the text between
** the positions "replaceLeft" and "replaceRight", to a separate pop-up dialog
** (OUTPUT_TO_DIALOG), or to a macro-language string (OUTPUT_TO_STRING).  If
** "input" is NULL, no input is fed to the process.  If an input string is
** provided, it is freed when the command completes.  Flags:
**
**   ACCUMULATE     	Causes output from the command to be saved up until
**  	    	    	the command completes.
**   ERROR_DIALOGS  	Presents stderr output separately in popup a dialog,
**  	    	    	and also reports failed exit status as a popup dialog
**  	    	    	including the command output.
**   REPLACE_SELECTION  Causes output to replace the selection in textW.
**   RELOAD_FILE_AFTER  Causes the file to be completely reloaded after the
**  	    	    	command completes.
**   OUTPUT_TO_DIALOG   Send output to a pop-up dialog instead of textW
**   OUTPUT_TO_NEW_WINDOW   textW is in a new window
**   OUTPUT_TO_STRING   Output to a macro-language string instead of a text
**  	    	    	widget or dialog.
**
** REPLACE_SELECTION, ERROR_DIALOGS, and OUTPUT_TO_STRING can only be used
** along with ACCUMULATE (these operations can't be done incrementally).
*/
static void issueCommand(WindowInfo *window, char *command, char *input,
	int inputLen, int flags, Widget textW, int replaceLeft,
	int replaceRight, int fromMacro)
{
#ifdef _WIN32
    HANDLE stdinH = INVALID_HANDLE_VALUE;
	HANDLE stdoutH = INVALID_HANDLE_VALUE;
	HANDLE stderrH = INVALID_HANDLE_VALUE;
	HANDLE childHandle = INVALID_HANDLE_VALUE;
	DWORD childPid = 0;
#else
    int stdinFD = -1, stdoutFD = -1, stderrFD = -1;
    pid_t childPid;
#endif
    XtAppContext context = XtWidgetToApplicationContext(window->shell);
    shellCmdInfo *cmdData;
    
    /* verify consistency of input parameters */
    if ((flags & ERROR_DIALOGS || flags & REPLACE_SELECTION ||
	    flags & OUTPUT_TO_STRING) && !(flags & ACCUMULATE))
    	return;
    
    /* a shell command called from a macro must be executed in the same
       window as the macro, regardless of where the output is directed,
       so the user can cancel them as a unit */
    if (fromMacro)
    	window = MacroRunWindow();
    
    /* put up a watch cursor over the waiting window */
    if (!fromMacro)
    	BeginWait(window->shell);
    
    /* enable the cancel menu item */
    if (!fromMacro)
    	XtSetSensitive(window->cancelShellItem, True);

#ifdef _WIN32
    /* fork the subprocess and issue the command */
    /* fork the subprocess and issue the command */
	forkCommand(
		window->shell, 
		command, 
		window->editorInfo->path, 
		&stdinH,
	    &stdoutH, 
		(flags & ERROR_DIALOGS) ? &stderrH : NULL,
		&childHandle,
		&childPid
	);

	if(childHandle == INVALID_HANDLE_VALUE) {
	    if (!fromMacro)
			XtSetSensitive(window->cancelShellItem, False);
		EndWait(window->shell);
		return;
	}
    
    /* if there's nothing to write to the process' stdin, close it now */
    if (input == NULL) {
		MYCLOSEHANDLE(stdinH);
    }
	
#else /* !_WIN32 */
	childPid = forkCommand(
		window->shell, 
		command, 
		window->editorInfo->path, 
		&stdinFD,
	    &stdoutFD, 
		(flags & ERROR_DIALOGS) ? &stderrFD : NULL
	);

	if(childPid == -1) {
	    if (!fromMacro)
			XtSetSensitive(window->cancelShellItem, False);
		EndWait(window->shell);
		return;
	}

    /* set the pipes connected to the process for non-blocking i/o */
	if (fcntl(stdinFD, F_SETFL, O_NONBLOCK) < 0) {
    	PERROR("Internal error (fcntl)");
		return;
	}
    if (fcntl(stdoutFD, F_SETFL, O_NONBLOCK) < 0) {
    	PERROR("Internal error (fcntl1)");
		return;
	}
    if (flags & ERROR_DIALOGS) {
		if (fcntl(stderrFD, F_SETFL, O_NONBLOCK) < 0) {
    	    PERROR("Internal error (fcntl2)");
			return;
		}
    }

    /* if there's nothing to write to the process' stdin, close it now */
    if (input == NULL) {
		MYCLOSEFD(stdinFD);
    }
	
#endif /* !_WIN32 */
    
    /* Create a data structure for passing process information around
       amongst the callback routines which will process i/o and completion */
    cmdData = createShellCmdInfo();

    window->shellCmdData = cmdData;
    cmdData->flags = flags;
	cmdData->outBufs = NULL;
	cmdData->errBufs = NULL;
	cmdData->input = input;
	cmdData->inPtr = input;
	cmdData->textW = textW;
	cmdData->bannerIsUp = False;
	cmdData->fromMacro = fromMacro;
	cmdData->leftPos = replaceLeft;
	cmdData->rightPos = replaceRight;
	cmdData->inLength = inputLen;
	cmdData->flushTimeoutID = 0;
	cmdData->bannerTimeoutID = 0;
#ifdef _WIN32
    cmdData->stdinH = stdinH;
    cmdData->stdoutH = stdoutH;
    cmdData->stderrH = stderrH;
    cmdData->childHandle = childHandle;
    cmdData->childPid = childPid;
	cmdData->terminatedOnError = FALSE;
	cmdData->terminationErrorMessage = "";
	cmdData->stdinThreadHandle = INVALID_HANDLE_VALUE;
	cmdData->stdinThreadExitEventHandle = INVALID_HANDLE_VALUE;
	cmdData->stdoutThreadHandle = INVALID_HANDLE_VALUE;
	cmdData->stdoutThreadExitEventHandle = INVALID_HANDLE_VALUE;
	cmdData->stderrThreadHandle = INVALID_HANDLE_VALUE;
	cmdData->stderrThreadExitEventHandle = INVALID_HANDLE_VALUE;
#else /* !_WIN32 */
    cmdData->stdinFD = stdinFD;
    cmdData->stdoutFD = stdoutFD;
    cmdData->stderrFD = stderrFD;
    cmdData->childPid = childPid;
#endif /* !_WIN32 */
    
    /* Set up timer proc for putting up banner when process takes too long */
    if (!fromMacro)
    	cmdData->bannerTimeoutID = XtAppAddTimeOut(context, BANNER_WAIT_TIME,
    	    	bannerTimeoutProc, window);

    /* Set up timer proc for flushing output buffers periodically */
	if (!(flags & ACCUMULATE) && textW != NULL) {
		cmdData->flushTimeoutID = XtAppAddTimeOut(context, OUTPUT_FLUSH_FREQ,
	    	flushTimeoutProc, window);
	}
    	
#ifdef _WIN32
	createStdoutReadThread(window);
    if (input != NULL) {
		createStdinWriteThread(window);
	}
    if (flags & ERROR_DIALOGS) {
		createStderrReadThread(window);
	}
	/* Start the threads now that we are all setup. */
	ResumeThread(cmdData->stdinThreadHandle);
	ResumeThread(cmdData->stdoutThreadHandle);
	ResumeThread(cmdData->stderrThreadHandle);

	{
	threadCompletionTimerProcStruct TimerProcInfo[1];
	TimerProcInfo->done = False;
	TimerProcInfo->cmdData = cmdData;
	TimerProcInfo->window = window;

	/* Register a work procedure to test if the treads have finished. */
	XtAppAddTimeOut(
		XtWidgetToApplicationContext(window->shell),
		25,
		(XtTimerCallbackProc)threadCompletionTimerProc,
		TimerProcInfo
	);

	/* Wait here for the command and threads to finish; */
	while(!TimerProcInfo->done) {
   		XEvent event;
       	XtAppNextEvent(XtWidgetToApplicationContext(window->shell), &event);
   		ServerDispatchEvent(&event);
	}
	} /* local vars */
	
	if(cmdData->terminatedOnError) {
		DialogF(DF_ERR, window->shell, 1,
			"Error running command.\ncommand = %s\nerror = %s",
			"Dismiss",
			command,
			cmdData->terminationErrorMessage
		);
	}

	finishCmdExecution(window, cmdData->terminatedOnError);
#else
    /* set up callbacks for activity on the file descriptors */
	cmdData->stdoutInputID = XtAppAddInput(context, stdoutFD, (XtPointer)
			XtInputReadMask,
			stdoutReadProc, window);

    if (input != NULL) {
    	cmdData->stdinInputID = XtAppAddInput(context, stdinFD,
    	    	(XtPointer)XtInputWriteMask, stdinWriteProc, window);
	}
    else
    	cmdData->stdinInputID = 0;

    if (flags & ERROR_DIALOGS) {
		cmdData->stderrInputID = XtAppAddInput(context, stderrFD, (XtPointer)
			XtInputReadMask,
    		stderrReadProc, window);
    }
    else
    	cmdData->stderrInputID = 0;
    
    /* If this was called from a macro, preempt the macro until shell
       command completes */
    if (fromMacro)
    	PreemptMacro();
#endif
}

/*
** Timer proc for putting up the "Shell Command in Progress" banner if
** the process is taking too long.
*/
static void bannerTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
    
    cmdData->bannerIsUp = True;
    SetModeMessage(window,
    	    "Shell Command in Progress -- Press Ctrl+. to Cancel");
    cmdData->bannerTimeoutID = 0;
}

/*
** Timer proc for flushing output buffers periodically when the process
** takes too long.
*/
static void flushTimeoutProc(XtPointer clientData, XtIntervalId *id)
{
    WindowInfo *window = (WindowInfo *)clientData;
    shellCmdInfo *cmdData = window->shellCmdData;
	textBuffer *buf;
    int len;
    char *outText;
    
	if(cmdData == NULL) {
#if 0
		DialogF(DF_WARN, window->shell, 1, "flushTimeoutProc(): cmdData == 0", "OK");
#endif
		return;
	}
    
#ifdef _WIN32
	EnterCriticalSection(&cmdData->lock);
#endif

    buf = TextGetBuffer(cmdData->textW);

    /* shouldn't happen, but it would be bad if it did */
    if (cmdData->textW == NULL) {
#ifdef _WIN32
		LeaveCriticalSection(&cmdData->lock);
#endif
    	return;
	}

	outText = coalesceOutput(&cmdData->outBufs, &len);
	if (len != 0) {
		if (BufSubstituteNullChars(outText, len, buf)) {
			BufReplace(buf, cmdData->leftPos, cmdData->rightPos, outText);
			TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
			cmdData->leftPos += len;
			cmdData->rightPos = cmdData->leftPos;
		} else {
			PERROR("Too much binary data");
		}
    }
    XtFree(outText);

    /* re-establish the timer proc (this routine) to continue processing */
    cmdData->flushTimeoutID = XtAppAddTimeOut(
    	    XtWidgetToApplicationContext(window->shell),
    	    OUTPUT_FLUSH_FREQ, flushTimeoutProc, clientData);

#ifdef _WIN32
	LeaveCriticalSection(&cmdData->lock);
#endif
}

/*
** Clean up after the execution of a shell command sub-process and present
** the output/errors to the user as requested in the initial issueCommand
** call.  If "terminatedOnError" is true, don't bother trying to read the
** output, just close the i/o descriptors, free the memory, and restore the
** user interface state.
*/
static void finishCmdExecution(WindowInfo *window, int terminatedOnError)
{
    shellCmdInfo *cmdData = window->shellCmdData;
    textBuffer *buf;
    int status, failure = 0, errorReport, outTextLen, errTextLen;
    int resp, cancel = False, fromMacro = cmdData->fromMacro;
    char *outText, *errText = "";
	int loadAfter = (cmdData->flags & RELOAD_FILE_AFTER);
	
	/* Guard against multiple executions */
	if (cmdData == NULL) {
		return;
	}
	
#ifdef _WIN32
	/* Clean up the thread and event handles. */
	MYCLOSEHANDLE(cmdData->stdinThreadHandle);
	MYCLOSEHANDLE(cmdData->stdoutThreadHandle);
	MYCLOSEHANDLE(cmdData->stderrThreadHandle);
	MYCLOSEHANDLE(cmdData->stdinThreadExitEventHandle);
	MYCLOSEHANDLE(cmdData->stdoutThreadExitEventHandle);
	MYCLOSEHANDLE(cmdData->stderrThreadExitEventHandle);

    /* Close any file handles remaining open */
	MYCLOSEHANDLE(cmdData->stdoutH);
	MYCLOSEHANDLE(cmdData->stderrH);
	MYCLOSEHANDLE(cmdData->stdinH);
#else
    /* Cancel any pending i/o on the file descriptors */
    if (cmdData->stdoutInputID != 0) {
    	XtRemoveInput(cmdData->stdoutInputID);
		cmdData->stdoutInputID = 0;
	}
    if (cmdData->stdinInputID != 0) {
    	XtRemoveInput(cmdData->stdinInputID);
		cmdData->stdinInputID = 0;
	}
    if (cmdData->stderrInputID != 0) {
    	XtRemoveInput(cmdData->stderrInputID);
		cmdData->stderrInputID = 0;
	}

    /* Close any file handles remaining open */
	MYCLOSEFD(cmdData->stdoutFD);
	MYCLOSEFD(cmdData->stderrFD);
	MYCLOSEFD(cmdData->stdinFD);
#endif

    /* Free the provided input text */
	if (cmdData->input != NULL) {
		XtFree(cmdData->input);
		cmdData->input = NULL;
	}
    
    /* Cancel pending timeouts */
    if (cmdData->flushTimeoutID != 0) {
    	XtRemoveTimeOut(cmdData->flushTimeoutID);
		cmdData->flushTimeoutID = 0;
	}
    if (cmdData->bannerTimeoutID != 0) {
    	XtRemoveTimeOut(cmdData->bannerTimeoutID);
		cmdData->bannerTimeoutID = 0;
	}
    
    /* Clean up waiting-for-shell-command-to-complete mode */
    if (!cmdData->fromMacro) {
		EndWait(window->shell);
		XtSetSensitive(window->cancelShellItem, False);
		if (cmdData->bannerIsUp)
    	    ClearModeMessage(window);
    }
    
    /* If the process was killed or became inaccessable, give up */
    if (terminatedOnError) {
		freeBufList(&cmdData->outBufs);
		freeBufList(&cmdData->errBufs);
		goto cmdDone;
    }

    /* Assemble the output from the process' stderr and stdout streams into
       null terminated strings, and free the buffer lists used to collect it */
    outText = coalesceOutput(&cmdData->outBufs, &outTextLen);
    if (cmdData->flags & ERROR_DIALOGS)
    	errText = coalesceOutput(&cmdData->errBufs, &errTextLen);

    /* Wait for the child process to complete and get its return status */
#ifdef _WIN32
	failure = False;
	if(cmdData->childHandle != INVALID_HANDLE_VALUE) {
		while(1)         {
			if(!GetExitCodeProcess(cmdData->childHandle, &status)) {
				failure = True;
				break;
			}
			if(status != STILL_ACTIVE) {
				break;
			}
		}
		if(!failure) {
			failure = (status != 0);
		}
		MYCLOSEHANDLE(cmdData->childHandle);
	}
#else
    waitpid(cmdData->childPid, &status, 0);
	failure = WIFEXITED(status) && WEXITSTATUS(status) != 0;
#endif
    
    /* Present error and stderr-information dialogs.  If a command returned
       error output, or if the process' exit status indicated failure,
       present the information to the user. */
    if (cmdData->flags & ERROR_DIALOGS) {
	errorReport = *errText != '\0';
	if (failure && errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_WARN, window->shell, 2, "%s",
    	    	    "Cancel", "Proceed", errText);
    	    cancel = resp == 1;
	} else if (failure) {
    	    truncateString(outText, DF_MAX_MSG_LENGTH-70);
    	    resp = DialogF(DF_WARN, window->shell, 2,
    	       "Command reported failed exit status.\nOutput from command:\n%s",
    		    "Cancel", "Proceed", outText);
    	    cancel = resp == 1;
	} else if (errorReport) {
    	    removeTrailingNewlines(errText);
    	    truncateString(errText, DF_MAX_MSG_LENGTH);
    	    resp = DialogF(DF_INF, window->shell, 2, "%s",
    	    	    "Proceed", "Cancel", errText);
    	    cancel = resp == 2;
	}
	XtFree(errText);
	if (cancel) {
	    XtFree(outText);
    	    goto cmdDone;
	}
    }
    
    /* If output is to a dialog, present the dialog.  Otherwise insert the
       (remaining) output in the text widget as requested, and move the
       insert point to the end */
    if (cmdData->flags & OUTPUT_TO_DIALOG) {
    	removeTrailingNewlines(outText);
		if (*outText != '\0') {
    		Widget widget;
    	    widget = createOutputDialog(window, outText,
    	    	loadAfter ? XmDIALOG_PRIMARY_APPLICATION_MODAL : XmDIALOG_MODELESS);

    		/* Wait for the dialog to be dismissed if loadAfter is set */
    		if(loadAfter) {
 				CheckingMode checkingModeSave;
			
				/* disable file modification checking inside the application loop
				   below. */
				checkingModeSave = window->editorInfo->checkingMode;
				window->editorInfo->checkingMode = CHECKING_MODE_DISABLED;
   				while(XtIsManaged(widget)) {
   					XEvent event;
       				XtAppNextEvent(XtWidgetToApplicationContext(window->shell), &event);
   					ServerDispatchEvent(&event);
    			}
    			/* reenable modification checking */
				window->editorInfo->checkingMode = checkingModeSave;
    		}
    	}
    } else if (cmdData->flags & OUTPUT_TO_STRING) {
    	ReturnShellCommandOutput(window, outText, WEXITSTATUS(status));
    } else {
		buf = TextGetBuffer(cmdData->textW);
		if (!BufSubstituteNullChars(outText, outTextLen, buf)) {
	    	PERROR("Too much binary data in shell cmd output");
	    	outText[0] = '\0';
		}
		if (cmdData->flags & REPLACE_SELECTION) {
	    	BufReplaceSelected(buf, outText, True);
	    	TextSetCursorPos(cmdData->textW, buf->cursorPosHint);
		} else {
	    	BufReplace(buf, cmdData->leftPos, cmdData->rightPos, outText);
			if (cmdData->flags & OUTPUT_TO_NEW_WINDOW) {
    			/* If the output is to a new window then turn off the modified
    			   status so the window can be closed without being prompted to
    			   save it. Also move the cursor to the beginning of the text .*/
    			SetWindowModified(window, False);
				TextSetCursorPos(window->textArea, 0);
			} else {
	    		TextSetCursorPos(cmdData->textW, cmdData->leftPos+strlen(outText));
			}
		}
    }

    /* If the command requires the file to be reloaded afterward, reload it */
    if (loadAfter) {
		if (window->editorInfo->filenameSet && window->editorInfo->fileChanged) {
			int b;
			CheckingMode checkingModeSave;
			
			/* disable file modification checking inside the application loop
    	       in DialogF()*/
			checkingModeSave = window->editorInfo->checkingMode;
			window->editorInfo->checkingMode = CHECKING_MODE_DISABLED;
	    	b = DialogF(DF_QUES, window->shell, 2, 
	    		"Attemping to re-load file after executing command.\nThis will overwrite your changes.\nDo you want to re-load it anyway?",
    		    "Don't Re-load", "Re-load");
    		/* reenable modification checking */
			window->editorInfo->checkingMode = checkingModeSave;
    		if(b == 2) {
    			RevertToSaved(window, True);
    		} else {
    			/* If they answered no then don't prompt that the file changed */
    			char fullname[MAXPATHLEN];
    			struct stat statbuf;
    			
    			/* Get the full name of the file */
    			strcpy(fullname, window->editorInfo->path);
    			strcat(fullname, window->editorInfo->filename);
    			
    			/* get the current mtime */
    			if (stat(fullname, &statbuf) == 0) {
	   				window->editorInfo->st__mtime = statbuf.st_mtime;
    			}
    		}
    	} else {
    		RevertToSaved(window, True);
    	}
    }

    /* Command is complete, free data structure and continue macro execution */
    XtFree(outText);
cmdDone:
    XtFree((char *)cmdData);
    window->shellCmdData = NULL;
    if (fromMacro)
    	ResumeMacroExecution(window);
}

/*
** Fork a subprocess to execute a command, return file descriptors for pipes
** connected to the subprocess' stdin, stdout, and stderr streams.  cmdDir
** sets the default directory for the subprocess.  If stderrFD is passed as
** NULL, the pipe represented by stdoutFD is connected to both stdin and
** stderr.  The function value returns the pid of the new subprocess, or -1
** if an error occured.
*/
#ifdef _WIN32

static void forkCommand(Widget parent, char *command, char *cmdDir,
	HANDLE *stdinHP, HANDLE *stdoutHP, HANDLE *stderrHP, HANDLE *childHP, DWORD *childPidDWP)
{
	HANDLE childStdoutRead = INVALID_HANDLE_VALUE;
	HANDLE childStdoutWrite = INVALID_HANDLE_VALUE;
	HANDLE childStdinRead = INVALID_HANDLE_VALUE;
	HANDLE childStdinWrite = INVALID_HANDLE_VALUE;
	HANDLE childStderrRead = INVALID_HANDLE_VALUE;
	HANDLE childStderrWrite = INVALID_HANDLE_VALUE;
	HANDLE childStdoutReadNI = INVALID_HANDLE_VALUE;
	HANDLE childStdinWriteNI = INVALID_HANDLE_VALUE;
	HANDLE childStderrReadNI = INVALID_HANDLE_VALUE;
    HANDLE childHandle = INVALID_HANDLE_VALUE;
    DWORD childPid = 0;
	SECURITY_ATTRIBUTES saAttr;
	char cmd[2*MAX_SHELL_CMD_LEN+1] = "";
	char shell[MAX_SHELL_CMD_LEN+1] = "";
	DWORD creationFlags = 0;
	STARTUPINFO si[1];
	PROCESS_INFORMATION pi[1];
	
	/* Clear the return values */
   	*childHP = INVALID_HANDLE_VALUE;
	*childPidDWP = 0;

	if(stdoutHP != NULL) {
		*stdoutHP = INVALID_HANDLE_VALUE;
	}
    if(stdinHP != NULL) {
		*stdinHP = INVALID_HANDLE_VALUE;
	}
    if(stderrHP != NULL) {
		*stderrHP = INVALID_HANDLE_VALUE;
	}
	
	/* Set the bInheritHandle flag so pipe handles are inherited. */
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = TRUE;
	saAttr.lpSecurityDescriptor = NULL;
	
	/* Create a pipe for the child process's standard i/o. */
	if (!CreatePipe(&childStdoutRead, &childStdoutWrite, &saAttr, IO_BUF_SIZE)) {
		PERRORWIN32("Internal error (opening stdout pipe)");
	   	*childHP = INVALID_HANDLE_VALUE;
        return;
    }
	if (!CreatePipe(&childStdinRead, &childStdinWrite, &saAttr, IO_BUF_SIZE)) {
		PERRORWIN32("Internal error (opening stdin pipe)");
	   	*childHP = INVALID_HANDLE_VALUE;
        return;
    }
	/* Redirect stderr to stdout if stderrHP is NULL */
	if (stderrHP == NULL) {
		/* Dup the stdout write handle for the stderr write handle */
		if(!DuplicateHandle(
				GetCurrentProcess(), childStdoutWrite,
				GetCurrentProcess(), &childStderrWrite,
				0, TRUE, DUPLICATE_SAME_ACCESS
			)
		) {
   			PERRORWIN32("Internal error DuplicateHandle for stderr to stdout failed:");
	   		*childHP = INVALID_HANDLE_VALUE;
        	return;
		}
	} else {
		if(!CreatePipe(&childStderrRead, &childStderrWrite, &saAttr, IO_BUF_SIZE)) {
			PERRORWIN32("Internal error (opening stderr pipe)");
	   		*childHP = INVALID_HANDLE_VALUE;
        	return;
		}
    }
	
	/* Create noninheritable i/o handles and close the inheritable */
	/* handles. Our ends of the pipes must not be inherited by the child. */
	if(childStdoutRead != INVALID_HANDLE_VALUE) {
		if(!DuplicateHandle(
				GetCurrentProcess(), childStdoutRead,
				GetCurrentProcess(), &childStdoutReadNI,
				0, FALSE, DUPLICATE_SAME_ACCESS
			)
		) {
   			PERRORWIN32("Internal error DuplicateHandle for stdout failed:");
	   		*childHP = INVALID_HANDLE_VALUE;
        	return;
		}
	}
	if(childStdinWrite != INVALID_HANDLE_VALUE) {
		if(!DuplicateHandle(
			GetCurrentProcess(), childStdinWrite,
			GetCurrentProcess(), &childStdinWriteNI,
			0, FALSE, DUPLICATE_SAME_ACCESS
	      )
		) {
	   	    PERRORWIN32("Internal error DuplicateHandle for stdin failed:");
	   		*childHP = INVALID_HANDLE_VALUE;
        	return;
		}
	}
	if(childStderrRead != INVALID_HANDLE_VALUE) {
		if(!DuplicateHandle(
			GetCurrentProcess(), childStderrRead,
			GetCurrentProcess(), &childStderrReadNI,
			0, FALSE, DUPLICATE_SAME_ACCESS
		  )
		) {
    		PERRORWIN32("Internal error DuplicateHandle for stderr failed:");
	   		*childHP = INVALID_HANDLE_VALUE;
        	return;
		}
	}

	MYCLOSEHANDLE(childStdoutRead);
	MYCLOSEHANDLE(childStdinWrite);
	MYCLOSEHANDLE(childStderrRead);

	if(stdoutHP) {
		*stdoutHP = childStdoutReadNI;
	} else {
		MYCLOSEHANDLE(childStdoutReadNI);
	}
	if(stdinHP) {
		*stdinHP = childStdinWriteNI;
	} else {
		MYCLOSEHANDLE(childStdinWriteNI);
	}
	if(stderrHP) {
		*stderrHP = childStderrReadNI;
	} else {
		MYCLOSEHANDLE(childStderrReadNI);
	}

	creationFlags = CREATE_NO_WINDOW | CREATE_NEW_PROCESS_GROUP;
	
	memset(si, 0, sizeof(STARTUPINFO));
	si->cb = sizeof(STARTUPINFO);
	si->dwFlags = STARTF_USESTDHANDLES; // Needed to make the pipe work.
	si->dwFlags |= STARTF_USESHOWWINDOW;
	si->wShowWindow = SW_HIDE | SW_MINIMIZE;
	
	/* redirect the childs i/o to the pipes we created */
	si->hStdInput = childStdinRead;
	si->hStdOutput = childStdoutWrite;
	si->hStdError = childStderrWrite;
	
#if 0
	ExpandEnvironmentStrings(GetPrefShell(), shell, sizeof(shell));
	sprintf(cmd, "%s -c %s", shell, command);
	if(CreateProcess(
		NULL,
		cmd,
		NULL,
		NULL,
		TRUE,
		creationFlags,
		NULL,
		(cmdDir[0] != 0) ? cmdDir : NULL,
		si,
		pi
	)) {
		childHandle = pi->hProcess;
		childPid = pi->dwProcessId;
	}
#endif
	if(childHandle == INVALID_HANDLE_VALUE) {
		/* If the current directory is a unc path then just
		** execute the command directly because the currend cmd.exe
		** does not support a unc path as its current directory.
		*/
		if(cmdDir[0] != 0 && isUNCPath(cmdDir)) {
			ExpandEnvironmentStrings(command, cmd, sizeof(cmd));
		} else {
			/* Fall back to comspec value */
			ExpandEnvironmentStrings("%COMSPEC%", shell, sizeof(shell));
			sprintf(cmd, "%s /c %s", shell, command);
		}
		if(CreateProcess(
			NULL,
			cmd,
			NULL,
			NULL,
			TRUE,
			creationFlags,
			NULL,
			(cmdDir[0] != 0) ? cmdDir : NULL,
			si,
			pi
		)) {
			childHandle = pi->hProcess;
			childPid = pi->dwProcessId;
		}
	}
	if(childHandle == INVALID_HANDLE_VALUE) {
		DialogF(DF_ERR, parent, 1,
			"Error running command!\nCommand: %s\nReason: %s",
			"Dismiss",
			cmd,
			GetLastErrorString()
		);
	}
	
	/* Close the file handles inherited by the child process */
	MYCLOSEHANDLE(childStdoutWrite);
	MYCLOSEHANDLE(childStdinRead);
	MYCLOSEHANDLE(childStderrWrite);
		
	*childHP = childHandle;
	*childPidDWP = childPid;
	
    return;
}    
#else /* !_WIN32 */

static pid_t forkCommand(Widget parent, char *command, char *cmdDir,
	int *stdinFD, int *stdoutFD, int *stderrFD)
{
    int childStdoutFD, childStdinFD, childStderrFD, pipeFDs[2];
    int dupFD;
    pid_t childPid;
    
    /* Create pipes to communicate with the sub process.  One end of each is
       returned to the caller, the other half is spliced to stdin, stdout
       and stderr in the child process */
    if (pipe(pipeFDs) != 0) {
		PERROR("Internal error (opening stdout pipe)");
        return -1;
    }
    *stdoutFD = pipeFDs[0];
    childStdoutFD = pipeFDs[1];
    if (pipe(pipeFDs) != 0) {
    	PERROR("Internal error (opening stdin pipe)");
        return -1;
    }
    *stdinFD = pipeFDs[1];
    childStdinFD = pipeFDs[0];
    if (stderrFD == NULL)
    	childStderrFD = childStdoutFD;
    else {
	if (pipe(pipeFDs) != 0) {
    	    PERROR("Internal error (opening stderr pipe)");
            return -1;
        }
	*stderrFD = pipeFDs[0];
	childStderrFD = pipeFDs[1];
    }
    
    /* Fork the process */
    childPid = fork();
    
    /*
    ** Child process context (fork returned 0), clean up the
    ** child ends of the pipes and execute the command
    */
    if (0 == childPid) {

		/* Close the connection to the X display in the child. */
		close(ConnectionNumber(TheDisplay));

		/* close the parent end of the pipes in the child process   */
		close(*stdinFD);
		close(*stdoutFD);
		if (stderrFD != NULL)
	    	close(*stderrFD);

		/* close current stdin, stdout, and stderr file descriptors before
		   substituting pipes */
		close(fileno(stdin));
		close(fileno(stdout));
		close(fileno(stderr));

		/* duplicate the child ends of the pipes to have the same numbers
		   as stdout & stderr, so it can substitute for stdout & stderr */
 		dupFD = dup2(childStdinFD, fileno(stdin));
		if (dupFD == -1) {
	    	PERROR("dup of stdin failed");
			return 1;
		}
 		dupFD = dup2(childStdoutFD, fileno(stdout));
		if (dupFD == -1) {
	    	PERROR("dup of stdout failed");
			return 1;
		}
 		dupFD = dup2(childStderrFD, fileno(stderr));
		if (dupFD == -1) {
	    	PERROR("dup of stderr failed");
			return 1;
		}

		/* make this process the leader of a new process group, so the sub
		   processes can be killed, if necessary, with a killpg call */
		setsid();

		/* change the current working directory to the directory of the current
		   file. */ 
		if(cmdDir[0] != 0) {
			if(chdir(cmdDir) == -1) {
				PERROR("chdir to directory of current file failed");
				return 1;
			}
		}

		/* execute the command using the shell specified by preferences */
		execl(GetPrefShell(), GetPrefShell(), "-c", command, 0);

		{
		char buf[10240];
		sprintf(buf, 
			"Error starting preferred shell: %s\nUsing /bin/sh as a last resort...",
			GetPrefShell());
		PERROR(buf);
		}

		/* if that failed try /bin/sh as a last resort. */
		execl("/bin/sh", "/bin/sh", "-c", command, 0);

		/* if we reach here, execl failed */
		{
		char buf[10240];
		sprintf(buf, "Error starting shell: %s", "/bin/sh");
		PERROR(buf);
		}
		exit(1);
    }
    
    /* close the child ends of the pipes */
    close(childStdinFD);
    close(childStdoutFD);
    if (stderrFD != NULL)
    	close(childStderrFD);

    /* Parent process context, check if fork succeeded */
    if (childPid == -1) {
    	DialogF(DF_ERR, parent, 1,
		"Error starting shell command process\n(fork failed)",
		"Dismiss");
	}

    return childPid;
}    
#endif /* !_WIN32 */

/*
** Add a buffer full of output to a buffer list
*/
static void addOutput(buffer **bufList, buffer *buf)
{
    buf->next = *bufList;
    *bufList = buf;
}

/*
** coalesce the contents of a list of buffers into a contiguous memory block,
** freeing the memory occupied by the buffer list.  Returns the memory block
** as the function result, and its length as parameter "length".
*/
static char *coalesceOutput(buffer **bufList, int *outLength)
{
    buffer *buf, *rBufList = NULL;
    char *outBuf, *outPtr, *p;
    int i, length = 0;
    
	/* find the total length of data read */
    for (buf=*bufList; buf!=NULL; buf=buf->next)
    	length += buf->length;
    
    /* allocate contiguous memory for returning data */
    outBuf = XtMalloc(length+1);
    
    /* reverse the buffer list */
    while (*bufList != NULL) {
    	buf = *bufList;
    	*bufList = buf->next;
    	buf->next = rBufList;
    	rBufList = buf;
    }
    
    /* copy the buffers into the output buffer */
    outPtr = outBuf;
    for (buf=rBufList; buf!=NULL; buf=buf->next) {
    	p = buf->contents;
    	for (i=0; i<buf->length; i++)
    	    *outPtr++ = *p++;
    }
    
    /* terminate with a null */
    *outPtr = '\0';

    /* free the buffer list */
    freeBufList(&rBufList);
    
    *outLength = outPtr - outBuf;

    return outBuf;
}

static void freeBufList(buffer **bufList)
{
	buffer *buf;

	while (*bufList != NULL) {
		buf = *bufList;
		*bufList = buf->next;
		destroyBuffer(buf);
	}
}

/*
** Remove trailing newlines from a string by substituting nulls
*/
static void removeTrailingNewlines(char *string)
{
    char *endPtr = &string[strlen(string)-1];
    
    while (endPtr >= string && *endPtr == '\n')
    	*endPtr-- = '\0';
}

/*
** Create a dialog for the output of a shell command.  The dialog lives until
** the user presses the Dismiss button, and is then destroyed
*/
static Widget createOutputDialog(WindowInfo *window, char *text, 
	int dialogStyle)
{
    Arg al[50];
    int ac, rows, cols, hasScrollBar;
    Widget form, textW, button;
    XmString st1;

    /* measure the width and height of the text to determine size for dialog */
    measureText(text, MAX_OUT_DIALOG_COLS, &rows, &cols);
    if (rows > MAX_OUT_DIALOG_ROWS) {
    	rows = MAX_OUT_DIALOG_ROWS;
    	hasScrollBar = True;
    } else
    	hasScrollBar = False;
    if (cols > MAX_OUT_DIALOG_COLS)
    	cols = MAX_OUT_DIALOG_COLS;
    if (cols == 0)
    	cols = 1;
    
    ac = 0;
    XtSetArg(al[ac], XmNdialogStyle, dialogStyle); ac++;
    form = XmCreateFormDialog(window->shell, "shellOutForm", al, ac);

    ac = 0;
    XtSetArg(al[ac], XmNlabelString, st1=MKSTRING("Dismiss")); ac++;
    XtSetArg(al[ac], XmNhighlightThickness, 0);  ac++;
    XtSetArg(al[ac], XmNbottomAttachment, XmATTACH_FORM);  ac++;
    XtSetArg(al[ac], XmNtopAttachment, XmATTACH_NONE);  ac++;
    button = XmCreatePushButtonGadget(form, "dismiss", al, ac);
    XtManageChild(button);
    XtVaSetValues(form, XmNdefaultButton, button, NULL);
    XmStringFree(st1);
    XtAddCallback(button, XmNactivateCallback, destroyOutDialogCB,
    	    XtParent(form));
    
	{
	Widget sw;
    int emTabDist, wrapMargin;
    char *delimiters;
    /* Create a text widget inside of a scrolled window widget */
    sw = XtVaCreateManagedWidget("scrolledW", xmScrolledWindowWidgetClass, form,
    	    XmNspacing, 0, 
#if 0
    	    XmNpaneMaximum, SHRT_MAX,
    	    XmNpaneMinimum, PANE_MIN_HEIGHT, 
#endif
    	    XmNhighlightThickness, 0, 
    	    XmNtopAttachment, XmATTACH_FORM,
    	    XmNleftAttachment, XmATTACH_FORM,
    	    XmNbottomAttachment, XmATTACH_WIDGET,
    	    XmNrightAttachment, XmATTACH_FORM,
    	    XmNbottomWidget, button,
    	    NULL); 
    XtVaGetValues(window->textArea,
    		textNemulateTabs, &emTabDist,
    		textNwrapMargin, &wrapMargin, 
    		textNwordDelimiters, &delimiters, 
    		NULL);
    textW = XtVaCreateManagedWidget("text", textWidgetClass, sw,
    	    textNrows, rows, 
    	    textNcolumns, cols,
    	    textNfont, GetDefaultFontStruct(window->fontList),
    		textNemulateTabs, emTabDist,
    		textNwrapMargin, wrapMargin, 
    		textNwordDelimiters, delimiters, 
    	    textNautoIndent, window->editorInfo->indentStyle == AUTO_INDENT,
    	    textNsmartIndent, window->editorInfo->indentStyle == SMART_INDENT,
    	    textNautoWrap, window->editorInfo->wrapMode == NEWLINE_WRAP,
    	    textNcontinuousWrap, window->editorInfo->wrapMode == CONTINUOUS_WRAP,
    	    textNoverstrike, window->editorInfo->overstrike,
    	    NULL);
    TextSetString(textW, text);
    }
    
    XtVaSetValues(XtParent(form), XmNtitle, "Nedit - Output from Command", NULL);
    ManageDialogCentered(form);
    return form;
}

/*
** Dispose of the command output dialog when user presses Dismiss button
*/
static void destroyOutDialogCB(Widget w, XtPointer callback, XtPointer closure)
{
    XtDestroyWidget((Widget)callback);
}

/*
** Measure the width and height of a string of text.  Assumes 8 character
** tabs.  wrapWidth specifies a number of columns at which text wraps.
*/
static void measureText(char *text, int wrapWidth, int *rows, int *cols)
{
    int maxCols = 0, line = 0, col = 0;
    char *c;
    
    for (c=text; *c!='\0'; c++) {
    	if (*c=='\n' || col > wrapWidth) {
    	    line++;
    	    col = 0;
    	} else {
    	    if (*c == '\t')
    		col += 8 - (col % 8);
    	    else
    		col++;
    	    if (col > maxCols)
    	    	maxCols = col;
    	}
    }
    /* If there is no newline before the end of the string then add
    ** another line for the end of the string.
    */
    if(c[-1] != '\n') line++;
    
    *rows = line;
    *cols = maxCols;
}

/*
** Truncate a string to a maximum of length characters.  If it shortens the
** string, it appends "..." to show that it has been shortened. It assumes
** that the string that it is passed is writeable.
*/
static void truncateString(char *string, unsigned int length)
{
    if (strlen(string) > length)
	memcpy(&string[length-3], "...", 4);
}

/*
** Substitute the string subsStr in inStr wherever % appears, storing the
** result in outStr.  Returns False if the resulting string would be
** longer than outLen
*/
/* .b: argument change */
static int substitutePercent(WindowInfo *window, char *outStr, char *inStr,
        int outLen, char *selection)
/* .e */
{
    char *inChar, *outChar, *c;

    inChar = inStr;
    outChar = outStr;
    while (*inChar != '\0') {
    	/* pass an escaped % to the output */
    	if (*inChar == '\\' && inChar[1] == '%') {
    		inChar++;
    	    *outChar++ = *inChar++;
    	}
    	if (*inChar == '%') {
			/* .b: Replace "%t" and parse "%:message:default_value%" syntax */
			if (*(inChar+1) == '%') { /* %% is % */
    	    	inChar += 2;
    	    	*outChar++ = '%';
    	    } else if (*(inChar+1) == 't') { /* %t is tabDist */
    	        sprintf(outChar, "%d", window->editorInfo->buffer->tabDist);
    	        inChar += 2;
                outChar += strlen(outChar);
    	    } else if (*(inChar+1) == 's') { /* %s is the selection contents */
				if(selection) {
    				strcpy(outChar, selection);
	                outChar += strlen(outChar);
				}
    	        inChar += 2;
    	    } else if (*(inChar+1) == 'd') { /* %d is the directory */
   				strcpy(outChar, window->editorInfo->path);
                outChar += strlen(outChar);
    	        inChar += 2;
    	    } else if (*(inChar+1) == 'f') { /* %f is the basename */
   				strcpy(outChar, window->editorInfo->filename);
                outChar += strlen(outChar);
    	        inChar += 2;
    	    } else if (*(inChar+1) == ':') { /* Ask for arguments */

    		/* Saves outChar position */
    		c = outChar;

                /* Creates dialog_message */
    		for (inChar += 2; *inChar != ':'; inChar++, outChar++) {
    		    if (*inChar == '\0') {
    		        DialogF(DF_ERR, window->shell, 1,
	                       "Syntax error: Separator ':' not found", "OK");
	                return False;
	            } else if (*inChar == '\\') {
	                inChar++;
	            }
	            *outChar = *inChar;
   	    	}
   	    	*outChar++ = ':';
		*outChar++ = '\0';
		
		/* Creates default_value */
    		for (inChar++; *inChar != '%'; inChar++, outChar++) {
    		    if (*inChar == '\0') {
    		        DialogF(DF_ERR, window->shell, 1,
	                       "Syntax error: Final '%%' not found", "OK");
	                return False;
	            } else if (*inChar == '\\') {
	                inChar++;
	            }
	            *outChar = *inChar;
   	    	}
		inChar++;
		*outChar = '\0';

		/* Display request dialog */
		if (DialogF(DF_PROMPT2, window->shell, 2,c,
		            c+strlen(c)+1, c, "OK", "Cancel") == 2)
        	    return False;

    		outChar=outStr+strlen(outStr);
    	    } else {
				/* Substitute the current file name for % in the shell command */
				strcpy(outChar, window->editorInfo->path);
				strcat(outChar, window->editorInfo->filename);
    		inChar++;
				if(*inChar == 'p') inChar++; /* %p is the pathname */
	   			outChar=outStr+strlen(outStr);
    	    }
    	} else
    	    *outChar++ = *inChar++;

    	if (outChar - outStr >= outLen) {
            DialogF(DF_ERR, window->shell, 1,
	"Shell command is too long due to\nfilename substitutions with '%%'",
	            "OK");
    	    return False;
        }
    }
    *outChar = '\0';
    return True;
}
