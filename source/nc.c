/*******************************************************************************
*									       *
* nc.c -- Nirvana Editor client program for nedit server processes	       *
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
#ifdef VMS
#include <lib$routines.h>
#include descrip
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
#ifdef _WIN32
# include <windows.h>
# include <X11/Xw32defs.h>
#endif
#include <Xm/Xm.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#ifdef WIN32
# include <X11/XLIBXTRA.h>
#endif
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include "../util/fileUtils.h"
#include "../util/prefFile.h"
#include "server_common.h"

#define APP_NAME "nc"
#define APP_CLASS "NEditClient"

#ifdef DEBUG
#define PROPERTY_CHANGE_TIMEOUT (1 * 1) /* milliseconds */
#define SERVER_START_TIMEOUT (1 * 1) /* milliseconds */
#define REQUEST_TIMEOUT (1 * 1) /* milliseconds */
#define FILE_OPEN_TIMEOUT (1 * 1) /* milliseconds */
#else
#define PROPERTY_CHANGE_TIMEOUT (10 * 1000) /* milliseconds */
#define SERVER_START_TIMEOUT (30 * 1000) /* milliseconds */
#define REQUEST_TIMEOUT (10 * 1000) /* milliseconds */
#define FILE_OPEN_TIMEOUT (30 * 1000) /* milliseconds */
#endif

static void timeOutProc(Boolean *timeOutReturn, XtIntervalId *id);
static int startServer(char *message, char *commandLine);
static char *parseCommandLine(int argc, char **argv);
static void nextArg(int argc, char **argv, int *argIndex);

Display *TheDisplay;
char *appName = APP_NAME;
char *appClass = APP_CLASS;

static char cmdLineHelp[] =
#ifndef VMS
"Usage:  nc [-read] [-create] [-line n | +n] [-do command] [-[no]ask] \
[-sn|-svrname unique_name] [-wait] [-openwait] [-lm|-logmonitor] [file...]\n";
#else
"";
#endif /*VMS*/

/* Structure to hold X Resource values */
static struct {
    int autoStart;
    char serverCmd[MAXPATHLEN];
    char serverName[MAXSERVERNAMELEN];
    int waitForOpen;
    int waitForClose;
    int logMonitor;
} Preferences;

/* Application resources */
static PrefDescripRec PrefDescrip[] = {
    {"autoStart", "AutoStart", PREF_BOOLEAN, "True",
      &Preferences.autoStart, NULL, True},
    {"serverCommand", "serverCommand", PREF_STRING, "nedit",
      Preferences.serverCmd, (void *)sizeof(Preferences.serverCmd), False},
    {"serverName", "ServerName", PREF_STRING, DEFAULTSERVERNAME,
      Preferences.serverName, (void *)sizeof(Preferences.serverName), False},
    {"waitForOpen", "waitForOpen", PREF_BOOLEAN, "False",
      &Preferences.waitForOpen, NULL, False},
    {"waitForClose", "WaitForClose", PREF_BOOLEAN, "False",
      &Preferences.waitForClose, NULL, False},
    {"logMonitor", "LogMonitor", PREF_BOOLEAN, "False",
      &Preferences.logMonitor, NULL, False},
};

/* Resource related command line options */
static XrmOptionDescRec OpTable[] = {
    {"-ask", ".autoStart", XrmoptionNoArg, (caddr_t)"False"},
    {"-noask", ".autoStart", XrmoptionNoArg, (caddr_t)"True"},
    {"-lm", ".logMonitor", XrmoptionNoArg, (caddr_t)"True"},
    {"-logmonitor", ".logMonitor", XrmoptionNoArg, (caddr_t)"True"},
    {"-sn", ".serverName", XrmoptionSepArg, NULL},
    {"-svrname", ".serverName", XrmoptionSepArg, NULL},
    {"-wait", ".waitForClose", XrmoptionNoArg, (caddr_t)"True"},
    {"-openwait", ".waitForOpen", XrmoptionNoArg, (caddr_t)"True"},
};

typedef enum _OpenStatus { NONE, OPEN, CLOSED } OpenStatus;

/* Struct to hold if a file is still being edited. */
typedef struct _FileOpenStatus {
	Atom fileOpenAtom;
	OpenStatus openStatus;
	struct _FileOpenStatus *next;
} FileOpenStatus;

FileOpenStatus *fileOpenList = 0;

/* Create the file open atom for path and initialize its status structure. */
static void addFileToFileOpenList(char *path) {
	Atom atom;
	FileOpenStatus *item;
	
    /* Create the property atom for path. */
	CreateServerFileOpenAtom(Preferences.serverName, path, &atom);
	
	/* see if the atom already exists in the list */
	for(item = fileOpenList; item; item = item->next) {
		if(item->fileOpenAtom == atom) {
			break;
		}
	}
	
	/* Add the atom to the head of the file open list if it wasn't found. */
	if(item == 0) {
		item = malloc(sizeof(item[0]));
		item->fileOpenAtom = atom;
		item->openStatus = NONE;
		item->next = fileOpenList;
		fileOpenList = item;
	}
}

int main(int argc, char **argv)
{
    XtAppContext context;
    Window rootWindow;
    int length = 0, getFmt;
    char *commandString;
    unsigned char *propValue;
    Atom dummyAtom;
    unsigned long dummyULong, nItems;
    XEvent event;
    XPropertyEvent *e = (XPropertyEvent *)&event;
    Atom serverExistsAtom, serverRequestAtom;
    XrmDatabase prefDB;
    XtIntervalId timerId;
    Boolean requestTimeOut;
    Boolean fileOpenTimeOut;
    Boolean noServer;

#ifdef WIN32
	HCLXtInit();
#endif

    /* Initialize toolkit and get an application context */
    XtToolkitInitialize();
    context = XtCreateApplicationContext();
    
#ifdef VMS
    /* Convert the command line to Unix style */
    ConvertVMSCommandLine(&argc, &argv);
#endif /*VMS*/
    
    /* Read the preferences command line and (very) optional .nc file
       into a database */
    prefDB = CreatePreferencesDatabase(".nc", APP_CLASS, 
	    OpTable, XtNumber(OpTable), (unsigned *)&argc, argv);

    /* Open the display and find the root window */
    TheDisplay = XtOpenDisplay (context, NULL, NULL, APP_CLASS, NULL,
    	    0, &argc, argv);
    if (!TheDisplay) {
	XtWarning ("nc: Can't open display\n");
	return 1;
    }
    XtGetApplicationNameAndClass(TheDisplay, &appName, &appClass);
    rootWindow = RootWindow(TheDisplay, DefaultScreen(TheDisplay));
    
    /* Read the application resources into the Preferences data structure */
    RestorePreferences(prefDB, XtDatabase(TheDisplay), appName,
    	    appClass, PrefDescrip, XtNumber(PrefDescrip));
    	    
    /* Convert command line arguments into a command string for the server */
    commandString = parseCommandLine(argc, argv);
    
    /* Monitor the properties on the root window. 
    ** Also adding a bunch of other event types to make sure
    ** we get other events so we can detect the timeout. */
   	XSelectInput(TheDisplay, rootWindow, PropertyChangeMask | 
   		KeyPressMask | KeyReleaseMask | EnterWindowMask |
   		LeaveWindowMask | PointerMotionMask | ButtonMotionMask |
   		ExposureMask | VisibilityChangeMask | FocusChangeMask);

    /* Create the server property atoms on the current DISPLAY. */
    CreateServerPropertyAtoms(Preferences.serverName, &serverExistsAtom,
    	&serverRequestAtom);
    
    /* See if there is a server already running on the current DISPLAY.
       If not then start one on the current host. If the property exists
       make sure the server is running. */
    noServer = False;
    if (XGetWindowProperty(TheDisplay, rootWindow, serverExistsAtom, 0,
    	    INT_MAX, False, XA_STRING, &dummyAtom, &getFmt, &nItems,
    	    &dummyULong, &propValue) != Success || dummyAtom == None) {
    	noServer = True;
    } else {
    	Boolean propertyChangeTimeOut = False;

    	XFree(propValue);
    	
    	/* Remove the server exists property to make sure the server is
    	** running. If it is running it will get recreated.
    	*/
    	XDeleteProperty(TheDisplay, rootWindow, serverExistsAtom);
    	XSync(TheDisplay, False);
    	timerId = XtAppAddTimeOut(context, PROPERTY_CHANGE_TIMEOUT,
    	    	(XtTimerCallbackProc)timeOutProc, &propertyChangeTimeOut);
    	while (!propertyChangeTimeOut) {
    		/* NOTE: XtAppNextEvent() does call the timeout routine but
    		** doesn't return unless there are more events. See
    		** XSelectInput above. */
        	XtAppNextEvent(context, &event);

        	/* We will get a PropertyNewValue when the server recreates
        	** the server exists atom. */
        	if (e->type == PropertyNotify && e->window == rootWindow && e->atom == serverExistsAtom) {
        		if(e->state == PropertyNewValue) {
        			break;
        		}
            }
    		XtDispatchEvent(&event);
    	}
    	/* Start a new server if the timeout expired. The server exists
    	** property was not recreated. */
    	if(propertyChangeTimeOut) {
#ifdef DEBUG
        	fprintf(stderr, "%s: propertyChangeTimeOut has occurred.\n", appName);
#endif
    		noServer = True;
    	} else {
    		XtRemoveTimeOut(timerId);
    	}
    }
    if(noServer) {
    	Boolean serverStartTimeOut;
    	
    	if(!startServer("No servers available, start one? (y|n)[y]: ", "")) {
        	XtCloseDisplay(TheDisplay);
    		exit(1);
    	}

    	/* Set up a timeout proc in case the server is dead.  The standard
    	   selection timeout is probably a good guess at how long to wait
    	   for this style of inter-client communication as well */
    	timerId = XtAppAddTimeOut(context, SERVER_START_TIMEOUT,
    	    	(XtTimerCallbackProc)timeOutProc, &serverStartTimeOut);

    	/* Wait for the server to start */
    	serverStartTimeOut = False;
    	while (!serverStartTimeOut) {
    		/* NOTE: XtAppNextEvent() does call the timeout routine but
    		** doesn't return unless there are more events. See
    		** XSelectInput above. */
        	XtAppNextEvent(context, &event);

        	/* We will get a PropertyNewValue when the server updates
        	** the server exists atom. If the property is deleted the
        	** the server must have died. */
        	if (e->type == PropertyNotify && e->window == rootWindow && e->atom == serverExistsAtom) {
        		if(e->state == PropertyNewValue) {
        			break;
        		} else if(e->state == PropertyDelete) {
        			fprintf(stderr, "%s: The server failed to start.\n", appName);
        			XtCloseDisplay(TheDisplay);
        			exit(1);
        		}
            }
    		XtDispatchEvent(&event);
    	}
    	/* Exit if the timeout expired. */
    	if(serverStartTimeOut) {
        	fprintf(stderr, "%s: The server failed to start.\n", appName);
        	XtCloseDisplay(TheDisplay);
    		exit(1);
    	} else {
    		XtRemoveTimeOut(timerId);
    	}
    }

    /* Change the NEDIT_SERVER_REQUEST_... property on the root
       window to communicate the request to the server */
    XChangeProperty(TheDisplay, rootWindow, serverRequestAtom, XA_STRING, 8,
    	    PropModeReplace, (unsigned char *)commandString,
    	    strlen(commandString));

    /* Set up a timeout proc in case the server is dead.  The standard
       selection timeout is probably a good guess at how long to wait
       for this style of inter-client communication as well */
    timerId = XtAppAddTimeOut(context, REQUEST_TIMEOUT,
    	    (XtTimerCallbackProc)timeOutProc, &requestTimeOut);

    /* Wait for the property to be deleted to know the request was processed */
    requestTimeOut = False;
    while (!requestTimeOut) {
        XtAppNextEvent(context, &event);
        if (e->type == PropertyNotify && e->window == rootWindow && e->atom == serverRequestAtom &&
        	    e->state == PropertyDelete)
            break;
    	XtDispatchEvent(&event);
    }
    /* Exit if the timeout expired. */
    if(requestTimeOut) {
        fprintf(stderr, "%s: The server did not respond to the request.\n", appName);
        XtCloseDisplay(TheDisplay);
    	exit(1);
    } else {
    	XtRemoveTimeOut(timerId);
    }

    /* Set up a timeout proc so we don't wait forever if the server is dead.
       The standard selection timeout is probably a good guess at how
       long to wait for this style of inter-client communication as
       well */
    if(Preferences.waitForOpen) {
    	timerId = XtAppAddTimeOut(context, FILE_OPEN_TIMEOUT, 
    		(XtTimerCallbackProc)timeOutProc, &fileOpenTimeOut);
    }

    /* Wait for all of the windows to be closed if -wait was supplied */
    fileOpenTimeOut = False;
    while (Preferences.waitForOpen || Preferences.waitForClose) {
    	FileOpenStatus *item;

        XtAppNextEvent(context, &event);

        /* Update the fileOpenList and check if all of the files have
        ** been closed.
        */
        if(e->type == PropertyNotify && e->window == rootWindow) {
    		Boolean stillOpen = False;
    		Boolean stillToBeOpen = False;
    		
			for(item = fileOpenList; item; item = item->next) {
         		if (e->atom == item->fileOpenAtom) {
         			/* The file open property is deleted when the file is closed */
         			if(e->state == PropertyDelete) {
         				item->openStatus = CLOSED;
         			}
         			/* The file open property is modified when the file is opened */
         			else if(e->state == PropertyNewValue) {
         	        	item->openStatus = OPEN;
         	        	
         	        	/* reset the file open time out if we are waiting for open. */
    					if(Preferences.waitForOpen) {
    						XtRemoveTimeOut(timerId);
    						timerId = XtAppAddTimeOut(context, FILE_OPEN_TIMEOUT, 
    							(XtTimerCallbackProc)timeOutProc, &fileOpenTimeOut);
    					}
         			}
       			}
       			if(item->openStatus == OPEN) {
       				stillOpen = True;
       			} else if(item->openStatus == NONE && !fileOpenTimeOut) {
       				stillToBeOpen = True;
       			}
       		}
       		/* we are finished if all of the files are open */
       		if(Preferences.waitForOpen && !Preferences.waitForClose && !stillToBeOpen) {
       			break;
       		}
       		/* we are finished if there are no open files */
       		if(Preferences.waitForClose && !stillOpen && !stillToBeOpen) {
       			break;
       		}
       	}
       	
       	/* we are finished if we are only waiting for the files to open and
       	** the file open timeout has expired. */
       	if(Preferences.waitForOpen && !Preferences.waitForClose && fileOpenTimeOut) {
#ifdef DEBUG
        	fprintf(stderr, "%s: fileOpenTimeOut has occurred.\n", appName);
#endif
       		break;
       	}
       	
    	XtDispatchEvent(&event);
    }
    XtFree(commandString);
    XtCloseDisplay(TheDisplay);
    exit(0);
    return 0;
}


/*
** Xt timer procedure for timeouts on NEdit server startup
*/
static void timeOutProc(Boolean *timeOutReturn, XtIntervalId *id)
{
	/* Flag that the timeout has occurred. */
    *timeOutReturn = True;
}

/*
** Prompt the user about starting a server, with "message", then start server
*/
static int startServer(char *message, char *commandLineArgs)
{
    char *commandLine;
    char *display_string;
#ifdef VMS
    int spawnFlags = 1 /* + 1<<3 */;			/* NOWAIT, NOKEYPAD */
    int spawn_sts;
    struct dsc$descriptor_s *cmdDesc;
    char *nulDev = "NL:";
    struct dsc$descriptor_s *nulDevDesc;
#endif /*VMS*/
    
    /* prompt user whether to start server */
#ifndef _WIN32
	{
	char c;
    if (!Preferences.autoStart) {
	printf(message);
	do {
    	    c = getc(stdin);
	} while (c == ' ' || c == '\t');
	if (c != 'Y' && c != 'y' && c != '\n')
    	    return False;
    }
	} /* scope */
#endif
    
    /* get the display value to pass to nedit */
    display_string = DisplayString(TheDisplay);
    
    /* start the server */
    commandLine = XtMalloc(
    	    strlen(Preferences.serverCmd) + 
    	    strlen(" -server -display ") +  
    	    strlen(display_string) + 2 +
    	    strlen(" -svrname ") +  
    	    strlen(Preferences.serverName) + 2 +
    	    strlen(commandLineArgs) + 
			(Preferences.logMonitor ? strlen(" -logmonitor ") : 0) +
    	    512/* some extra space to cover the whitespace and other chars
    	          we may add in the future to the command in the sprintf 
    	          below */
    	    );
#ifdef VMS
    sprintf(commandLine, "%s -server -display \"%s\" -svrname \"%s\"%s %s", 
		Preferences.serverCmd, display_string, Preferences.serverName, 
		(Preferences.logMonitor ? " -logmonitor" : ""), commandLineArgs);
    cmdDesc = NulStrToDesc(commandLine);	/* build command descriptor */
    nulDevDesc = NulStrToDesc(nulDev);		/* build "NL:" descriptor */
    spawn_sts = lib$spawn(cmdDesc, nulDevDesc, 0, &spawnFlags, 0,0,0,0,0,0,0,0);
    if (spawn_sts != SS$_NORMAL) {
	fprintf(stderr, "Error return from lib$spawn: %d\n", spawn_sts);
	fprintf(stderr, "Nedit server not started.\n");
	return False;
    }
    FreeStrDesc(cmdDesc);
#else
#ifdef _WIN32
    sprintf(commandLine, "-server -display \"%s\" -svrname \"%s\"%s %s", 
		display_string, Preferences.serverName, 
		(Preferences.logMonitor ? " -logmonitor" : ""), commandLineArgs);
	{SHELLEXECUTEINFO sei[1];
	memset(sei, 0, sizeof(*sei));
	sei->cbSize = sizeof(*sei);
	sei->fMask = SEE_MASK_DOENVSUBST | SEE_MASK_NOCLOSEPROCESS ;
	sei->hwnd = NULL;
	sei->lpVerb = "open";
	sei->lpFile = Preferences.serverCmd;
	sei->lpParameters = commandLine;
	sei->lpDirectory = NULL;
	sei->nShow = SW_HIDE;
	ShellExecuteEx(sei);
	}
	_fcloseall();
#else
    sprintf(commandLine, "%s -server -display \"%s\" -svrname \"%s\"%s %s", 
		Preferences.serverCmd, display_string, Preferences.serverName, 
		(Preferences.logMonitor ? " -logmonitor" : ""), commandLineArgs);
    system(commandLine);
#endif
#endif
    XtFree(commandLine);
    return True;
}

/*
** Converts command line into a command string suitable for passing to
** the server
*/
static char *parseCommandLine(int argc, char **argv)
{
    char name[MAXPATHLEN], path[MAXPATHLEN];
    char *toDoCommand = "";
    char *commandString, *outPtr;
    int lineNum = 0, read = 0, create = 0, length = 0;
    int i, lineArg, nRead, charsWritten;
    
    /* Allocate a string for output, for the maximum possible length.  The
       maximum length is calculated by assuming every argument is a file,
       and a complete record of maximum length is created for it */
    for (i=1; i<argc; i++)
    	length += 38 + strlen(argv[i]) + MAXPATHLEN;
    commandString = XtMalloc(length+1);
    
    /* Parse the arguments and write the output string */
    outPtr = commandString;
    for (i=1; i<argc; i++) {
    	if (!strcmp(argv[i], "-do")) {
    	    nextArg(argc, argv, &i);
    	    toDoCommand = argv[i];
    	} else if (!strcmp(argv[i], "-read")) {
    	    read = 1;
    	} else if (!strcmp(argv[i], "-create")) {
    	    create = 1;
    	} else if (!strcmp(argv[i], "-line")) {
    	    nextArg(argc, argv, &i);
			nRead = sscanf(argv[i], "%d", &lineArg);
			if (nRead != 1)
				fprintf(stderr, "%s: argument to line should be a number\n", appName);
    	    else
    	    	lineNum = lineArg;
    	} else if (*argv[i] == '+') {
    	    nRead = sscanf((argv[i]+1), "%d", &lineArg);
			if (nRead != 1)
				fprintf(stderr, "%s: argument to + should be a number\n", appName);
    	    else
    	    	lineNum = lineArg;
    	} else if (*argv[i] == '-') {
#ifdef VMS
	    *argv[i] = '/';
#endif /*VMS*/
    	    fprintf(stderr, "%s: Unrecognized option %s\n%s", appName, argv[i],
    	    	    cmdLineHelp);
        	XtCloseDisplay(TheDisplay);
    	    exit(1);
    	} else {
#ifdef VMS
	    int numFiles, j, oldLength;
	    char **nameList = NULL, *newCommandString;
	    /* Use VMS's LIB$FILESCAN for filename in argv[i] to process */
	    /* wildcards and to obtain a full VMS file specification     */
	    numFiles = VMSFileScan(argv[i], &nameList, NULL, INCLUDE_FNF);
	    /* for each expanded file name do: */
	    for (j = 0; j < numFiles; ++j) {
	    	oldLength = outPtr-commandString;
	    	newCommandString = XtMalloc(oldLength+length+1);
	    	strncpy(newCommandString, commandString, oldLength);
	    	XtFree(commandString);
	    	commandString = newCommandString;
	    	outPtr = newCommandString + oldLength;
	    	ParseFilename(nameList[j], name, path);
    		strcat(path, name);
    		sprintf(outPtr, "%d %d %d %d %d %d\n%s\n%s\n%n", lineNum, read,
    			create, strlen(path), strlen(toDoCommand), Preferences.logMonitor, path,
    			toDoCommand, &charsWritten);
    		outPtr += charsWritten;
    		XtFree(nameList[j]);
    		
    		/* Create the file open atoms for the paths supplied */
    		addFileToFileOpenList(path);
	    }
	    if (nameList != NULL)
	    	XtFree(nameList);
#else
    	    ParseFilename(argv[i], name, path);
    	    strcat(path, name);
    	    /* SunOS acc or acc and/or its runtime library has a bug
    	       such that %n fails (segv) if it follows a string in a
    	       printf or sprintf.  The silly code below avoids this */
    	    sprintf(outPtr, "%d %d %d %d %d %d\n%n", lineNum, read, create,
    	    	    strlen(path), strlen(toDoCommand), Preferences.logMonitor, &charsWritten);
    	    outPtr += charsWritten;
    	    strcpy(outPtr, path);
    	    outPtr += strlen(path);
    	    *outPtr++ = '\n';
    	    strcpy(outPtr, toDoCommand);
    	    outPtr += strlen(toDoCommand);
    	    *outPtr++ = '\n';
			toDoCommand = "";
    		
    		/* Create the file open atoms for the paths supplied */
    		addFileToFileOpenList(path);
#endif
    	}
    }
#ifdef VMS
    VMSFileScanDone();
#endif /*VMS*/
    
    /* If ther's an un-written -do command, write it with an empty file name */
    if (toDoCommand[0] != '\0') {
    	sprintf(outPtr, "0 0 0 0 %d 0\n\n%n", strlen(toDoCommand), &charsWritten);
    	outPtr += charsWritten;
    	strcpy(outPtr, toDoCommand);
    	outPtr += strlen(toDoCommand);
    	*outPtr++ = '\n';
    }
    
    *outPtr = '\0';
    return commandString;
}

static void nextArg(int argc, char **argv, int *argIndex)
{
    if (*argIndex + 1 >= argc) {
#ifdef VMS
	    *argv[*argIndex] = '/';
#endif /*VMS*/
    	fprintf(stderr, "NEdit: %s requires an argument\n%s", argv[*argIndex],
    	        cmdLineHelp);
        XtCloseDisplay(TheDisplay);
    	exit(1);
    }
    (*argIndex)++;
}
