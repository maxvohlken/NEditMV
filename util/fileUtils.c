/*******************************************************************************
*									       *
* fileUtils.c -- File utilities for Nirvana applications		       *
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
* July 28, 1992								       *
*									       *
* Written by Mark Edel							       *
*									       *
* Modified by:	DMR - Ported to VMS (1st stage for Histo-Scope)		       *
*									       *
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <string.h>
#ifdef VAXC
#define NULL (void *) 0
#endif /*VAXC*/
#ifdef VMS
#include "vmsparam.h"
#include <stat.h>
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <sys/stat.h>
#ifndef _WIN32
# include <pwd.h>
#else
# include <direct.h>
#endif
#endif /*VMS*/
#include "fileUtils.h"

#define TRUE 1
#define FALSE 0

#ifndef S_ISLNK
# ifdef _WIN32
#  define S_ISLNK(mode)   (FALSE)
# else
#  define S_ISLNK(mode)   (((mode)&0xF000) == S_IFLNK)
# endif
#endif

static int normalizePathname(char *pathname);
static int compressPathname(char *pathname);
static char *nextSlash(char *ptr);
static char *prevSlash(char *ptr);
static int compareThruSlash(char *string1, char *string2);
static void copyThruSlash(char **toString, char **fromString);

/*
** Decompose a Unix file name into a file name and a path
*/
int ParseFilename(char *fullname, char *filename, char *pathname)
{
#ifdef _WIN32
	char tmp[_MAX_PATH];
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	char *vep;
	char viewExtendPath[_MAX_PATH] = "";

	/* Check for clearcase version extended paths */
	if(  (vep = strstr(fullname, "@@/")) != NULL 
	  || (vep = strstr(fullname, "@@\\")) != NULL) {
		strcpy(viewExtendPath, vep);
		*vep = 0;
	}
	else {
	}

	if(_fullpath(tmp, fullname, _MAX_PATH) == NULL) {
		return FALSE;
	}
	_splitpath(tmp, drive, dir, fname, ext);
	_makepath(filename, NULL, NULL, fname, ext);
	_makepath(pathname, drive, dir, NULL, NULL);
	
	/* Add the clearcase version extended path to the filename. */
	strcat(filename, viewExtendPath);
	
	return TRUE;
#else /* !defined(_WIN32) */
    int fullLen = strlen(fullname);
    int i, pathLen, fileLen;
	char *viewExtendPath;
	int scanStart;
	    
#ifdef VMS
    /* find the last ] or : */
    for (i=fullLen-1; i>=0; i--) {
    	if (fullname[i] == ']' || fullname[i] == ':')
	    break;
    }
#else  /* UNIX */
	/* Check for clearcase version extended paths */
	if((  viewExtendPath = strstr(fullname, "@@/")) != NULL 
	  || (viewExtendPath = strstr(fullname, "@@\\")) != NULL) {
		/* find the last slash before the '@@/' */
		scanStart = viewExtendPath-fullname-1;
	}
	else {
		scanStart = fullLen-1;
	}
    /* find the last slash */
    for (i=scanStart; i>=0; i--) {
    	if (fullname[i] == '/'
#ifdef _WIN32
		|| fullname[i] == '\\'
#endif
		)
	    break;
    }
#endif

    /* move chars before / (or ] or :) into pathname,& after into filename */
    pathLen = i + 1;
    fileLen = fullLen - pathLen;
    strncpy(pathname, fullname, pathLen);
    pathname[pathLen] = 0;
    strncpy(filename, &fullname[pathLen], fileLen);
    filename[fileLen] = 0;
#ifdef VMS
    return TRUE;
#else     /* UNIX specific... Modify at a later date for VMS */
    return normalizePathname(pathname);
#endif
#endif
}

#ifndef VMS

/*
** Expand tilde characters which begin file names as done by the shell
*/
int ExpandTilde(char *pathname)
{
#ifndef _WIN32
	struct passwd *passwdEntry;
    char username[MAXPATHLEN], temp[MAXPATHLEN], *nameEnd;
    
    if (pathname[0] != '~')
	return TRUE;
    nameEnd = strchr(&pathname[1], '/');
    if (nameEnd == NULL)
	nameEnd = pathname + strlen(pathname);
    strncpy(username, &pathname[1], nameEnd - &pathname[1]);
    username[nameEnd - &pathname[1]] = '\0';
    if (username[0] == '\0')
    	passwdEntry = getpwuid(getuid());
    else
    	passwdEntry = getpwnam(username);
    if (passwdEntry == NULL)
	return FALSE;
    sprintf(temp, "%s%s", passwdEntry->pw_dir, nameEnd);
#ifdef DEBUG
    printf("expanded path name to \"%s\"\n", temp);
#endif
    strcpy(pathname, temp);
#endif
    return TRUE;
}
    
static int normalizePathname(char *pathname)
{
    char oldPathname[MAXPATHLEN+1], wd[MAXPATHLEN+1];

    /* if this is a relative pathname, prepend current directory */
	if (!(pathname[0] == '/')) {
		/* make a copy of pathname to work from */
		strcpy(oldPathname, pathname);
		/* Terminate the buffer so we can use strcat */
		pathname[0] = 0;
		/* get the working directory */
		getcwd(wd, MAXPATHLEN);
		/* remove /tmp_mnt from the getcwd() result */
		if(strncmp(wd, "/tmp_mnt/", 9) == 0)
			strcat(pathname, wd + 8);
		else
			/* prepend it to the path */
			strcat(pathname, wd);
		strcat(pathname, "/");
		strcat(pathname, oldPathname);
    }
    /* compress out .. and . */
    return compressPathname(pathname);
}


static int compressPathname(char *pathname)
{
	char buf[MAXPATHLEN+1];
	char *inPtr, *outPtr;
	struct stat statbuf;

	/* compress out . and .. */
	inPtr = pathname;
	outPtr = buf;
	/* copy initial / */
	copyThruSlash(&outPtr, &inPtr);
	while (inPtr != NULL) {
	 	/* if the next component is "../", remove previous component */
		if (compareThruSlash(inPtr, "../")) {
			*outPtr = 0;
			/* Just copy the ../ if at the beginning the filesystem will take
			   care of it. And Just copy the ../ if the previous component
			   is a symbolic link. It is not valid to compress
		      ../ when the previous component is a symbolic link because
		      ../ is not relative to the previous component anymore. It instead
		      relative to where the link points. */
#ifdef _WIN32
			if(outPtr-1 == buf) {
#else
		    if(outPtr-1 == buf || (lstat(buf, &statbuf) == 0 && S_ISLNK(statbuf.st_mode))) {
#endif
				copyThruSlash(&outPtr, &inPtr);
			} else {
				/* back up outPtr to remove last path name component */
				outPtr = prevSlash(outPtr);
				inPtr = nextSlash(inPtr);
			}
		} else if (compareThruSlash(inPtr, "./")) {
			/* don't copy the component if it's the redundant "./" */
			inPtr = nextSlash(inPtr);
		} else {
			/* copy the component to outPtr */
			copyThruSlash(&outPtr, &inPtr);
		}
	}
	/* updated pathname with the new value */
	strcpy(pathname, buf);
	return TRUE;
}

static char *nextSlash(char *ptr)
{
    for(; *ptr!='/'
#ifdef _WIN32
	&& *ptr!='\\'
#endif
	; ptr++) {
    	if (*ptr == '\0')
	    return NULL;
    }
    return ptr + 1;
}

static char *prevSlash(char *ptr)
{
    for(ptr -= 2; *ptr!='/'
#ifdef _WIN32
	&& *ptr!='\\'
#endif
	; ptr--);
    return ptr + 1;
}

static int compareThruSlash(char *string1, char *string2)
{
    while (TRUE) {
    	if (*string1 != *string2)
	    return FALSE;
	if (*string1 =='\0' || *string1=='/'
#ifdef _WIN32
	|| *string1=='\\'
#endif
	)
	    return TRUE;
	string1++;
	string2++;
    }
}

static void copyThruSlash(char **toString, char **fromString)
{
    char *to = *toString;
    char *from = *fromString;
    
    while (TRUE) {
        *to = *from;
        if (*from =='\0') {
            *fromString = NULL;
            return;
        }
	if (*from=='/'
#ifdef _WIN32
	|| *from=='\\'
#endif
	) {
	    *toString = to + 1;
	    *fromString = from + 1;
	    return;
	}
	from++;
	to++;
    }
}
#endif /* UNIX */
