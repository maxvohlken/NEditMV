
/*******************************************************************************
*									       *
* server_common.h -- Nirvana Editor common server stuff			       *
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

#ifndef _server_common_h_
#define _server_common_h_

/* If anyone knows where to get this from system include files (in a machine
   independent way), please change this (L_cuserid is apparently not ANSI) */
#define MAXUSERNAMELEN 32

/* Lets limit the unique server name to MAXPATHLEN */
#define MAXSERVERNAMELEN MAXPATHLEN

#define DEFAULTSERVERNAME ""

void CreateServerPropertyAtoms(
	char *serverName, 
	Atom *serverExistsAtomReturn, 
	Atom *serverRequestAtomReturn
);
void CreateServerFileOpenAtom(
	char *serverName, 
	char *path,
	Atom *fileOpenAtomReturn
);

#endif
