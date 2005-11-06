dnl @synopsis VL_LIB_READLINE
dnl
dnl Searches for a readline compatible library. If found, defines
dnl `HAVE_LIBREADLINE'. If the found library has the `add_history'
dnl function, sets also `HAVE_READLINE_HISTORY'. Also checks for the
dnl locations of the necessary include files and sets `HAVE_READLINE_H'
dnl or `HAVE_READLINE_READLINE_H' and `HAVE_READLINE_HISTORY_H' or
dnl 'HAVE_HISTORY_H' if the corresponding include files exists.
dnl
dnl The libraries that may be readline compatible are `libedit',
dnl `libeditline' and `libreadline'. Sometimes we need to link a
dnl termcap library for readline to work, this macro tests these cases
dnl too by trying to link with `libtermcap', `libcurses' or
dnl `libncurses' before giving up.
dnl This macro sets the READLINE_LIBS variable accordingly, and AC_SUBST it.
dnl
dnl Here is an example of how to use the information provided by this
dnl macro to perform the necessary includes or declarations in a C
dnl file:
dnl
dnl   #ifdef HAVE_LIBREADLINE
dnl   #  if defined(HAVE_READLINE_READLINE_H)
dnl   #    include <readline/readline.h>
dnl   #  elif defined(HAVE_READLINE_H)
dnl   #    include <readline.h>
dnl   #  else /* !defined(HAVE_READLINE_H) */
dnl   extern char *readline ();
dnl   #  endif /* !defined(HAVE_READLINE_H) */
dnl   char *cmdline = NULL;
dnl   #else /* !defined(HAVE_READLINE_READLINE_H) */
dnl     /* no readline */
dnl   #endif /* HAVE_LIBREADLINE */
dnl
dnl   #ifdef HAVE_READLINE_HISTORY
dnl   #  if defined(HAVE_READLINE_HISTORY_H)
dnl   #    include <readline/history.h>
dnl   #  elif defined(HAVE_HISTORY_H)
dnl   #    include <history.h>
dnl   #  else /* !defined(HAVE_HISTORY_H) */
dnl   extern void add_history ();
dnl   extern int write_history ();
dnl   extern int read_history ();
dnl   #  endif /* defined(HAVE_READLINE_HISTORY_H) */
dnl     /* no history */
dnl   #endif /* HAVE_READLINE_HISTORY */
dnl
dnl @category InstalledPackages
dnl @author Ville Laurikari <vl@iki.fi>
dnl @version 2002-04-04-patched:
dnl	patch 2005-11-06 by R. Turboult : 
dnl		1) output READLINE_LIBS instead of LIBS
dnl		2) correct behaviour when cache is used (LIBS was not set ...)
dnl @license AllPermissive

AC_DEFUN([VL_LIB_READLINE], [
  AC_CACHE_CHECK([for a readline compatible library],
                 vl_cv_lib_readline, [
    ORIG_LIBS="$LIBS"
    for readline_lib in readline edit editline; do
      for termcap_lib in "" termcap curses ncurses; do
        if test -z "$termcap_lib"; then
          TRY_LIB="-l$readline_lib"
        else
          TRY_LIB="-l$readline_lib -l$termcap_lib"
        fi
        LIBS="$ORIG_LIBS $TRY_LIB"
        AC_TRY_LINK_FUNC(readline, vl_cv_lib_readline="$TRY_LIB")
        if test -n "$vl_cv_lib_readline"; then
          break
        fi
      done
      if test -n "$vl_cv_lib_readline"; then
        break
      fi
    done
    if test -z "$vl_cv_lib_readline"; then
      vl_cv_lib_readline="no"
    fi
    # patch R. Turboult 2005-11-06
    LIBS="$ORIG_LIBS"
  ])


  if test "$vl_cv_lib_readline" != "no"; then

    # patch R. Turboult 2005-11-06
    READLINE_LIBS=$vl_cv_lib_readline

    AC_DEFINE(HAVE_LIBREADLINE, 1,
              [Define if you have a readline compatible library])
    AC_CHECK_HEADERS(readline.h readline/readline.h)
    AC_CACHE_CHECK([whether readline supports history],
                   vl_cv_lib_readline_history, [
      vl_cv_lib_readline_history="no"
      AC_TRY_LINK_FUNC(add_history, vl_cv_lib_readline_history="yes")
    ])
    if test "$vl_cv_lib_readline_history" = "yes"; then
      AC_DEFINE(HAVE_READLINE_HISTORY, 1,
                [Define if your readline library has \`add_history'])
      AC_CHECK_HEADERS(history.h readline/history.h)
    fi
  fi

  # patch R. Turboult 2005-11-06
  AC_SUBST(READLINE_LIBS)

])dnl
