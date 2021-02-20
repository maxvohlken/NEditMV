/*******************************************************************************
*									       *
* menu.c -- Nirvana Editor menus					       *
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
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include <X11/X.h>
#include <Xm/Xm.h>
#include <Xm/CascadeB.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/RowColumn.h>
#include <Xm/Separator.h>
#include <Xm/MenuShell.h>
#include <Xm/Text.h>
#include "../util/getfiles.h"
#include "../util/fontsel.h" /*... get rid of this */
#include "../util/DialogF.h"
#include "../util/misc.h"
#include "../util/fileUtils.h"
#include "textBuf.h"
#include "text.h"
#include "nedit.h"
#include "file.h"
#include "menu.h"
#include "window.h"
#include "search.h"
#include "selection.h"
#include "undo.h"
#include "shift.h"
#include "help.h"
#include "preferences.h"
#include "tags.h"
#include "userCmds.h"
#include "shell.h"
#include "macro.h"
#include "highlight.h"
#include "highlightData.h"
#include "interpret.h"
#include "smartIndent.h"

/* File name for Open Previous file database */
#ifdef VMS
#define NEDIT_DB_FILE_NAME ".neditdb;1"
#else
#define NEDIT_DB_FILE_NAME ".neditdb"
#endif /*VMS*/

/* Menu modes for SGI_CUSTOM short-menus feature */
enum menuModes {FULL, SHORT};

typedef void (*menuCallbackProc)();

typedef struct _ActionInfo {
	WindowInfo *window;
	char *action;
} ActionInfo;

static void doActionCB(Widget w, XtPointer clientData, XtPointer callData);
static ActionInfo *actionInfoRec(WindowInfo* window, char *action);
static void readOnlyCB(Widget w, XtPointer clientData, XtPointer callData);
static void pasteColCB(Widget w, XtPointer clientData, XtPointer callData); 
static void shiftLeftCB(Widget w, XtPointer clientData, XtPointer callData);
static void shiftRightCB(Widget w, XtPointer clientData, XtPointer callData);
static void findSameCB(Widget w, XtPointer clientData, XtPointer callData);
static void findSelCB(Widget w, XtPointer clientData, XtPointer callData);
static void findCB(Widget w, XtPointer clientData, XtPointer callData);
static void replaceCB(Widget w, XtPointer clientData, XtPointer callData);
static void replaceSameCB(Widget w, XtPointer clientData, XtPointer callData);
static void replaceFindSameCB(Widget w, XtPointer clientData, XtPointer callData);
static void gotoISearchTextCB(Widget w, XtPointer clientData, XtPointer callData);
static void markCB(Widget w, XtPointer clientData, XtPointer callData);
static void gotoMarkCB(Widget w, XtPointer clientData, XtPointer callData);
static void overstrikeCB(Widget w, WindowInfo *window, XtPointer callData);
static void highlightCB(Widget w, WindowInfo *window, XtPointer callData);
static void autoIndentOffCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void smartIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void preserveCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveCB(Widget w, WindowInfo *window, caddr_t callData);
static void newlineWrapCB(Widget w, WindowInfo *window, caddr_t callData);
static void noWrapCB(Widget w, WindowInfo *window, caddr_t callData);
static void continuousWrapCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapMarginCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabsCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsCB(Widget w, WindowInfo *window, caddr_t callData);
static void iSearchLineCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentOffDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoIndentDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void smartIndentDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void autoSaveDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void preserveDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void noWrapDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void newlineWrapDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void contWrapDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void wrapMarginDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void statsLineDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void iSearchLineDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void highlightOffDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void highlightDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void warnOnExitDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void allowReadOnlyEditsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void tabsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void showMatchingDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void fontDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void languageDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void highlightingDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void smartMacrosDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void stylesDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void shellDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void macroDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void bgMenuDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void keepSearchDlogsDefCB(Widget w, WindowInfo *window,
	caddr_t callData);
static void dialogCenterPointerDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void dialogCenterWindowDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchLiteralCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchCaseSenseCB(Widget w, WindowInfo *window, caddr_t callData);
static void searchRegexCB(Widget w, WindowInfo *window, caddr_t callData);
static void size24x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size40x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size60x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void size80x80CB(Widget w, WindowInfo *window, caddr_t callData);
static void sizeCustomCB(Widget w, WindowInfo *window, caddr_t callData);
static void checkingModePromptToReloadDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void checkingModeDisabledDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void checkingModeTailMinusFDefCB(Widget w, WindowInfo *window, caddr_t callData);
static void savePrefCB(Widget w, WindowInfo *window, caddr_t callData);
static void formFeedCB(Widget w, XtPointer clientData, XtPointer callData);
static void cancelShellCB(Widget w, WindowInfo *window, XtPointer callData);
static void learnCB(Widget w, WindowInfo *window, caddr_t callData);
static void finishLearnCB(Widget w, WindowInfo *window, caddr_t callData);
static void cancelLearnCB(Widget w, WindowInfo *window, caddr_t callData);
static void replayCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMouseCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpFillCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpTabsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSyntaxCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCtagsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpServerCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpLearnCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMacroLangCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMacroSubrsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpResourcesCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpBindingCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpActionsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpPatternsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpSmartIndentCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData);
static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData);
static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData);
static void prevOpenMenuCB(Widget w, WindowInfo *window, caddr_t callData);
static void newAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void openDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void openAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void openSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void closeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void saveAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void saveAsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void saveAsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void revertDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void revertAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void includeDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void includeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void loadMacroDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) ;
static void loadMacroAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void loadTagsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs); 
static void loadTagsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void printAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void printSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void exitAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void undoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void redoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void clearAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void selAllAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shiftLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shiftLeftTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shiftRightAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shiftRightTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void findIncrementalAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void findSameAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void findSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void replaceDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void replaceFindAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void replaceAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceInSelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void replaceFindSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void gotoDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void repeatDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void repeatMacroAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs);
static void markAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void markDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoMarkAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void gotoMarkDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void gotoMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void findDefAP(Widget w, XEvent *event, String *args, Cardinal *nArgs); 
static void splitWindowAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void closePaneAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void cloneWindowAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void capitalizeAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void lowercaseAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void fillAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void controlDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
#ifndef VMS
static void filterDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void shellFilterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void execDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void execAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void execLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void shellMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
#endif
static void macroMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void bgMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs);
static void beginningOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static void endOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
static Time timeOfEvent(XEvent *event);
static Widget createMenu(Widget parent, char *name, char *label,
    	char mnemonic, Widget *cascadeBtn, int mode);
static Widget createMenuItem(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int mode);
static Widget createFakeMenuItem(Widget parent, char *name,
	menuCallbackProc callback, void *cbArg);
static Widget createMenuToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set,
	int mode);
static Widget createMenuRadioToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set,
	int mode);
static Widget createMenuSeparator(Widget parent, char *name, int mode);
static void invalidatePrevOpenMenus(void);
static void updateWindowMenu(WindowInfo *window);
static void updatePrevOpenMenu(WindowInfo *window);
static int searchDirection(int ignoreArgs, String *args, Cardinal *nArgs);
static int searchType(int ignoreArgs, String *args, Cardinal *nArgs);
static char **shiftKeyToDir(XtPointer callData);
static void raiseCB(Widget w, WindowInfo *window, caddr_t callData);
static void openPrevCB(Widget w, char *name, caddr_t callData);
static int cmpStrPtr(const void *strA, const void *strB);
static void checkingModePromptToReloadCB(Widget w, WindowInfo *window, caddr_t callData);
static void checkingModeDisabledCB(Widget w, WindowInfo *window, caddr_t callData);
static void checkingModeTailMinusFCB(Widget w, WindowInfo *window, caddr_t callData);
static void setWindowSizeDefault(int rows, int cols);
static void updateWindowSizeMenus(void);
static void updateWindowSizeMenu(WindowInfo *win);
static void setPrefCheckingMode(WindowInfo *window, CheckingMode mode);
static int compareWindowNames(const void *windowA, const void *windowB);
static void bgMenuPostAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs);
#ifdef SGI_CUSTOM
static void shortMenusCB(Widget w, WindowInfo *window, caddr_t callData);
static void addToToggleShortList(Widget w);
static int shortPrefAskDefault(Widget parent, Widget w, char *settingName);
#endif

/* Application action table */
static XtActionsRec Actions[] = {
    {"new", newAP},
    {"open", openAP},
    {"open-dialog", openDialogAP},
    {"open_dialog", openDialogAP},
    {"open-selected", openSelectedAP},
    {"open_selected", openSelectedAP},
    {"close", closeAP},
    {"save", saveAP},
    {"save-as", saveAsAP},
    {"save_as", saveAsAP},
    {"save-as-dialog", saveAsDialogAP},
    {"save_as_dialog", saveAsDialogAP},
    {"revert-to-saved", revertAP},
    {"revert_to_saved", revertAP},
    {"revert-to-saved_dialog", revertDialogAP},
    {"revert_to_saved_dialog", revertDialogAP},
    {"include-file", includeAP},
    {"include_file", includeAP},
    {"include-file-dialog", includeDialogAP},
    {"include_file_dialog", includeDialogAP},
    {"load-macro-file", loadMacroAP},
    {"load_macro_file", loadMacroAP},
    {"load-macro-file-dialog", loadMacroDialogAP},
    {"load_macro_file_dialog", loadMacroDialogAP},
    {"load-tags-file", loadTagsAP},
    {"load_tags_file", loadTagsAP},
    {"load-tags-file-dialog", loadTagsDialogAP},
    {"load_tags_file_dialog", loadTagsDialogAP},
    {"print", printAP},
    {"print-selection", printSelAP},
    {"print_selection", printSelAP},
    {"exit", exitAP},
    {"undo", undoAP},
    {"redo", redoAP},
    {"delete", clearAP},
    {"select-all", selAllAP},
    {"select_all", selAllAP},
    {"shift-left", shiftLeftAP},
    {"shift_left", shiftLeftAP},
    {"shift-left-by-tab", shiftLeftTabAP},
    {"shift_left_by_tab", shiftLeftTabAP},
    {"shift-right", shiftRightAP},
    {"shift_right", shiftRightAP},
    {"shift-right-by-tab", shiftRightTabAP},
    {"shift_right_by_tab", shiftRightTabAP},
    {"uppercase", capitalizeAP},
    {"lowercase", lowercaseAP},
    {"fill-paragraph", fillAP},
    {"fill_paragraph", fillAP},
    {"control-code-dialog", controlDialogAP},
    {"control_code_dialog", controlDialogAP},
    {"find", findAP},
    {"find-incremental", findIncrementalAP},
    {"find_incremental", findIncrementalAP},
    {"find-dialog", findDialogAP},
    {"find_dialog", findDialogAP},
    {"find-again", findSameAP},
    {"find_again", findSameAP},
    {"find-selection", findSelAP},
    {"find_selection", findSelAP},
    {"replace", replaceAP},
    {"replace-dialog", replaceDialogAP},
    {"replace_dialog", replaceDialogAP},
    {"replace-all", replaceAllAP},
    {"replace_all", replaceAllAP},
    {"replace-in-selection", replaceInSelAP},
    {"replace_in_selection", replaceInSelAP},
    {"replace-again", replaceSameAP},
    {"replace_again", replaceSameAP},
    {"replace-find", replaceFindAP},
    {"replace_find", replaceFindAP},
    {"replace-find-same", replaceFindSameAP},
    {"replace_find_same", replaceFindSameAP},
    {"replace-find-again", replaceFindSameAP},
    {"replace_find_again", replaceFindSameAP},
    {"goto-line-number", gotoAP},
    {"goto_line_number", gotoAP},
    {"goto-line-number-dialog", gotoDialogAP},
    {"goto_line_number_dialog", gotoDialogAP},
    {"goto-selected", gotoSelectedAP},
    {"goto_selected", gotoSelectedAP},
    {"mark", markAP},
    {"mark-dialog", markDialogAP},
    {"mark_dialog", markDialogAP},
    {"goto-mark", gotoMarkAP},
    {"goto_mark", gotoMarkAP},
    {"goto-mark-dialog", gotoMarkDialogAP},
    {"goto_mark_dialog", gotoMarkDialogAP},
    {"match", findMatchingAP},
    {"select-matching", findMatchingAP},
    {"select_matching", findMatchingAP},
    {"goto-matching", gotoMatchingAP},
    {"goto_matching", gotoMatchingAP},
    {"find-definition", findDefAP},
    {"find_definition", findDefAP},
    {"split-window", splitWindowAP},
    {"split_window", splitWindowAP},
    {"close-pane", closePaneAP},
    {"close_pane", closePaneAP},
    {"clone-window", cloneWindowAP},
    {"clone_window", cloneWindowAP},
#ifndef VMS
    {"filter-selection-dialog", filterDialogAP},
    {"filter_selection_dialog", filterDialogAP},
    {"filter-selection", shellFilterAP},
    {"filter_selection", shellFilterAP},
    {"execute-command", execAP},
    {"execute_command", execAP},
    {"execute-command-dialog", execDialogAP},
    {"execute_command_dialog", execDialogAP},
    {"execute-command-line", execLineAP},
    {"execute_command_line", execLineAP},
    {"execute-selection-or-line", execLineAP},
    {"execute_selection_or_line", execLineAP},
    {"shell-menu-command", shellMenuAP},
    {"shell_menu_command", shellMenuAP},
#endif /*VMS*/
    {"macro-menu-command", macroMenuAP},
    {"macro_menu_command", macroMenuAP},
    {"bg-menu-command", bgMenuAP},
    {"bg_menu_command", bgMenuAP},
    {"post-window-bg-menu", bgMenuPostAP},
    {"post_window_bg_menu", bgMenuPostAP},
    {"beginning-of-selection", beginningOfSelectionAP},
    {"beginning_of_selection", beginningOfSelectionAP},
    {"end-of-selection", endOfSelectionAP},
    {"end_of_selection", endOfSelectionAP},
    {"repeat-macro", repeatMacroAP},
    {"repeat_macro", repeatMacroAP},
    {"repeat-dialog", repeatDialogAP},
    {"repeat_dialog", repeatDialogAP},
};

/* List of previously opened files for File menu */
static int NPrevOpen = 0;
static char **PrevOpen;

#ifdef SGI_CUSTOM
/* Window to receive items to be toggled on and off in short menus mode */
static WindowInfo *ShortMenuWindow;
#endif

/*
** Install actions for use in translation tables and macro recording, relating
** to menu item commands
*/
void InstallMenuActions(XtAppContext context)
{
    XtAppAddActions(context, Actions, XtNumber(Actions));
}

/*
** Return the (statically allocated) action table for menu item actions.
*/
XtActionsRec *GetMenuActions(int *nActions)
{
    *nActions = XtNumber(Actions);
    return Actions;
}

/*
** Create the menu bar
*/
Widget CreateMenuBar(Widget parent, WindowInfo *window)
{
    Widget menuBar, menuPane, btn, subPane, subSubPane, subSubSubPane, cascade;

    /*
    ** Create the menu bar (row column) widget
    */
    menuBar = XmCreateMenuBar(parent, "menuBar", NULL, 0);

#ifdef SGI_CUSTOM
    /*
    ** Short menu mode is a special feature for the SGI system distribution
    ** version of NEdit.
    **
    ** To make toggling short-menus mode faster (re-creating the menus was
    ** too slow), a list is kept in the window data structure of items to
    ** be turned on and off.  Initialize that list and give the menu creation
    ** routines a pointer to the window on which this list is kept.  This is
    ** (unfortunately) a global variable to keep the interface simple for
    ** the mainstream case.
    */
    ShortMenuWindow = window;
    window->nToggleShortItems = 0;
#endif

    /*
    ** "File" pull down menu.
    */
    menuPane = createMenu(menuBar, "fileMenu", "File", 0, NULL, SHORT);
    createMenuItem(menuPane, "new", "New", 'N', doActionCB, actionInfoRec(window, "new"), SHORT);
    createMenuItem(menuPane, "open", "Open...", 'O', doActionCB, actionInfoRec(window, "open_dialog"),
    	    SHORT);
    createMenuItem(menuPane, "openSelected", "Open Selected", 'd',
    	    doActionCB, actionInfoRec(window, "open_selected"), FULL);
    if (GetPrefMaxPrevOpenFiles() != 0) {
	window->prevOpenMenuPane = createMenu(menuPane, "openPrevious",
    		"Open Previous", 'v', &window->prevOpenMenuItem, SHORT);
	XtSetSensitive(window->prevOpenMenuItem, NPrevOpen != 0);
	XtAddCallback(window->prevOpenMenuItem, XmNcascadingCallback,
    		(XtCallbackProc)prevOpenMenuCB, window);
    }
    createMenuSeparator(menuPane, "sep1", SHORT);
    window->closeItem = createMenuItem(menuPane, "close", "Close", 'C',
    	    doActionCB, actionInfoRec(window, "close"), SHORT);
    createMenuItem(menuPane, "save", "Save", 'S', doActionCB, actionInfoRec(window, "save"), SHORT);
    createMenuItem(menuPane, "saveAs", "Save As...", 'A', doActionCB,
    	    actionInfoRec(window, "save_as_dialog"), SHORT);
    createMenuItem(menuPane, "revertToSaved", "Revert to Saved", 'R',
    	    doActionCB, actionInfoRec(window, "revert_to_saved_dialog"), SHORT);
    createMenuSeparator(menuPane, "sep2", SHORT);
    createMenuItem(menuPane, "includeFile", "Include File...", 'I',
    	    doActionCB, actionInfoRec(window, "include_file_dialog"), SHORT);
    createMenuItem(menuPane, "loadMacroFile", "Load Macro File...", 'M',
    	    doActionCB, actionInfoRec(window, "load_macro_file_dialog"), FULL);
    createMenuItem(menuPane, "loadTagsFile", "Load Tags File...", 'T',
    	    doActionCB, actionInfoRec(window, "load_tags_file_dialog"), FULL);
    createMenuSeparator(menuPane, "sep3", SHORT);
    createMenuItem(menuPane, "print", "Print...", 'P', doActionCB, actionInfoRec(window, "print"),
    	    SHORT);
    window->printSelItem = createMenuItem(menuPane, "printSelection",
    	    "Print Selection...", 't', doActionCB, actionInfoRec(window, "print_selection"),
    	    SHORT);
    XtSetSensitive(window->printSelItem, window->wasSelected);
    createMenuSeparator(menuPane, "sep4", SHORT);
    createMenuItem(menuPane, "exit", "Exit", 'x', doActionCB, actionInfoRec(window, "exit"), SHORT);   
    CheckCloseDim();

    /* 
    ** "Edit" pull down menu.
    */
    menuPane = createMenu(menuBar, "editMenu", "Edit", 0, NULL, SHORT);
    window->undoItem = createMenuItem(menuPane, "undo", "Undo", 'U',
    	    doActionCB, actionInfoRec(window, "undo"), SHORT);
    XtSetSensitive(window->undoItem, False);
    window->redoItem = createMenuItem(menuPane, "redo", "Redo", 'R',
    	    doActionCB, actionInfoRec(window, "redo"), SHORT);
    XtSetSensitive(window->redoItem, False);
    createMenuSeparator(menuPane, "sep1", SHORT);
    window->cutItem = createMenuItem(menuPane, "cut", "Cut", 't', doActionCB,
    	    actionInfoRec(window, "cut_clipboard"), SHORT);
    XtSetSensitive(window->cutItem, window->wasSelected);
    window->copyItem = createMenuItem(menuPane, "copy", "Copy", 'C', doActionCB,
    	    actionInfoRec(window, "copy_clipboard"), SHORT);
    XtSetSensitive(window->copyItem, window->wasSelected);
    createMenuItem(menuPane, "paste", "Paste", 'P', doActionCB,
    	    actionInfoRec(window, "paste_clipboard"), SHORT);
    createMenuItem(menuPane, "pasteColumn", "Paste Column", 's', pasteColCB,
    	    window, SHORT);
    createMenuItem(menuPane, "delete", "Delete", 'D', doActionCB, actionInfoRec(window, "delete"),
    	    SHORT);
    createMenuItem(menuPane, "selectAll", "Select All", 'A', doActionCB,
    	    actionInfoRec(window, "select_all"), SHORT);
    createMenuSeparator(menuPane, "sep2", SHORT);
    createMenuItem(menuPane, "shiftLeft", "Shift Left", 'L',
    	    shiftLeftCB, window, SHORT);
    createFakeMenuItem(menuPane, "shiftLeftShift", shiftLeftCB, window);
    createMenuItem(menuPane, "shiftRight", "Shift Right", 'g',
    	    shiftRightCB, window, SHORT);
    createFakeMenuItem(menuPane, "shiftRightShift", shiftRightCB, window);
    createMenuItem(menuPane, "lowerCase", "Lower_case", 'w',
    	    doActionCB, actionInfoRec(window, "lowercase"), SHORT);
    createMenuItem(menuPane, "upperCase", "Upper_case", 'e',
    	    doActionCB, actionInfoRec(window, "uppercase"), SHORT);
    createMenuItem(menuPane, "fillParagraph", "Fill Paragraph", 'F',
    	    doActionCB, actionInfoRec(window, "fill_paragraph"), SHORT);
    createMenuSeparator(menuPane, "sep3", FULL);
    createMenuItem(menuPane, "insertFormFeed", "Insert Form Feed", 'I',
    	    formFeedCB, window, FULL);
    createMenuItem(menuPane, "insertCtrlCode", "Insert Ctrl Code", 'n',
    	    doActionCB, actionInfoRec(window, "control_code_dialog"), FULL);

    /* 
    ** "Search" pull down menu.
    */
    menuPane = createMenu(menuBar, "searchMenu", "Search", 0, NULL, SHORT);
    createMenuItem(menuPane, "find", "Find...", 'F', findCB, window, SHORT);
    createFakeMenuItem(menuPane, "findShift", findCB, window);
    createMenuItem(menuPane, "findIncremental", "Find Incremental", 'I', 
    		gotoISearchTextCB, window, SHORT);
    createFakeMenuItem(menuPane, "findIncrementalShift", gotoISearchTextCB, 
    		window);
    createMenuItem(menuPane, "findAgain", "Find Again", 'i', findSameCB, window,
    	    SHORT);
    createFakeMenuItem(menuPane, "findAgainShift", findSameCB, window);
    createMenuItem(menuPane, "findSelection", "Find Selection", 'S',
    	    findSelCB, window, SHORT);
    createFakeMenuItem(menuPane, "findSelectionShift", findSelCB, window);
    createMenuItem(menuPane, "replace", "Replace...", 'R', replaceCB, window,
    	    SHORT);
    createFakeMenuItem(menuPane, "replaceShift", replaceCB, window);
    createMenuItem(menuPane, "replaceFindAgain", "Replace Find Again", 'A',
    	    replaceFindSameCB, window, SHORT);
    createFakeMenuItem(menuPane, "replaceFindAgainShift", replaceFindSameCB, window);
    createMenuItem(menuPane, "replaceAgain", "Replace Again", 'p',
    	    replaceSameCB, window, SHORT);
    createFakeMenuItem(menuPane, "replaceAgainShift", replaceSameCB, window);
    createMenuSeparator(menuPane, "sep1", FULL);
    createMenuItem(menuPane, "gotoLineNumber", "Goto Line Number...", 'L',
    	    doActionCB, actionInfoRec(window, "goto_line_number_dialog"), FULL);
    createMenuItem(menuPane, "gotoSelected", "Goto Selected", 'G',
    	    doActionCB, actionInfoRec(window, "goto_selected"), FULL);
    createMenuSeparator(menuPane, "sep2", FULL);
    createMenuItem(menuPane, "mark", "Mark", 'k', markCB, window, FULL);
    createMenuItem(menuPane, "gotoMark", "Goto Mark", 'o', gotoMarkCB, window,
    	    FULL);
    createFakeMenuItem(menuPane, "gotoMarkShift", gotoMarkCB, window);
    createMenuSeparator(menuPane, "sep3", FULL);
    createMenuItem(menuPane, "match", "Select Matching", 'M',
    	    doActionCB, actionInfoRec(window, "select_matching"), FULL);
    createMenuItem(menuPane, "gotoMatching", "Goto Matching", 't',
    	    doActionCB, actionInfoRec(window, "goto_matching"), FULL);
    window->findDefItem = createMenuItem(menuPane, "findDefinition",
    	    "Find Definition", 'D', doActionCB, actionInfoRec(window, "find_definition"), FULL);
    
    /*
    ** Preferences menu, Default Settings sub menu
    */
    menuPane = createMenu(menuBar, "preferencesMenu", "Preferences", 0, NULL,
    	    SHORT);
    subPane = createMenu(menuPane, "defaultSettings", "Default Settings", 'D',
    	    NULL, FULL);
    createMenuItem(subPane, "languageModes", "Language Modes...", 'L',
    	    languageDefCB, window, FULL);
    
	/* Syntax Highlighting sub menu */
    subSubPane = createMenu(subPane, "syntaxHighlighting","Syntax Highlighting",
    	    'H', NULL, FULL);
    window->highlightOffDefItem = createMenuRadioToggle(subSubPane, "off","Off",
    	    'O', highlightOffDefCB, window, !GetPrefHighlightSyntax(), FULL);
    window->highlightDefItem = createMenuRadioToggle(subSubPane, "on",
    	    "On", 'n', highlightDefCB, window, GetPrefHighlightSyntax(), FULL);
    createMenuSeparator(subSubPane, "sep1", SHORT);
    createMenuItem(subSubPane, "recognitionPatterns", "Recognition Patterns...",
    	    'R', highlightingDefCB, window, FULL);
    createMenuItem(subSubPane, "textDrawingStyles", "Text Drawing Styles...", 'T',
    	    stylesDefCB, window, FULL);

    /* Auto Indent sub menu */
    subSubPane = createMenu(subPane, "autoIndent", "Auto Indent", 'A',
    	    NULL, FULL);
    window->autoIndentOffDefItem = createMenuRadioToggle(subSubPane, "off",
    	    "Off", 'O', autoIndentOffDefCB, window,
    	    GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == NO_AUTO_INDENT, SHORT);
    window->autoIndentDefItem = createMenuRadioToggle(subSubPane, "on",
    	    "On", 'n', autoIndentDefCB, window,
    	    GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == AUTO_INDENT, SHORT);
    window->smartIndentDefItem = createMenuRadioToggle(subSubPane, "smart",
    	    "Smart", 'S', smartIndentDefCB, window,
    	    GetPrefAutoIndent(PLAIN_LANGUAGE_MODE) == SMART_INDENT, SHORT);
    createMenuSeparator(subSubPane, "sep1", SHORT);
    createMenuItem(subSubPane, "ProgramSmartIndent", "Program Smart Indent...",
    	    'P', smartMacrosDefCB, window, FULL);
    
    /* Wrap sub menu */
    subSubPane = createMenu(subPane, "wrap", "Wrap", 'W', NULL, FULL);
    window->noWrapDefItem = createMenuRadioToggle(subSubPane,
    	    "none", "None", 'N', noWrapDefCB,
	    window, GetPrefWrap(PLAIN_LANGUAGE_MODE) == NO_WRAP, SHORT);
    window->newlineWrapDefItem = createMenuRadioToggle(subSubPane,
    	    "autoNewline", "Auto Newline", 'A', newlineWrapDefCB,
	    window, GetPrefWrap(PLAIN_LANGUAGE_MODE) == NEWLINE_WRAP, SHORT);
    window->contWrapDefItem = createMenuRadioToggle(subSubPane, "continuous",
    	    "Continuous", 'C', contWrapDefCB, window,
    	    GetPrefWrap(PLAIN_LANGUAGE_MODE) == CONTINUOUS_WRAP, SHORT);
    createMenuSeparator(subSubPane, "sep1", SHORT);
    createMenuItem(subSubPane, "wrapMargin", "Wrap Margin...", 'W',
    	    wrapMarginDefCB, window, SHORT);

    createMenuItem(subPane, "tabDistance", "Tabs...", 'T', tabsDefCB, window,
    	    SHORT);
    createMenuItem(subPane, "textFont", "Text Font...", 'F', fontDefCB, window,
    	    FULL);
    
    /* Customize Menus sub menu */
    subSubPane = createMenu(subPane, "customizeMenus", "Customize Menus",
    	    'C', NULL, FULL);
#ifndef VMS
    createMenuItem(subSubPane, "shellMenu", "Shell Menu...", 'S',
    	    shellDefCB, window, FULL);
#endif
    createMenuItem(subSubPane, "macroMenu", "Macro Menu...", 'M',
    	    macroDefCB, window, FULL);
    createMenuItem(subSubPane, "windowBackgroundMenu",
	    "Window Background Menu...", 'W', bgMenuDefCB, window, FULL);

    /* Search sub menu */
    subSubPane = createMenu(subPane, "searching", "Searching",
    	    'S', NULL, FULL);
    window->searchDlogsDefItem = createMenuToggle(subSubPane, "verbose",
    	    "Verbose", 'V', searchDlogsDefCB, window,
    	    GetPrefSearchDlogs(), SHORT);
    window->keepSearchDlogsDefItem = createMenuToggle(subSubPane,
    	    "keepDialogsUp", "Keep Dialogs Up", 'K',
    	    keepSearchDlogsDefCB, window, GetPrefKeepSearchDlogs(), SHORT);
    subSubSubPane = AddSubMenu(subSubPane, "defaultSearchStyle",
    	    "Default Search Style", 'D');
    XtVaSetValues(subSubSubPane, XmNradioBehavior, True, NULL); 
    window->searchLiteralDefItem = createMenuToggle(subSubSubPane, "literal",
    	    "Literal", 'L', searchLiteralCB, window,
    	    GetPrefSearch() == SEARCH_LITERAL, FULL);
    window->searchCaseSenseDefItem = createMenuToggle(subSubSubPane,
    	    "caseSensitive", "Case Sensitive", 'C', searchCaseSenseCB, window,
    	    GetPrefSearch() == SEARCH_CASE_SENSE, FULL);
    window->searchRegexDefItem = createMenuToggle(subSubSubPane,
    	    "regularExpression", "Regular Expression", 'R', searchRegexCB,
    	    window, GetPrefSearch() == SEARCH_REGEX, FULL);

    window->statsLineDefItem = createMenuToggle(subPane, "statisticsLine",
    	    "Statistics Line", 'S', statsLineDefCB, window, GetPrefStatsLine(),
    	    SHORT);
    window->iSearchLineDefItem = createMenuToggle(subPane, "searchLine",
    	    "Incremental Search Line", 'n', iSearchLineDefCB, window, 
    	    GetPrefShowISearchLine(), SHORT);
    {char buf[512];
    sprintf(buf, "Make Backup Copy (*%s)", GetPrefBackupSuffix());
    window->saveLastDefItem = createMenuToggle(subPane, "preserveLastVersion",
    	    buf, 'e', preserveDefCB, window,
    	    GetPrefSaveOldVersion(), SHORT);
    }
    window->autoSaveDefItem = createMenuToggle(subPane, "incrementalBackup",
    	    "Incremental Backup", 'B', autoSaveDefCB, window, GetPrefAutoSave(),
    	    SHORT);
    window->showMatchingDefItem = createMenuToggle(subPane, "showMatching",
    	    "Show Matching (..)", 'M', showMatchingDefCB, window,
    	    GetPrefShowMatching(), FULL);
    {Widget subPane2, btn;
    subPane2 = createMenu(subPane, "dialogPopUpLocationMenu", "Pop Up Dialog Location", 'U',
    	    NULL, FULL);
    window->dialogCenterPointerDefItem = btn = createMenuRadioToggle(subPane2, "popupsCenteredPointer",
    	    "Centered Under Pointer", 'P', dialogCenterPointerDefCB, window,
    	    GetPrefRepositionDialogs() == REPOSITION_DIALOGS_ON_POINTER, FULL);
    window->dialogCenterWindowDefItem = btn = createMenuRadioToggle(subPane2, "popupsCenteredWindow",
    	    "Centered Over Window", 'W', dialogCenterWindowDefCB, window,
    	    GetPrefRepositionDialogs() == REPOSITION_DIALOGS_DISABLED, FULL);
	}
    window->warnOnExitDefItem = createMenuToggle(subPane, "warnOnExit",
    	    "On Exit Open Files Warning", 0, warnOnExitDefCB, window,
    	    GetPrefWarnOnExit(), SHORT);
    window->allowReadOnlyEditsDefItem = createMenuToggle(subPane, "allowReadOnlyEdits",
    	    "Allow Read Only Edits", 0, allowReadOnlyEditsDefCB, window,
    	    GetPrefAllowReadOnlyEdits(), SHORT);
    {Widget subPane2, btn;
    subPane2 = createMenu(subPane, "checkingModeMenu", "Checking Mode", 'd',
    	    NULL, FULL);
    window->checkingModePromptToReloadDefItem = btn = createMenuRadioToggle(subPane2, 
    		"checkingModePromptToReload", "Prompt To Reload", 'P', checkingModePromptToReloadDefCB, 
    		window, (GetPrefCheckingMode() == CHECKING_MODE_PROMPT_TO_RELOAD), SHORT);
    window->checkingModeDisabledDefItem = btn = createMenuRadioToggle(subPane2, 
    		"checkingModePromptToReload", "Disabled", 'D', checkingModeDisabledDefCB, 
    		window, (GetPrefCheckingMode() == CHECKING_MODE_DISABLED), SHORT);
    window->checkingModeTailMinusFDefItem = btn = createMenuRadioToggle(subPane2, 
    		"checkingModeTailMinusF", "Log File Monitor", 'L', checkingModeTailMinusFDefCB, 
    		window, (GetPrefCheckingMode() == CHECKING_MODE_TAIL_MINUS_F), SHORT);
    }

    /* Initial Window Size sub menu (simulates radioBehavior) */
    subSubPane = AddSubMenu(subPane, "initialwindowSize",
    	    "Initial Window Size", 'I');
    /* XtVaSetValues(subSubPane, XmNradioBehavior, True, NULL);  */
    window->size24x80DefItem = btn = createMenuToggle(subSubPane, "24X80",
    	    "24 x 80", '2', size24x80CB, window, False, SHORT);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, NULL);
    window->size40x80DefItem = btn = createMenuToggle(subSubPane, "40X80",
    	    "40 x 80", '4', size40x80CB, window, False, SHORT);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, NULL);
    window->size60x80DefItem = btn = createMenuToggle(subSubPane, "60X80",
    	    "60 x 80", '6', size60x80CB, window, False, SHORT);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, NULL);
    window->size80x80DefItem = btn = createMenuToggle(subSubPane, "80X80",
    	    "80 x 80", '8', size80x80CB, window, False, SHORT);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, NULL);
    window->sizeCustomDefItem = btn = createMenuToggle(subSubPane, "custom",
    	    "Custom...", 'C', sizeCustomCB, window, False, SHORT);
    XtVaSetValues(btn, XmNindicatorType, XmONE_OF_MANY, NULL);
    updateWindowSizeMenu(window);
    
    /*
    ** Remainder of Preferences menu
    */
    createMenuItem(menuPane, "saveDefaults", "Save Defaults...", 'v',
    	    savePrefCB, window, FULL);
#ifdef SGI_CUSTOM
    window->shortMenusDefItem = createMenuToggle(menuPane,
    	    "shortMenus", "Short Menus", 'h', shortMenusCB, window,
    	    GetPrefShortMenus(), SHORT);
#endif
    createMenuSeparator(menuPane, "sep1", SHORT);
    window->statisticsLineItem = createMenuToggle(menuPane, "statisticsLine", "Statistics Line", 'S',
    	    statsCB, window, window->showStats, SHORT);
    window->iSearchLineItem = createMenuToggle(menuPane, "searchLine", "Incremental Search Line", 'n',
    	    iSearchLineCB, window, window->showISearchLine, SHORT);
    CreateLanguageModeSubMenu(window, menuPane, "languageMode",
    	    "Language Mode", 'L');
    window->highlightItem = createMenuToggle(menuPane, "highlightSyntax",
	    "Highlight Syntax", 'H', highlightCB, window,
	    GetPrefHighlightSyntax(), SHORT);
    subPane = createMenu(menuPane, "autoIndent", "Auto Indent",
	    'A', NULL, FULL);
    window->autoIndentOffItem = createMenuRadioToggle(subPane, "off", "Off",
    	    'O', autoIndentOffCB, window, window->editorInfo->indentStyle == NO_AUTO_INDENT,
	    SHORT);
    window->autoIndentItem = createMenuRadioToggle(subPane, "on", "On", 'n',
    	    autoIndentCB, window, window->editorInfo->indentStyle == AUTO_INDENT, SHORT);
    window->smartIndentItem = createMenuRadioToggle(subPane, "smart", "Smart",
    	    'S', smartIndentCB, window, window->editorInfo->indentStyle == SMART_INDENT,
	    SHORT);
    subPane = createMenu(menuPane, "wrap", "Wrap",
	    'W', NULL, FULL);
    window->noWrapItem = createMenuRadioToggle(subPane, "none",
    	    "None", 'N', noWrapCB, window,
    	    window->editorInfo->wrapMode==NO_WRAP, SHORT);
    window->newlineWrapItem = createMenuRadioToggle(subPane, "autoNewlineWrap",
    	    "Auto Newline", 'A', newlineWrapCB, window,
    	    window->editorInfo->wrapMode==NEWLINE_WRAP, SHORT);
    window->continuousWrapItem = createMenuRadioToggle(subPane,
    	    "continuousWrap", "Continuous", 'C', continuousWrapCB, window,
    	    window->editorInfo->wrapMode==CONTINUOUS_WRAP, SHORT);
    createMenuSeparator(subPane, "sep1", SHORT);
    createMenuItem(subPane, "wrapMargin", "Wrap Margin...", 'W',
    	    wrapMarginCB, window, SHORT);
    createMenuItem(menuPane, "tabs", "Tabs...", 'T', tabsCB, window, SHORT);
#ifndef IBM_DESTROY_BUG
    createMenuItem(menuPane, "textFont", "Text Font...", 'F', fontCB, window,
    	    FULL);
#endif /*IBM_DESTROY_BUG*/
#ifndef VMS
    {char buf[512];
    sprintf(buf, "Make Backup Copy (*%s)", GetPrefBackupSuffix());
    window->saveLastItem = createMenuToggle(menuPane, "makeBackupCopy",
    	    buf, 'e', preserveCB, window,
    	    window->editorInfo->saveOldVersion, SHORT);
    }
#endif
    window->autoSaveItem = createMenuToggle(menuPane, "incrementalBackup",
    	    "Incremental Backup", 'B', autoSaveCB, window, window->editorInfo->autoSave,
    	    SHORT);
    window->showMatchingItem = createMenuToggle(menuPane, "showMatching", "Show Matching (..)", 'M',
    	    showMatchingCB, window, window->editorInfo->showMatching, FULL);
    {Widget subPane2;
    subPane2 = createMenu(menuPane, "checkingModeMenu", "Checking Mode", 'M',
    	    NULL, FULL);
    window->checkingModePromptToReloadItem = createMenuRadioToggle(subPane2, "checkingModePromptToReload", "Prompt To Reload", 'P',
    	    checkingModePromptToReloadCB, window, (window->editorInfo->checkingMode == CHECKING_MODE_PROMPT_TO_RELOAD), SHORT);
    window->checkingModeDisabledItem = createMenuRadioToggle(subPane2, "checkingModePromptToReload", "Disabled", 'D',
    	    checkingModeDisabledCB, window, (window->editorInfo->checkingMode == CHECKING_MODE_DISABLED), SHORT);
    window->checkingModeTailMinusFItem = createMenuRadioToggle(subPane2, "checkingModeTailMinusF", "Log File Monitor", 'L',
    	    checkingModeTailMinusFCB, window, (window->editorInfo->checkingMode == CHECKING_MODE_TAIL_MINUS_F), SHORT);
    }
#ifndef SGI_CUSTOM
    createMenuSeparator(menuPane, "sep2", SHORT);
    window->overstrikeItem = createMenuToggle(menuPane, "overtype", "Overtype", 'O',
    	    overstrikeCB, window, False, SHORT);
    window->readOnlyItem = createMenuToggle(menuPane, "readOnly", "Read Only",
    	    'y', readOnlyCB, window, window->editorInfo->lockWrite, FULL);
#endif

#ifndef VMS
    /*
    ** Create the Shell menu
    */
    menuPane = window->shellMenuPane =
    	    createMenu(menuBar, "shellMenu", "Shell", 0, NULL, FULL);
    btn = createMenuItem(menuPane, "executeCommand", "Execute Command...",
    	    'E', doActionCB, actionInfoRec(window, "execute_command_dialog"), SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    btn = createMenuItem(menuPane, "executeCommandLine", "Execute Selection or \nLine at Cursor",
    	    'x', doActionCB, actionInfoRec(window, "execute_command_line"), SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    window->filterItem = createMenuItem(menuPane, "filterSelection",
    	    "Filter Selection...", 'F', doActionCB, actionInfoRec(window, "filter_selection_dialog"),
    	    SHORT);
    XtVaSetValues(window->filterItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, window->wasSelected, NULL);
    window->cancelShellItem = createMenuItem(menuPane, "cancelShellCommand",
    	    "Cancel Shell Command", 'C', cancelShellCB, window, SHORT);
    XtVaSetValues(window->cancelShellItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, NULL);
    btn = createMenuSeparator(menuPane, "sep1", SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    /* UpdateShellMenu(window) now done in DetermineLanguageMode */
#endif

    /*
    ** Create the Macro menu
    */
    menuPane = window->macroMenuPane =
    	    createMenu(menuBar, "macroMenu", "Macro", 0, NULL, FULL);
    window->learnItem = createMenuItem(menuPane, "learnKeystrokes",
    	    "Learn Keystrokes", 'L', learnCB, window, SHORT);
    XtVaSetValues(window->learnItem , XmNuserData, PERMANENT_MENU_ITEM, NULL);
    window->finishLearnItem = createMenuItem(menuPane, "finishLearn",
    	    "Finish Learn", 'F', finishLearnCB, window, SHORT);
    XtVaSetValues(window->finishLearnItem , XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, NULL);
    window->cancelMacroItem = createMenuItem(menuPane, "cancelLearn",
    	    "Cancel Learn", 'C', cancelLearnCB, window, SHORT);
    XtVaSetValues(window->cancelMacroItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, NULL);
    window->replayItem = createMenuItem(menuPane, "replayKeystrokes",
    	    "Replay Keystrokes", 'K', replayCB, window, SHORT);
    XtVaSetValues(window->replayItem, XmNuserData, PERMANENT_MENU_ITEM,
    	    XmNsensitive, False, NULL);
    window->repeatItem = createMenuItem(menuPane, "repeat",
    	    "Repeat...", 'R', doActionCB, actionInfoRec(window, "repeat_dialog"), SHORT);
    XtVaSetValues(window->repeatItem, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    btn = createMenuSeparator(menuPane, "sep1", SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    /* UpdateMacroMenu(window) now done in DetermineLanguageMode */

    /*
    ** Create the Windows menu
    */
    menuPane = window->windowMenuPane = createMenu(menuBar, "windowsMenu",
    	    "Windows", 0, &cascade, FULL);
    XtAddCallback(cascade, XmNcascadingCallback, (XtCallbackProc)windowMenuCB,
    	    window);
    window->splitWindowItem = createMenuItem(menuPane, "splitWindow",
    	    "Split Window", 'S', doActionCB, actionInfoRec(window, "split_window"), SHORT);
    XtVaSetValues(window->splitWindowItem, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    window->closePaneItem = createMenuItem(menuPane, "closePane",
    	    "Close Pane", 'C', doActionCB, actionInfoRec(window, "close_pane"), SHORT);
    XtVaSetValues(window->closePaneItem, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    XtSetSensitive(window->closePaneItem, False);
    btn = createMenuItem(menuPane, "cloneWindow",
    	    "Clone Window", 'W', doActionCB, actionInfoRec(window, "clone_window"), SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    btn = createMenuSeparator(menuPane, "sep1", SHORT);
    XtVaSetValues(btn, XmNuserData, PERMANENT_MENU_ITEM, NULL);
    
    /* 
    ** Create "Help" pull down menu.
    */
    menuPane = createMenu(menuBar, "helpMenu", "Help", 0, &cascade, SHORT);
    XtVaSetValues(menuBar, XmNmenuHelpWidget, cascade, NULL);
    createMenuItem(menuPane, "gettingStarted", "Getting Started", 'G',
    	    helpStartCB, window, SHORT);
    subPane = createMenu(menuPane, "basicOperation", "Basic Operation",
	    'B', NULL, FULL);
    createMenuItem(subPane, "selectingText", "Selecting Text", 'S',
    	    helpSelectCB, window, SHORT);
    createMenuItem(subPane, "findingReplacingText",
    	    "Finding and Replacing Text", 'F', helpSearchCB, window, SHORT);
    createMenuItem(subPane, "cutPaste", "Cut and Paste", 'C',
    	    helpClipCB, window, SHORT);
    createMenuItem(subPane, "usingTheMouse",
    	    "Using the Mouse", 'U', helpMouseCB, window, SHORT);
    createMenuItem(subPane, "keyboardShortcuts",
    	    "Keyboard Shortcuts", 'K', helpKbdCB, window, SHORT);
    createMenuItem(subPane, "shiftingAndFilling",
    	    "Shifting and Filling", 'h', helpFillCB, window, SHORT);
    subPane = createMenu(menuPane, "featuresForProgramming",
	    "Features for Programming", 'F', NULL, FULL);
    createMenuItem(subPane, "programmingWithNEdit",
    	    "Programming with NEdit", 'a', helpProgCB, window, SHORT);
    createMenuItem(subPane, "tabsEmulatedTabs",
    	    "Tabs", 'T', helpTabsCB, window, SHORT);
    createMenuItem(subPane, "automaticIndent",
    	    "Automatic Indent", 'A', helpIndentCB, window, SHORT);
    createMenuItem(subPane, "syntaxHighlighting",
    	    "Syntax Highlighting", 'S', helpSyntaxCB, window, SHORT);
    createMenuItem(subPane, "FindingDeclarationsCtags",
    	    "Finding Declarations (ctags)", 'F', helpCtagsCB, window, SHORT);
    createMenuItem(menuPane, "regularExpressions", "Regular Expressions", 'R',
    	    helpRegexCB, window, SHORT);
    subPane = createMenu(menuPane, "macroShellExtensions",
	    "Macro / Shell Extensions", 'M', NULL, FULL);
#ifndef VMS
    createMenuItem(subPane, "shellCommandsAndFilters",
    	    "Shell Commands and Filters", 'S', helpShellCB, window, SHORT);
#endif
    createMenuItem(subPane, "learnReplay", "Learn / Replay", 'L',
    	    helpLearnCB, window, SHORT);
    createMenuItem(subPane, "macroLanguage", "Macro Language", 'M',
    	    helpMacroLangCB, window, SHORT);
    createMenuItem(subPane, "macro Subroutines", "Macro Subroutines", 'a',
    	    helpMacroSubrsCB, window, SHORT);
    createMenuItem(subPane, "action routines", "Action Routines", 'A',
    	    helpActionsCB, window, SHORT);
    subPane = createMenu(menuPane, "customizing",
	    "Customizing", 'C', NULL, FULL);
    createMenuItem(subPane, "customizingNEdit", "Customizing NEdit", 'z',
    	    helpCustCB, window, SHORT);
    createMenuItem(subPane, "preferences", "Preferences", 'P',
    	    helpPrefCB, window, SHORT);
    createMenuItem(subPane, "xResources", "X Resources", 'X',
    	    helpResourcesCB, window, SHORT);
    createMenuItem(subPane, "keyBinding", "Key Binding", 'K',
    	    helpBindingCB, window, SHORT);
    createMenuItem(subPane, "highlightingPatterns",
    	    "Highlighting Patterns", 'H', helpPatternsCB, window, SHORT);
    createMenuItem(subPane, "smartIndentMacros",
    	    "Smart Indent Macros", 'S', helpSmartIndentCB, window, SHORT);
    createMenuItem(menuPane, "neditCommandLine", "NEdit Command Line", 'N',
    	    helpCmdLineCB, window, SHORT);
    createMenuItem(menuPane, "serverModeAndNc", "Server Mode and nc", 'S',
    	    helpServerCB, window, SHORT);
    createMenuItem(menuPane, "crashRecovery", "Crash Recovery", 'a',
    	    helpRecoveryCB, window, SHORT);
    createMenuSeparator(menuPane, "sep1", SHORT);
    createMenuItem(menuPane, "version", "Version", 'V',
    	    helpVerCB, window, SHORT);
    createMenuItem(menuPane, "distributionPolicy", "Distribution Policy", 'D',
    	    helpDistCB, window, SHORT);
    createMenuItem(menuPane, "mailingLists", "Mailing Lists", 'L',
    	    helpMailingCB, window, SHORT);
    createMenuItem(menuPane, "problemsBugs", "Problems/Bugs", 'P',
    	    helpBugsCB, window, SHORT);

    return menuBar;
}

static void doActionCB(Widget w, XtPointer clientData, XtPointer callData)
{
	ActionInfo *actionInfo = (ActionInfo *)clientData;

    XtCallActionProc(actionInfo->window->lastFocus, actionInfo->action,
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static ActionInfo *actionInfoRec(WindowInfo* window, char *action)
{
	ActionInfo *actionInfo = (ActionInfo *)XtMalloc(sizeof(actionInfo[0]));
	actionInfo->window = window;
	actionInfo->action = action;
	return actionInfo;
}

static void readOnlyCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
    
    window->editorInfo->lockWrite = XmToggleButtonGetState(w);
    UpdateWindowTitle(window);
    UpdateWindowReadOnly(window);
}

static void pasteColCB(Widget w, XtPointer clientData, XtPointer callData) 
{
    static char *params[1] = {"rect"};
    
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "paste_clipboard",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

static void shiftLeftCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus,
    	    ((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask
    	    ? "shift_left" : "shift_left_by_tab",
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static void shiftRightCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus,
    	    ((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask
    	    ? "shift_right" : "shift_right_by_tab",
    	    ((XmAnyCallbackStruct *)callData)->event, NULL, 0);
}

static void findSameCB(Widget w, XtPointer clientData, XtPointer callData)
{
     XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find_again",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void findSelCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find_selection",
    	    ((XmAnyCallbackStruct *)callData)->event, 
    	    shiftKeyToDir(callData), 1);
}

static void findCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "find_dialog",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void replaceCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "replace_dialog",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void replaceSameCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "replace_again",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void replaceFindSameCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "replace_find_same",
    	    ((XmAnyCallbackStruct *)callData)->event,
    	    shiftKeyToDir(callData), 1);
}

static void gotoISearchTextCB(Widget w, XtPointer clientData, XtPointer callData)
{
    WindowInfo *window = (WindowInfo *)clientData;
	
	if(window->iSearchText) {
		ShowISearchLine(window, True);
   		window->iSearchDirection = 
   			(((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask) ?
   				SEARCH_BACKWARD : SEARCH_FORWARD;
		XmTextSetString(window->iSearchText, "");
		window->iSearchHistIndex = 0;
		XmProcessTraversal(window->iSearchText, XmTRAVERSE_CURRENT);
	}
}

static void markCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XEvent *event = ((XmAnyCallbackStruct *)callData)->event;
    WindowInfo *window = (WindowInfo *)clientData;
    
    if (event->type == KeyPress)
    	BeginMarkCommand(window);
    else
    	XtCallActionProc(window->lastFocus, "mark_dialog", event, NULL, 0);
}

static void gotoMarkCB(Widget w, XtPointer clientData, XtPointer callData)
{
    XEvent *event = ((XmAnyCallbackStruct *)callData)->event;
    WindowInfo *window = (WindowInfo *)clientData;
    int extend = event->xbutton.state & ShiftMask;
    static char *params[1] = {"extend"};
    
    if (event->type == KeyPress)
    	BeginGotoMarkCommand(window, extend);
    else
    	XtCallActionProc(window->lastFocus, "goto_mark_dialog", event, params,
		extend ? 1 : 0);
}

static void overstrikeCB(Widget w, WindowInfo *window, XtPointer callData)
{
    SetOverstrike(window, XmToggleButtonGetState(w));
}

static void highlightCB(Widget w, WindowInfo *window, XtPointer callData)
{
    int state = XmToggleButtonGetState(w);
    if (state) {
    	StartHighlighting(window, True);
    } else
    	StopHighlighting(window);
}

static void autoIndentOffCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Auto Indent Off")) {
	autoIndentOffDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoIndent(window, NO_AUTO_INDENT);
}

static void autoIndentCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Auto Indent")) {
	autoIndentDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoIndent(window, AUTO_INDENT);
}

static void smartIndentCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Smart Indent")) {
	smartIndentDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoIndent(window, SMART_INDENT);
}

static void autoSaveCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Incremental Backup")) {
	autoSaveDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
	SetAutoSave(window, XmToggleButtonGetState(w));
}

static void preserveCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Make Backup Copy")) {
    	preserveDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
	SetSaveOldVersion(window, XmToggleButtonGetState(w));
}

static void fontCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ChooseFonts(window, True);
}

static void noWrapCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "No Wrap")) {
	noWrapDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoWrap(window, NO_WRAP);
}

static void newlineWrapCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Auto Newline Wrap")) {
	newlineWrapDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoWrap(window, NEWLINE_WRAP);
}

static void continuousWrapCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Continuous Wrap")) {
    	contWrapDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    SetAutoWrap(window, CONTINUOUS_WRAP);
}

static void wrapMarginCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WrapMarginDialog(window->shell, window);
}

static void showMatchingCB(Widget w, WindowInfo *window, caddr_t callData)
{
	SetShowMatching(window, XmToggleButtonGetState(w));
}

static void tabsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    TabsPrefDialog(window->shell, window);
}

static void statsCB(Widget w, WindowInfo *window, caddr_t callData)
{
#ifdef SGI_CUSTOM
    if (shortPrefAskDefault(window->shell, w, "Statistics Line")) {
	statsLineDefCB(w, window, callData);
	SaveNEditPrefs(window->shell, GetPrefShortMenus());
    }
#endif
    ShowStatsLine(window, XmToggleButtonGetState(w));
}

static void iSearchLineCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ShowISearchLine(window, XmToggleButtonGetState(w));
}

static void checkingModePromptToReloadCB(Widget w, WindowInfo *window, caddr_t callData)
{
	SetCheckingMode(window, CHECKING_MODE_PROMPT_TO_RELOAD);
}

static void checkingModeDisabledCB(Widget w, WindowInfo *window, caddr_t callData)
{
	SetCheckingMode(window, CHECKING_MODE_DISABLED);
}

static void checkingModeTailMinusFCB(Widget w, WindowInfo *window, caddr_t callData)
{
	SetCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
}

static void autoIndentOffDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SetPrefAutoIndent(NO_AUTO_INDENT);
}

static void autoIndentDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefAutoIndent(AUTO_INDENT);
}

static void smartIndentDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefAutoIndent(SMART_INDENT);
}

static void autoSaveDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefAutoSave(XmToggleButtonGetState(w));
}

static void preserveDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefSaveOldVersion(XmToggleButtonGetState(w));
}

static void fontDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    ChooseFonts(window, False);
}

static void noWrapDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefWrap(NO_WRAP);
}

static void newlineWrapDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefWrap(NEWLINE_WRAP);
}

static void contWrapDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefWrap(CONTINUOUS_WRAP);
}

static void wrapMarginDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WrapMarginDialog(window->shell, NULL);
}

static void tabsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    TabsPrefDialog(window->shell, NULL);
}

static void showMatchingDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefShowMatching(XmToggleButtonGetState(w));
}

static void highlightOffDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefHighlightSyntax(False);
}

static void highlightDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefHighlightSyntax(True);
}

static void highlightingDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditHighlightPatterns(window);
}

static void smartMacrosDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditSmartIndentMacros(window);
}

static void stylesDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditHighlightStyles(window->shell, NULL);
}

static void languageDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditLanguageModes(window->shell);
}

#ifndef VMS
static void shellDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditShellMenu(window);
}
#endif /* VMS */

static void macroDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditMacroMenu(window);
}

static void bgMenuDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    EditBGMenu(window);
}

static void searchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefSearchDlogs(XmToggleButtonGetState(w));
}

static void keepSearchDlogsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefKeepSearchDlogs(XmToggleButtonGetState(w));
}

static void dialogCenterPointerDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
	if(state) {
    	SetPointerCenteredDialogs(state);
    	SetPrefRepositionDialogs(REPOSITION_DIALOGS_ON_POINTER);
	}
}

static void dialogCenterWindowDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    int state = XmToggleButtonGetState(w);

    /* Set the preference and make the other windows' menus agree */
	if(state) {
    	SetPointerCenteredDialogs(FALSE);
    	SetPrefRepositionDialogs(REPOSITION_DIALOGS_DISABLED);
	}
}

static void statsLineDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefStatsLine(XmToggleButtonGetState(w));
}

static void iSearchLineDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefShowISearchLine(XmToggleButtonGetState(w));
}

static void warnOnExitDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefWarnOnExit(XmToggleButtonGetState(w));
}

static void allowReadOnlyEditsDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    SetPrefAllowReadOnlyEdits(XmToggleButtonGetState(w));
    UpdateWindowReadOnly(window);
}

static void searchLiteralCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_LITERAL);
    }
}

static void searchCaseSenseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_CASE_SENSE);
    }
}

static void searchRegexCB(Widget w, WindowInfo *window, caddr_t callData)
{
    /* Set the preference and make the other windows' menus agree */
    if (XmToggleButtonGetState(w)) {
    	SetPrefSearch(SEARCH_REGEX);
    }
}

static void size24x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(24, 80);
}

static void size40x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(40, 80);
}

static void size60x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(60, 80);
}

static void size80x80CB(Widget w, WindowInfo *window, caddr_t callData)
{
    setWindowSizeDefault(80, 80);
}

static void sizeCustomCB(Widget w, WindowInfo *window, caddr_t callData)
{
    RowColumnPrefDialog(window->shell);
    updateWindowSizeMenus();
}

static void checkingModePromptToReloadDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
	setPrefCheckingMode(window, CHECKING_MODE_PROMPT_TO_RELOAD);
}

static void checkingModeDisabledDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
	setPrefCheckingMode(window, CHECKING_MODE_DISABLED);
}

static void checkingModeTailMinusFDefCB(Widget w, WindowInfo *window, caddr_t callData)
{
	setPrefCheckingMode(window, CHECKING_MODE_TAIL_MINUS_F);
}

static void savePrefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    SaveNEditPrefs(window->shell, False);
}

static void formFeedCB(Widget w, XtPointer clientData, XtPointer callData)
{
    static char *params[1] = {"\f"};
    
    XtCallActionProc(((WindowInfo *)clientData)->lastFocus, "insert_string",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
}

static void cancelShellCB(Widget w, WindowInfo *window, XtPointer callData)
{
#ifndef VMS
    AbortShellCommand(window);
#endif
}

static void learnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    BeginLearn(window);
}

static void finishLearnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    FinishLearn();
}

static void cancelLearnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    CancelMacroOrLearn(window);
}

static void replayCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Replay(window);
}

static void helpStartCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_START);
}

static void helpSearchCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SEARCH);
}

static void helpSelectCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SELECT);
}

static void helpClipCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_CLIPBOARD);
}

static void helpProgCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_PROGRAMMER);
}

static void helpMouseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MOUSE);
}

static void helpKbdCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_KEYBOARD);
}

static void helpFillCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_FILL);
}

static void helpTabsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_TABS);
}

static void helpIndentCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_INDENT);
}

static void helpSyntaxCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SYNTAX);
}

static void helpCtagsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_TAGS);
}

static void helpRecoveryCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_RECOVERY);
}

static void helpPrefCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_PREFERENCES);
}

static void helpShellCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SHELL);
}

static void helpRegexCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_REGEX);
}

static void helpCmdLineCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_COMMAND_LINE);
}

static void helpServerCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SERVER);
}

static void helpCustCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_CUSTOMIZE);
}

static void helpLearnCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_LEARN);
}


static void helpMacroLangCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MACRO_LANG);
}


static void helpMacroSubrsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MACRO_SUBRS);
}

static void helpResourcesCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_RESOURCES);
}

static void helpBindingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_BINDING);
}

static void helpPatternsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_PATTERNS);
}

static void helpSmartIndentCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_SMART_INDENT);
}

static void helpActionsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_ACTIONS);
}

static void helpVerCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_VERSION);
}

static void helpDistCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_DISTRIBUTION);
}

static void helpMailingCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_MAILING_LIST);
}

static void helpBugsCB(Widget w, WindowInfo *window, caddr_t callData)
{
    Help(window->shell, HELP_BUGS);
}

static void windowMenuCB(Widget w, WindowInfo *window, caddr_t callData)
{
    if (!window->windowMenuValid) {
    	updateWindowMenu(window);
    	window->windowMenuValid = True;
    }
}

static void prevOpenMenuCB(Widget w, WindowInfo *window, caddr_t callData)
{
    if (!window->prevOpenMenuValid) {
    	updatePrevOpenMenu(window);
    	window->prevOpenMenuValid = True;
    }
}

/*
** Action Procedures for menu item commands
*/
static void newAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    EditNewFile(window, True);
    CheckCloseDim();
}

static void openDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char fullname[MAXPATHLEN], *params[1];
    int response;
    
    response = PromptForExistingFile(window, "File to Edit:", fullname, "Open");
    if (response != GFN_OK)
    	return;
    params[0] = fullname;
    XtCallActionProc(window->lastFocus, "open", event, params, 1);
    CheckCloseDim();
}

static void openAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], pathname[MAXPATHLEN];
    
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "open action requires file argument", "OK");
    	return;
    }
    ParseFilename(args[0], filename, pathname);
    EditExistingFile(WindowList, filename, pathname, 0, True);
    CheckCloseDim();
}

static void openSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    OpenSelectedFile(WidgetToWindow(w), timeOfEvent(event));
    CheckCloseDim();
}

static void closeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    CloseFileAndWindow(WidgetToWindow(w));
    CheckCloseDim();
}

static void saveAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);

    if(CheckReadOnly(window))
    	return;
    SaveWindow(window);
}

static void saveAsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    int response, addWrap;
    char fullname[MAXPATHLEN], *params[2];
    
/* .b: Gives complete name to PromptForNewFile ("Save As" mode) */
	strcpy(fullname, window->editorInfo->path);
	if(window->editorInfo->filenameSet)
		strcat(fullname, window->editorInfo->filename);
/* .e */

    response = PromptForNewFile(window, "Save File As:", fullname, &addWrap);
    if (response != GFN_OK)
    	return;
    params[0] = fullname;
    params[1] = "wrapped";
    XtCallActionProc(window->lastFocus, "save_as", event, params, addWrap?2:1);
}

static void saveAsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "save_as action requires file argument", "OK");
    	return;
    }
    SaveWindowAs(window, args[0],
    	    *nArgs == 2 && !StrCaseCmp(args[1], "wrapped"));
}

static void revertDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    RevertToSaved(WidgetToWindow(w), False);
}


static void revertAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    RevertToSaved(WidgetToWindow(w), True);
}

static void includeDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], *params[1];
    int response;
    
    if(CheckReadOnly(window))
    	return;
    response = PromptForExistingFile(window, "File to include:", filename, "Include");
    if (response != GFN_OK)
    	return;
    params[0] = filename;
    XtCallActionProc(window->lastFocus, "include_file", event, params, 1);
}

static void includeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);

    if(CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "include requires file argument", "OK");
    	return;
    }
    IncludeFile(window, args[0]);
}

static void loadMacroDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], *params[1];
    int response;
    
    response = PromptForExistingFile(window, "NEdit macro file:", filename, "Load Macros");
    if (response != GFN_OK)
    	return;
    params[0] = filename;
    XtCallActionProc(window->lastFocus, "load_macro_file", event, params, 1);
}

static void loadMacroAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    if (*nArgs == 0) {
    	fprintf(stderr,"NEdit: load_macro_file action requires file argument\n");
    	return;
    }
    ReadMacroFile(WidgetToWindow(w), args[0], True);
}

static void loadTagsDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    char filename[MAXPATHLEN], *params[1];
    int response;
    
    response = PromptForExistingFile(window, "ctags file:", filename, "Load Tags");
    if (response != GFN_OK)
    	return;
    params[0] = filename;
    XtCallActionProc(window->lastFocus, "load_tags_file", event, params, 1);
}

static void loadTagsAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "load_tags_file action requires file argument", "OK");
    	return;
    }
    if (!LoadTagsFile(args[0]))
    	DialogF(DF_WARN, window->shell, 1,
    		"Error reading ctags file,\ntags not loaded", "Dismiss");
}

static void printAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    NEditPrintWindow(WidgetToWindow(w), False);
}

static void printSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    NEditPrintWindow(WidgetToWindow(w), True);
}

static void exitAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
	if(GetPrefWarnOnExit()) {
    	int resp, titleLen;
    	char exitMsg[DF_MAX_MSG_LENGTH], *ptr;
    	WindowInfo *win;

    	/* If this is the last window, don't ask, just try to close and exit */
    	if (window == WindowList && window->next == NULL) {
		if (CloseAllFilesAndWindows())
    	    	exit(0);
    		else
    	    	return;
    	}

    	/* List the windows being edited and make sure the
    	   user really wants to exit */
    	ptr = exitMsg;
    	strcpy(ptr, "Editing:\n"); ptr += 9;
    	for (win=WindowList; win!=NULL; win=win->next) {
			char title[MAXPATHLEN * 2 + 1];
    		strcpy(title, FormatWindowTitle(win));
			/* Append the full pathname to the end of the title if set */
			if(win->editorInfo->filenameSet) {
				strcat(title, " - ");
				strcat(title, win->editorInfo->path);
			}
    		titleLen = strlen(title);
    		if (titleLen > DF_MAX_MSG_LENGTH - 30) {
    	    	sprintf(ptr, "   ...\n"); ptr += 7;
    	    	break;
    		}
    		sprintf(ptr, "   %s\n", title); ptr += titleLen + 4;
    	}
    	sprintf(ptr, " \nExit NEdit?");
    	resp = DialogF(DF_QUES, window->shell, 2, "%s", "Exit", "Cancel", exitMsg);
    	if (resp == 2)
    		return;
    }

    if (!CheckPrefsChangesSaved(window->shell))
    	return;

    /* Close all files and exit when the last one is closed */
    if (CloseAllFilesAndWindows())
    	exit(0);
}

static void undoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    Undo(window);
}

static void redoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    Redo(window);
}

static void clearAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    BufRemoveSelected(window->editorInfo->buffer);
}

static void selAllAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    BufSelect(window->editorInfo->buffer, 0, window->editorInfo->buffer->length, CHAR_SELECT);
}

static void shiftLeftAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_LEFT, False);
}

static void shiftLeftTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_LEFT, True);
}

static void shiftRightAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_RIGHT, False);
}

static void shiftRightTabAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ShiftSelection(window, SHIFT_RIGHT, True);
}

static void findDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    DoFindReplaceDlog(window, searchDirection(0, args, nArgs), FIND_REPLACE_FIND_BUTTON_DEFAULT, timeOfEvent(event));
}

static void findAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "find action requires search string argument", "OK");
    	return;
    }
    SearchAndSelect(window, searchDirection(1, args, nArgs), args[0],
    	    searchType(1, args, nArgs));    
}

static void findIncrementalAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "find_incremental action requires search string argument", "OK");
    	return;
    }
    SearchAndSelectIncremental(window, searchDirection(1, args, nArgs), args[0],
    	    searchType(1, args, nArgs));    
}

static void findSameAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    SearchAndSelectSame(WidgetToWindow(w), searchDirection(0, args, nArgs));
}

static void findSelAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    SearchForSelected(WidgetToWindow(w), searchDirection(0, args, nArgs),
    	    timeOfEvent(event));
}

static void replaceDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    DoFindReplaceDlog(window, searchDirection(0, args, nArgs), FIND_REPLACE_REPLACE_BUTTON_DEFAULT, timeOfEvent(event));
}

static void replaceAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
		DialogF(DF_WARN, window->shell, 1, "replace action requires search and replace string arguments", "OK");
    	return;
    }
    SearchAndReplace(window, searchDirection(2, args, nArgs),
    	    args[0], args[1], searchType(2, args, nArgs));
}

static void replaceFindAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
		DialogF(DF_WARN, window->shell, 1, "replace_find action requires search and replace string arguments", "OK");
    	return;
    }
    ReplaceAndSearch(window, searchDirection(2, args, nArgs),
    	    args[0], args[1], searchType(2, args, nArgs));
}

static void replaceAllAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
		DialogF(DF_WARN, window->shell, 1, "replace_all action requires search and replace string arguments", "OK");
    	return;
    }
    ReplaceAll(window, args[0], args[1], searchType(2, args, nArgs));
}

static void replaceInSelAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    if (*nArgs < 2) {
		DialogF(DF_WARN, window->shell, 1, "replace_in_selection action requires search and replace string arguments", "OK");
    	return;
    }
    ReplaceInSelection(window, args[0], args[1],
    	    searchType(2, args, nArgs));
}

static void replaceSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ReplaceSame(window, searchDirection(0, args, nArgs));
}

static void replaceFindSameAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    ReplaceFindSame(window, searchDirection(0, args, nArgs));
}

static void gotoAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    int lineNum;
    
    if (*nArgs == 0 || sscanf(args[0], "%d", &lineNum) != 1) {
		DialogF(DF_WARN, window->shell, 1, "goto_line_number action requires line number argument", "OK");
    	return;
    }
    SelectNumberedLine(window, lineNum);
}

static void gotoDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoLineNumber(WidgetToWindow(w));
}

static void gotoSelectedAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoSelectedLineNumber(WidgetToWindow(w), timeOfEvent(event));
}

static void repeatDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    RepeatDialog(WidgetToWindow(w));
}

static void repeatMacroAP(Widget w, XEvent *event, String *args,
    	Cardinal *nArgs)
{
    int how;
    
    if (*nArgs != 2) {
    	fprintf(stderr, "NEdit: repeat_macro requires two arguments\n");
    	return;
    }
    if (!strcmp(args[0], "in_selection"))
	how = REPEAT_IN_SEL;
    else if (!strcmp(args[0], "to_end"))
	how = REPEAT_TO_END;
    else if (sscanf(args[0], "%d", &how) != 1) {
    	fprintf(stderr, "NEdit: repeat_macro requires method/count\n");
    	return;
    }
    RepeatMacro(WidgetToWindow(w), args[1], how);
}

static void markAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0 || strlen(args[0]) != 1 || !isalnum(args[0][0])) {
		DialogF(DF_WARN, window->shell, 1, "mark action requires a single-letter label argument", "OK");
    	return;
    }
    AddMark(window, w, args[0][0]);
}

static void markDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    MarkDialog(WidgetToWindow(w));
}

static void gotoMarkAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0 || strlen(args[0]) != 1 || !isalnum(args[0][0])) {
		DialogF(DF_WARN, window->shell, 1, "goto_mark action requires a single-letter label argument", "OK");
     	return;
    }
    GotoMark(WidgetToWindow(w), w, args[0][0], *nArgs > 1 &&
	    !strcmp(args[1], "extend"));
}

static void gotoMarkDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoMarkDialog(WidgetToWindow(w), *nArgs!=0 && !strcmp(args[0], "extend"));
}

static void findMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    MatchSelectedCharacter(WidgetToWindow(w));
}

static void gotoMatchingAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    GotoMatchingCharacter(WidgetToWindow(w));
}

static void findDefAP(Widget w, XEvent *event, String *args, Cardinal *nArgs) 
{
    FindDefinition(WidgetToWindow(w), timeOfEvent(event));
}

static void splitWindowAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    SplitWindow(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void closePaneAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    ClosePane(window);
    XtSetSensitive(window->splitWindowItem, window->nPanes < MAX_PANES);
    XtSetSensitive(window->closePaneItem, window->nPanes > 0);
}

static void cloneWindowAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    CreateSlaveWindow(WidgetToWindow(w));
}

static void capitalizeAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    UpcaseSelection(window);
}

static void lowercaseAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    DowncaseSelection(window);
}

static void fillAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    if(CheckReadOnly(window))
    	return;
    FillSelection(window);
}

static void controlDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    unsigned char charCodeString[2];
    char charCodeText[DF_MAX_PROMPT_LENGTH], *params[1];
    int charCode, nRead, response;
    
    if(CheckReadOnly(window))
    	return;
    response = DialogF(DF_PROMPT, window->shell, 2,
    	    "ASCII Character Code (decimal):", charCodeText, "OK", "Cancel");
    if (response == 2)
    	return;
    nRead = sscanf(charCodeText, "%d", &charCode);
    if (nRead != 1 || charCode < 0 || charCode >= 256) {
    	XBell(TheDisplay, 0);
	return;
    }
    charCodeString[0] = (unsigned char)charCode;
    charCodeString[1] = '\0';
    params[0] = (char *)charCodeString;
    if (!BufSubstituteNullChars((char *)charCodeString, 1, window->editorInfo->buffer)) {
	DialogF(DF_ERR, window->shell, 1, "Too much binary data","Dismiss");
	return;
    }
    XtCallActionProc(w, "insert_string", event, params, 1);
}

#ifndef VMS
static void filterDialogAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    char *params[1];
    static char cmdText[DF_MAX_PROMPT_LENGTH];
    int resp;
    static char **cmdHistory = NULL;
    static int nHistoryCmds = 0;
    
    if(CheckReadOnly(window))
    	return;
    if (!window->editorInfo->buffer->primary.selected) {
    	XBell(TheDisplay, 0);
	return;
    }
    
    SetDialogFPromptHistory(cmdHistory, nHistoryCmds);
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Shell command:   (use up arrow key to recall previous)",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    AddToHistoryList(cmdText, &cmdHistory, &nHistoryCmds);
    params[0] = cmdText;
    XtCallActionProc(w, "filter_selection", event, params, 1);
}

static void shellFilterAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if(CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "filter_selection action requires shell command argument", "OK");
    	return;
    }
    FilterSelection(window, args[0],
    	    event->xany.send_event == MACRO_EVENT_MARKER);
}

static void execDialogAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    char *params[1], cmdText[DF_MAX_PROMPT_LENGTH];
    int resp;
    static char **cmdHistory = NULL;
    static int nHistoryCmds = 0;

    if (CheckReadOnly(window))
    	return;
    SetDialogFPromptHistory(cmdHistory, nHistoryCmds);
    resp = DialogF(DF_PROMPT, window->shell, 2,
    	    "Shell command:   (use up arrow key to recall previous)",
    	    cmdText, "OK", "Cancel");
    if (resp == 2)
    	return;
    AddToHistoryList(cmdText, &cmdHistory, &nHistoryCmds);
    params[0] = cmdText;
    XtCallActionProc(w, "execute_command", event, params, 1);;
}

static void execAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if(CheckReadOnly(window))
    	return;
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "execute_command action requires shell command argument", "OK");
    	return;
    }
    ExecShellCommand(window, args[0],
    	    event->xany.send_event == MACRO_EVENT_MARKER);
}

static void execLineAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);

    if(CheckReadOnly(window))
    	return;
    ExecCursorLine(window, event->xany.send_event == MACRO_EVENT_MARKER);
}

static void shellMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "shell_menu_command action requires item-name argument", "OK");
    	return;
    }
    DoNamedShellMenuCmd(WidgetToWindow(w), args[0],
    	    event->xany.send_event == MACRO_EVENT_MARKER);
}
#endif

static void macroMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    if (*nArgs == 0) {
		DialogF(DF_WARN, window->shell, 1, "macro_menu_command action requires item-name argument", "OK");
    	return;
    }
    DoNamedMacroMenuCmd(window, args[0]);
}

static void bgMenuAP(Widget w, XEvent *event, String *args, Cardinal *nArgs)
{
    if (*nArgs == 0) {
    	fprintf(stderr,
    		"NEdit: bg_menu_command requires item-name argument\n");
    	return;
    }
    DoNamedBGMenuCmd(WidgetToWindow(w), args[0]);
}

static void beginningOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textBuffer *buf = TextGetBuffer(w);
    int start, end, isRect, rectStart, rectEnd;
    
    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    if (!isRect)
    	TextSetCursorPos(w, start);
    else
    	TextSetCursorPos(w, BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, start), rectStart));
}

static void endOfSelectionAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    textBuffer *buf = TextGetBuffer(w);
    int start, end, isRect, rectStart, rectEnd;
    
    if (!BufGetSelectionPos(buf, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    if (!isRect)
    	TextSetCursorPos(w, end);
    else
    	TextSetCursorPos(w, BufCountForwardDispChars(buf,
    		BufStartOfLine(buf, end), rectEnd));
}

/*
** Same as AddSubMenu from libNUtil.a but 1) mnemonic is optional (NEdit
** users like to be able to re-arrange the mnemonics so they can set Alt
** key combinations as accelerators), 2) supports the short/full option
** of SGI_CUSTOM mode, 3) optionally returns the cascade button widget
** in "cascadeBtn" if "cascadeBtn" is non-NULL.
*/
static Widget createMenu(Widget parent, char *name, char *label,
    	char mnemonic, Widget *cascadeBtn, int mode)
{
    Widget menu, cascade;
    XmString st1;
    
    menu = XmCreatePulldownMenu(parent, name, NULL, 0);
    cascade = XtVaCreateWidget(name, xmCascadeButtonWidgetClass, parent, 
    	XmNlabelString, st1=XmStringCreateSimple(label),
    	XmNsubMenuId, menu, NULL);
    XmStringFree(st1);
    if (mnemonic != 0)
    	XtVaSetValues(cascade, XmNmnemonic, mnemonic, NULL);
#ifdef SGI_CUSTOM
    if (mode == SHORT || !GetPrefShortMenus())
    	XtManageChild(cascade);
    if (mode == FULL)
    	addToToggleShortList(cascade);
#else
    XtManageChild(cascade);
#endif
    if (cascadeBtn != NULL)
    	*cascadeBtn = cascade;
    return menu;
}

/*
** Same as AddMenuItem from libNUtil.a without setting the accelerator
** (these are set in the fallback app-defaults so users can change them),
** and with the short/full option required in SGI_CUSTOM mode.
*/
static Widget createMenuItem(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int mode)
{
    Widget button;
    XmString st1;
    

    button = XtVaCreateWidget(name, xmPushButtonWidgetClass, parent, 
    	    XmNlabelString, st1=XmStringCreateSimple(label),
    	    XmNmnemonic, mnemonic, NULL);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)callback, cbArg);
    XmStringFree(st1);
#ifdef SGI_CUSTOM
    if (mode == SHORT || !GetPrefShortMenus())
    	XtManageChild(button);
    if (mode == FULL)
    	addToToggleShortList(button);
    XtVaSetValues(button, XmNuserData, PERMANENT_MENU_ITEM, NULL);
#else
    XtManageChild(button);
#endif
    return button;
}

/*
** "fake" menu items allow accelerators to be attached, but don't show up
** in the menu.  They are necessary to process the shifted menu items because
** Motif does not properly process the event descriptions in accelerator
** resources, and you can't specify "shift key is optional"
*/
static Widget createFakeMenuItem(Widget parent, char *name,
	menuCallbackProc callback, void *cbArg)
{
    Widget button;
    XmString st1;
    
    button = XtVaCreateManagedWidget(name, xmPushButtonWidgetClass, parent,
    	    XmNlabelString, st1=XmStringCreateSimple(""),
    	    XmNshadowThickness, 0,
    	    XmNmarginHeight, 0,
    	    XmNheight, 0, NULL);
    XtAddCallback(button, XmNactivateCallback, (XtCallbackProc)callback, cbArg);
    XmStringFree(st1);
    XtVaSetValues(button, XmNtraversalOn, False, NULL);

    return button;
}

/*
** Add a toggle button item to an already established pull-down or pop-up
** menu, including mnemonics, accelerators and callbacks.
*/
static Widget createMenuToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set,
	int mode)
{
    Widget button;
    XmString st1;
    
    button = XtVaCreateWidget(name, xmToggleButtonWidgetClass, parent, 
    	    XmNlabelString, st1=XmStringCreateSimple(label),
    	    XmNmnemonic, mnemonic,
    	    XmNset, set, NULL);
    XtAddCallback(button, XmNvalueChangedCallback, (XtCallbackProc)callback,
    	    cbArg);
    XmStringFree(st1);
#ifdef SGI_CUSTOM
    if (mode == SHORT || !GetPrefShortMenus())
    	XtManageChild(button);
    if (mode == FULL)
    	addToToggleShortList(button);
    XtVaSetValues(button, XmNuserData, PERMANENT_MENU_ITEM, NULL);
#else
    XtManageChild(button);
#endif
    return button;
}

/*
** Create a toggle button with a diamond (radio-style) appearance
*/
static Widget createMenuRadioToggle(Widget parent, char *name, char *label,
	char mnemonic, menuCallbackProc callback, void *cbArg, int set,
	int mode)
{
    Widget button;
    button = createMenuToggle(parent, name, label, mnemonic, callback, cbArg,
	    set, mode);
    XtVaSetValues(button, XmNindicatorType, XmONE_OF_MANY, NULL);
    return button;
}

static Widget createMenuSeparator(Widget parent, char *name, int mode)
{
    Widget button;
    
    button = XmCreateSeparator(parent, name, NULL, 0);
#ifdef SGI_CUSTOM
    if (mode == SHORT || !GetPrefShortMenus())
    	XtManageChild(button);
    if (mode == FULL)
    	addToToggleShortList(button);
    XtVaSetValues(button, XmNuserData, PERMANENT_MENU_ITEM, NULL);
#else
    XtManageChild(button);
#endif
    return button;
}

/*
** Make sure the close menu item is dimmed appropriately for the current
** set of windows.  It should be dim only for the last Untitled, unmodified,
** editor window, and sensitive otherwise.
*/
void CheckCloseDim(void)
{
    WindowInfo *window;
    
    if (WindowList == NULL)
    	return;
    if (WindowList->next==NULL &&
    	    !WindowList->editorInfo->filenameSet && !WindowList->editorInfo->fileChanged) {
    	XtSetSensitive(WindowList->closeItem, FALSE);
    	return;
    }
    
    for (window=WindowList; window!=NULL; window=window->next)
    	XtSetSensitive(window->closeItem, True);
}

/*
** Invalidate the Window menus of all NEdit windows to but don't change
** the menus until they're needed (Originally, this was "UpdateWindowMenus",
** but creating and destroying menu items for every window every time a
** new window was created or something changed, made things move very
** slowly with more than 10 or so windows).
*/
void InvalidateWindowMenus(void)
{
    WindowInfo *w;

    /* Mark the window menus invalid (to be updated when the user pulls one
       down), unless the menu is torn off, meaning it is visible to the user
       and should be updated immediately */
    for (w=WindowList; w!=NULL; w=w->next) {
    	if (!XmIsMenuShell(XtParent(w->windowMenuPane)))
    	    updateWindowMenu(w);
    	else
    	    w->windowMenuValid = False;
    }
}

/*
** Mark the Previously Opened Files menus of all NEdit windows as invalid.
** Since actually changing the menus is slow, they're just marked and updated
** when the user pulls one down.
*/
static void invalidatePrevOpenMenus(void)
{
    WindowInfo *w;

    /* Mark the menus invalid (to be updated when the user pulls one
       down), unless the menu is torn off, meaning it is visible to the user
       and should be updated immediately */
    for (w=WindowList; w!=NULL; w=w->next) {
    	if (XmIsMenuShell(XtParent(w->windowMenuPane)))
    	    updatePrevOpenMenu(w);
    	else
    	    w->prevOpenMenuValid = False;
    }
}

/*
** Add a file to the list of previously opened files for display in the
** File menu.
*/
void AddToPrevOpenMenu(char *filename)
{
    int i;
    char *nameCopy;
    WindowInfo *w;
    
    /* If the Open Previous command is disabled, just return */
    if (GetPrefMaxPrevOpenFiles() == 0)
    	return;
    
    /* If the name is already in the list, move it to the start */
    for (i=0; i<NPrevOpen; i++) {
    	if (!FILECOMPARE(filename, PrevOpen[i])) {
    	    nameCopy = PrevOpen[i];
    	    memmove(&PrevOpen[1], &PrevOpen[0], sizeof(char *) * i);
    	    PrevOpen[0] = nameCopy;
    	    invalidatePrevOpenMenus();
    	    return;
    	}
    }
    
    /* If the list is already full, make room */
    if (NPrevOpen == GetPrefMaxPrevOpenFiles())
    	XtFree(PrevOpen[--NPrevOpen]);
    
    /* Add it to the list */
    nameCopy = XtMalloc(strlen(filename) + 1);
    strcpy(nameCopy, filename);
    memmove(&PrevOpen[1], &PrevOpen[0], sizeof(char *) * NPrevOpen);
    PrevOpen[0] = nameCopy;
    NPrevOpen++;
    
    /* Mark the Previously Opened Files menu as invalid in all windows */
    invalidatePrevOpenMenus();

    /* Undim the menu in all windows if it was previously empty */
    if (NPrevOpen == 1)
    	for (w=WindowList; w!=NULL; w=w->next)
    	    XtSetSensitive(w->prevOpenMenuItem, True);
    
    /* Write the menu contents to disk to restore in later sessions */
    WriteNEditDB();
}

/*
** Update the Window menu of a single window to reflect the current state of
** all NEdit windows as determined by the global WindowList.
*/
static void updateWindowMenu(WindowInfo *window)
{
    WindowInfo *w;
    Widget btn;
    WidgetList items;
    int nItems, n;
	XtArgVal userData;
    XmString st1;
    int i, nWindows, windowIndex;
    WindowInfo **windows;
    
    /* Make a sorted list of windows */
    for (w=WindowList, nWindows=0; w!=NULL; w=w->next, nWindows++);
    windows = (WindowInfo **)XtMalloc(sizeof(WindowInfo *) * nWindows);
    for (w=WindowList, i=0; w!=NULL; w=w->next, i++)
    	windows[i] = w;
    qsort(windows, nWindows, sizeof(WindowInfo *), compareWindowNames);
    
    /* While it is not possible on some systems (ibm at least) to substitute
       a new menu pane, it is possible to substitute menu items, as long as
       at least one remains in the menu at all times. This routine assumes
       that the menu contains permanent items marked with the value PERMANENT
       _MENU_ITEM in the userData resource, and adds and removes items which
       it marks with the value TEMPORARY_MENU_ITEM */
    
    /* Go thru all of the items in the menu and rename them to
       match the window list.  Delete any extras */
    XtVaGetValues(window->windowMenuPane, XmNchildren, &items,
    	    XmNnumChildren, &nItems,NULL);
    windowIndex = 0;
    for (n=0; n<nItems; n++) {
    	XtVaGetValues(items[n], XmNuserData, &userData, NULL);
    	if (userData == TEMPORARY_MENU_ITEM) {
	    if (windowIndex >= nWindows) {
    		/* unmanaging before destroying stops parent from displaying */
    		XtUnmanageChild(items[n]);
    		XtDestroyWidget(items[n]);	    	
	    } else {
		char title[MAXPATHLEN * 2 + 1];
    	strcpy(title, FormatWindowTitle(windows[windowIndex]));
		/* Append the full pathname to the end of the title if set */
		if(windows[windowIndex]->editorInfo->filenameSet) {
			strcat(title, " - ");
			strcat(title, windows[windowIndex]->editorInfo->path);
		}
		XtVaSetValues(items[n], XmNlabelString,
    	    		st1=XmStringCreateSimple(title), NULL);
		XtRemoveAllCallbacks(items[n], XmNactivateCallback);
		XtAddCallback(items[n], XmNactivateCallback,
			(XtCallbackProc)raiseCB, windows[windowIndex]);
	    	XmStringFree(st1);
		windowIndex++;
	    }
	}
    }
    
    /* Add new items for the titles of the remaining windows to the menu */
    for (; windowIndex<nWindows; windowIndex++) {
		char title[MAXPATHLEN * 2 + 1];
    	strcpy(title, FormatWindowTitle(windows[windowIndex]));
		/* Append the full pathname to the end of the title if set */
		if(windows[windowIndex]->editorInfo->filenameSet) {
			strcat(title, " - ");
			strcat(title, windows[windowIndex]->editorInfo->path);
		}
    	btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
    		window->windowMenuPane, 
    		XmNlabelString, st1=XmStringCreateSimple(title),
    		XmNuserData, TEMPORARY_MENU_ITEM, NULL);
	XtAddCallback(btn, XmNactivateCallback, (XtCallbackProc)raiseCB, 
	    	windows[windowIndex]);
    	XmStringFree(st1);
    }
    XtFree((char *)windows);
}

/*
** Update the Previously Opened Files menu of a single window to reflect the
** current state of the list as retrieved from GetPrevOpenFiles.
** Thanks to Markus Schwarzenberg for the sorting part.
*/
static void updatePrevOpenMenu(WindowInfo *window)
{
    Widget btn;
    WidgetList items;
    int nItems, n, index;
    XmString st1;
    char **prevOpenSorted;
                
    /* Sort the previously opened file list */
    prevOpenSorted = (char **)XtMalloc(NPrevOpen * sizeof(char*));
    memcpy(prevOpenSorted, PrevOpen, NPrevOpen * sizeof(char*));
    qsort(prevOpenSorted, NPrevOpen, sizeof(char*), cmpStrPtr);

    /* Go thru all of the items in the menu and rename them to match the file
       list.  In older Motifs (particularly ibm), it was dangerous to replace
       a whole menu pane, which would be much simpler.  However, since the
       code was already written for the Windows menu and is well tested, I'll
       stick with this weird method of re-naming the items */
    XtVaGetValues(window->prevOpenMenuPane, XmNchildren, &items,
            XmNnumChildren, &nItems,NULL);
    index = 0;
    for (n=0; n<nItems; n++) {
        if (index >= NPrevOpen) {
            /* unmanaging before destroying stops parent from displaying */
            XtUnmanageChild(items[n]);
            XtDestroyWidget(items[n]);          
        } else {
            XtVaSetValues(items[n], XmNlabelString,
                    st1=XmStringCreateSimple(prevOpenSorted[index]), NULL);
            XtRemoveAllCallbacks(items[n], XmNactivateCallback);
            XtAddCallback(items[n], XmNactivateCallback,
                    (XtCallbackProc)openPrevCB, prevOpenSorted[index]);
            XmStringFree(st1);
            index++;
        }
    }
    
    /* Add new items for the remaining file names to the menu */
    for (; index<NPrevOpen; index++) {
        btn = XtVaCreateManagedWidget("win", xmPushButtonWidgetClass,
                window->prevOpenMenuPane, 
                XmNlabelString, st1=XmStringCreateSimple(prevOpenSorted[index]),
                XmNmarginHeight, 0,
                XmNuserData, TEMPORARY_MENU_ITEM, NULL);
        XtAddCallback(btn, XmNactivateCallback, (XtCallbackProc)openPrevCB, 
                prevOpenSorted[index]);
        XmStringFree(st1);
    }
                
    XtFree((char*)prevOpenSorted);
}

/*
** Comparison function for sorting file names for the Open Previous submenu
*/
static int cmpStrPtr(const void *strA, const void *strB)
{                       
    return FILECOMPARE(*((char**)strA), *((char**)strB));
}

/*
** Write dynamic database of file names for "Open Previous".  Eventually,
** this may hold window positions, and possibly file marks, in which case,
** it should be moved to a different module, but for now it's just a list
** of previously opened files.
*/
void WriteNEditDB(void)
{
    char fullName[MAXPATHLEN];
    FILE *fp;
    int i;
    static char fileHeader[] =
    	    "# File name database for NEdit Open Previous command\n";
    
    /* If the Open Previous command is disabled, just return */
    if (GetPrefMaxPrevOpenFiles() == 0)
    	return;

    /* the nedit database file resides in the home directory, prepend the
       contents of the $HOME environment variable */
#ifdef VMS
    sprintf(fullName, "%s%s", "SYS$LOGIN:", NEDIT_DB_FILE_NAME);
#else
    sprintf(fullName, "%s/%s", getenv("HOME"), NEDIT_DB_FILE_NAME);
#endif /*VMS*/

    /* open the file */
    if ((fp = fopen(fullName, "w")) == NULL)
    	return;
    
    /* write the file header text to the file */
    fprintf(fp, "%s", fileHeader);
    
    /* Write the list of file names */
    for (i=0; i<NPrevOpen; i++)
    	fprintf(fp, "%s\n", PrevOpen[i]);

    /* Close the file */
    fclose(fp);
}

/*
** Read dynamic database of file names for "Open Previous".  Eventually,
** this may hold window positions, and possibly file marks, in which case,
** it should be moved to a different module, but for now it's just a list
** of previously opened files.  This routine should only be called once,
** at startup time, before any windows are open.
*/
void ReadNEditDB(void)
{
    char fullName[MAXPATHLEN], line[MAXPATHLEN];
    char *nameCopy;
    FILE *fp;
    int lineLen;
    
    /* If the Open Previous command is disabled, just return */
    if (GetPrefMaxPrevOpenFiles() == 0)
    	return;
    
    /* Initialize the files list and allocate a (permanent) block memory
       of the size prescribed by the maxPrevOpenFiles resource */
    PrevOpen = (char **)XtMalloc(sizeof(char *) * GetPrefMaxPrevOpenFiles());
    NPrevOpen = 0;
    
    /* the nedit database file resides in the home directory, prepend the
       contents of the $HOME environment variable */
#ifdef VMS
    sprintf(fullName, "%s%s", "SYS$LOGIN:", NEDIT_DB_FILE_NAME);
#else
    sprintf(fullName, "%s/%s", getenv("HOME"), NEDIT_DB_FILE_NAME);
#endif /*VMS*/

    /* open the file */
    if ((fp = fopen(fullName, "r")) == NULL)
    	return;

    /* read lines of the file, lines beginning with # are considered to be
       comments and are thrown away.  Lines are subject to cursory checking,
       then just copied to the Open Previous file menu list */
    while (True) {
    	if (fgets(line, MAXPATHLEN, fp) == NULL) {
    	    fclose(fp);
    	    return;					 /* end of file */
    	}
    	if (line[0] == '#')
    	    continue;
    	lineLen = strlen(line); 			     /* comment */
    	if (line[lineLen-1] != '\n') {
    	    fprintf(stderr, ".neditdb line too long\n");
    	    fclose(fp);
	    return;		      /* no newline, probably truncated */
	}
    	line[--lineLen] = '\0';
    	if (lineLen == 0)
    	    continue;		/* blank line */
	if (strcspn(line, BadFilenameChars) != lineLen) {
    	    fprintf(stderr, ".neditdb file is corrupted\n");
    	    fclose(fp);
	    return;			     /* non-filename characters */
	}
	nameCopy = XtMalloc(lineLen+1);
	strcpy(nameCopy, line);
	PrevOpen[NPrevOpen++] = nameCopy;
    	if (NPrevOpen >= GetPrefMaxPrevOpenFiles()) {
    	    fclose(fp);
    	    return;				    /* too many entries */
    	}
    }
}

static void setWindowSizeDefault(int rows, int cols)
{
    SetPrefRows(rows);
    SetPrefCols(cols);
    updateWindowSizeMenus();
}

static void updateWindowSizeMenus(void)
{
    WindowInfo *win;
    
    for (win=WindowList; win!=NULL; win=win->next)
    	updateWindowSizeMenu(win);
}

static void updateWindowSizeMenu(WindowInfo *win)
{
    int rows = GetPrefRows(), cols = GetPrefCols();
    char title[50];
    XmString st1;
    
    XmToggleButtonSetState(win->size24x80DefItem, (rows==24&&cols==80),False);
    XmToggleButtonSetState(win->size40x80DefItem, (rows==40&&cols==80),False);
    XmToggleButtonSetState(win->size60x80DefItem, (rows==60&&cols==80),False);
    XmToggleButtonSetState(win->size80x80DefItem, (rows==80&&cols==80),False);
    if ((rows!=24 && rows!=40 && rows!=60 && rows!=80) || cols!=80) {
    	XmToggleButtonSetState(win->sizeCustomDefItem, True, False);
    	sprintf(title, "Custom... (%d x %d)", rows, cols);
    	XtVaSetValues(win->sizeCustomDefItem,
    	    	XmNlabelString, st1=XmStringCreateSimple(title), NULL);
    	XmStringFree(st1);
    } else {
    	XmToggleButtonSetState(win->sizeCustomDefItem, False, False);
    	XtVaSetValues(win->sizeCustomDefItem,
    	    	XmNlabelString, st1=XmStringCreateSimple("Custom..."), NULL);
    	XmStringFree(st1);
    }
}

static void setPrefCheckingMode(WindowInfo *window, CheckingMode mode)
{
    SetPrefCheckingMode(mode);
}

/*
** Scans action argument list for arguments "forward" or "backward" to
** determine search direction for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchDirection(int ignoreArgs, String *args, Cardinal *nArgs)
{
    int i;
    
    for (i=ignoreArgs; i<*nArgs; i++) {
    	if (!StrCaseCmp(args[i], "forward"))
    	    return SEARCH_FORWARD;
    	if (!StrCaseCmp(args[i], "backward"))
    	    return SEARCH_BACKWARD;
    }
    return SEARCH_FORWARD;
}

/*
** Scans action argument list for arguments "literal", "case" or "regex" to
** determine search type for search and replace actions.  "ignoreArgs"
** tells the routine how many required arguments there are to ignore before
** looking for keywords
*/
static int searchType(int ignoreArgs, String *args, Cardinal *nArgs)
{
    int i;
    
    for (i=ignoreArgs; i<*nArgs; i++) {
    	if (!StrCaseCmp(args[i], "literal"))
    	    return SEARCH_LITERAL;
    	if (!StrCaseCmp(args[i], "case"))
    	    return SEARCH_CASE_SENSE;
    	if (!StrCaseCmp(args[i], "regex"))
    	    return SEARCH_REGEX;
    }
    return GetPrefSearch();
}

/*
** Return a pointer to the string describing search direction for search action
** routine parameters given a callback XmAnyCallbackStruct pointed to by
** "callData", by checking if the shift key is pressed (for search callbacks).
*/
static char **shiftKeyToDir(XtPointer callData)
{
    static char *backwardParam[1] = {"backward"};
    static char *forwardParam[1] = {"forward"};
    if (((XmAnyCallbackStruct *)callData)->event->xbutton.state & ShiftMask)
    	return backwardParam;
    return forwardParam;
}

static void raiseCB(Widget w, WindowInfo *window, caddr_t callData)
{
    RaiseShellWindow(window->shell);
}

static void openPrevCB(Widget w, char *name, caddr_t callData)
{
    char *params[1];
#if XmVersion >= 1002
    Widget menu = XmGetPostedFromWidget(XtParent(w)); /* If menu is torn off */
#else
    Widget menu = w;
#endif
    
    params[0] = name;
    XtCallActionProc(WidgetToWindow(menu)->lastFocus, "open",
    	    ((XmAnyCallbackStruct *)callData)->event, params, 1);
    CheckCloseDim();
}

/*
** Comparison function for sorting windows by title for the window menu.
** Windows are sorted by Untitled and then alphabetically by filename and
** then alphabetically by path.
*/
static int compareWindowNames(const void *windowA, const void *windowB)
{
	int rc;
	const WindowInfo *a = *((WindowInfo**)windowA);
	const WindowInfo *b = *((WindowInfo**)windowB);
	/* Untitled first */
	rc = a->editorInfo->filenameSet == b->editorInfo->filenameSet ? 0 : 
	     a->editorInfo->filenameSet && !b->editorInfo->filenameSet ? 1 : -1;
	if(rc != 0)
		return rc;
	rc = FILECOMPARE(a->editorInfo->filename, b->editorInfo->filename);
	if(rc != 0)
		return rc;
	rc = FILECOMPARE(a->editorInfo->path, b->editorInfo->path);
	return rc;
}

/*
** Create popup for right button programmable menu
*/
Widget CreateBGMenu(WindowInfo *window)
{
    Arg args[1];
    
    /* There is still some mystery here.  It's important to get the XmNmenuPost
       resource set to the correct menu button, or the menu will not post
       properly, but there's also some danger that it will take over the entire
       button and interfere with text widget translations which use the button
       with modifiers.  I don't entirely understand why it works properly now
       when it failed often in development, and certainly ignores the ~ syntax
       in translation event specifications. */
    XtSetArg(args[0], XmNmenuPost, GetPrefBGMenuBtn());
    return XmCreatePopupMenu(window->textArea, "bgMenu", args, 1);
}

/*
** Add a translation to the text widget to trigger the background menu using
** the mouse-button + modifier combination specified in the resource:
** nedit.bgMenuBtn.
*/
void AddBGMenuAction(Widget widget)
{
    static XtTranslations table = NULL;

    if (table == NULL) {
	char translations[MAX_ACCEL_LEN + 25];
	sprintf(translations, "%s: post_window_bg_menu()\n",GetPrefBGMenuBtn());
    	table = XtParseTranslationTable(translations);
    }
    XtOverrideTranslations(widget, table);
}

static void bgMenuPostAP(Widget w, XEvent *event, String *args,
	Cardinal *nArgs)
{
    WindowInfo *window = WidgetToWindow(w);
    
    /* The Motif popup handling code BLOCKS events while the menu is posted,
       including the matching btn-up events which complete various dragging
       operations which it may interrupt.  Cancel to head off problems */
    XtCallActionProc(window->lastFocus, "process_cancel", event, NULL, 0);
    
    /* Pop up the menu */
    XmMenuPosition(window->bgMenuPane, (XButtonPressedEvent *)event);
    XtManageChild(window->bgMenuPane); 
    XtPopup(XtParent(window->bgMenuPane), XtGrabNonexclusive);
    XtMapWidget(XtParent(window->bgMenuPane));
    XtMapWidget(window->bgMenuPane);
}

static Time timeOfEvent(XEvent *event)
{
	return event->type == KeyPress ? event->xkey.time :
	event->type == KeyRelease ? event->xkey.time :
	event->type == ButtonPress ? event->xbutton.time :
	event->type == ButtonRelease ? event->xbutton.time :
	event->type == MotionNotify ? event->xmotion.time :
	event->type == EnterNotify ? event->xcrossing.time :
	event->type == LeaveNotify ? event->xcrossing.time :
	event->type == PropertyNotify ? event->xproperty.time :
	CurrentTime;
}

#ifdef SGI_CUSTOM
static void shortMenusCB(Widget w, WindowInfo *window, caddr_t callData)
{
    WindowInfo *win;
    int i, state = XmToggleButtonGetState(w);
    Widget parent;

    /* Set the preference */
    SetPrefShortMenus(state);
    
    /* Re-create the menus for all windows */
    for (win=WindowList; win!=NULL; win=win->next) {
    	for (i=0; i<win->nToggleShortItems; i++) {
    	    if (state)
    	    	XtUnmanageChild(win->toggleShortItems[i]);
    	    else
    	    	XtManageChild(win->toggleShortItems[i]);
    	}
    }
    if (GetPrefShortMenus())
    	SaveNEditPrefs(window->shell, True);
}

static void addToToggleShortList(Widget w)
{
    if (ShortMenuWindow->nToggleShortItems >= MAX_SHORTENED_ITEMS) {
    	fprintf(stderr,"NEdit, internal error: increase MAX_SHORTENED_ITEMS\n");
    	return;
    }
    ShortMenuWindow->toggleShortItems[ShortMenuWindow->nToggleShortItems++] = w;
}   	       

/*
** Present the user a dialog for specifying whether or not a short
** menu mode preference should be applied toward the default setting.
** Return True if user requested to reset and save the default value.
** If operation was canceled, will return toggle widget "w" to it's 
** original (opposite) state.
*/
static int shortPrefAskDefault(Widget parent, Widget w, char *settingName)
{
    char msg[100] = "";
    
    if (!GetPrefShortMenus()) {
    	return False;
    }
    
    sprintf(msg, "%s: %s\nSave as default for future windows as well?",
    	    settingName, XmToggleButtonGetState(w) ? "On" : "Off");
    switch(DialogF (DF_QUES, parent, 3, msg, "Yes", "No", "Cancel")) {
        case 1: /* yes */
            return True;
        case 2: /* no */
           return False;
        case 3: /* cancel */
            XmToggleButtonSetState(w, !XmToggleButtonGetState(w), False);
            return False;
    }
    return False; /* not reached */
}
#endif
