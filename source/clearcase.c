/*******************************************************************************
*									       *
* clearcase.c -- Nirvana Editor clearcase support					       *
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
#include <string.h>
#include <stdlib.h>
#include "clearcase.h"

char *GetClearCaseViewTag(void)
{
	static char viewTagBuffer[MAXCCASEVIEWTAGLEN] = "";
	static char *viewTag = 0;
	char *envPtr;
	char *tagPtr;

	if(viewTag) {
		return viewTag;
	}

	/* viewTag has not been setup */
	viewTag = viewTagBuffer;
	viewTag[0] = '\0';

	envPtr = getenv("CLEARCASE_ROOT");
	if(envPtr)
	{
		tagPtr = strrchr(envPtr, '/');
		if(tagPtr)
		{
			strcpy(viewTag, tagPtr + 1);
		}
	}

	return(viewTag);
}
    
