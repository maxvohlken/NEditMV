/*
** Test the BufGetLines() function.
*/
#include "textBuf.h"

char buf40[] = "\
This is line 1 line -40\n\
This is line 2 line -39\n\
This is line 3 line -38\n\
This is line 4 line -37\n\
This is line 5 line -36\n\
This is line 6 line -35\n\
This is line 7 line -34\n\
This is line 8 line -33\n\
This is line 9 line -32\n\
This is line 10 line -31\n\
This is line 11 line -30\n\
This is line 12 line -29\n\
This is line 13 line -28\n\
This is line 14 line -27\n\
This is line 15 line -26\n\
This is line 16 line -25\n\
This is line 17 line -24\n\
This is line 18 line -23\n\
This is line 19 line -22\n\
This is line 20 line -21\n\
This is line 21 line -20\n\
This is line 22 line -19\n\
This is line 23 line -18\n\
This is line 24 line -17\n\
This is line 25 line -16\n\
This is line 26 line -15\n\
This is line 27 line -14\n\
This is line 28 line -13\n\
This is line 29 line -12\n\
This is line 30 line -11\n\
This is line 31 line -10\n\
This is line 32 line -9\n\
This is line 33 line -8\n\
This is line 34 line -7\n\
This is line 35 line -6\n\
This is line 36 line -5\n\
This is line 37 line -4\n\
This is line 38 line -3\n\
This is line 39 line -2\n\
This is line 40 line -1\n\
";

typedef struct _TestInfo {
	int startLine;
	int endLine;
	char *buffer;
} TestInfo;

TestInfo testInfo[] = {
	{1, 1, buf40},
	{1, 0, buf40},
	{1, 20, buf40},
	{10, 30, buf40},
	{20, 20, buf40},
	{20, 10, buf40},
	{-1, 0, buf40},
	{-1, -1, buf40},
	{-5, 0, buf40},
	{-25, -20, buf40},
	{20, -30, buf40},
	{0, 0, buf40},
	{0, 1, buf40},
	{0, -1, buf40},
	{0, -2, buf40},
	{0, 0, 0},
};

void testCase1(void)
{
    textBuffer *buf;
    TestInfo *testInfoP;
    
	buf = BufCreate();
	for(testInfoP = testInfo; testInfoP->buffer; testInfoP++) {
		char *lines;
    	BufSetAll(buf, testInfoP->buffer);
    	lines = BufGetLines(buf, testInfoP->startLine, testInfoP->endLine);
		printf("%d,%d\n:%s:\n", testInfoP->startLine, testInfoP->endLine, lines);
		XtFree(lines);
    }
	BufFree(buf);
}

void main(void)
{
	testCase1();
}

