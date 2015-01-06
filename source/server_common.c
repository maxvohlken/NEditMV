/*******************************************************************************
*									       *
* server_common.c -- Nirvana Editor common server stuff			       *
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
* September, 1996								       *
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
#ifndef _WIN32
# include <pwd.h>
#endif
#endif
#include <Xm/Xm.h>
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "nedit.h"
#include "clearcase.h"
#include "server_common.h"

/*
** Return a pointer to the username of the real user in a statically
** allocated string. If we can not get the user name from the
** password info then return the real uid as a static string.
*/
static char *getUserName(void)
{
#ifdef VMS
    return cuserid(NULL);
#elif defined(_WIN32)
	return "xxxx";
#else
    struct passwd *passwdEntry;
    static char name[16] = "";
    
    passwdEntry = getpwuid(getuid());
    if (passwdEntry == NULL) {
    	sprintf(name, "%05d", (unsigned int)getuid());
    }
    else {
    	strncpy(name, passwdEntry->pw_name, sizeof(name));
		name[sizeof(name)-1] = 0;
    }
    return name;
#endif
}

/*
 * Create the server property atoms for the server with serverName.
 * Atom names are generated as follows:
 * 
 * NEDIT_SERVER_EXISTS_<server_name>_<user>[_<clearcase_view_tag>]
 * NEDIT_SERVER_REQUEST_<server_name>_<user>[_<clearcase_view_tag>]
 * 
 * <server_name> is the name that can be set by the user to allow
 * for multiple servers to run on the same display. <server_name>
 * defaults to "" if not supplied by the user.
 * 
 * <user> is the user name of the current user.
 * 
 * <clearcase_view_tag> is the clearcase view tag attached to the
 * current process.
 */
void CreateServerPropertyAtoms(
	char *serverName, 
	Atom *serverExistsAtomReturn, 
	Atom *serverRequestAtomReturn
) {
    char propName[24+MAXSERVERNAMELEN+MAXUSERNAMELEN+MAXPATHLEN+MAXCCASEVIEWTAGLEN];
    char *userName;
    char *ccaseViewTag;

    userName = getUserName();
    ccaseViewTag = GetClearCaseViewTag();
    if(ccaseViewTag && FILECOMPARE(ccaseViewTag, "") != 0) {
    	sprintf(propName, "NEDIT_SERVER_EXISTS_%s_%s_%s", serverName, userName, ccaseViewTag);
    	*serverExistsAtomReturn = XInternAtom(TheDisplay, propName, False);
    	sprintf(propName, "NEDIT_SERVER_REQUEST_%s_%s_%s", serverName, userName, ccaseViewTag);
    	*serverRequestAtomReturn = XInternAtom(TheDisplay, propName, False);
    }
    else {
    	sprintf(propName, "NEDIT_SERVER_EXISTS_%s_%s", serverName, userName);
    	*serverExistsAtomReturn = XInternAtom(TheDisplay, propName, False);
    	sprintf(propName, "NEDIT_SERVER_REQUEST_%s_%s", serverName, userName);
    	*serverRequestAtomReturn = XInternAtom(TheDisplay, propName, False);
    }
    return;
}

/*
 * Create the individual property atoms for each file being
 * opened by the server with serverName. This atom is used
 * by nc to monitor if the file has been closed.
 *
 * Atom names are generated as follows:
 * 
 * NEDIT_FILE_<server_name>_<user>[_<clearcase_view_tag>]_<path>
 * 
 * <server_name> is the name that can be set by the user to allow
 * for multiple servers to run on the same display. <server_name>
 * defaults to "" if not supplied by the user.
 * 
 * <user> is the user name of the current user.
 * 
 * <clearcase_view_tag> is the clearcase view tag attached to the
 * current process.
 * 
 * <path> is the path of the file being edited.
 */
void CreateServerFileOpenAtom(
	char *serverName, 
	char *path,
	Atom *fileOpenAtomReturn
) {
    char propName[18+MAXSERVERNAMELEN+1+MAXUSERNAMELEN+1+MAXCCASEVIEWTAGLEN+1+MAXPATHLEN+1];
    char *userName;
    char *ccaseViewTag;

#ifdef _WIN32
	char pathbuf[MAXPATHLEN+1];
	char *p;

	/* Make the path all lowercase on WIN32 so different cased files are equivalent. */
	strcpy(pathbuf, path);
	for(p = pathbuf; *p; p++) {
		*p = tolower(*p);
	}
	path = pathbuf;
#endif

    userName = getUserName();
    ccaseViewTag = GetClearCaseViewTag();
    if(ccaseViewTag && FILECOMPARE(ccaseViewTag, "") != 0) {
    	sprintf(propName, "NEDIT_SERVER_FILE_%s_%s_%s_%s", serverName, userName, ccaseViewTag, path);
    }
    else {
    	sprintf(propName, "NEDIT_SERVER_FILE_%s_%s_%s", serverName, userName, path);
    }
    *fileOpenAtomReturn = XInternAtom(TheDisplay, propName, False);
    return;
}
