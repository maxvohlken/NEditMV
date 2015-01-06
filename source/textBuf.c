/*******************************************************************************
*                                                                              *
* textBuf.c - Manage source text for one or more text areas                    *
*                                                                              *
* Copyright (c) 1991 Universities Research Association, Inc.                   *
* All rights reserved.                                                         *
*                                                                              *
* This material resulted from work developed under a Government Contract and   *
* is subject to the following license:  The Government retains a paid-up,      *
* nonexclusive, irrevocable worldwide license to reproduce, prepare derivative *
* works, perform publicly and display publicly by or for the Government,       *
* including the right to distribute to other Government contractors.  Neither  *
* the United States nor the United States Department of Energy, nor any of     *
* their employees, makes any warranty, express or implied, or assumes any      *
* legal liability or responsibility for the accuracy, completeness, or         *
* usefulness of any information, apparatus, product, or process disclosed, or  *
* represents that its use would not infringe privately owned rights.           *
*                                                                              *
* Fermilab Nirvana GUI Library                                                 *
* June 15, 1995                                                                *
*                                                                              *
* Written by Mark Edel                                                         *
*                                                                              *
*******************************************************************************/
#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <Xm/Xm.h>
#include "textBuf.h"

#define PREFERRED_GAP_SIZE 80	/* Initial size for the buffer gap (empty space
                                   in the buffer where text might be inserted
                                   if the user is typing sequential chars) */

#define normalizePosition(buf, pos) \
	if (*(pos) > (buf)->length) *(pos) = (buf)->length; \
	if (*(pos) < 0) *(pos) = 0;
	
#define normalizePositions(buf, start, end) \
	normalizePosition((buf), (start)); \
	normalizePosition((buf), (end)); \
	if (*(start) > *(end)) { \
		int temp = *(start); \
		*(start) = *(end); \
		*(end) = temp; \
	}
	
#ifndef normalizePosition
static void normalizePosition(textBuffer *buf, int *pos);
#endif
#ifndef normalizePositions
static void normalizePositions(textBuffer *buf, int *start, int *end);
#endif
static void histogramCharacters(char *string, int length, char hist[256],
	int init);
static void subsChars(char *string, int length, char fromChar, char toChar);
static char chooseNullSubsChar(char hist[256]);
static int insert(textBuffer *buf, int pos, char *text);
static void deleteRange(textBuffer *buf, int start, int end);
static void deleteRect(textBuffer *buf, int start, int end, int rectStart,
	int rectEnd, int *replaceLen, int *endPos);
static void insertCol(textBuffer *buf, int column, int startPos, char *insText,
	int *nDeleted, int *nInserted, int *endPos, int *insertWidth);
static void overlayRect(textBuffer *buf, int startPos, int rectStart,
    	int rectEnd, char *insText, int *nDeleted, int *nInserted, int *endPos);
static void insertColInLine(char *line, char *insLine, int column, int insWidth,
	int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
	int *endOffset);
static void deleteRectFromLine(char *line, int rectStart, int rectEnd,
	int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
	int *endOffset);
static void overlayRectInLine(char *line, char *insLine, int rectStart,
    	int rectEnd, int tabDist, int useTabs, char nullSubsChar, char *outStr,
    	int *outLen, int *endOffset);
static void callModifyCBs(textBuffer *buf, int pos, int nDeleted,
	int nInserted, int nRestyled, char *deletedText);
static void redisplaySelection(textBuffer *buf, selection *oldSelection,
	selection *newSelection);
static void moveGap(textBuffer *buf, int pos);
static void reallocateBuf(textBuffer *buf, int newGapStart, int newGapLen);
static void setSelection(selection *sel, int start, int end, selType type);
static void setRectSelect(selection *sel, int start, int end,
	int rectStart, int rectEnd);
static void updateSelections(textBuffer *buf, int pos, int nDeleted,
	int nInserted);
static void updateSelection(selection *sel, int pos, int nDeleted,
	int nInserted);
static int getSelectionPos(selection *sel, int *start, int *end,
        int *isRect, int *rectStart, int *rectEnd);
static char *getSelectionText(textBuffer *buf, selection *sel);
static void removeSelected(textBuffer *buf, selection *sel);
static void replaceSelected(textBuffer *buf, selection *sel, char *text, Boolean staySelected);
static void addPadding(char *string, int startIndent, int toIndent,
	int tabDist, int useTabs, char nullSubsChar, int *charsAdded);
static int searchForward(textBuffer *buf, int startPos, char searchChar,
	int *foundPos);
static int searchBackward(textBuffer *buf, int startPos, char searchChar,
	int *foundPos);
static char *copyLine(char *text, int *lineLen);
static int countLines(char *string);
static int textWidth(char *text, int tabDist, char nullSubsChar);
static void findRectSelBoundariesForCopy(textBuffer *buf, int lineStartPos,
	int rectStart, int rectEnd, int *selStart, int *selEnd);
static char *realignTabs(char *text, int origIndent, int newIndent,
	int tabDist, int useTabs, char nullSubsChar, int *newLength);
static char *expandTabs(char *text, int startIndent, int tabDist,
	char nullSubsChar, int *newLen);
static char *unexpandTabs(char *text, int startIndent, int tabDist,
	char nullSubsChar, int *newLen);
#ifndef _WIN32
static int max(int i1, int i2);
static int min(int i1, int i2);
#endif

static char *ControlCodeTable[32] = {
     "nul", "soh", "stx", "etx", "eot", "enq", "ack", "bel",
     "bs", "ht", "nl", "vt", "np", "cr", "so", "si",
     "dle", "dc1", "dc2", "dc3", "dc4", "nak", "syn", "etb",
     "can", "em", "sub", "esc", "fs", "gs", "rs", "us"};

/*
** Create an empty text buffer
*/
textBuffer *BufCreate(void)
{
    return BufCreatePreallocated(0);
}

/*
** Create an empty text buffer of a pre-determined size (use this to
** avoid unnecessary re-allocation if you know exactly how much the buffer
** will need to hold
*/
textBuffer *BufCreatePreallocated(int requestedSize)
{
    textBuffer *buf;
    
    buf = (textBuffer *)XtMalloc(sizeof(textBuffer));
    buf->length = 0;
    buf->buf = XtMalloc(requestedSize + PREFERRED_GAP_SIZE);
    buf->gapStart = 0;
    buf->gapEnd = PREFERRED_GAP_SIZE;
    buf->tabDist = 8;
    buf->useTabs = True;
    setSelection(&buf->primary, 0, 0, CHAR_SELECT);
    setSelection(&buf->secondary, 0, 0, CHAR_SELECT);
    setSelection(&buf->highlight, 0, 0, CHAR_SELECT);
    buf->modifyProcs = NULL;
    buf->cbArgs = NULL;
    buf->nModifyProcs = 0;
    buf->nullSubsChar = '\0';
#ifdef PURIFY
    {int i; for (i=buf->gapStart; i<buf->gapEnd; i++) buf->buf[i] = '.';}
#endif
    return buf;
}

/*
** Free a text buffer
*/
void BufFree(textBuffer *buf)
{
    XtFree(buf->buf);
    if (buf->nModifyProcs != 0) {
    	XtFree((char *)buf->modifyProcs);
    	XtFree((char *)buf->cbArgs);
    }
    XtFree((char *)buf);
}

/*
** Get the entire contents of a text buffer.  Memory is allocated to contain
** the returned string, which the caller must free. An additional byte is
** also allocated in case the caller wants to add a newline at the end of
** the buffer.
*/
char *BufGetAll(textBuffer *buf)
{
    char *text;
    
    text = XtMalloc(buf->length+2);
    memcpy(text, buf->buf, buf->gapStart);
    memcpy(&text[buf->gapStart], &buf->buf[buf->gapEnd],
            buf->length - buf->gapStart);
    text[buf->length] = '\0';
    return text;
}

/*
** Replace the entire contents of the text buffer
*/
void BufSetAll(textBuffer *buf, char *text)
{
    int length, deletedLength;
    char *deletedText;
    
    /* Save information for redisplay, and get rid of the old buffer */
    deletedText = BufGetAll(buf);
    deletedLength = buf->length;
    XtFree(buf->buf);
    
    /* Start a new buffer with a gap of PREFERRED_GAP_SIZE in the center */
    length = strlen(text);
    buf->buf = XtMalloc(length + PREFERRED_GAP_SIZE);
    buf->length = length;
    buf->gapStart = length/2;
    buf->gapEnd = buf->gapStart + PREFERRED_GAP_SIZE;
    memcpy(buf->buf, text, buf->gapStart);
    memcpy(&buf->buf[buf->gapEnd], &text[buf->gapStart], length-buf->gapStart);
#ifdef PURIFY
    {int i; for (i=buf->gapStart; i<buf->gapEnd; i++) buf->buf[i] = '.';}
#endif
    
    /* Zero all of the existing selections */
    updateSelections(buf, 0, deletedLength, 0);
    
    /* Call the saved display routine(s) to update the screen */
    callModifyCBs(buf, 0, deletedLength, length, 0, deletedText);
    XtFree(deletedText);
}

/*
** Return a copy of the text between "start" and "end" character positions
** from text buffer "buf".  Positions start at 0, and the range does not
** include the character pointed to by "end". Characters are always returned
** going forward in the buffer, meaning if end is greater that start then
** start become the end and end becomes the start.
*/
char *BufGetRange(textBuffer *buf, int start, int end)
{
    char *text;
    int length, part1Length;
    
    /* Make sure start and end are ok, and allocate memory for returned string.
       If start is bad, return "", if end is bad, adjust it. */
    if (start < 0 || start > buf->length) {
    	text = XtMalloc(1);
	text[0] = '\0';
        return text;
    }
    normalizePositions(buf, &start, &end);
    length = end - start;
    text = XtMalloc(length+1);
    
    /* Copy the text from the buffer to the returned string */
    if (end <= buf->gapStart) {
        memcpy(text, &buf->buf[start], length);
    } else if (start >= buf->gapStart) {
        memcpy(text, &buf->buf[start+(buf->gapEnd-buf->gapStart)], length);
    } else {
        part1Length = buf->gapStart - start;
        memcpy(text, &buf->buf[start], part1Length);
        memcpy(&text[part1Length], &buf->buf[buf->gapEnd], length-part1Length);
    }
    text[length] = '\0';
    return text;
}

/*
** Return the character at buffer position "pos".  Positions start at 0.
*/
#ifndef BufGetCharacter
unsigned char BufGetCharacter(textBuffer *buf, int pos)
{
	/* Guard against array bounds reads. Note that pos == buf->length 
	** is also an array bounds read. 
	*/
    if (pos < 0 || pos >= buf->length) {
#ifdef PURIFY
		purify_printf_with_call_chain("Internal error: BufGetCharacter(pos < 0 || pos >= buf->length): pos: %d buf->length: %d\n", pos, buf->length);
#endif
        return '\0';
    }
    return _BufGetCharacter(buf, pos);
}
#endif

/*
** Return a copy of the text from the start of line "startLineNum" to
** the end of line "endLineNum". The line at "endLineNum" is included.
** Lines are numbered starting at 1. Negative line numbers are used
** to count lines from the end of the file. Line number 0 refers to the
** end of the file. When endLineNum is greater than startLineNum it means
** you will get the text from the end of endLineNum to the beginning of
** startLineNum.
*/
char *BufGetLines(textBuffer *buf, int startLineNum, int endLineNum)
{
    int lineStart, lineEnd;

	if(startLineNum == 0) { /* EOF */
		lineStart = buf->length;
	} else if(startLineNum < 0) { /* startLineNum is from the end of the file */
    	lineStart = BufCountBackwardNLines(buf, buf->length-1, -startLineNum - 1);
	} else {
    	lineStart = BufCountForwardNLines(buf, 0, startLineNum - 1);
	}
	/* lineEnd should be 1 character greater than the actual end of line */
    if(endLineNum == 0 || endLineNum == -1) { 
    	/* EOF and end of line -1 are the same place. */
    	lineEnd = buf->length;
    } else if(endLineNum < 0) {
    	/* endLineNum is from the end of the file */
    	/* nLines == 0 means beginning of the last line. endLineNum == -2 is also
    	** the beginning of the last line. So adjust. */
    	lineEnd = BufCountBackwardNLines(buf, buf->length-1, -endLineNum - 2); 
    } else { 
    	/* Find the end position relative to the starting line if 
    	** the ending line is greater that the starting line. If it is less
    	** than then just count from the begining of the file. */
		int nLines = endLineNum - startLineNum;
    	if(nLines >= 0) {
    		lineEnd = BufCountForwardNLines(buf, lineStart, nLines + 1);
    	} else {
    		lineEnd = BufCountForwardNLines(buf, 0, endLineNum);
    	}
    }
    
    /* BufGetRange will handle if lineEnd is greater than lineStart */
    return BufGetRange(buf, lineStart, lineEnd);
}

/*
** Insert null-terminated string "text" at position "pos" in "buf"
*/
void BufInsert(textBuffer *buf, int pos, char *text)
{
    int nInserted;
    
    /* if pos is not contiguous to existing text, make it */
    normalizePosition(buf, &pos);

    /* insert and redisplay */
    nInserted = insert(buf, pos, text);
    buf->cursorPosHint = pos + nInserted;
    callModifyCBs(buf, pos, 0, nInserted, 0, NULL);
}

/*
** Delete the characters between "start" and "end", and insert the
** null-terminated string "text" in their place in in "buf"
*/
int BufReplace(textBuffer *buf, int start, int end, char *text)
{
    char *deletedText;
    int nInserted;
    
    normalizePositions(buf, &start, &end);
    deletedText = BufGetRange(buf, start, end);
    deleteRange(buf, start, end);
    nInserted = insert(buf, start, text);
    buf->cursorPosHint = start + nInserted;
    callModifyCBs(buf, start, end-start, nInserted, 0, deletedText);
    XtFree(deletedText);
    return nInserted;
}

void BufRemove(textBuffer *buf, int start, int end)
{
    char *deletedText;
    
    /* Make sure the arguments make sense */
    normalizePositions(buf, &start, &end);

    /* Remove and redisplay */
    deletedText = BufGetRange(buf, start, end);
    deleteRange(buf, start, end);
    buf->cursorPosHint = start;
    callModifyCBs(buf, start, end-start, 0, 0, deletedText);
    XtFree(deletedText);
}

void BufCopyFromBuf(textBuffer *fromBuf, textBuffer *toBuf, int fromStart,
    	int fromEnd, int toPos)
{
    int length = fromEnd - fromStart;
    int part1Length;

    /* Prepare the buffer to receive the new text.  If the new text fits in
       the current buffer, just move the gap (if necessary) to where
       the text should be inserted.  If the new text is too large, reallocate
       the buffer with a gap large enough to accomodate the new text and a
       gap of PREFERRED_GAP_SIZE */
    if (length > toBuf->gapEnd - toBuf->gapStart)
    	reallocateBuf(toBuf, toPos, length + PREFERRED_GAP_SIZE);
    else if (toPos != toBuf->gapStart)
	moveGap(toBuf, toPos);
    
    /* Insert the new text (toPos now corresponds to the start of the gap) */
    if (fromEnd <= fromBuf->gapStart) {
        memcpy(&toBuf->buf[toPos], &fromBuf->buf[fromStart], length);
    } else if (fromStart >= fromBuf->gapStart) {
        memcpy(&toBuf->buf[toPos],
            	&fromBuf->buf[fromStart+(fromBuf->gapEnd-fromBuf->gapStart)],
            	length);
    } else {
        part1Length = fromBuf->gapStart - fromStart;
        memcpy(&toBuf->buf[toPos], &fromBuf->buf[fromStart], part1Length);
        memcpy(&toBuf->buf[toPos+part1Length], &fromBuf->buf[fromBuf->gapEnd],
            	length-part1Length);
    }
    toBuf->gapStart += length;
    toBuf->length += length;
    updateSelections(toBuf, toPos, 0, length);
} 

/*
** Insert "text" columnwise into buffer starting at displayed character
** position "column" on the line beginning at "startPos".  Opens a rectangular
** space the width and height of "text", by moving all text to the right of
** "column" right.  If charsInserted and charsDeleted are not NULL, the
** number of characters inserted and deleted in the operation (beginning
** at startPos) are returned in these arguments
*/
void BufInsertCol(textBuffer *buf, int column, int startPos, char *text,
    	int *charsInserted, int *charsDeleted)
{
    int nLines, lineStartPos, nDeleted, insertDeleted, nInserted, insertWidth;
    char *deletedText;
    
    nLines = countLines(text);
    lineStartPos = BufStartOfLine(buf, startPos);
    nDeleted = BufEndOfLine(buf, BufCountForwardNLines(buf, startPos, nLines)) -
    	    lineStartPos;
    deletedText = BufGetRange(buf, lineStartPos, lineStartPos + nDeleted);
    insertCol(buf, column, lineStartPos, text, &insertDeleted, &nInserted,
    	    &buf->cursorPosHint, &insertWidth);
    if (nDeleted != insertDeleted)
    	fprintf(stderr, "internal consistency check ins1 failed");
    callModifyCBs(buf, lineStartPos, nDeleted, nInserted, 0, deletedText);
    XtFree(deletedText);
    if (charsInserted != NULL)
    	*charsInserted = nInserted;
    if (charsDeleted != NULL)
    	*charsDeleted = nDeleted;
}

/*
** Overlay "text" between displayed character positions "rectStart" and
** "rectEnd" on the line beginning at "startPos".  If charsInserted and
** charsDeleted are not NULL, the number of characters inserted and deleted
** in the operation (beginning at startPos) are returned in these arguments.
*/
void BufOverlayRect(textBuffer *buf, int startPos, int rectStart,
    	int rectEnd, char *text, int *charsInserted, int *charsDeleted)
{
    int nLines, lineStartPos, nDeleted, insertDeleted, nInserted;
    char *deletedText;
    
    nLines = countLines(text);
    lineStartPos = BufStartOfLine(buf, startPos);
    nDeleted = BufEndOfLine(buf, BufCountForwardNLines(buf, startPos, nLines)) -
    	    lineStartPos;
    deletedText = BufGetRange(buf, lineStartPos, lineStartPos + nDeleted);
    overlayRect(buf, lineStartPos, rectStart, rectEnd, text, &insertDeleted,
    	    &nInserted, &buf->cursorPosHint);
    if (nDeleted != insertDeleted)
    	fprintf(stderr, "internal consistency check ovly1 failed");
    callModifyCBs(buf, lineStartPos, nDeleted, nInserted, 0, deletedText);
    XtFree(deletedText);
    if (charsInserted != NULL)
    	*charsInserted = nInserted;
    if (charsDeleted != NULL)
    	*charsDeleted = nDeleted;
}

/*
** Replace a rectangular area in buf, given by "start", "end", "rectStart",
** and "rectEnd", with "text".  If "text" is vertically longer than the
** rectangle, add extra lines to make room for it.
*/
void BufReplaceRect(textBuffer *buf, int start, int end, int rectStart,
	int rectEnd, char *text, BufRectangle *rectReturn)
{
    char *deletedText, *insText, *insPtr;
    int i, nInsertedLines, nDeletedLines, insLen, hint;
    int insertDeleted, insertInserted, deleteInserted, insertWidth;
    int linesPadded = 0;
    
    /* Make sure start and end refer to complete lines, since the
       columnar delete and insert operations will replace whole lines */
    start = BufStartOfLine(buf, start);
    end = BufEndOfLine(buf, end);
    
    /* If more lines will be deleted than inserted, pad the inserted text
       with newlines to make it as long as the number of deleted lines.  This
       will indent all of the text to the right of the rectangle to the same
       column.  If more lines will be inserted than deleted, insert extra
       lines in the buffer at the end of the rectangle to make room for the
       additional lines in "text" */
    nInsertedLines = countLines(text);
    nDeletedLines = BufCountLines(buf, start, end);
    if (nInsertedLines < nDeletedLines) {
    	insLen = strlen(text);
    	insText = XtMalloc(insLen + nDeletedLines - nInsertedLines + 1);
    	strcpy(insText, text);
    	insPtr = insText + insLen;
    	for (i=0; i<nDeletedLines-nInsertedLines; i++)
    	    *insPtr++ = '\n';
    	*insPtr = '\0';
    } else if (nDeletedLines < nInsertedLines) {
    	linesPadded = nInsertedLines-nDeletedLines;
    	for (i=0; i<linesPadded; i++)
    	    insert(buf, end, "\n");
	insText = text;
    } else /* nDeletedLines == nInsertedLines */
	insText = text;
    
    /* Save a copy of the text which will be modified for the modify CBs */
    deletedText = BufGetRange(buf, start, end);
    	  
    /* Delete then insert */
    deleteRect(buf, start, end, rectStart, rectEnd, &deleteInserted, &hint);
    insertCol(buf, rectStart, start, insText, &insertDeleted, &insertInserted,
    	    &buf->cursorPosHint, &insertWidth);
    
    /* Figure out how many chars were inserted and call modify callbacks */
    if (insertDeleted != deleteInserted + linesPadded)
    	fprintf(stderr, "NEdit: internal consistency check repl1 failed\n");
    callModifyCBs(buf, start, end-start, insertInserted, 0, deletedText);
    XtFree(deletedText);
    if (nInsertedLines < nDeletedLines)
    	XtFree(insText);
    
    rectReturn->start = start;
    rectReturn->end = buf->cursorPosHint;
    rectReturn->rectStart = rectStart;
    rectReturn->rectEnd = rectStart + insertWidth;
}

/*
** Remove a rectangular swath of characters between character positions start
** and end and horizontal displayed-character offsets rectStart and rectEnd.
*/
void BufRemoveRect(textBuffer *buf, int start, int end, int rectStart,
	int rectEnd)
{
    char *deletedText;
    int nInserted;
    
    start = BufStartOfLine(buf, start);
    end = BufEndOfLine(buf, end);
    deletedText = BufGetRange(buf, start, end);
    deleteRect(buf, start, end, rectStart, rectEnd, &nInserted,
    	    &buf->cursorPosHint);
    callModifyCBs(buf, start, end-start, nInserted, 0, deletedText);
    XtFree(deletedText);
}

/*
** Clear a rectangular "hole" out of the buffer between character positions
** start and end and horizontal displayed-character offsets rectStart and
** rectEnd.
*/
void BufClearRect(textBuffer *buf, int start, int end, int rectStart,
	int rectEnd)
{
    int i, nLines;
    char *newlineString;
    
    normalizePositions(buf, &start, &end);
    nLines = BufCountLines(buf, start, end);
    newlineString = XtMalloc(nLines+1);
    for (i=0; i<nLines; i++)
    	newlineString[i] = '\n';
    newlineString[i] = '\0';
    BufOverlayRect(buf, start, rectStart, rectEnd, newlineString,
    	    NULL, NULL);
    XtFree(newlineString);
}

char *BufGetTextInRect(textBuffer *buf, int start, int end,
	int rectStart, int rectEnd)
{
    int lineStart, selLeft, selRight, len;
    char *textOut, *textIn, *outPtr, *retabbedStr;
   
    start = BufStartOfLine(buf, start);
    end = BufEndOfLine(buf, end);
    textOut = XtMalloc((end - start) + 1);
    lineStart = start;
    outPtr = textOut;
    while (lineStart <= end) {
        findRectSelBoundariesForCopy(buf, lineStart, rectStart, rectEnd,
        	&selLeft, &selRight);
        textIn = BufGetRange(buf, selLeft, selRight);
        len = selRight - selLeft;
        memcpy(outPtr, textIn, len);
        XtFree(textIn);
        outPtr += len;
        lineStart = BufEndOfLine(buf, selRight) + 1;
        *outPtr++ = '\n';
    }
    if (outPtr != textOut)
    	outPtr--;  /* don't leave trailing newline */
    *outPtr = '\0';
    
    /* If necessary, realign the tabs in the selection as if the text were
       positioned at the left margin */
    retabbedStr = realignTabs(textOut, rectStart, 0, buf->tabDist,
    	    buf->useTabs, buf->nullSubsChar, &len);
    XtFree(textOut);
    return retabbedStr;
}

/*
** Get the hardware tab distance used by all displays for this buffer,
** and used in computing offsets for rectangular selection operations.
*/
int BufGetTabDistance(textBuffer *buf)
{
    return buf->tabDist;
}

/*
** Set the hardware tab distance used by all displays for this buffer,
** and used in computing offsets for rectangular selection operations.
*/
void BufSetTabDistance(textBuffer *buf, int tabDist)
{
    char *deletedText;
    
    /* Change the tab setting */
    buf->tabDist = tabDist;
    
    /* Force any display routines to redisplay everything (unfortunately,
       this means copying the whole buffer contents to provide "deletedText" */
    deletedText = BufGetAll(buf);
    callModifyCBs(buf, 0, buf->length, buf->length, 0, deletedText);
    XtFree(deletedText);
}

void BufSelect(textBuffer *buf, int start, int end, selType type)
{
    selection oldSelection = buf->primary;

    normalizePositions(buf, &start, &end);
    setSelection(&buf->primary, start, end, type);
    redisplaySelection(buf, &oldSelection, &buf->primary);
}

void BufUnselect(textBuffer *buf)
{
    selection oldSelection = buf->primary;

    buf->primary.selected = False;
    buf->primary.type = CHAR_SELECT;
    redisplaySelection(buf, &oldSelection, &buf->primary);
}

void BufRectSelect(textBuffer *buf, int start, int end, int rectStart,
        int rectEnd)
{
    selection oldSelection = buf->primary;

    normalizePositions(buf, &start, &end);
    setRectSelect(&buf->primary, start, end, rectStart, rectEnd);
    redisplaySelection(buf, &oldSelection, &buf->primary);
}

int BufGetSelectionPos(textBuffer *buf, int *start, int *end,
        int *isRect, int *rectStart, int *rectEnd)
{
    return getSelectionPos(&buf->primary, start, end, isRect, rectStart,
    	    rectEnd);
}

char *BufGetSelectionText(textBuffer *buf)
{
    return getSelectionText(buf, &buf->primary);
}

selType BufGetSelectionType(textBuffer *buf)
{
    return buf->primary.type;
}

void BufRemoveSelected(textBuffer *buf)
{
    removeSelected(buf, &buf->primary);
}

void BufReplaceSelected(textBuffer *buf, char *text, Boolean staySelected)
{
    replaceSelected(buf, &buf->primary, text, staySelected);
}

void BufSecondarySelect(textBuffer *buf, int start, int end, selType type)
{
    selection oldSelection = buf->secondary;

    normalizePositions(buf, &start, &end);
    setSelection(&buf->secondary, start, end, type);
    redisplaySelection(buf, &oldSelection, &buf->secondary);
}

void BufSecondaryUnselect(textBuffer *buf)
{
    selection oldSelection = buf->secondary;

    buf->secondary.selected = False;
    buf->secondary.type = CHAR_SELECT;
    redisplaySelection(buf, &oldSelection, &buf->secondary);
}

void BufSecRectSelect(textBuffer *buf, int start, int end,
        int rectStart, int rectEnd)
{
    selection oldSelection = buf->secondary;

    normalizePositions(buf, &start, &end);
    setRectSelect(&buf->secondary, start, end, rectStart, rectEnd);
    redisplaySelection(buf, &oldSelection, &buf->secondary);
}

int BufGetSecSelectPos(textBuffer *buf, int *start, int *end,
        int *isRect, int *rectStart, int *rectEnd)
{
    return getSelectionPos(&buf->secondary, start, end, isRect, rectStart,
    	    rectEnd);
}

char *BufGetSecSelectText(textBuffer *buf)
{
    return getSelectionText(buf, &buf->secondary);
}

selType BufGetSecSelectType(textBuffer *buf)
{
    return buf->secondary.type;
}

void BufRemoveSecSelect(textBuffer *buf)
{
    removeSelected(buf, &buf->secondary);
}

void BufReplaceSecSelect(textBuffer *buf, char *text)
{
    replaceSelected(buf, &buf->secondary, text, False);
}

void BufHighlight(textBuffer *buf, int start, int end)
{
    selection oldSelection = buf->highlight;

    normalizePositions(buf, &start, &end);
    setSelection(&buf->highlight, start, end, CHAR_SELECT);
    redisplaySelection(buf, &oldSelection, &buf->highlight);
}

void BufUnhighlight(textBuffer *buf)
{
    selection oldSelection = buf->highlight;

    buf->highlight.selected = False;
    buf->highlight.type = CHAR_SELECT;
    redisplaySelection(buf, &oldSelection, &buf->highlight);
}

void BufRectHighlight(textBuffer *buf, int start, int end,
        int rectStart, int rectEnd)
{
    selection oldSelection = buf->highlight;

    normalizePositions(buf, &start, &end);
    setRectSelect(&buf->highlight, start, end, rectStart, rectEnd);
    redisplaySelection(buf, &oldSelection, &buf->highlight);
}

int BufGetHighlightPos(textBuffer *buf, int *start, int *end,
        int *isRect, int *rectStart, int *rectEnd)
{
    return getSelectionPos(&buf->highlight, start, end, isRect, rectStart,
    	    rectEnd);
}

char *BufGetHighlightText(textBuffer *buf)
{
    return getSelectionText(buf, &buf->highlight);
}

/*
** Add a callback routine to be called when the buffer is modified
*/
void BufAddModifyCB(textBuffer *buf, bufModifyCallbackProc bufModifiedCB,
	void *cbArg)
{
    bufModifyCallbackProc *newModifyProcs;
    void **newCBArgs;
    int i;
    
    newModifyProcs = (bufModifyCallbackProc *)
    	    XtMalloc(sizeof(bufModifyCallbackProc *) * (buf->nModifyProcs+1));
    newCBArgs = (void *)XtMalloc(sizeof(void *) * (buf->nModifyProcs+1));
    for (i=0; i<buf->nModifyProcs; i++) {
    	newModifyProcs[i] = buf->modifyProcs[i];
    	newCBArgs[i] = buf->cbArgs[i];
    }
    if (buf->nModifyProcs != 0) {
	XtFree((char *)buf->modifyProcs);
	XtFree((char *)buf->cbArgs);
    }
    newModifyProcs[buf->nModifyProcs] = bufModifiedCB;
    newCBArgs[buf->nModifyProcs] = cbArg;
    buf->nModifyProcs++;
    buf->modifyProcs = newModifyProcs;
    buf->cbArgs = newCBArgs;
}

void BufRemoveModifyCB(textBuffer *buf, bufModifyCallbackProc bufModifiedCB,
	void *cbArg, Boolean warnIfNotFound)
{
    int i, toRemove = -1;
    bufModifyCallbackProc *newModifyProcs;
    void **newCBArgs;

    /* find the matching callback to remove */
    for (i=0; i<buf->nModifyProcs; i++) {
    	if (buf->modifyProcs[i] == bufModifiedCB && buf->cbArgs[i] == cbArg) {
    	    toRemove = i;
    	    break;
    	}
    }
    if (toRemove == -1) {
    	if(warnIfNotFound) {
    		fprintf(stderr, "Internal Error: Can't find modify CB to remove\n");
	    }
    	return;
    }
    
    /* Allocate new lists for remaining callback procs and args (if
       any are left) */
    buf->nModifyProcs--;
    if (buf->nModifyProcs == 0) {
    	buf->nModifyProcs = 0;
    	XtFree((char *)buf->modifyProcs);
    	buf->modifyProcs = NULL;
	XtFree((char *)buf->cbArgs);
	buf->cbArgs = NULL;
	return;
    }
    newModifyProcs = (bufModifyCallbackProc *)
    	    XtMalloc(sizeof(bufModifyCallbackProc *) * (buf->nModifyProcs));
    newCBArgs = (void *)XtMalloc(sizeof(void *) * (buf->nModifyProcs));
    
    /* copy out the remaining members and free the old lists */
    for (i=0; i<toRemove; i++) {
    	newModifyProcs[i] = buf->modifyProcs[i];
    	newCBArgs[i] = buf->cbArgs[i];
    }
    for (; i<buf->nModifyProcs; i++) {
	newModifyProcs[i] = buf->modifyProcs[i+1];
    	newCBArgs[i] = buf->cbArgs[i+1];
    }
    XtFree((char *)buf->modifyProcs);
    XtFree((char *)buf->cbArgs);
    buf->modifyProcs = newModifyProcs;
    buf->cbArgs = newCBArgs;
}

/*
** Return the text from the entire line containing position "pos"
*/
char *BufGetLineText(textBuffer *buf, int pos)
{
    return BufGetRange(buf, BufStartOfLine(buf, pos), BufEndOfLine(buf, pos));
}

/*
** Find the position of the start of the line containing position "pos"
*/
int BufStartOfLine(textBuffer *buf, int pos)
{
    int startPos;
    
    /* if pos is not contiguous to existing text, make it */
    normalizePosition(buf, &pos);
    
    if (!searchBackward(buf, pos, '\n', &startPos))
    	return 0;
    return startPos + 1;
}


/*
** Find the position of the end of the line containing position "pos"
** (which is either a pointer to the newline character ending the line,
** or a pointer to one character beyond the end of the buffer)
*/
int BufEndOfLine(textBuffer *buf, int pos)
{
    int endPos;
    
    /* if pos is not contiguous to existing text, make it */
    normalizePosition(buf, &pos);

    if (!searchForward(buf, pos, '\n', &endPos))
    	endPos = buf->length;
    return endPos;
}

/*
** Get a character from the text buffer expanded into it's screen
** representation (which may be several characters for a tab or a
** control code).  Returns the number of characters written to "outStr".
** "indent" is the number of characters from the start of the line
** for figuring tabs.  Output string is guranteed to be shorter or
** equal in length to MAX_EXP_CHAR_LEN
*/
int BufGetExpandedChar(textBuffer *buf, int pos, int indent, char *outStr)
{
    return BufExpandCharacter(BufGetCharacter(buf, pos), indent, outStr,
    	    buf->tabDist, buf->nullSubsChar);
}

/*
** Expand a single character from the text buffer into it's screen
** representation (which may be several characters for a tab or a
** control code).  Returns the number of characters added to "outStr".
** "indent" is the number of characters from the start of the line
** for figuring tabs.  Output string is guranteed to be shorter or
** equal in length to MAX_EXP_CHAR_LEN
*/
int BufExpandCharacter(char c, int indent, char *outStr, int tabDist,
	char nullSubsChar)
{
    int i, nSpaces;
    
    /* Convert tabs to spaces */
	if (c == '\t') {
		nSpaces = tabDist - (indent % tabDist);
		for (i=0; i<nSpaces; i++)
	    	outStr[i] = ' ';
		outStr[nSpaces] = 0;
		return nSpaces;
	}
    
    /* Convert control codes to readable character sequences */
    /*... is this safe with international character sets? */
#if 1
# ifdef _WIN32
    if (c == '\r') {
#  if 0
		outStr[0] = 0;
		return 0;
#  else
#   if 0
		strcpy(outStr, "<>");
		return 2;
#   else
		outStr[0] = 0xac;
		outStr[1] = 0;
		return 1;
#   endif
#  endif
    } else 
# endif
#endif
    if (((unsigned char)c) <= 31) {
    	sprintf(outStr, "<%s>", ControlCodeTable[(unsigned char)c]);
    	return strlen(outStr);
    } else if (c == 127) {
    	strcpy(outStr, "<del>");
    	return 5;
    } else if (c == nullSubsChar) {
		strcpy(outStr, "<nul>");
    	return 5;
    }
    
    /* Otherwise, just return the character */
    outStr[0] = c;
	outStr[1] = 0;
    return 1;
}

/*
** Return the length in displayed characters of character "c" expanded
** for display (as discussed above in BufGetExpandedChar).  If the
** buffer for which the character width is being measured is doing null
** substitution, nullSubsChar should be passed as that character (or nul
** to ignore).
*/
int BufCharWidth(unsigned char c, int indent, int tabDist, char nullSubsChar)
{
    /* Note, this code must parallel that in BufExpandCharacter */
    if (c == '\t')
	return tabDist - (indent % tabDist);
    else if ((unsigned char)c <= 31)
    	return strlen(ControlCodeTable[(unsigned char)c]) + 2;
    else if (c == 127)
    	return 5;
    else if (c == (unsigned char)nullSubsChar)
    	return 5;
    return 1;
}

/*
** Count the number of displayed characters between buffer position
** "lineStartPos" and "targetPos". (displayed characters are the characters
** shown on the screen to represent characters in the buffer, where tabs and
** control characters are expanded)
*/
int BufCountDispChars(textBuffer *buf, int lineStartPos, int targetPos)
{
    int pos, charCount = 0;
    char expandedChar[MAX_EXP_CHAR_LEN];
    
    normalizePositions(buf, &lineStartPos, &targetPos);
    pos = lineStartPos;
    while (pos < targetPos)
    	charCount += BufGetExpandedChar(buf, pos++, charCount, expandedChar);
    return charCount;
}

/*
** Count forward from buffer position "startPos" in displayed characters
** (displayed characters are the characters shown on the screen to represent
** characters in the buffer, where tabs and control characters are expanded)
*/
int BufCountForwardDispChars(textBuffer *buf, int lineStartPos, int nChars)
{
    int pos, charCount = 0;
    char c;
    
    normalizePosition(buf, &lineStartPos);
    pos = lineStartPos;
    while (charCount < nChars && pos < buf->length) {
    	c = _BufGetCharacter(buf, pos);
    	if (c == '\n')
    	    return pos;
    	charCount += BufCharWidth(c, charCount, buf->tabDist,buf->nullSubsChar);
    	pos++;
    }
    return pos;
}

/*
** Count the number of newlines between startPos and endPos in buffer "buf".
** The character at position "endPos" is not counted.
*/
int BufCountLines(textBuffer *buf, int startPos, int endPos)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    int lineCount = 0;
    
    normalizePositions(buf, &startPos, &endPos);
    pos = startPos;
    while (pos < buf->gapStart) {
        if (pos == endPos)
            return lineCount;
        if (buf->buf[pos++] == '\n')
            lineCount++;
    }
    while (pos < buf->length) {
        if (pos == endPos)
            return lineCount;
    	if (buf->buf[pos++ + gapLen] == '\n')
            lineCount++;
    }
    return lineCount;
}

/*
** Find the first character of the line "nLines" forward from "startPos"
** in "buf" and return its position
*/
int BufCountForwardNLines(textBuffer *buf, int startPos, int nLines)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    int lineCount = 0;
    
    if (nLines == 0)
    	return startPos;
    
    normalizePosition(buf, &startPos);
    pos = startPos;
    while (pos < buf->gapStart) {
        if (buf->buf[pos++] == '\n') {
            lineCount++;
            if (lineCount == nLines)
            	return pos;
        }
    }
    while (pos < buf->length) {
    	if (buf->buf[pos++ + gapLen] == '\n') {
            lineCount++;
            if (lineCount >= nLines)
            	return pos;
        }
    }
    return pos;
}

/*
** Find the position of the first character of the line "nLines" backwards
** from "startPos" (not counting the character pointed to by "startpos" if
** that is a newline) in "buf".  nLines == 0 means find the beginning of
** the line
*/
int BufCountBackwardNLines(textBuffer *buf, int startPos, int nLines)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    int lineCount = -1;
    
    normalizePosition(buf, &startPos);
    /* backup one if startPos is pointing at a newline or at EOF. */
    pos = startPos;
    if(pos == buf->length || _BufGetCharacter(buf, pos) == '\n') {
    	pos = startPos - 1;
    }
    if (pos <= 0)
    	return 0;
    
    while (pos >= buf->gapStart) {
    	if (buf->buf[pos + gapLen] == '\n') {
            if (++lineCount >= nLines)
            	return pos + 1;
        }
        pos--;
    }
    while (pos >= 0) {
        if (buf->buf[pos] == '\n') {
            if (++lineCount >= nLines)
            	return pos + 1;
        }
        pos--;
    }
    return 0;
}

/*
** Search forwards in buffer "buf" for characters in "searchChars", starting
** with the character "startPos", and returning the result in "foundPos"
** returns True if found, False if not.
*/
int BufSearchForward(textBuffer *buf, int startPos, char *searchChars,
	int *foundPos)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    char *c;
    
    pos = startPos;
    while (pos < buf->gapStart) {
        for (c=searchChars; *c!='\0'; c++) {
	    if (buf->buf[pos] == *c) {
        	*foundPos = pos;
        	return True;
            }
        }
        pos++;
    }
    while (pos < buf->length) {
    	for (c=searchChars; *c!='\0'; c++) {
    	    if (buf->buf[pos + gapLen] == *c) {
        	*foundPos = pos;
        	return True;
            }
        }
        pos++;
    }
    *foundPos = buf->length;
    return False;
}

/*
** Search backwards in buffer "buf" for characters in "searchChars", starting
** with the character BEFORE "startPos", returning the result in "foundPos"
** returns True if found, False if not.
*/
int BufSearchBackward(textBuffer *buf, int startPos, char *searchChars,
	int *foundPos)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    char *c;
    
    if (startPos == 0) {
    	*foundPos = 0;
    	return False;
    }
    pos = startPos == 0 ? 0 : startPos - 1;
    while (pos >= buf->gapStart) {
    	for (c=searchChars; *c!='\0'; c++) {
    	    if (buf->buf[pos + gapLen] == *c) {
        	*foundPos = pos;
        	return True;
            }
        }
        pos--;
    }
    while (pos >= 0) {
    	for (c=searchChars; *c!='\0'; c++) {
            if (buf->buf[pos] == *c) {
        	*foundPos = pos;
        	return True;
            }
        }
        pos--;
    }
    *foundPos = 0;
    return False;
}

#ifndef normalizePosition
/*
** Routine to make sure the position make sense for the current buffer.
*/
static void normalizePosition(textBuffer *buf, int *pos)
{
	/* Make sure the position does not exceed the dimensions of the buffer. */
	if (*pos > buf->length) *pos = buf->length;
	if (*pos < 0) *pos = 0;
}
#endif

#ifndef normalizePositions
/*
** Routine to make sure the start and end positions make sense for
** the current buffer.
*/
static void normalizePositions(textBuffer *buf, int *start, int *end)
{
	/* Make sure neither start nor end exceed the dimensions of the buffer. */
	normalizePosition(buf, start);
	normalizePosition(buf, end);

	/* Make sure end is greater than start */
	if (*start > *end) {
		int temp = *start;
		*start = *end;
		*end = temp;
	}
}
#endif

/*
** A horrible design flaw in NEdit (from the very start, before we knew that
** NEdit would become so popular), is that it uses C NULL terminated strings
** to hold text.  This means editing text containing NUL characters is not
** possible without special consideration.  Here is the special consideration.
** The routines below maintain a special substitution-character which stands
** in for a null, and translates strings an buffers back and forth from/to
** the substituted form, figure out what to substitute, and figure out
** when we're in over our heads and no translation is possible.
*/

/*
** The primary routine for integrating new text into a text buffer with
** substitution of another character for ascii nuls.  This substitutes null
** characters in the string in preparation for being copied or replaced
** into the buffer, and if neccessary, adjusts the buffer as well, in the
** event that the string contains the character it is currently using for
** substitution.  Returns False, if substitution is no longer possible
** because all non-printable characters are already in use.
*/
int BufSubstituteNullChars(char *string, int length, textBuffer *buf)
{
    char histogram[256];

    /* Find out what characters the string contains */
    histogramCharacters(string, length, histogram, True);
    
    /* Does the string contain the null-substitute character?  If so, re-
       histogram the buffer text to find a character which is ok in both the
       string and the buffer, and change the buffer's null-substitution
       character.  If none can be found, give up and return False */
    if (histogram[(unsigned char)buf->nullSubsChar] != 0) {
	char *bufString, newSubsChar;
	bufString = BufGetAll(buf);
	histogramCharacters(bufString, buf->length, histogram, False);
	newSubsChar = chooseNullSubsChar(histogram);
	if (newSubsChar == '\0')
	    return False;
	subsChars(bufString, buf->length, buf->nullSubsChar, newSubsChar);
	deleteRange(buf, 0, buf->length);
	insert(buf, 0, bufString);
	XtFree(bufString);
	buf->nullSubsChar = newSubsChar;
    }

    /* If the string contains null characters, substitute them with the
       buffer's null substitution character */
    if (histogram[0] != 0)
	subsChars(string, length, '\0', buf->nullSubsChar);
    return True;
}

/*
** Convert strings obtained from buffers which contain null characters, which
** have been substituted for by a special substitution character, back to
** a null-containing string.  There is no time penalty for calling this
** routine if no substitution has been done.
*/
void BufUnsubstituteNullChars(char *string, textBuffer *buf)
{
    register char *c, subsChar = buf->nullSubsChar;
    
    if (subsChar == '\0')
	return;
    for (c=string; *c != '\0'; c++)
    	if (*c == subsChar)
	    *c = '\0';
}

/*
** Create a pseudo-histogram of the characters in a string (don't actually
** count, because we don't want overflow, just mark the character's presence
** with a 1).  If init is true, initialize the histogram before acumulating.
** if not, add the new data to an existing histogram.
*/
static void histogramCharacters(char *string, int length, char hist[256],
	int init)
{
    int i;
    char *c;

    if (init)
	for (i=0; i<256; i++)
	    hist[i] = 0;
    for (c=string; c < &string[length]; c++)
        hist[*((unsigned char *)c)] |= 1;
}

/*
** Substitute fromChar with toChar in string.
*/
static void subsChars(char *string, int length, char fromChar, char toChar)
{
    char *c;
    
    for (c=string; c < &string[length]; c++)
	if (*c == fromChar) *c = toChar;
}

/*
** Search through ascii control characters in histogram in order of least
** likelihood of use, find an unused character to use as a stand-in for a
** null.  If the character set is full (no available characters outside of
** the printable set, return the null character.
*/
static char chooseNullSubsChar(char hist[256])
{
#define N_REPLACEMENTS 25
    static char replacements[N_REPLACEMENTS] = {1,2,3,4,5,6,14,15,16,17,18,19,
	    20,21,22,23,24,25,26,28,29,30,31,11,7};
    int i;
    for (i = 0; i < N_REPLACEMENTS; i++)
	if (hist[replacements[i]] == 0)
	    return replacements[i];
    return '\0';
}
	    
/*
** Internal (non-redisplaying) version of BufInsert.  Returns the length of
** text inserted (this is just strlen(text), however this calculation can be
** expensive and the length will be required by any caller who will continue
** on to call redisplay).  pos must be contiguous with the existing text in
** the buffer (i.e. not past the end).
*/
static int insert(textBuffer *buf, int pos, char *text)
{
    int length = strlen(text);

    /* Prepare the buffer to receive the new text.  If the new text fits in
       the current buffer, just move the gap (if necessary) to where
       the text should be inserted.  If the new text is too large, reallocate
       the buffer with a gap large enough to accomodate the new text and a
       gap of PREFERRED_GAP_SIZE */
    if (length > buf->gapEnd - buf->gapStart)
    	reallocateBuf(buf, pos, length + PREFERRED_GAP_SIZE);
    else if (pos != buf->gapStart)
	moveGap(buf, pos);
    
    /* Insert the new text (pos now corresponds to the start of the gap) */
    memcpy(&buf->buf[pos], text, length);
    buf->gapStart += length;
    buf->length += length;
    updateSelections(buf, pos, 0, length);
    
    return length;
}

/*
** Internal (non-redisplaying) version of BufRemove.  Removes the contents
** of the buffer between start and end (and moves the gap to the site of
** the delete).
*/
static void deleteRange(textBuffer *buf, int start, int end)
{
    /* if the gap is not contiguous to the area to remove, move it there */
    if (start > buf->gapStart)
    	moveGap(buf, start);
    else if (end < buf->gapStart)
    	moveGap(buf, end);

    /* expand the gap to encompass the deleted characters */
    buf->gapEnd += end - buf->gapStart;
    buf->gapStart -= buf->gapStart - start;
    
    /* update the length */
    buf->length -= end - start;
    
    /* fix up any selections which might be affected by the change */
    updateSelections(buf, start, end-start, 0);
}

/*
** Insert a column of text without calling the modify callbacks.  Note that
** in some pathological cases, inserting can actually decrease the size of
** the buffer because of spaces being coalesced into tabs.  "nDeleted" and
** "nInserted" return the number of characters deleted and inserted beginning
** at the start of the line containing "startPos".  "endPos" returns buffer
** position of the lower left edge of the inserted column (as a hint for
** routines which need to set a cursor position).
*/
static void insertCol(textBuffer *buf, int column, int startPos, char *insText,
	int *nDeleted, int *nInserted, int *endPos, int *insertWidth)
{
    int nLines, start, end, insWidth, lineStart, lineEnd;
    int expReplLen, expInsLen, len, endOffset;
    char *c, *outStr, *outPtr, *line, *replText, *expText, *insLine, *insPtr;

    if (column < 0)
    	column = 0;
    	
    /* Allocate a buffer for the replacement string large enough to hold 
       possibly expanded tabs in both the inserted text and the replaced
       area, as well as per line: 1) an additional 2*MAX_EXP_CHAR_LEN
       characters for padding where tabs and control characters cross the
       column of the selection, 2) up to "column" additional spaces per
       line for padding out to the position of "column", 3) padding up
       to the width of the inserted text if that must be padded to align
       the text beyond the inserted column.  (Space for additional
       newlines if the inserted text extends beyond the end of the buffer
       is counted with the length of insText) */
    start = BufStartOfLine(buf, startPos);
    nLines = countLines(insText) + 1;
    insWidth = textWidth(insText, buf->tabDist, buf->nullSubsChar);
    end = BufEndOfLine(buf, BufCountForwardNLines(buf, start, nLines-1));
    replText = BufGetRange(buf, start, end);
    expText = expandTabs(replText, 0, buf->tabDist, buf->nullSubsChar,
	    &expReplLen);
    XtFree(replText);
    XtFree(expText);
    expText = expandTabs(insText, 0, buf->tabDist, buf->nullSubsChar,
	    &expInsLen);
    XtFree(expText);
    outStr = XtMalloc(expReplLen + expInsLen +
    	    nLines * (column + insWidth + MAX_EXP_CHAR_LEN) + 1);
    
    /* Loop over all lines in the buffer between start and end removing the
       text between rectStart and rectEnd and padding appropriately.  Trim
       trailing space from line (whitespace at the ends of lines otherwise
       tends to multiply, since additional padding is added to maintain it */
    outPtr = outStr;
    lineStart = start;
    insPtr = insText;
    while (True) {
    	lineEnd = BufEndOfLine(buf, lineStart);
    	line = BufGetRange(buf, lineStart, lineEnd);
    	insLine = copyLine(insPtr, &len);
    	insPtr += len;
    	insertColInLine(line, insLine, column, insWidth, buf->tabDist,
    		buf->useTabs, buf->nullSubsChar, outPtr, &len, &endOffset);
    	XtFree(line);
    	XtFree(insLine);
    	for (c=outPtr+len-1; c>outPtr && isspace(*c); c--)
    	    len--;
	outPtr += len;
	*outPtr++ = '\n';
    	lineStart = lineEnd < buf->length ? lineEnd + 1 : buf->length;
    	if (*insPtr == '\0')
    	    break;
    	insPtr++;
    }
    if (outPtr != outStr)
    	outPtr--; /* trim back off extra newline */
    *outPtr = '\0';
    
    /* replace the text between start and end with the new stuff */
    deleteRange(buf, start, end);
    insert(buf, start, outStr);
    *nInserted = outPtr - outStr;
    *nDeleted = end - start;
    *endPos = start + (outPtr - outStr) - len + endOffset;
    *insertWidth = insWidth;
    XtFree(outStr);
}

/*
** Delete a rectangle of text without calling the modify callbacks.  Returns
** the number of characters replacing those between start and end.  Note that
** in some pathological cases, deleting can actually increase the size of
** the buffer because of tab expansions.  "endPos" returns the buffer position
** of the point in the last line where the text was removed (as a hint for
** routines which need to position the cursor after a delete operation)
*/
static void deleteRect(textBuffer *buf, int start, int end, int rectStart,
	int rectEnd, int *replaceLen, int *endPos)
{
    int nLines, lineStart, lineEnd, len, endOffset;
    char *outStr, *outPtr, *line, *text, *expText;
    
    /* allocate a buffer for the replacement string large enough to hold 
       possibly expanded tabs as well as an additional  MAX_EXP_CHAR_LEN * 2
       characters per line for padding where tabs and control characters cross
       the edges of the selection */
    start = BufStartOfLine(buf, start);
    end = BufEndOfLine(buf, end);
    nLines = BufCountLines(buf, start, end) + 1;
    text = BufGetRange(buf, start, end);
    expText = expandTabs(text, 0, buf->tabDist, buf->nullSubsChar, &len);
    XtFree(text);
    XtFree(expText);
    outStr = XtMalloc(len + nLines * MAX_EXP_CHAR_LEN * 2 + 1);
    
    /* loop over all lines in the buffer between start and end removing
       the text between rectStart and rectEnd and padding appropriately */
    lineStart = start;
    outPtr = outStr;
    while (lineStart <= buf->length && lineStart <= end) {
    	lineEnd = BufEndOfLine(buf, lineStart);
    	line = BufGetRange(buf, lineStart, lineEnd);
    	deleteRectFromLine(line, rectStart, rectEnd, buf->tabDist,
    		buf->useTabs, buf->nullSubsChar, outPtr, &len, &endOffset);
    	XtFree(line);
	outPtr += len;
	*outPtr++ = '\n';
    	lineStart = lineEnd + 1;
    }
    if (outPtr != outStr)
    	outPtr--; /* trim back off extra newline */
    *outPtr = '\0';
    
    /* replace the text between start and end with the newly created string */
    deleteRange(buf, start, end);
    insert(buf, start, outStr);
    *replaceLen = outPtr - outStr;
    *endPos = start + (outPtr - outStr) - len + endOffset;
    XtFree(outStr);
}

/*
** Overlay a rectangular area of text without calling the modify callbacks.
** "nDeleted" and "nInserted" return the number of characters deleted and
** inserted beginning at the start of the line containing "startPos".
** "endPos" returns buffer position of the lower left edge of the inserted
** column (as a hint for routines which need to set a cursor position).
*/
static void overlayRect(textBuffer *buf, int startPos, int rectStart,
    	int rectEnd, char *insText, int *nDeleted, int *nInserted, int *endPos)
{
    int nLines, start, end, lineStart, lineEnd;
    int expInsLen, len, endOffset;
    char *c, *outStr, *outPtr, *line, *expText, *insLine, *insPtr;

    /* Allocate a buffer for the replacement string large enough to hold
       possibly expanded tabs in the inserted text, as well as per line: 1)
       an additional 2*MAX_EXP_CHAR_LEN characters for padding where tabs
       and control characters cross the column of the selection, 2) up to
       "column" additional spaces per line for padding out to the position
       of "column", 3) padding up to the width of the inserted text if that
       must be padded to align the text beyond the inserted column.  (Space
       for additional newlines if the inserted text extends beyond the end
       of the buffer is counted with the length of insText) */
    start = BufStartOfLine(buf, startPos);
    nLines = countLines(insText) + 1;
    end = BufEndOfLine(buf, BufCountForwardNLines(buf, start, nLines-1));
    expText = expandTabs(insText, 0, buf->tabDist, buf->nullSubsChar,
	    &expInsLen);
    XtFree(expText);
    outStr = XtMalloc(end-start + expInsLen +
    	    nLines * (rectEnd + MAX_EXP_CHAR_LEN) + 1);
    
    /* Loop over all lines in the buffer between start and end overlaying the
       text between rectStart and rectEnd and padding appropriately.  Trim
       trailing space from line (whitespace at the ends of lines otherwise
       tends to multiply, since additional padding is added to maintain it */
    outPtr = outStr;
    lineStart = start;
    insPtr = insText;
    while (True) {
    	lineEnd = BufEndOfLine(buf, lineStart);
    	line = BufGetRange(buf, lineStart, lineEnd);
    	insLine = copyLine(insPtr, &len);
    	insPtr += len;
    	overlayRectInLine(line, insLine, rectStart, rectEnd, buf->tabDist,
		buf->useTabs, buf->nullSubsChar, outPtr, &len, &endOffset);
    	XtFree(line);
    	XtFree(insLine);
    	for (c=outPtr+len-1; c>outPtr && isspace(*c); c--)
    	    len--;
	outPtr += len;
	*outPtr++ = '\n';
    	lineStart = lineEnd < buf->length ? lineEnd + 1 : buf->length;
    	if (*insPtr == '\0')
    	    break;
    	insPtr++;
    }
    if (outPtr != outStr)
    	outPtr--; /* trim back off extra newline */
    *outPtr = '\0';
    
    /* replace the text between start and end with the new stuff */
    deleteRange(buf, start, end);
    insert(buf, start, outStr);
    *nInserted = outPtr - outStr;
    *nDeleted = end - start;
    *endPos = start + (outPtr - outStr) - len + endOffset;
    XtFree(outStr);
}

/*
** Insert characters from single-line string "insLine" in single-line string
** "line" at "column", leaving "insWidth" space before continuing line.
** "outLen" returns the number of characters written to "outStr", "endOffset"
** returns the number of characters from the beginning of the string to
** the right edge of the inserted text (as a hint for routines which need
** to position the cursor).
*/
static void insertColInLine(char *line, char *insLine, int column, int insWidth,
	int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
	int *endOffset)
{
    char *c, *linePtr, *outPtr, *retabbedStr;
    int indent, toIndent, len, postColIndent;
        
    /* copy the line up to "column" */ 
    outPtr = outStr;
    indent = 0;
    for (linePtr=line; *linePtr!='\0'; linePtr++) {
	len = BufCharWidth(*linePtr, indent, tabDist, nullSubsChar);
	if (indent + len > column)
    	    break;
    	indent += len;
	*outPtr++ = *linePtr;
    }
    
    /* If "column" falls in the middle of a character, and the character is a
       tab, leave it off and leave the indent short and it will get padded
       later.  If it's a control character, insert it and adjust indent
       accordingly. */
    if (indent < column && *linePtr != '\0') {
    	postColIndent = indent + len;
    	if (*linePtr == '\t')
    	    linePtr++;
    	else {
    	    *outPtr++ = *linePtr++;
    	    indent += len;
    	}
    } else
    	postColIndent = indent;
    
    /* If there's no text after the column and no text to insert, that's all */
    if (*insLine == '\0' && *linePtr == '\0') {
    	*outLen = *endOffset = outPtr - outStr;
    	return;
    }
    
    /* pad out to column if text is too short */
    if (indent < column) {
	addPadding(outPtr, indent, column, tabDist, useTabs, nullSubsChar,&len);
	outPtr += len;
	indent = column;
    }
    
    /* Copy the text from "insLine" (if any), recalculating the tabs as if
       the inserted string began at column 0 to its new column destination */
    if (*insLine != '\0') {
	retabbedStr = realignTabs(insLine, 0, indent, tabDist, useTabs,
		nullSubsChar, &len);
	for (c=retabbedStr; *c!='\0'; c++) {
    	    *outPtr++ = *c;
    	    len = BufCharWidth(*c, indent, tabDist, nullSubsChar);
    	    indent += len;
	}
	XtFree(retabbedStr);
    }
    
    /* If the original line did not extend past "column", that's all */
    if (*linePtr == '\0') {
    	*outLen = *endOffset = outPtr - outStr;
    	return;
    }
    
    /* Pad out to column + width of inserted text + (additional original
       offset due to non-breaking character at column) */
    toIndent = column + insWidth + postColIndent-column;
    addPadding(outPtr, indent, toIndent, tabDist, useTabs, nullSubsChar, &len);
    outPtr += len;
    indent = toIndent;
    
    /* realign tabs for text beyond "column" and write it out */
    retabbedStr = realignTabs(linePtr, postColIndent, indent, tabDist,
    	useTabs, nullSubsChar, &len);
    strcpy(outPtr, retabbedStr);
    XtFree(retabbedStr);
    *endOffset = outPtr - outStr;
    *outLen = (outPtr - outStr) + len;
}

/*
** Remove characters in single-line string "line" between displayed positions
** "rectStart" and "rectEnd", and write the result to "outStr", which is
** assumed to be large enough to hold the returned string.  Note that in
** certain cases, it is possible for the string to get longer due to
** expansion of tabs.  "endOffset" returns the number of characters from
** the beginning of the string to the point where the characters were
** deleted (as a hint for routines which need to position the cursor).
*/
static void deleteRectFromLine(char *line, int rectStart, int rectEnd,
	int tabDist, int useTabs, char nullSubsChar, char *outStr, int *outLen,
	int *endOffset)
{
    int indent, preRectIndent, postRectIndent, len;
    char *c, *outPtr;
    char *retabbedStr;
    
    /* copy the line up to rectStart */
    outPtr = outStr;
    indent = 0;
    for (c=line; *c!='\0'; c++) {
	if (indent > rectStart)
	    break;
	len = BufCharWidth(*c, indent, tabDist, nullSubsChar);
	if (indent + len > rectStart && (indent == rectStart || *c == '\t'))
    	    break;
    	indent += len;
	*outPtr++ = *c;
    }
    preRectIndent = indent;
    
    /* skip the characters between rectStart and rectEnd */
    for(; *c!='\0' && indent<rectEnd; c++)
	indent += BufCharWidth(*c, indent, tabDist, nullSubsChar);
    postRectIndent = indent;
    
    /* If the line ended before rectEnd, there's nothing more to do */
    if (*c == '\0') {
    	*outPtr = '\0';
    	*outLen = *endOffset = outPtr - outStr;
    	return;
    }
    
    /* fill in any space left by removed tabs or control characters
       which straddled the boundaries */
    indent = max(rectStart + postRectIndent-rectEnd, preRectIndent);
    addPadding(outPtr, preRectIndent, indent, tabDist, useTabs, nullSubsChar,
	    &len);
    outPtr += len;

    /* Copy the rest of the line.  If the indentation has changed, preserve
       the position of non-whitespace characters by converting tabs to
       spaces, then back to tabs with the correct offset */
    retabbedStr = realignTabs(c, postRectIndent, indent, tabDist, useTabs,
    	    nullSubsChar, &len);
    strcpy(outPtr, retabbedStr);
    XtFree(retabbedStr);
    *endOffset = outPtr - outStr;
    *outLen = (outPtr - outStr) + len;
}

/*
** Overlay characters from single-line string "insLine" on single-line string
** "line" between displayed character offsets "rectStart" and "rectEnd".
** "outLen" returns the number of characters written to "outStr", "endOffset"
** returns the number of characters from the beginning of the string to
** the right edge of the inserted text (as a hint for routines which need
** to position the cursor).
*/
static void overlayRectInLine(char *line, char *insLine, int rectStart,
    	int rectEnd, int tabDist, int useTabs, char nullSubsChar, char *outStr,
    	int *outLen, int *endOffset)
{
    char *c, *linePtr, *outPtr, *retabbedStr;
    int inIndent, outIndent, len, postRectIndent;
        
    /* copy the line up to "rectStart" */ 
    outPtr = outStr;
    inIndent = outIndent = 0;
    for (linePtr=line; *linePtr!='\0'; linePtr++) {
	len = BufCharWidth(*linePtr, inIndent, tabDist, nullSubsChar);
	if (inIndent + len > rectStart)
    	    break;
    	inIndent += len;
    	outIndent += len;
	*outPtr++ = *linePtr;
    }
    
    /* If "rectStart" falls in the middle of a character, and the character
       is a tab, leave it off and leave the outIndent short and it will get
       padded later.  If it's a control character, insert it and adjust
       outIndent accordingly. */
    if (inIndent < rectStart && *linePtr != '\0') {
    	if (*linePtr == '\t') {
    	    linePtr++;
    	    inIndent += len;
    	} else {
    	    *outPtr++ = *linePtr++;
    	    outIndent += len;
    	    inIndent += len;
    	}
    }
    
    /* skip the characters between rectStart and rectEnd */
    postRectIndent = rectEnd;
    for(; *linePtr!='\0'; linePtr++) {
	inIndent += BufCharWidth(*linePtr, inIndent, tabDist, nullSubsChar);
	if (inIndent >= rectEnd) {
	    linePtr++;
	    postRectIndent = inIndent;
	    break;
	}
    }
    
    /* If there's no text after rectStart and no text to insert, that's all */
    if (*insLine == '\0' && *linePtr == '\0') {
    	*outLen = *endOffset = outPtr - outStr;
    	return;
    }

    /* pad out to rectStart if text is too short */
    if (outIndent < rectStart) {
	addPadding(outPtr, outIndent, rectStart, tabDist, useTabs, nullSubsChar,
		&len);
	outPtr += len;
    }
    outIndent = rectStart;
    
    /* Copy the text from "insLine" (if any), recalculating the tabs as if
       the inserted string began at column 0 to its new column destination */
    if (*insLine != '\0') {
	retabbedStr = realignTabs(insLine, 0, rectStart, tabDist, useTabs,
		nullSubsChar, &len);
	for (c=retabbedStr; *c!='\0'; c++) {
    	    *outPtr++ = *c;
    	    len = BufCharWidth(*c, outIndent, tabDist, nullSubsChar);
    	    outIndent += len;
	}
	XtFree(retabbedStr);
    }
    
    /* If the original line did not extend past "rectStart", that's all */
    if (*linePtr == '\0') {
    	*outLen = *endOffset = outPtr - outStr;
    	return;
    }
    
    /* Pad out to rectEnd + (additional original offset
       due to non-breaking character at right boundary) */
    addPadding(outPtr, outIndent, postRectIndent, tabDist, useTabs,
	    nullSubsChar, &len);
    outPtr += len;
    outIndent = postRectIndent;
    
    /* copy the text beyond "rectEnd" */
    strcpy(outPtr, linePtr);
    *endOffset = outPtr - outStr;
    *outLen = (outPtr - outStr) + strlen(linePtr);
}

static void setSelection(selection *sel, int start, int end, selType type)
{
    sel->selected = start != end;
    sel->rectangular = False;
    sel->start = min(start, end);
    sel->end = max(start, end);
    sel->type = type;
}

static void setRectSelect(selection *sel, int start, int end,
	int rectStart, int rectEnd)
{
    sel->selected = rectStart < rectEnd;
    sel->rectangular = True;
    sel->start = start;
    sel->end = end;
    sel->rectStart = rectStart;
    sel->rectEnd = rectEnd;
    sel->type = CHAR_SELECT;
}

static int getSelectionPos(selection *sel, int *start, int *end,
        int *isRect, int *rectStart, int *rectEnd)
{
    if (!sel->selected)
    	return False;
    *isRect = sel->rectangular;
    *start = sel->start;
    *end = sel->end;
    if (sel->rectangular) {
	*rectStart = sel->rectStart;
	*rectEnd = sel->rectEnd;
    }
    return True;
}

static char *getSelectionText(textBuffer *buf, selection *sel)
{
    int start, end, isRect, rectStart, rectEnd;
    char *text;
    
    /* If there's no selection, return an allocated empty string */
    if (!getSelectionPos(sel, &start, &end, &isRect, &rectStart, &rectEnd)) {
    	text = XtMalloc(1);
    	*text = '\0';
    	return text;
    }
    
    /* If the selection is not rectangular, return the selected range */
    if (isRect)
    	return BufGetTextInRect(buf, start, end, rectStart, rectEnd);
    else
    	return BufGetRange(buf, start, end);
}

static void removeSelected(textBuffer *buf, selection *sel)
{
    int start, end;
    int isRect, rectStart, rectEnd;
    
    if (!getSelectionPos(sel, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    if (isRect)
        BufRemoveRect(buf, start, end, rectStart, rectEnd);
    else
        BufRemove(buf, start, end);
}

static void replaceSelected(textBuffer *buf, selection *sel, char *text, Boolean staySelected)
{
    int start, end, isRect, rectStart, rectEnd;
    selection oldSelection;
    
    /* If there's no selection, return */
    if (!getSelectionPos(sel, &start, &end, &isRect, &rectStart, &rectEnd))
    	return;
    
    /* Do the appropriate type of replace */
    if (isRect) {
    	BufRectangle rect;
    	BufReplaceRect(buf, start, end, rectStart, rectEnd, text, &rect);
    	oldSelection = *sel;
    	if(staySelected) {
    		setRectSelect(sel, rect.start, rect.end, rect.rectStart, rect.rectEnd);
    	}
    }
    else {
    	int nInserted;
    	nInserted = BufReplace(buf, start, end, text);
    	oldSelection = *sel;
    	if(staySelected) {
    		setSelection(sel, start, start+nInserted, sel->type);
    	}
    }
    
    if(!staySelected) {
    	sel->selected = False;
    	sel->type = CHAR_SELECT;
    }
    
    redisplaySelection(buf, &oldSelection, sel);
}

static void addPadding(char *string, int startIndent, int toIndent,
	int tabDist, int useTabs, char nullSubsChar, int *charsAdded)
{
    char *outPtr;
    int len, indent;
    
    indent = startIndent;
    outPtr = string;
    if (useTabs) {
	while (indent < toIndent) {
	    len = BufCharWidth('\t', indent, tabDist, nullSubsChar);
	    if (len > 1 && indent + len <= toIndent) {
		*outPtr++ = '\t';
		indent += len;
	    } else {
		*outPtr++ = ' ';
		indent++;
	    }
	}
    } else {
    	while (indent < toIndent) {
	    *outPtr++ = ' ';
	    indent++;
	}
    }
    *charsAdded = outPtr - string;
}

/*
** Call the stored modify callback procedure(s) for this buffer to update the
** changed area(s) on the screen and any other listeners.
*/
static void callModifyCBs(textBuffer *buf, int pos, int nDeleted,
	int nInserted, int nRestyled, char *deletedText)
{
    int i;
    
    for (i=0; i<buf->nModifyProcs; i++)
    	(*buf->modifyProcs[i])(pos, nInserted, nDeleted, nRestyled,
    		deletedText, buf->cbArgs[i]);
}

/*
** Call the stored redisplay procedure(s) for this buffer to update the
** screen for a change in a selection.
*/
static void redisplaySelection(textBuffer *buf, selection *oldSelection,
	selection *newSelection)
{
    int oldStart, oldEnd, newStart, newEnd, ch1Start, ch1End, ch2Start, ch2End;
    
    /* If either selection is rectangular, add an additional character to
       the end of the selection to request the redraw routines to wipe out
       the parts of the selection beyond the end of the line */
    oldStart = oldSelection->start;
    newStart = newSelection->start;
    oldEnd = oldSelection->end;
    newEnd = newSelection->end;
    if (oldSelection->rectangular)
    	oldEnd++;
    if (newSelection->rectangular)
    	newEnd++;

    /* If the old or new selection is unselected, just redisplay the
       single area that is (was) selected and return */
    if (!oldSelection->selected && !newSelection->selected)
    	return;
    if (!oldSelection->selected) {
    	callModifyCBs(buf, newStart, 0, 0, newEnd-newStart, NULL);
    	return;
    }
    if (!newSelection->selected) {
    	callModifyCBs(buf, oldStart, 0, 0, oldEnd-oldStart, NULL);
    	return;
    }

    /* If the selection changed from normal to rectangular or visa versa, or
       if a rectangular selection changed boundaries, redisplay everything */
    if ((oldSelection->rectangular && !newSelection->rectangular) ||
    	    (!oldSelection->rectangular && newSelection->rectangular) ||
    	    (oldSelection->rectangular && (
    	    	(oldSelection->rectStart != newSelection->rectStart) ||
    	    	(oldSelection->rectEnd != newSelection->rectEnd)))) {
    	callModifyCBs(buf, min(oldStart, newStart), 0, 0,
    		max(oldEnd, newEnd) - min(oldStart, newStart), NULL);
    	return;
    }
    
    /* If the selections are non-contiguous, do two separate updates
       and return */
    if (oldEnd < newStart || newEnd < oldStart) {
	callModifyCBs(buf, oldStart, 0, 0, oldEnd-oldStart, NULL);
	callModifyCBs(buf, newStart, 0, 0, newEnd-newStart, NULL);
	return;
    }
    
    /* Otherwise, separate into 3 separate regions: ch1, and ch2 (the two
       changed areas), and the unchanged area of their intersection,
       and update only the changed area(s) */
    ch1Start = min(oldStart, newStart);
    ch2End = max(oldEnd, newEnd);
    ch1End = max(oldStart, newStart);
    ch2Start = min(oldEnd, newEnd);
    if (ch1Start != ch1End)
    	callModifyCBs(buf, ch1Start, 0, 0, ch1End-ch1Start, NULL);
    if (ch2Start != ch2End)
    	callModifyCBs(buf, ch2Start, 0, 0, ch2End-ch2Start, NULL);
}

static void moveGap(textBuffer *buf, int pos)
{
    int gapLen = buf->gapEnd - buf->gapStart;
    
    if (pos > buf->gapStart)
    	memmove(&buf->buf[buf->gapStart], &buf->buf[buf->gapEnd],
			pos - buf->gapStart);
    else
    	memmove(&buf->buf[pos + gapLen], &buf->buf[pos], buf->gapStart - pos);
    buf->gapEnd += pos - buf->gapStart;
    buf->gapStart += pos - buf->gapStart;
}

/*
** reallocate the text storage in "buf" to have a gap starting at "newGapStart"
** and a gap size of "newGapLen", preserving the buffer's current contents.
*/
static void reallocateBuf(textBuffer *buf, int newGapStart, int newGapLen)
{
    char *newBuf;
    int newGapEnd;

    newBuf = XtMalloc(buf->length + newGapLen);
    newGapEnd = newGapStart + newGapLen;
    if (newGapStart <= buf->gapStart) {
	memcpy(newBuf, buf->buf, newGapStart);
	memcpy(&newBuf[newGapEnd], &buf->buf[newGapStart],
		buf->gapStart - newGapStart);
	memcpy(&newBuf[newGapEnd + buf->gapStart - newGapStart],
		&buf->buf[buf->gapEnd], buf->length - buf->gapStart);
    } else { /* newGapStart > buf->gapStart */
	memcpy(newBuf, buf->buf, buf->gapStart);
	memcpy(&newBuf[buf->gapStart], &buf->buf[buf->gapEnd],
		newGapStart - buf->gapStart);
	memcpy(&newBuf[newGapEnd],
		&buf->buf[buf->gapEnd + newGapStart - buf->gapStart],
		buf->length - newGapStart);
    }
    XtFree(buf->buf);
    buf->buf = newBuf;
    buf->gapStart = newGapStart;
    buf->gapEnd = newGapEnd;
#ifdef PURIFY
    {int i; for (i=buf->gapStart; i<buf->gapEnd; i++) buf->buf[i] = '.';}
#endif
}

/*
** Update all of the selections in "buf" for changes in the buffer's text
*/
static void updateSelections(textBuffer *buf, int pos, int nDeleted,
	int nInserted)
{
    updateSelection(&buf->primary, pos, nDeleted, nInserted);
    updateSelection(&buf->secondary, pos, nDeleted, nInserted);
    updateSelection(&buf->highlight, pos, nDeleted, nInserted);
}

/*
** Update an individual selection for changes in the corresponding text
*/
static void updateSelection(selection *sel, int pos, int nDeleted,
	int nInserted)
{
    if (!sel->selected || pos > sel->end)
    	return;
    if (pos+nDeleted <= sel->start) {
    	sel->start += nInserted - nDeleted;
	sel->end += nInserted - nDeleted;
    } else if (pos <= sel->start && pos+nDeleted >= sel->end) {
    	sel->start = pos;
    	sel->end = pos;
    	sel->selected = False;
		sel->type = CHAR_SELECT;
    } else if (pos <= sel->start && pos+nDeleted < sel->end) {
    	sel->start = pos;
    	sel->end = nInserted + sel->end - nDeleted;
    } else if (pos < sel->end) {
    	sel->end += nInserted - nDeleted;
	if (sel->end <= sel->start)
	    sel->selected = False;
	    sel->type = CHAR_SELECT;
    }
}

/*
** Search forwards in buffer "buf" for character "searchChar", starting
** with the character "startPos", and returning the result in "foundPos"
** returns True if found, False if not.  (The difference between this and
** BufSearchForward is that it's optimized for single characters.  The
** overall performance of the text widget is dependent on its ability to
** count lines quickly, hence searching for a single character: newline)
*/
static int searchForward(textBuffer *buf, int startPos, char searchChar,
	int *foundPos)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    
    pos = startPos;
    while (pos < buf->gapStart) {
        if (buf->buf[pos] == searchChar) {
            *foundPos = pos;
            return True;
        }
        pos++;
    }
    while (pos < buf->length) {
    	if (buf->buf[pos + gapLen] == searchChar) {
            *foundPos = pos;
            return True;
        }
        pos++;
    }
    *foundPos = buf->length;
    return False;
}

/*
** Search backwards in buffer "buf" for character "searchChar", starting
** with the character BEFORE "startPos", returning the result in "foundPos"
** returns True if found, False if not.  (The difference between this and
** BufSearchBackward is that it's optimized for single characters.  The
** overall performance of the text widget is dependent on its ability to
** count lines quickly, hence searching for a single character: newline)
*/
static int searchBackward(textBuffer *buf, int startPos, char searchChar,
	int *foundPos)
{
    int pos, gapLen = buf->gapEnd - buf->gapStart;
    
    if (startPos == 0) {
    	*foundPos = 0;
    	return False;
    }
    pos = startPos == 0 ? 0 : startPos - 1;
    while (pos >= buf->gapStart) {
    	if (buf->buf[pos + gapLen] == searchChar) {
            *foundPos = pos;
            return True;
        }
        pos--;
    }
    while (pos >= 0) {
        if (buf->buf[pos] == searchChar) {
            *foundPos = pos;
            return True;
        }
        pos--;
    }
    *foundPos = 0;
    return False;
}

/*
** Copy from "text" to end up to but not including newline (or end of "text")
** and return the copy as the function value, and the length of the line in
** "lineLen"
*/
static char *copyLine(char *text, int *lineLen)
{
    int len = 0;
    char *c, *outStr;
    
    for (c=text; *c!='\0' && *c!='\n'; c++)
    	len++;
    outStr = XtMalloc(len + 1);
    strncpy(outStr, text, len);
    outStr[len] = '\0';
    *lineLen = len;
    return outStr;
}

/*
** Count the number of newlines in a null-terminated text string;
*/
static int countLines(char *string)
{
    char *c;
    int lineCount = 0;
    
    for (c=string; *c!='\0'; c++)
    	if (*c == '\n') lineCount++;
    return lineCount;
}

/*
** Measure the width in displayed characters of string "text"
*/
static int textWidth(char *text, int tabDist, char nullSubsChar)
{
    int width = 0, maxWidth = 0;
    char *c;
    
    for (c=text; *c!='\0'; c++) {
    	if (*c == '\n') {
    	    if (width > maxWidth)
    	    	maxWidth = width;
    	    width = 0;
    	} else
    	    width += BufCharWidth(*c, width, tabDist, nullSubsChar);
    }
    if (width > maxWidth)
    	return width;
    return maxWidth;
}

/*
** Find the first and last character position in a line withing a rectangular
** selection (for copying).  Includes tabs which cross rectStart, but not
** control characters which do so.  Leaves off tabs which cross rectEnd.
**
** Technically, the calling routine should convert tab characters which
** cross the right boundary of the selection to spaces which line up with
** the edge of the selection.  Unfortunately, the additional memory
** management required in the parent routine to allow for the changes
** in string size is not worth all the extra work just for a couple of
** shifted characters, so if a tab protrudes, just lop it off and hope
** that there are other characters in the selection to establish the right
** margin for subsequent columnar pastes of this data.
*/
static void findRectSelBoundariesForCopy(textBuffer *buf, int lineStartPos,
	int rectStart, int rectEnd, int *selStart, int *selEnd)
{
    int pos, width, indent = 0;
    char c;
    
    /* find the start of the selection */
    for (pos=lineStartPos; pos<buf->length; pos++) {
    	c = _BufGetCharacter(buf, pos);
    	if (c == '\n')
    	    break;
    	width = BufCharWidth(c, indent, buf->tabDist, buf->nullSubsChar);
    	if (indent + width > rectStart) {
    	    if (indent != rectStart && c != '\t') {
    	    	pos++;
    	    	indent += width;
    	    }
    	    break;
    	}
    	indent += width;
    }
    *selStart = pos;
    
    /* find the end */
    for (; pos<buf->length; pos++) {
    	c = _BufGetCharacter(buf, pos);
    	if (c == '\n')
    	    break;
    	width = BufCharWidth(c, indent, buf->tabDist, buf->nullSubsChar);
    	indent += width;
    	if (indent > rectEnd) {
    	    if (indent-width != rectEnd && c != '\t')
    	    	pos++;
    	    break;
    	}
    }
    *selEnd = pos;
}

/*
** Adjust the space and tab characters from string "text" so that non-white
** characters remain stationary when the text is shifted from starting at
** "origIndent" to starting at "newIndent".  Returns an allocated string
** which must be freed by the caller with XtFree.
*/
static char *realignTabs(char *text, int origIndent, int newIndent,
	int tabDist, int useTabs, char nullSubsChar, int *newLength)
{
    char *expStr, *outStr;
    int len;
    
    /* If the tabs settings are the same, retain original tabs */
    if (origIndent % tabDist == newIndent %tabDist) {
    	len = strlen(text);
    	outStr = XtMalloc(len + 1);
    	strcpy(outStr, text);
    	*newLength = len;
    	return outStr;
    }
    
    /* If the tab settings are not the same, brutally convert tabs to
       spaces, then back to tabs in the new position */
    expStr = expandTabs(text, origIndent, tabDist, nullSubsChar, &len);
    if (!useTabs) {
    	*newLength = len;
    	return expStr;
    }
    outStr = unexpandTabs(expStr, newIndent, tabDist, nullSubsChar, newLength);
    XtFree(expStr);
    return outStr;
}    

/*
** Expand tabs to spaces for a block of text.  The additional parameter
** "startIndent" if nonzero, indicates that the text is a rectangular selection
** beginning at column "startIndent"
*/
static char *expandTabs(char *text, int startIndent, int tabDist,
	char nullSubsChar, int *newLen)
{
    char *outStr, *outPtr, *c;
    int indent, len, outLen = 0;

    /* rehearse the expansion to figure out length for output string */
    indent = startIndent;
    for (c=text; *c!='\0'; c++) {
    	if (*c == '\t') {
    	    len = BufCharWidth(*c, indent, tabDist, nullSubsChar);
    	    outLen += len;
    	    indent += len;
    	} else if (*c == '\n') {
    	    indent = startIndent;
    	    outLen++;
    	} else {
    	    indent += BufCharWidth(*c, indent, tabDist, nullSubsChar);
    	    outLen++;
    	}
    }
    
    /* do the expansion */
    outStr = XtMalloc(outLen+1);
    outPtr = outStr;
    indent = startIndent;
    for (c=text; *c!= '\0'; c++) {
    	if (*c == '\t') {
    	    len = BufExpandCharacter(*c, indent, outPtr, tabDist, nullSubsChar);
    	    outPtr += len;
    	    indent += len;
    	} else if (*c == '\n') {
    	    indent = startIndent;
    	    *outPtr++ = *c;
    	} else {
    	    indent += BufCharWidth(*c, indent, tabDist, nullSubsChar);
    	    *outPtr++ = *c;
    	}
    }
    outStr[outLen] = '\0';
    *newLen = outLen;
    return outStr;
}

/*
** Convert sequences of spaces into tabs.  The threshold for conversion is
** when 3 or more spaces can be converted into a single tab, this avoids
** converting double spaces after a period withing a block of text.
*/
static char *unexpandTabs(char *text, int startIndent, int tabDist,
	char nullSubsChar, int *newLen)
{
    char *outStr, *outPtr, *c, expandedChar[MAX_EXP_CHAR_LEN];
    int indent, len;
    
    outStr = XtMalloc(strlen(text)+1);
    outPtr = outStr;
    indent = startIndent;
    for (c=text; *c!='\0';) {
    	if (*c == ' ') {
    	    len = BufExpandCharacter('\t', indent, expandedChar, tabDist,
		    nullSubsChar);
    	    if (len >= 3 && !strncmp(c, expandedChar, len)) {
    	    	c += len;
    	    	*outPtr++ = '\t';
    	    	indent += len;
    	    } else {
    	    	*outPtr++ = *c++;
    	    	indent++;
    	    }
    	} else if (*c == '\n') {
    	    indent = startIndent;
    	    *outPtr++ = *c++;
    	} else {
    	    *outPtr++ = *c++;
    	    indent++;
    	}
    }
    *outPtr = '\0';
    *newLen = outPtr - outStr;
    return outStr;
}

#ifndef _WIN32
static int max(int i1, int i2)
{
    return i1 >= i2 ? i1 : i2;
}

static int min(int i1, int i2)
{
    return i1 <= i2 ? i1 : i2;
}
#endif
