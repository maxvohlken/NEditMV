
@BOTTOM@

#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
# endif
# ifndef HAVE_MEMMOVE
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
#endif

#ifdef _WIN32
# include <stdlib.h>
# define MAXPATHLEN _MAX_PATH
# ifndef getcwd
#  define getcwd _getcwd
# endif
#if 0
# define WIN32_BINARY_FILE_MODE 1
#endif
# define FILECOMPARE(file1, file2) _stricmp((file1), (file2))
# define FILENCOMPARE(file1, file2, count) _strnicmp((file1), (file2), (count))
#else
# define FILECOMPARE(file1, file2) strcmp((file1), (file2))
# define FILENCOMPARE(file1, file2, count) strncmp((file1), (file2), (count))
#endif

#define PERROR(message) \
{\
	char buf[10240]; \
	sprintf(buf, "%s: %s\n", message, strerror(errno)); \
	DialogF(DF_WARN, WindowList->shell, 1, message, "OK"); \
}

/* This is the NEditMV package unless overriden elsewhere */
#ifndef PACKAGE
#define PACKAGE "NEditMV"
#endif
