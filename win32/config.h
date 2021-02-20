/* config.h.in.  Generated automatically from configure.in by autoheader.  */

/* Define if on AIX 3.
   System headers sometimes define this.
   We just want to avoid a redefinition error message.  */
#ifndef _ALL_SOURCE
#undef _ALL_SOURCE
#endif

/* Define if using alloca.c.  */
#undef C_ALLOCA

/* Define to empty if the keyword does not work.  */
#undef const

/* Define to one of _getb67, GETB67, getb67 for Cray-2 and Cray-YMP systems.
   This function is required for alloca.c support on those systems.  */
#undef CRAY_STACKSEG_END

/* Define if you have alloca, as a function or macro.  */
#define HAVE_ALLOCA

/* Define if you have <alloca.h> and it should be used (not on Ultrix).  */
#undef HAVE_ALLOCA_H

/* Define if you don't have vprintf but do have _doprnt.  */
#undef HAVE_DOPRNT

/* Define if you have <sys/wait.h> that is POSIX.1 compatible.  */
#undef HAVE_SYS_WAIT_H

/* Define if utime(file, NULL) sets file's timestamp to the present.  */
#undef HAVE_UTIME_NULL

/* Define if you have the vprintf function.  */
#define HAVE_VPRINTF

/* Define as __inline if that's what the C compiler calls it.  */
#undef inline

/* Define if on MINIX.  */
#undef _MINIX

/* Define to `int' if <sys/types.h> doesn't define.  */
#define pid_t int

/* Define if the system does not provide POSIX.1 features except
   with this defined.  */
#undef _POSIX_1_SOURCE

/* Define if you need to in order for stat and other things to work.  */
#undef _POSIX_SOURCE

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at run-time.
 STACK_DIRECTION > 0 => grows toward higher addresses
 STACK_DIRECTION < 0 => grows toward lower addresses
 STACK_DIRECTION = 0 => direction of growth unknown
 */
#undef STACK_DIRECTION

/* Define if you have the ANSI C header files.  */
#define STDC_HEADERS 1

/* Define if you can safely include both <sys/time.h> and <time.h>.  */
#undef TIME_WITH_SYS_TIME

/* Define if the X Window System is missing or not being used.  */
#undef X_DISPLAY_MISSING

/* Define if you have the XSupportsLocale function.  */
#undef HAVE_XSUPPORTSLOCALE

/* Define if you have the XmImRegister function.  */
#define HAVE_XMIMREGISTER

/* Define if you have the _XEditResCheckMessages function.  */
#define HAVE__XEDITRESCHECKMESSAGES

/* Define if you have the _XmOSGetDirEntries function.  */
#define HAVE__XMOSGETDIRENTRIES

/* Define if you have the atexit function.  */
#define HAVE_ATEXIT

/* Define if you have the getcwd function.  */
#define HAVE_GETCWD

/* Define if you have the memcpy function.  */
#define HAVE_MEMCPY

/* Define if you have the memmove function.  */
#define HAVE_MEMMOVE

/* Define if you have the on_exit function.  */
#undef HAVE_ON_EXIT

/* Define if you have the select function.  */
#define HAVE_SELECT

/* Define if you have the strcasecmp function.  */
#undef HAVE_STRCASECMP

/* Define if you have the strchr function.  */
#define HAVE_STRCHR

/* Define if you have the strcspn function.  */
#define HAVE_STRCSPN

/* Define if you have the strerror function.  */
#define HAVE_STRERROR

/* Define if you have the strspn function.  */
#define HAVE_STRSPN

/* Define if you have the strstr function.  */
#define HAVE_STRSTR

/* Define if you have the strtod function.  */
#define HAVE_STRTOD

/* Define if you have the strtol function.  */
#define HAVE_STRTOL

/* Define if you have the usleep function.  */
#undef HAVE_USLEEP

/* Define if you have the <X11/Xlocale.h> header file.  */
#undef HAVE_X11_XLOCALE_H

/* Define if you have the <X11/Xmu/Editres.h> header file.  */
#define HAVE_X11_XMU_EDITRES_H

/* Define if you have the <dirent.h> header file.  */
#undef HAVE_DIRENT_H

/* Define if you have the <fcntl.h> header file.  */
#define HAVE_FCNTL_H

/* Define if you have the <glob.h> header file.  */
#undef HAVE_GLOB_H

/* Define if you have the <limits.h> header file.  */
#define HAVE_LIMITS_H

/* Define if you have the <ndir.h> header file.  */
#undef HAVE_NDIR_H

/* Define if you have the <sys/dir.h> header file.  */
#undef HAVE_SYS_DIR_H

/* Define if you have the <sys/ndir.h> header file.  */
#undef HAVE_SYS_NDIR_H

/* Define if you have the <sys/param.h> header file.  */
#undef HAVE_SYS_PARAM_H

/* Define if you have the <sys/select.h> header file.  */
#undef HAVE_SYS_SELECT_H

/* Define if you have the <sys/time.h> header file.  */
#undef HAVE_SYS_TIME_H

/* Define if you have the <sys/utsname.h> header file.  */
#undef HAVE_SYS_UTSNAME_H

/* Define if you have the <unistd.h> header file.  */
#undef HAVE_UNISTD_H

/* Define if you have the <utime.h> header file.  */
#undef HAVE_UTIME_H

/* Define if you have the m library (-lm).  */
#undef HAVE_LIBM

/* Define if you have the socket library (-lsocket).  */
#undef HAVE_LIBSOCKET

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
# define USE_LPR_PRINT_CMD
# define USE_ENVIRONMENT_AS_MACRO_GLOBALS
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

#define PERRORWIN32(message) \
{\
	char buf[10240]; \
	sprintf(buf, "%s: %s\n", message, GetLastErrorString()); \
	DialogF(DF_WARN, WindowList->shell, 1, message, "OK"); \
}

/* This is the NEditMV package unless overriden elsewhere */
#ifndef PACKAGE
#define PACKAGE "NEditMV"
#endif

#ifndef DISPATCH_EVENT
#define DISPATCH_EVENT(event) ServerDispatchEvent(event)
#endif

