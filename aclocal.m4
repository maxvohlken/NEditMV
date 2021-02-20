# Local additions to Autoconf macros.

#
# Check for Motif.
#
AC_DEFUN(AC_PATH_MOTIF,
[
AC_REQUIRE([AC_PATH_X])dnl
# If we find Motif, set shell vars motif_includes and motif_libraries to the
# paths, otherwise set no_motif=yes.
# Uses ac_ vars as temps to allow command line to override cache and checks.
# --without-motif overrides everything else, but does not touch the cache.
AC_MSG_CHECKING(for Motif)

AC_ARG_WITH(motif, [  --with-motif=[DIR | INCDIR:LIBDIR] use Motif
    To specify that Motif is in multiple locations use:
    --with-motif=[DIR1,DIR2[,...] | INCDIR1:LIBDIR1:INCDIR2:LIBDIR2[:...]]
    This is useful on machines that use -R to tell the executable where to
    find the shared libraries at runtime like Solaris.])
motif_includes=NONE
motif_libraries=NONE
# $have_motif is `yes', `no', `disabled', or empty when we do not yet know.
if test "x$with_motif" = xno; then
  # The user explicitly disabled Motif.
  have_motif=disabled
else
  # Multiple separate include and lib directories.
  case "$with_motif" in
  *:*)
    motif_includes=
    motif_libraries=
  	set -- `echo "$with_motif" | sed -e 's/:/ /g'`
	while test [$]# -gt 0; do
    	motif_includes="$motif_includes [$]1"
    	motif_libraries="$motif_libraries [$]2"
		shift
		shift
	done
    ;;
  # Multiple home directories.
  *,*)
    motif_includes=
    motif_libraries=
  	set -- `echo "$with_motif" | sed -e 's/,/ /g'`
	while test [$]# -gt 0; do
    	motif_includes="$motif_includes [$]1/include"
    	motif_libraries="$motif_libraries [$]1/lib"
		shift
	done
    ;;
  */*)
    motif_includes="$with_motif/include"
    motif_libraries="$with_motif/lib"
    ;;
  esac
  if test "x$motif_includes" != xNONE && test "x$motif_libraries" != xNONE; then
    # Both variables are already set.
    have_motif=yes
  else
AC_CACHE_VAL(ac_cv_have_motif,
[# One or both of the vars are not set, and there is no cached value.
ac_motif_includes=NO ac_motif_libraries=NO
dnl AC_PATH_MOTIF_XMKMF
AC_PATH_MOTIF_DIRECT
if test "$ac_motif_includes" = NO || test "$ac_motif_libraries" = NO; then
  # Didn't find Motif anywhere.  Cache the known absence of Motif.
  ac_cv_have_motif="have_motif=no"
else
  # Record where we found Motif for the cache.
  ac_cv_have_motif="have_motif=yes \
	        ac_motif_includes='$ac_motif_includes' ac_motif_libraries='$ac_motif_libraries'"
fi])dnl
  fi
  eval "$ac_cv_have_motif"
fi # $with_motif != no

if test "$have_motif" != yes; then
  AC_MSG_RESULT($have_motif)
  no_motif=yes
else
  # If each of the values was on the command line, it overrides each guess.
  test "x$motif_includes" = xNONE && motif_includes="$ac_motif_includes"
  test "x$motif_libraries" = xNONE && motif_libraries="$ac_motif_libraries"
  # Update the cache value to reflect the command line values.
  ac_cv_have_motif="have_motif=yes \
		ac_motif_includes='$motif_includes' ac_motif_libraries='$motif_libraries'"
  AC_MSG_RESULT([libraries $motif_libraries, headers $motif_includes])
fi
])

dnl Internal subroutine of AC_PATH_MOTIF.
dnl Set ac_motif_includes and/or ac_motif_libraries.
AC_DEFUN(AC_PATH_MOTIF_XMKMF,
[rm -fr conftestdir
if mkdir conftestdir; then
  cd conftestdir
  # Make sure to not put "make" in the Imakefile rules, since we grep it out.
  cat > Imakefile <<'EOF'
acfindx:
	@echo 'ac_im_incroot="${INCROOT}"; ac_im_usrlibdir="${USRLIBDIR}"; ac_im_libdir="${LIBDIR}"'
EOF
  if (xmkmf) >/dev/null 2>/dev/null && test -f Makefile; then
    # GNU make sometimes prints "make[1]: Entering...", which would confuse us.
    eval `${MAKE-make} acfindx 2>/dev/null | grep -v make`
    # Open Windows xmkmf reportedly sets LIBDIR instead of USRLIBDIR.
    for ac_extension in a so sl; do
      if test ! -f $ac_im_usrlibdir/libX11.$ac_extension &&
        test -f $ac_im_libdir/libX11.$ac_extension; then
        ac_im_usrlibdir=$ac_im_libdir; break
      fi
    done
    # Screen out bogus values from the imake configuration.  They are
    # bogus both because they are the default anyway, and because
    # using them would break gcc on systems where it needs fixed includes.
    case "$ac_im_incroot" in
	/usr/include) ;;
	*) test -f "$ac_im_incroot/Xm/Xm.h" && ac_motif_includes="$ac_im_incroot" ;;
    esac
    case "$ac_im_usrlibdir" in
	/usr/lib | /lib) ;;
	*) test -d "$ac_im_usrlibdir" && ac_motif_libraries="$ac_im_usrlibdir" ;;
    esac
  fi
  cd ..
  rm -fr conftestdir
fi
])

dnl Internal subroutine of AC_PATH_MOTIF.
dnl Set ac_motif_includes and/or ac_motif_libraries.
AC_DEFUN(AC_PATH_MOTIF_DIRECT,
[if test "$ac_motif_includes" = NO; then
  # Guess where to find include files, by looking for this one Xm .h file.
  test -z "$motif_direct_test_include" && motif_direct_test_include=Xm/Xm.h

  # First, try using that file with no special directory specified.
AC_TRY_CPP([#include <$motif_direct_test_include>],
[# We can compile using Motif headers with no special include directory.
ac_motif_includes=],
[# Look for the header file in a standard set of common directories.
# Check X11 before X11Rn because it is often a symlink to the current release.
  for ac_dir in               \
  	$x_includes               \
    /usr/X11/include          \
    /usr/X11R6/include        \
    /usr/X11R5/include        \
    /usr/X11R4/include        \
                              \
    /usr/include/X11          \
    /usr/include/X11R6        \
    /usr/include/X11R5        \
    /usr/include/X11R4        \
                              \
    /usr/local/X11/include    \
    /usr/local/X11R6/include  \
    /usr/local/X11R5/include  \
    /usr/local/X11R4/include  \
                              \
    /usr/local/include/X11    \
    /usr/local/include/X11R6  \
    /usr/local/include/X11R5  \
    /usr/local/include/X11R4  \
                              \
    /usr/X386/include         \
    /usr/x386/include         \
    /usr/XFree86/include/X11  \
                              \
    /usr/include              \
    /usr/local/include        \
    /usr/unsupported/include  \
    /usr/athena/include       \
    /usr/local/x11r5/include  \
    /usr/lpp/Xamples/include  \
                              \
    /usr/openwin/include      \
    /usr/openwin/share/include \
                              \
    /usr/include/Motif        \
    /usr/include/Motif1.2     \
    /usr/local/motif/include  \
    /usr/local/Motif/include  \
    /usr/dt/include           \
    /usr/dt/share/include     \
    /usr/include/Motif1.1     \
    ; \
  do
    if test -r "$ac_dir/$motif_direct_test_include"; then
      ac_motif_includes=$ac_dir
      break
    fi
  done])
fi # $ac_motif_includes = NO

if test "$ac_motif_libraries" = NO; then
  # Check for the libraries.

  test -z "$motif_direct_test_library" && motif_direct_test_library=Xm
  test -z "$motif_direct_test_function" && motif_direct_test_function=XmStringCreate

  # See if we find them without any special options.
  # Don't add to $LIBS permanently.
  ac_save_LIBS="$LIBS"
  LIBS="-l$motif_direct_test_library -lXt -lX11 $LIBS"
AC_TRY_LINK(, [${motif_direct_test_function}()],
[LIBS="$ac_save_LIBS"
# We can link Motif programs with no special library path.
ac_motif_libraries=],
[LIBS="$ac_save_LIBS"
# First see if replacing the include by lib works.
# Check X11 before X11Rn because it is often a symlink to the current release.
for ac_dir in $motif_libraries `echo "$ac_motif_includes" | sed s/include/lib/` \
    $x_libraries          \
    /usr/X11/lib          \
    /usr/X11R6/lib        \
    /usr/X11R5/lib        \
    /usr/X11R4/lib        \
                          \
    /usr/lib/X11          \
    /usr/lib/X11R6        \
    /usr/lib/X11R5        \
    /usr/lib/X11R4        \
                          \
    /usr/local/X11/lib    \
    /usr/local/X11R6/lib  \
    /usr/local/X11R5/lib  \
    /usr/local/X11R4/lib  \
                          \
    /usr/local/lib/X11    \
    /usr/local/lib/X11R6  \
    /usr/local/lib/X11R5  \
    /usr/local/lib/X11R4  \
                          \
    /usr/X386/lib         \
    /usr/x386/lib         \
    /usr/XFree86/lib/X11  \
                          \
    /usr/lib              \
    /usr/local/lib        \
    /usr/unsupported/lib  \
    /usr/athena/lib       \
    /usr/local/x11r5/lib  \
    /usr/lpp/Xamples/lib  \
    /lib/usr/lib/X11	  \
                          \
    /usr/openwin/lib      \
    /usr/openwin/share/lib \
                          \
    /usr/lib/Motif        \
    /usr/lib/Motif1.2     \
    /usr/local/motif/lib  \
    /usr/local/Motif/lib  \
    /usr/dt/lib           \
    /usr/lib/Motif1.1     \
    ; \
do
dnl Don''t even attempt the hair of trying to link an Motif program!
  for ac_extension in a so sl; do
    if test -r $ac_dir/lib${motif_direct_test_library}.$ac_extension; then
      ac_motif_libraries=$ac_dir
      break 2
    fi
  done
done])
fi # $ac_motif_libraries = NO
])

#
# Check for Motif.
# 
# Sets up the following substitution variables.
#
# MOTIF_CFLAGS     -- Additional C flags needed to compile a Motif program
# MOTIF_LDFLAGS    -- Additional ld flags needed to compile a Motif program
# MOTIF_PRE_LIBS   -- Libs needed before MOTIF_LIBS.
# MOTIF_LIBS       -- -lXm
# MOTIF_EXTRA_LIBS -- Libs needed after MOTIF_LIBS.
# 
dnl Find additional Motif libraries, magic flags, etc.
AC_DEFUN(AC_PATH_MOTIF_XTRA,
[AC_REQUIRE([AC_PATH_XTRA])dnl
AC_REQUIRE([AC_PATH_MOTIF])dnl
if test "$no_motif" = yes; then
  MOTIF_CFLAGS= MOTIF_PRE_LIBS= MOTIF_LDFLAGS= MOTIF_LIBS= MOTIF_EXTRA_LIBS=
else
  MOTIF_CFLAGS="$X_CFLAGS"
  MOTIF_LDFLAGS="$X_LDFLAGS"
  MOTIF_LIBS='-lXm'
  MOTIF_EXTRA_LIBS="-lXt $X_PRE_LIBS $X_LIBS -lX11 $X_EXTRA_LIBS"
  
  if test -n "$motif_includes"; then
  	for motif_include in $motif_includes; do
    	MOTIF_CFLAGS="$MOTIF_CFLAGS -I$motif_include"
	done
  fi

  # It would also be nice to do this for all -L options, not just this one.
  if test -n "$motif_libraries"; then
  	for motif_library in $motif_libraries; do
        MOTIF_LDFLAGS="$MOTIF_LDFLAGS -L$motif_library"
	done
    dnl FIXME banish uname from this macro!
    # For Solaris; some versions of Sun CC require a space after -R and
    # others require no space.  Words are not sufficient . . . .
    case "`(uname -sr) 2>/dev/null`" in
    "SunOS 5"*)
  	  for motif_library in $motif_libraries; do
        AC_MSG_CHECKING(whether -R must be followed by a space)
        ac_motifsave_LIBS="$LIBS"; LIBS="$LIBS -R$motif_library"
        AC_TRY_LINK(, , ac_R_nospace=yes, ac_R_nospace=no)
        if test "$ac_R_nospace" = yes; then
          AC_MSG_RESULT(no)
        else
          LIBS="$ac_motifsave_LIBS -R $motif_library"
          AC_TRY_LINK(, , ac_R_space=yes, ac_R_space=no)
          if test "$ac_R_space" = yes; then
            AC_MSG_RESULT(yes)
          else
            AC_MSG_RESULT(neither works)
          fi
        fi
        LIBS="$ac_motifsave_LIBS"
		# only check once
		break
	  done
  	  for motif_library in $motif_libraries; do
        if test "$ac_R_nospace" = yes; then
          MOTIF_LDFLAGS="$MOTIF_LDFLAGS -R$motif_library"
	    elif test "$ac_R_space" = yes; then
          MOTIF_LDFLAGS="$MOTIF_LDFLAGS -R $motif_library"
        fi
	  done
	  ;;
    esac
  fi
  
  # Check for libraries that Motif programs might need.
  # Linux Motif needs -lXpm and -lXext.
  # XPMLIBDIR can be set to the directory where libXpm is installed.
  ac_save_LDFLAGS="$LDFLAGS"
  AC_CHECK_LIB(Xmu, XmuInternAtom,
    [MOTIF_EXTRA_LIBS="-lXmu $MOTIF_EXTRA_LIBS"], ,[$MOTIF_LDFLAGS $MOTIF_EXTRA_LIBS])
  AC_CHECK_LIB(Xext, XShapeQueryVersion,
    [MOTIF_EXTRA_LIBS="-lXext $MOTIF_EXTRA_LIBS"], ,[$MOTIF_LDFLAGS $MOTIF_EXTRA_LIBS])
  if test x"$XPMLIBDIR" != x; then
    XPMLDFLAGS="-L$XPMLIBDIR"
    LDFLAGS="$LDFLAGS $XPMLDFLAGS"
  fi
  AC_CHECK_LIB(Xpm, XpmFreeAttributes,
    [MOTIF_EXTRA_LIBS="-lXpm $MOTIF_EXTRA_LIBS" MOTIF_LDFLAGS="$MOTIF_LDFLAGS $XPMLDFLAGS"], ,[$MOTIF_LDFLAGS $MOTIF_EXTRA_LIBS])
  LDFLAGS="$ac_save_LDFLAGS"

  # OS specific stuff.
  case "$target" in
  *-*-sunos4*)
    # Statically link the X libs on SunOS 4 unless enable_static_x is already set.
    if test "${enable_static_x+set}" != set; then
      enable_static_x=yes
    fi
    ;;
  esac

  # See if the user wants to link the X libraries statically.
  AC_ARG_ENABLE(static-x, [  --enable-static-x         statically link the X libraries], [dnl
    if test x"$enableval" != xno; then
      MOTIF_PRE_LIBS="-Wl,-Bstatic $MOTIF_PRE_LIBS"
      MOTIF_EXTRA_LIBS="$MOTIF_EXTRA_LIBS -Wl,-Bdynamic"
    fi
  ])
fi
AC_SUBST(MOTIF_CFLAGS)dnl
AC_SUBST(MOTIF_PRE_LIBS)dnl
AC_SUBST(MOTIF_LDFLAGS)dnl
AC_SUBST(MOTIF_LIBS)dnl
AC_SUBST(MOTIF_EXTRA_LIBS)dnl
])

