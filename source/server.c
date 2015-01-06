/*******************************************************************************
*									       *
* server.c -- Nirvana Editor edit-server component			       *
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
* November, 1995							       *
*									       *
* Written by Mark Edel							       *
*									       *
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef HAVE_LIMITS_H
# include <limits.h>
#endif
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef VMS
#include <lib$routines.h>
#include ssdef
#include syidef
#include "../util/VMSparam.h"
#include "../util/VMSutils.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#endif
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include "../util/fileUtils.h"
#include "../util/misc.h"
#include "textBuf.h"
#include "nedit.h"
#include "window.h"
#include "file.h"
#include "selection.h"
#include "macro.h"
#include "menu.h"
#include "server_common.h"
#include "server.h"
#include "preferences.h"

static void processServerCommand(void);
static void cleanUpServerCommunication(void);
static void processServerCommandString(char *string);

static Atom ServerRequestAtom = 0;
static Atom ServerExistsAtom = 0;

/*
** Make NEdit a daemon/background process.
*/
int DaemonInit(void)
{
#ifndef _WIN32
	pid_t pid;
	int fd;
	
	if((pid = fork()) < 0) {
		return(-1);
	} else if(pid != 0) {
		exit(0); /* parent exits */
	}
	
	/* The child should do the following:
	** - Become the session leader which sets the process group ID
	**   and session ID to our process ID, and releases the process's
	**   controlling terminal.
	*/
	setsid();
	
	/* Attach stdin, stderr, stdout to /dev/null so rsh won't hang
	** because we had stdout/stderr open back to it.
	*/
	fd = open("/dev/null", O_RDWR);
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	close(fd);
#endif

	return(0);
}

/*
** Set up inter-client communication for NEdit server end, expected to be
** called only once at startup time
*/
void InitServerCommunication(char *serverName)
{
    Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));

    /* Create the server property atoms on the current DISPLAY. */
    CreateServerPropertyAtoms(serverName, &ServerExistsAtom,
    	&ServerRequestAtom);
    
    /* Create the server-exists property on the root window to tell clients
       whether to try a request (otherwise clients would always have to
       try and wait for their timeouts to expire) */
    XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom, XA_STRING, 8,
    	    PropModeReplace, (unsigned char *)"Running", 7);
    
    /* Set up exit handler for cleaning up server-exists property */
#ifdef HAVE_ATEXIT
    atexit(cleanUpServerCommunication);
#else
# ifdef HAVE_ON_EXIT
    on_exit(cleanUpServerCommunication, 0);
# endif
#endif
    
    /* Pay attention to PropertyChangeNotify events on the root window */
    XSelectInput(TheDisplay, rootWindow, PropertyChangeMask);
}

/*
** Exit handler.  Removes server-exists property on root window
*/
static void cleanUpServerCommunication(void)
{
	WindowInfo *w;
	
    /* Delete the server-exists property from the root window (if it was
       assigned) and don't let the process exit until the X server has
       processed the delete request (otherwise it won't be done) */
    if (ServerExistsAtom != 0) {
	XDeleteProperty(TheDisplay, RootWindow(TheDisplay,
		DefaultScreen(TheDisplay)), ServerExistsAtom);
    }
    
    /* Delete any file open properties that still exist */
    for(w = WindowList; w; w = w->next) {
		if(w->editorInfo->fileOpenAtom != None) {
			XDeleteProperty(TheDisplay, RootWindow(TheDisplay,
				DefaultScreen(TheDisplay)), w->editorInfo->fileOpenAtom);
			w->editorInfo->fileOpenAtom = None;
		}
	}
	XSync(TheDisplay, True);
}

/*
** Special event loop for NEdit servers.  Processes PropertyNotify events on
** the root window (this would not be necessary if it were possible to
** register an Xt event-handler for a window, rather than only a widget).
** Invokes server routines when a server-request property appears,
** re-establishes server-exists property when another server exits and
** this server is still alive to take over.
*/
void ServerMainLoop(XtAppContext context)
{
   	while (TRUE) {
   		XEvent event;
       	XtAppNextEvent(context, &event);
   		ServerDispatchEvent(&event);
   	}
}

Boolean ServerDispatchEvent(XEvent *event)
{
	if(isServer) {
		XPropertyEvent *e = (XPropertyEvent *)event;
    	Window rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
    	if (e->type == PropertyNotify && e->window == rootWindow) {

        	if (e->atom == ServerRequestAtom && e->state == PropertyNewValue)
            	processServerCommand();
        	else if (e->atom == ServerExistsAtom && e->state == PropertyDelete)
            	 XChangeProperty(TheDisplay, rootWindow, ServerExistsAtom,
            		 XA_STRING, 8, PropModeReplace, (unsigned char *)"Running", 7);
        	else if(e->state == PropertyDelete) {
				WindowInfo *w;
    			/* If the property for an open window was deleted then recreate it. */
    			for(w = WindowList; w; w = w->next) {
					if(w->editorInfo->fileOpenAtom == e->atom) {
						XChangeProperty(TheDisplay, rootWindow, w->editorInfo->fileOpenAtom,
						XA_STRING, 8, PropModeReplace, (unsigned char *)"Open", 4);
					}
				}
        	}
		}
    }
    return XtDispatchEvent(event);
}

/*
** Update the file open property on the root window to inform nc that
** the file is open. 
*/
void CreateFileOpenProperty(WindowInfo *window) {
    char path[MAXPATHLEN];
    
    if(!window->editorInfo->filenameSet) {
    	/* If the filename is not set then make sure the atom has been
    	** destroyed.
    	*/
    	DestroyFileOpenProperty(window);
    	return;
    }
    
    strcpy(path, window->editorInfo->path);
    strcat(path, window->editorInfo->filename);
    
    /* Get the property atom for path. */
    if(window->editorInfo->fileOpenAtom == None) {
		CreateServerFileOpenAtom(GetPrefServerName(), path, &window->editorInfo->fileOpenAtom);
	}
 
    /* Create the file open property on the root window to communicate
       to nc that the file has been opened. */
    if(window->editorInfo->fileOpenAtom != None) {
    	XChangeProperty(TheDisplay, RootWindow(TheDisplay,
			DefaultScreen(TheDisplay)), window->editorInfo->fileOpenAtom, XA_STRING, 8,
    	    PropModeReplace, (unsigned char *)"Open", 4);
    }
}

void DestroyFileOpenProperty(WindowInfo *window) {
	/* destroy the file open atom to inform nc that this file has
	** been closed.
	*/
    if(window->editorInfo->fileOpenAtom != None) {
		XDeleteProperty(TheDisplay, RootWindow(TheDisplay,
			DefaultScreen(TheDisplay)), window->editorInfo->fileOpenAtom);
		XSync(TheDisplay, False);
		window->editorInfo->fileOpenAtom = None;
	}    
}

static void processServerCommand(void)
{
    Atom dummyAtom;
    unsigned long nItems, dummyULong;
    unsigned char *propValue;
    int getFmt;

    /* Get the value of the property, and delete it from the root window */
    if (XGetWindowProperty(TheDisplay, RootWindow(TheDisplay,
    	    DefaultScreen(TheDisplay)), ServerRequestAtom, 0, INT_MAX, True,
    	    XA_STRING, &dummyAtom, &getFmt, &nItems, &dummyULong, &propValue)
    	    != Success || getFmt != 8)
    	return;
    
    /* Invoke the command line processor on the string to process the request */
    processServerCommandString((char *)propValue);
    XFree(propValue);
}

static void processServerCommandString(char *string)
{
    char *fullname, filename[MAXPATHLEN], pathname[MAXPATHLEN];
    char *doCommand, *inPtr;
    int editFlags, stringLen = strlen(string);
    int lineNum, createFlag, readFlag, fileLen, doLen, charsRead, itemsRead;
    WindowInfo *window = 0;
	int logMonitorFlag = False;

    /* If the command string is empty, put up an empty, Untitled window
       (or just pop one up if it already exists) */
    if (string[0] == '\0') {
    	for (window=WindowList; window!=NULL; window=window->next)
    	    if (!window->editorInfo->filenameSet && !window->editorInfo->fileChanged)
    	    	break;
    	if (window == NULL) {
    	    EditNewFile(NULL, True);
    	    CheckCloseDim();
    	} else {
    	    RaiseShellWindow(window->shell);
		}
    	return;
    }

    /*
    ** Loop over all of the files in the command list
    */
    inPtr = string;
    while (TRUE) {
	
	if (*inPtr == '\0')
	    break;
	    
	/* Read a server command from the input string.  Header contains:
	   linenum createFlag fileLen doLen\n, followed by a filename and -do
	   command both followed by newlines.  This bit of code reads the
	   header, and converts the newlines following the filename and do
	   command to nulls to terminate the filename and doCommand strings */
	itemsRead = sscanf(inPtr, "%d %d %d %d %d%n", &lineNum, &readFlag,
    		&createFlag, &fileLen, &doLen, &charsRead);
	if (itemsRead != 5)
    	    goto readError;
	inPtr += charsRead + 1;
	if(*inPtr != '\n') {
		itemsRead = sscanf(inPtr, "%d%n", &logMonitorFlag, &charsRead);
		if(itemsRead == 1) {
			inPtr += charsRead + 1;
		}
	}
	if (inPtr - string + fileLen > stringLen)
	    goto readError;
	fullname = inPtr;
	inPtr += fileLen;
	*inPtr++ = '\0';
	if (inPtr - string + doLen > stringLen)
	    goto readError;
	doCommand = inPtr;
	inPtr += doLen;
	*inPtr++ = '\0';
	
	/* An empty file name means: choose a random window for
	   executing the -do macro upon */
	if (fileLen <= 0) {
	    if (*doCommand != '\0')
	    	DoMacro(WindowList, doCommand, "-do macro");
	    CheckCloseDim();
	    return;
	}
	
	/* Process the filename by looking for the files in an
	   existing window, or opening if they don't exist */
	editFlags = (readFlag ? FORCE_READ_ONLY : 0) | CREATE |
		(createFlag ? SUPPRESS_CREATE_WARN : 0);
	ParseFilename(fullname, filename, pathname);
	window = EditExistingFile(WindowList, filename, pathname, editFlags, True);
	if (window == NULL) {
	    continue;
	}
	if(logMonitorFlag) {
		SetCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
	}
	/* See if the iconic resource is set. If it is not then force the
	** window to the top. */
	{Boolean iconic;
	XtVaGetValues(window->shell, XmNiconic, &iconic, 0);
	if(!iconic) {
    	RaiseShellWindow(window->shell);
	}
	} /*scope*/
	
	/* Do the actions requested (note DoMacro is last, since the do
	   command can do anything, including closing the window!) */
	if (lineNum > 0)
	    SelectNumberedLine(window, lineNum);
	if (*doCommand != '\0')
	    DoMacro(window, doCommand, "-do macro");
   	
   	UpdateWindowReadOnly(window);
   	UpdateWindowTitle(window);
    }
    CheckCloseDim();
    return;

readError:
    fprintf(stderr, "NEdit: error processing server request\n");
    return;
}



