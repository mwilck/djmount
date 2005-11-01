dnl @synopsis RT_BOOL_ARG_ENABLE([FLAG],[DEFAULT],[HELP STRING])
dnl
dnl This macro declares a configure option with 'AC_ARG_ENABLE'.
dnl It is a boolean argument (yes or no values only), and 
dnl the corresponding shell variable 'enable_arg' is guaranteed to 
dnl be one or the other.
dnl A message is also printed.
dnl
dnl Arguments
dnl $1 = flag name e.g. [debug]
dnl $2 = default value, shall be constant, either [yes] or [no]
dnl $3 = help string (default value is appended) e.g. [compile debugging code]
dnl
dnl @version $Id$
dnl @author Rémi Turboult <r3mi@users.sourceforge.net>
dnl
AC_DEFUN([RT_BOOL_ARG_ENABLE],[
	AC_MSG_CHECKING([for --enable-$1 ])
	dnl
	dnl use some m4 sugar to have only one 'AC_ARG_ENABLE' declaration,
	dnl else "configure --help" is confused
	dnl
	AC_ARG_ENABLE([$1],
		      [m4_case([$2],
		      	[yes],AS_HELP_STRING([--disable-$1],
			     	             [$3 [[default=enabled]] ]),
			[no],AS_HELP_STRING([--enable-$1],
			     	             [$3 [[default=disabled]] ]),
			[m4_fatal([incorrect boolean argument '$2'])]
		               )])
	test "x$enable_[$1]" != [x]m4_if([$2],[yes],[no],[yes]) dnl
		&& enable_[$1]=[$2]
	AC_MSG_RESULT([$enable_$1])dnl
])dnl



