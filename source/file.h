/*******************************************************************************
*									       *
* file.h -- Nirvana Editor file i/o					       *
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
/* flags for EditExistingFile */
#define CREATE 1
#define SUPPRESS_CREATE_WARN 2
#define FORCE_READ_ONLY 4

WindowInfo* EditNewFile(WindowInfo *parentWindow, Boolean forceRaise);
WindowInfo* EditExistingFile(WindowInfo *inWindow, char *name, char *path, int flags, Boolean forceRaise);
void RevertToSaved(WindowInfo *window, int silent);
int SaveWindow(WindowInfo *window);
int SaveWindowAs(WindowInfo *window, char *newName, int addWrap);
int CloseAllFilesAndWindows(void);
int CloseFileAndWindow(WindowInfo *window);
void PrintWindow(WindowInfo *window, int selectedOnly);
void PrintString(char *string, int length, Widget parent, char *jobName);
int WriteBackupFile(WindowInfo *window);
int IncludeFile(WindowInfo *window, char *name);
int PromptForExistingFile(WindowInfo *window, char *prompt, char *fullname, char *title);
int PromptForNewFile(WindowInfo *window, char *prompt, char *fullname,
    	int *addWrap);
void CheckForChangesInFile(WindowInfo *window);
int CheckReadOnly(WindowInfo *window);
void RemoveBackupFile(WindowInfo *window);
void UniqueUntitledName(char *name);
void FindIncludeFile(char *path);
void SetCheckingMode(WindowInfo *window, CheckingMode checkingMode);
