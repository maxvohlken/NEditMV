
/*  A Bison parser, made from parse.y with Bison version GNU Bison version 1.21
  */

#define YYBISON 1  /* Identify Bison output.  */

#define	NUMBER	258
#define	STRING	259
#define	SYMBOL	260
#define	IF	261
#define	WHILE	262
#define	ELSE	263
#define	FOR	264
#define	BREAK	265
#define	CONTINUE	266
#define	RETURN	267
#define	ADDEQ	268
#define	SUBEQ	269
#define	MULEQ	270
#define	DIVEQ	271
#define	MODEQ	272
#define	ANDEQ	273
#define	OREQ	274
#define	CONCAT	275
#define	OR	276
#define	AND	277
#define	GT	278
#define	GE	279
#define	LT	280
#define	LE	281
#define	EQ	282
#define	NE	283
#define	UNARY_MINUS	284
#define	NOT	285
#define	INCR	286
#define	DECR	287
#define	POW	288

#line 1 "parse.y"

#include <config.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>
#ifdef VMS
#include "../util/VMSparam.h"
#else
#ifdef HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif
#endif /*VMS*/
#include "textBuf.h"
#include "nedit.h"
#include "interpret.h"
#include "parse.h"

/* Macros to add error processing to AddOp and AddSym calls */
#define ADD_OP(op) if (!AddOp(op, &ErrMsg)) return 1
#define ADD_SYM(sym) if (!AddSym(sym, &ErrMsg)) return 1
#define ADD_IMMED(val) if (!AddImmediate(val, &ErrMsg)) return 1
#define ADD_BR_OFF(to) if (!AddBranchOffset(to, &ErrMsg)) return 1
#define SET_BR_OFF(from, to) *((int *)from) = to - from

/* Max. length for a string constant (... there shouldn't be a maximum) */
#define MAX_STRING_CONST_LEN 5000

static int yylex(void);
static int follow(char expect, int yes, int no);
static int follow2(char expect1, int yes1, char expect2, int yes2, int no);
static Symbol *matchesActionRoutine(char **inPtr);

static char *ErrMsg;
static char *InPtr;


#line 37 "parse.y"
typedef union {
    Symbol *sym;
    Inst *inst;
    int nArgs;
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		151
#define	YYFLAG		-32768
#define	YYNTBASE	49

#define YYTRANSLATE(x) ((unsigned)(x) <= 288 ? yytranslate[x] : 66)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,    44,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,    36,    25,     2,    45,
    46,    34,    32,    48,    33,     2,    35,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,    47,     2,
    13,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,    42,    24,    43,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    14,    15,    16,
    17,    18,    19,    20,    21,    22,    23,    26,    27,    28,
    29,    30,    31,    37,    38,    39,    40,    41
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     3,     9,    11,    17,    19,    21,    24,    28,    35,
    45,    52,    63,    67,    71,    76,    80,    84,    88,    92,
    96,   100,   104,   108,   112,   117,   120,   123,   126,   129,
   131,   132,   134,   138,   139,   141,   145,   147,   150,   152,
   154,   156,   161,   165,   169,   173,   177,   181,   185,   189,
   192,   196,   200,   204,   208,   212,   216,   220,   224,   228,
   232,   235,   238,   241,   244,   247,   249,   251,   253,   254,
   256,   258,   260,   261
};

static const short yyrhs[] = {    65,
    51,     0,    65,    42,    65,    51,    43,     0,     1,     0,
    42,    65,    51,    43,    65,     0,    52,     0,    52,     0,
    51,    52,     0,    53,    44,    65,     0,     6,    45,    62,
    46,    65,    50,     0,     6,    45,    62,    46,    65,    50,
    61,    65,    50,     0,    59,    45,    62,    46,    65,    50,
     0,    60,    45,    55,    47,    62,    47,    55,    46,    65,
    50,     0,    10,    44,    65,     0,    11,    44,    65,     0,
    12,    57,    44,    65,     0,    12,    44,    65,     0,     5,
    13,    57,     0,    54,    14,    57,     0,    54,    15,    57,
     0,    54,    16,    57,     0,    54,    17,    57,     0,    54,
    18,    57,     0,    54,    19,    57,     0,    54,    20,    57,
     0,     5,    45,    56,    46,     0,    39,     5,     0,     5,
    39,     0,    40,     5,     0,     5,    40,     0,     5,     0,
     0,    53,     0,    55,    48,    53,     0,     0,    57,     0,
    56,    48,    57,     0,    58,     0,    57,    58,     0,     3,
     0,     4,     0,     5,     0,     5,    45,    56,    46,     0,
    45,    58,    46,     0,    58,    32,    58,     0,    58,    33,
    58,     0,    58,    34,    58,     0,    58,    35,    58,     0,
    58,    36,    58,     0,    58,    41,    58,     0,    33,    58,
     0,    58,    26,    58,     0,    58,    27,    58,     0,    58,
    28,    58,     0,    58,    29,    58,     0,    58,    30,    58,
     0,    58,    31,    58,     0,    58,    25,    58,     0,    58,
    24,    58,     0,    58,    63,    58,     0,    58,    64,    58,
     0,    38,    58,     0,    39,     5,     0,     5,    39,     0,
    40,     5,     0,     5,    40,     0,     7,     0,     9,     0,
     8,     0,     0,    58,     0,    23,     0,    22,     0,     0,
    65,    44,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    62,    63,    64,    66,    67,    69,    70,    72,    73,    74,
    76,    78,    82,    84,    86,    87,    89,    90,    91,    92,
    93,    94,    95,    97,    99,   101,   103,   105,   107,   110,
   112,   113,   114,   116,   117,   118,   120,   121,   123,   124,
   125,   126,   129,   130,   131,   132,   133,   134,   135,   136,
   137,   138,   139,   140,   141,   142,   143,   144,   145,   146,
   147,   148,   150,   152,   154,   157,   159,   161,   163,   164,
   166,   169,   172,   173
};

static const char * const yytname[] = {   "$","error","$illegal.","NUMBER","STRING",
"SYMBOL","IF","WHILE","ELSE","FOR","BREAK","CONTINUE","RETURN","'='","ADDEQ",
"SUBEQ","MULEQ","DIVEQ","MODEQ","ANDEQ","OREQ","CONCAT","OR","AND","'|'","'&'",
"GT","GE","LT","LE","EQ","NE","'+'","'-'","'*'","'/'","'%'","UNARY_MINUS","NOT",
"INCR","DECR","POW","'{'","'}'","'\\n'","'('","')'","';'","','","program","block",
"stmts","stmt","simpstmt","evalsym","comastmts","arglist","expr","numexpr","while",
"for","else","cond","and","or","blank",""
};
#endif

static const short yyr1[] = {     0,
    49,    49,    49,    50,    50,    51,    51,    52,    52,    52,
    52,    52,    52,    52,    52,    52,    53,    53,    53,    53,
    53,    53,    53,    53,    53,    53,    53,    53,    53,    54,
    55,    55,    55,    56,    56,    56,    57,    57,    58,    58,
    58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
    58,    58,    58,    58,    58,    58,    58,    58,    58,    58,
    58,    58,    58,    58,    58,    59,    60,    61,    62,    62,
    63,    64,    65,    65
};

static const short yyr2[] = {     0,
     2,     5,     1,     5,     1,     1,     2,     3,     6,     9,
     6,    10,     3,     3,     4,     3,     3,     3,     3,     3,
     3,     3,     3,     3,     4,     2,     2,     2,     2,     1,
     0,     1,     3,     0,     1,     3,     1,     2,     1,     1,
     1,     4,     3,     3,     3,     3,     3,     3,     3,     2,
     3,     3,     3,     3,     3,     3,     3,     3,     3,     3,
     2,     2,     2,     2,     2,     1,     1,     1,     0,     1,
     1,     1,     0,     2
};

static const short yydefact[] = {     0,
     3,     0,    30,     0,    66,    67,     0,     0,     0,     0,
     0,    73,    74,     1,     6,     0,     0,     0,     0,     0,
    27,    29,    34,    69,    73,    73,    39,    40,    41,     0,
     0,     0,     0,    73,     0,     0,    37,    26,    28,     0,
     7,    73,     0,     0,     0,     0,     0,     0,     0,    69,
    31,    17,     0,    35,    70,     0,    13,    14,    63,    65,
    34,    50,    61,    62,    64,    16,     0,    73,    38,    72,
    71,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     8,    18,
    19,    20,    21,    22,    23,    24,     0,    32,     0,    25,
     0,    73,     0,    43,    15,    58,    57,    51,    52,    53,
    54,    55,    56,    44,    45,    46,    47,    48,    49,    59,
    60,     2,    73,    69,     0,    36,     0,    42,     0,     0,
    33,    73,     9,     5,    11,    31,     0,    68,    73,     0,
     0,     0,    73,    73,    10,     0,     4,    12,     0,     0,
     0
};

static const short yydefgoto[] = {   149,
   133,    14,   134,    16,    17,    99,    53,    54,    37,    18,
    19,   139,    56,    86,    87,     2
};

static const short yypact[] = {   145,
-32768,   170,   -10,   -32,-32768,-32768,   -24,   -17,     4,    12,
    28,-32768,-32768,   249,-32768,    -8,   228,     0,     7,   133,
-32768,-32768,   133,   133,-32768,-32768,-32768,-32768,   -29,   133,
   133,    41,    50,-32768,   133,   120,   276,-32768,-32768,   193,
-32768,-32768,   133,   133,   133,   133,   133,   133,   133,   133,
     1,   133,   -27,   133,   276,    10,    16,    16,-32768,-32768,
   133,    20,    20,-32768,-32768,    16,   251,-32768,   276,-32768,
-32768,   133,   133,   133,   133,   133,   133,   133,   133,   133,
   133,   133,   133,   133,   133,   133,   133,   210,    16,   133,
   133,   133,   133,   133,   133,   133,    24,-32768,     3,-32768,
   133,-32768,   -22,-32768,    16,   293,   309,    47,    47,    47,
    47,    47,    47,   107,   107,    20,    20,    20,    20,   276,
   276,-32768,-32768,   133,     1,   133,   185,-32768,   185,    22,
-32768,-32768,    63,-32768,-32768,     1,   193,-32768,-32768,   -14,
   229,   185,-32768,-32768,-32768,   185,    16,-32768,    72,    74,
-32768
};

static const short yypgoto[] = {-32768,
  -124,   -15,    -2,   -49,-32768,   -52,    29,    19,    23,-32768,
-32768,-32768,   -46,-32768,-32768,   -11
};


#define	YYLAST		350


static const short yytable[] = {    15,
    40,    98,    20,    97,   135,     3,    27,    28,    29,    59,
    60,    41,    24,    57,    58,    61,    38,   145,   100,    25,
   101,   148,    66,   128,    88,   101,    26,    36,    21,    22,
    89,   143,    39,   125,    23,    42,    30,    15,    52,    10,
    11,    31,    32,    33,    50,    64,    55,    34,    35,   124,
   125,    51,    62,    63,    65,   102,   105,    67,    69,    13,
    85,    90,    91,    92,    93,    94,    95,    96,   136,   123,
   138,   150,    55,   151,    69,   131,    69,   130,    80,    81,
    82,    83,    84,   140,     0,    41,    98,    85,     0,   103,
   127,     0,     0,     0,   106,   107,   108,   109,   110,   111,
   112,   113,   114,   115,   116,   117,   118,   119,   120,   121,
     0,   129,    69,    69,    69,    69,    69,    69,    69,   126,
   137,   141,    27,    28,    29,     0,     0,   142,     0,     0,
     0,   146,   147,     0,    15,    27,    28,    29,    41,     0,
    82,    83,    84,     0,     0,     1,    55,    85,    69,   -73,
   -73,   -73,    30,   -73,   -73,   -73,   -73,    31,    32,    33,
     0,     0,     0,    68,    35,    30,     0,     0,     0,     0,
    31,    32,    33,     0,     3,     4,     5,    35,     6,     7,
     8,     9,     0,   -73,   -73,     0,   -73,     0,   -73,     3,
     4,     5,     0,     6,     7,     8,     9,     3,     4,     5,
     0,     6,     7,     8,     9,     0,     0,     0,    10,    11,
     0,    12,     0,    13,     3,     4,     5,     0,     6,     7,
     8,     9,     0,    10,    11,     0,   132,     0,    13,     0,
     0,    10,    11,     3,     4,     5,    13,     6,     7,     8,
     9,    43,    44,    45,    46,    47,    48,    49,    10,    11,
     0,     0,   122,     3,     4,     5,     0,     6,     7,     8,
     9,     0,     0,     0,     0,     0,     0,    10,    11,     0,
     0,   144,    70,    71,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    10,    11,     0,
     0,    85,     0,     0,     0,     0,   104,    70,    71,    72,
    73,    74,    75,    76,    77,    78,    79,    80,    81,    82,
    83,    84,     0,     0,     0,     0,    85,    73,    74,    75,
    76,    77,    78,    79,    80,    81,    82,    83,    84,     0,
     0,     0,     0,    85,    74,    75,    76,    77,    78,    79,
    80,    81,    82,    83,    84,     0,     0,     0,     0,    85
};

static const short yycheck[] = {     2,
    12,    51,    13,    50,   129,     5,     3,     4,     5,    39,
    40,    14,    45,    25,    26,    45,     5,   142,    46,    44,
    48,   146,    34,    46,    40,    48,    44,     9,    39,    40,
    42,    46,     5,    48,    45,    44,    33,    40,    20,    39,
    40,    38,    39,    40,    45,     5,    24,    44,    45,    47,
    48,    45,    30,    31,     5,    46,    68,    35,    36,    44,
    41,    43,    44,    45,    46,    47,    48,    49,    47,    46,
     8,     0,    50,     0,    52,   125,    54,   124,    32,    33,
    34,    35,    36,   136,    -1,    88,   136,    41,    -1,    61,
   102,    -1,    -1,    -1,    72,    73,    74,    75,    76,    77,
    78,    79,    80,    81,    82,    83,    84,    85,    86,    87,
    -1,   123,    90,    91,    92,    93,    94,    95,    96,   101,
   132,   137,     3,     4,     5,    -1,    -1,   139,    -1,    -1,
    -1,   143,   144,    -1,   137,     3,     4,     5,   141,    -1,
    34,    35,    36,    -1,    -1,     1,   124,    41,   126,     5,
     6,     7,    33,     9,    10,    11,    12,    38,    39,    40,
    -1,    -1,    -1,    44,    45,    33,    -1,    -1,    -1,    -1,
    38,    39,    40,    -1,     5,     6,     7,    45,     9,    10,
    11,    12,    -1,    39,    40,    -1,    42,    -1,    44,     5,
     6,     7,    -1,     9,    10,    11,    12,     5,     6,     7,
    -1,     9,    10,    11,    12,    -1,    -1,    -1,    39,    40,
    -1,    42,    -1,    44,     5,     6,     7,    -1,     9,    10,
    11,    12,    -1,    39,    40,    -1,    42,    -1,    44,    -1,
    -1,    39,    40,     5,     6,     7,    44,     9,    10,    11,
    12,    14,    15,    16,    17,    18,    19,    20,    39,    40,
    -1,    -1,    43,     5,     6,     7,    -1,     9,    10,    11,
    12,    -1,    -1,    -1,    -1,    -1,    -1,    39,    40,    -1,
    -1,    43,    22,    23,    24,    25,    26,    27,    28,    29,
    30,    31,    32,    33,    34,    35,    36,    39,    40,    -1,
    -1,    41,    -1,    -1,    -1,    -1,    46,    22,    23,    24,
    25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
    35,    36,    -1,    -1,    -1,    -1,    41,    25,    26,    27,
    28,    29,    30,    31,    32,    33,    34,    35,    36,    -1,
    -1,    -1,    -1,    41,    26,    27,    28,    29,    30,    31,
    32,    33,    34,    35,    36,    -1,    -1,    -1,    -1,    41
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is XtFree software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (_WIN32)
#include <stdlib.h>
# define alloca _alloca
#else
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca (unsigned int);
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not _WIN32 */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 184 "bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
#ifdef YYLSP_NEEDED
		 &yyls1, size * sizeof (*yylsp),
#endif
		 &yystacksize);

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 1:
#line 62 "parse.y"
{ ADD_OP(OP_RETURN_NO_VAL); return 0; ;
    break;}
case 2:
#line 63 "parse.y"
{ ADD_OP(OP_RETURN_NO_VAL); return 0; ;
    break;}
case 3:
#line 64 "parse.y"
{ return 1; ;
    break;}
case 9:
#line 73 "parse.y"
{ SET_BR_OFF((Inst *)yyvsp[-3].inst, GetPC()); ;
    break;}
case 10:
#line 75 "parse.y"
{ SET_BR_OFF(yyvsp[-6].inst, (yyvsp[-2].inst+1)); SET_BR_OFF(yyvsp[-2].inst, GetPC()); ;
    break;}
case 11:
#line 76 "parse.y"
{ ADD_OP(OP_BRANCH); ADD_BR_OFF(yyvsp[-5].inst);
    	    	SET_BR_OFF(yyvsp[-3].inst, GetPC()); FillLoopAddrs(GetPC(), yyvsp[-5].inst); ;
    break;}
case 12:
#line 79 "parse.y"
{ FillLoopAddrs(GetPC()+2+(yyvsp[-3].inst-(yyvsp[-5].inst+1)), GetPC());
    	    	  SwapCode(yyvsp[-5].inst+1, yyvsp[-3].inst, GetPC());
    	    	  ADD_OP(OP_BRANCH); ADD_BR_OFF(yyvsp[-7].inst); SET_BR_OFF(yyvsp[-5].inst, GetPC()); ;
    break;}
case 13:
#line 83 "parse.y"
{ ADD_OP(OP_BRANCH); ADD_BR_OFF(0); AddBreakAddr(GetPC()-1); ;
    break;}
case 14:
#line 85 "parse.y"
{ ADD_OP(OP_BRANCH); ADD_BR_OFF(0); AddContinueAddr(GetPC()-1); ;
    break;}
case 15:
#line 86 "parse.y"
{ ADD_OP(OP_RETURN); ;
    break;}
case 16:
#line 87 "parse.y"
{ ADD_OP(OP_RETURN_NO_VAL); ;
    break;}
case 17:
#line 89 "parse.y"
{ ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 18:
#line 90 "parse.y"
{ ADD_OP(OP_ADD); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 19:
#line 91 "parse.y"
{ ADD_OP(OP_SUB); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 20:
#line 92 "parse.y"
{ ADD_OP(OP_MUL); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 21:
#line 93 "parse.y"
{ ADD_OP(OP_DIV); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 22:
#line 94 "parse.y"
{ ADD_OP(OP_MOD); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 23:
#line 95 "parse.y"
{ ADD_OP(OP_BIT_AND); ADD_OP(OP_ASSIGN);
    	    	ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 24:
#line 97 "parse.y"
{ ADD_OP(OP_BIT_OR); ADD_OP(OP_ASSIGN);
    	    	ADD_SYM(yyvsp[-2].sym); ;
    break;}
case 25:
#line 99 "parse.y"
{ ADD_OP(OP_SUBR_CALL);
	    	ADD_SYM(PromoteToGlobal(yyvsp[-3].sym)); ADD_IMMED((void *)yyvsp[-1].nArgs); ;
    break;}
case 26:
#line 101 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ADD_OP(OP_INCR);
    	    	ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 27:
#line 103 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[-1].sym); ADD_OP(OP_INCR);
		ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-1].sym); ;
    break;}
case 28:
#line 105 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ADD_OP(OP_DECR);
	    	ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 29:
#line 107 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[-1].sym); ADD_OP(OP_DECR);
		ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-1].sym); ;
    break;}
case 30:
#line 110 "parse.y"
{ yyval.sym = yyvsp[0].sym; ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 31:
#line 112 "parse.y"
{ yyval.inst = GetPC(); ;
    break;}
case 32:
#line 113 "parse.y"
{ yyval.inst = GetPC(); ;
    break;}
case 33:
#line 114 "parse.y"
{ yyval.inst = GetPC(); ;
    break;}
case 34:
#line 116 "parse.y"
{ yyval.nArgs = 0;;
    break;}
case 35:
#line 117 "parse.y"
{ yyval.nArgs = 1; ;
    break;}
case 36:
#line 118 "parse.y"
{ yyval.nArgs = yyvsp[-2].nArgs + 1; ;
    break;}
case 38:
#line 121 "parse.y"
{ ADD_OP(OP_CONCAT); ;
    break;}
case 39:
#line 123 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 40:
#line 124 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 41:
#line 125 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 42:
#line 126 "parse.y"
{ ADD_OP(OP_SUBR_CALL);
	    	ADD_SYM(PromoteToGlobal(yyvsp[-3].sym)); ADD_IMMED((void *)yyvsp[-1].nArgs);
		ADD_OP(OP_FETCH_RET_VAL);;
    break;}
case 44:
#line 130 "parse.y"
{ ADD_OP(OP_ADD); ;
    break;}
case 45:
#line 131 "parse.y"
{ ADD_OP(OP_SUB); ;
    break;}
case 46:
#line 132 "parse.y"
{ ADD_OP(OP_MUL); ;
    break;}
case 47:
#line 133 "parse.y"
{ ADD_OP(OP_DIV); ;
    break;}
case 48:
#line 134 "parse.y"
{ ADD_OP(OP_MOD); ;
    break;}
case 49:
#line 135 "parse.y"
{ ADD_OP(OP_POWER); ;
    break;}
case 50:
#line 136 "parse.y"
{ ADD_OP(OP_NEGATE); ;
    break;}
case 51:
#line 137 "parse.y"
{ ADD_OP(OP_GT); ;
    break;}
case 52:
#line 138 "parse.y"
{ ADD_OP(OP_GE); ;
    break;}
case 53:
#line 139 "parse.y"
{ ADD_OP(OP_LT); ;
    break;}
case 54:
#line 140 "parse.y"
{ ADD_OP(OP_LE); ;
    break;}
case 55:
#line 141 "parse.y"
{ ADD_OP(OP_EQ); ;
    break;}
case 56:
#line 142 "parse.y"
{ ADD_OP(OP_NE); ;
    break;}
case 57:
#line 143 "parse.y"
{ ADD_OP(OP_BIT_AND); ;
    break;}
case 58:
#line 144 "parse.y"
{ ADD_OP(OP_BIT_OR); ;
    break;}
case 59:
#line 145 "parse.y"
{ ADD_OP(OP_AND); SET_BR_OFF(yyvsp[-1].inst, GetPC()); ;
    break;}
case 60:
#line 146 "parse.y"
{ ADD_OP(OP_OR); SET_BR_OFF(yyvsp[-1].inst, GetPC()); ;
    break;}
case 61:
#line 147 "parse.y"
{ ADD_OP(OP_NOT); ;
    break;}
case 62:
#line 148 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ADD_OP(OP_INCR);
    	    	ADD_OP(OP_DUP); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 63:
#line 150 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[-1].sym); ADD_OP(OP_DUP);
	    	ADD_OP(OP_INCR); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-1].sym); ;
    break;}
case 64:
#line 152 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[0].sym); ADD_OP(OP_DECR);
	    	ADD_OP(OP_DUP); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[0].sym); ;
    break;}
case 65:
#line 154 "parse.y"
{ ADD_OP(OP_PUSH_SYM); ADD_SYM(yyvsp[-1].sym); ADD_OP(OP_DUP);
	    	ADD_OP(OP_DECR); ADD_OP(OP_ASSIGN); ADD_SYM(yyvsp[-1].sym); ;
    break;}
case 66:
#line 157 "parse.y"
{ yyval.inst = GetPC(); StartLoopAddrList(); ;
    break;}
case 67:
#line 159 "parse.y"
{ StartLoopAddrList(); ;
    break;}
case 68:
#line 161 "parse.y"
{ ADD_OP(OP_BRANCH); yyval.inst = GetPC(); ADD_BR_OFF(0); ;
    break;}
case 69:
#line 163 "parse.y"
{ ADD_OP(OP_BRANCH_NEVER); yyval.inst = GetPC(); ADD_BR_OFF(0); ;
    break;}
case 70:
#line 164 "parse.y"
{ ADD_OP(OP_BRANCH_FALSE); yyval.inst = GetPC(); ADD_BR_OFF(0); ;
    break;}
case 71:
#line 166 "parse.y"
{ ADD_OP(OP_DUP); ADD_OP(OP_BRANCH_FALSE); yyval.inst = GetPC();
    	    	ADD_BR_OFF(0); ;
    break;}
case 72:
#line 169 "parse.y"
{ ADD_OP(OP_DUP); ADD_OP(OP_BRANCH_TRUE); yyval.inst = GetPC();
    	    	ADD_BR_OFF(0); ;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 457 "bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      XtFree(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 176 "parse.y"
 /* User Subroutines Section */

/*
** Parse a null terminated string and create a program from it (this is the
** parser entry point).  The program created by this routine can be
** executed using ExecuteProgram.  Returns program on success, or NULL
** on failure.  If the command failed, the error message is returned
** as a pointer to a static string in msg, and the length of the string up
** to where parsing failed in stoppedAt.
*/
Program *ParseMacro(char *expr, char **msg, char **stoppedAt)
{
    Program *prog;
    
    BeginCreatingProgram();
    
    /* call yyparse to parse the string and check for success.  If the parse
       failed, return the error message and string index (the grammar aborts
       parsing at the first error) */
    InPtr = expr;
    if (yyparse()) {
    	*msg = ErrMsg;
    	*stoppedAt = InPtr;
    	FreeProgram(FinishCreatingProgram());
    	return NULL;
    }
    
    /* get the newly created program */
    prog = FinishCreatingProgram();
    
    /* parse succeeded */
    *msg = "";
    *stoppedAt = InPtr;
    return prog;
}

static int yylex(void)
{
    int i, len;
    Symbol *s;
    static int stringConstIndex = 0;
    static DataValue value = {0, {0}};
    static char escape[] = "\\\"ntbrfav";
    static char replace[] = "\\\"\n\t\b\r\f\a\v";
    
    /* skip whitespace and backslash-newline combinations which are
       also considered whitespace */
    for (;;) {
    	if (*InPtr == '\\' && *(InPtr + 1) == '\n')
    	    InPtr += 2;
    	else if (*InPtr == ' ' || *InPtr == '\t')
    	    InPtr++;
    	else
    	    break;
    }
    
    /* skip comments */
    if (*InPtr == '#')
    	while (*InPtr != '\n' && *InPtr != '\0') InPtr++;
    
    /* return end of input at the end of the string */
    if (*InPtr == '\0') {
	return 0;
    }
    
    /* process number tokens */
    if (isdigit(*InPtr))  { /* number */
        char name[28];
        sscanf(InPtr, "%d%n", &value.val.n, &len);
        sprintf(name, "const %d", value.val.n);
        InPtr += len;
        value.tag = INT_TAG;
        if ((yylval.sym=LookupSymbol(name)) == NULL)
            yylval.sym = InstallSymbol(name, CONST_SYM, value);
        return NUMBER;
    }
    
    /* process symbol tokens.  "define" is a special case not handled
       by this parser, considered end of input.  Another special case
       is action routine names which are allowed to contain '-' despite
       the ambiguity, handled in matchesActionRoutine. */
    if (isalpha(*InPtr) || *InPtr == '$') {
        if ((s=matchesActionRoutine(&InPtr)) == NULL) {
            char symName[MAX_SYM_LEN+1], *p = symName;
            *p++ = *InPtr++;
            while (isalnum(*InPtr) || *InPtr=='_') {
		if (p >= symName + MAX_SYM_LEN)
		    InPtr++;
		else
		    *p++ = *InPtr++;
	    }
	    *p = '\0';
	    if (!strcmp(symName, "while")) return WHILE;
	    if (!strcmp(symName, "if")) return IF;
	    if (!strcmp(symName, "else")) return ELSE;
	    if (!strcmp(symName, "for")) return FOR;
	    if (!strcmp(symName, "break")) return BREAK;
	    if (!strcmp(symName, "continue")) return CONTINUE;
	    if (!strcmp(symName, "return")) return RETURN;
	    if (!strcmp(symName, "define")) {
	    	InPtr -= 6;
	    	return 0;
	    }
	    if ((s=LookupSymbol(symName)) == NULL) {
        	s = InstallSymbol(symName, symName[0]=='$' ? (isdigit(symName[1]) ?
            		ARG_SYM : GLOBAL_SYM) : LOCAL_SYM, value);
            	s->value.tag = NO_TAG;
            }
	}
	yylval.sym = s;
        return SYMBOL;
    }
    
    /* process quoted strings w/ embedded escape sequences */
    if (*InPtr == '\"') {
        char string[MAX_STRING_CONST_LEN], *p = string;
        char stringName[25];
        InPtr++;
        while (*InPtr != '\0' && *InPtr != '\"' && *InPtr != '\n') {
	    if (p >= string + MAX_STRING_CONST_LEN) {
	    	InPtr++;
	    	continue;
	    }
	    if (*InPtr == '\\') {
		InPtr++;
		if (*InPtr == '\n') {
		    InPtr++;
		    continue;
		}
		for (i=0; escape[i]!='\0'; i++) {
		    if (escape[i] == '\0') {
		    	*p++= *InPtr++;
		    	break;
		    } else if (escape[i] == *InPtr) {
		    	*p++ = replace[i];
		    	InPtr++;
		    	break;
		    }
		}
	    } else
		*p++= *InPtr++;
	}
	*p = '\0';
	InPtr++;
	value.val.str = AllocString(p-string+1);
	strcpy(value.val.str, string);
	value.tag = STRING_TAG;
	sprintf(stringName, "string #%d", stringConstIndex++);
	yylval.sym = InstallSymbol(stringName, CONST_SYM, value);
        return STRING;
    }
    
    /* process remaining two character tokens or return single char as token */
    switch(*InPtr++) {
    case '>':	return follow('=', GE, GT);
    case '<':	return follow('=', LE, LT);
    case '=':	return follow('=', EQ, '=');
    case '!':	return follow('=', NE, NOT);
    case '+':	return follow2('+', INCR, '=', ADDEQ, '+');
    case '-':	return follow2('-', DECR, '=', SUBEQ, '-');
    case '|':	return follow2('|', OR, '=', OREQ, '|');
    case '&':	return follow2('&', AND, '=', ANDEQ, '&');
    case '*':	return follow2('*', POW, '=', MULEQ, '*');
    case '/':   return follow('=', DIVEQ, '/');
    case '%':	return follow('=', MODEQ, '%');
    case '^':	return POW;
    default:	return *(InPtr-1);
    }
}

/*
** look ahead for >=, etc.
*/
static int follow(char expect, int yes, int no)
{
    if (*InPtr++ == expect)
	return yes;
    InPtr--;
    return no;
}
static int follow2(char expect1, int yes1, char expect2, int yes2, int no)
{
    char next = *InPtr++;
    if (next == expect1)
	return yes1;
    if (next == expect2)
    	return yes2;
    InPtr--;
    return no;
}

/*
** Look (way) ahead for hyphenated routine names which begin at inPtr.  A
** hyphenated name is allowed if it is pre-defined in the global symbol
** table.  If a matching name exists, returns the symbol, and update "inPtr".
**
** I know this is horrible language design, but existing nedit action routine
** names contain hyphens.  Handling them here in the lexical analysis process
** is much easier than trying to deal with it in the parser itself.  (sorry)
*/
static Symbol *matchesActionRoutine(char **inPtr)
{
    char *c, *symPtr;
    int hasDash = False;
    char symbolName[MAX_SYM_LEN+1];
    Symbol *s;
    
    symPtr = symbolName;
    for (c = *inPtr; isalnum(*c) || *c=='_' || (*c=='-'&&isalnum(*(c+1))); c++){
    	if (*c == '-')
    	    hasDash = True;
    	*symPtr++ = *c;
    }
    if (!hasDash)
    	return NULL;
    *symPtr = '\0';
    s = LookupSymbol(symbolName);
    if (s != NULL)
    	*inPtr = c;
    return s;
}

/*
** Called by yacc to report errors (just stores for returning when
** parsing is aborted.  The error token action is to immediate abort
** parsing, so this message is immediately reported to the caller
** of ParseExpr)
*/
yyerror(char *s)
{
    ErrMsg = s;
}
