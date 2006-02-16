dnl 
dnl We cannot use the AC_CHECK_TYPE macro to check for socklen_t because that 
dnl macro only checks in standard headers. This one checks in sys/socket.h 
dnl also. This code has been copied from Unix Network Programming examples
dnl by W. Richard Stevens
dnl
dnl OPENSLP_CHECK_TYPE(TYPE, DEFAULT, DESCRIPTION)
AC_DEFUN([OPENSLP_CHECK_TYPE],
[AC_REQUIRE([AC_HEADER_STDC])dnl
AC_MSG_CHECKING(for $1)
AC_CACHE_VAL(ac_cv_type_$1,
[AC_EGREP_CPP(dnl
changequote(<<,>>)dnl
<<(^|[^a-zA-Z_0-9])$1[^a-zA-Z_0-9]>>dnl
changequote([,]), [#include <sys/types.h>
#if STDC_HEADERS
#include <stdlib.h>
#include <stddef.h>
#endif
#include <sys/socket.h>], ac_cv_type_$1=yes, ac_cv_type_$1=no)])dnl
AC_MSG_RESULT($ac_cv_type_$1)
if test $ac_cv_type_$1 = no; then
  AC_DEFINE($1, $2, $3)
fi
])

dnl Check for the presence of SA_RESTORER field in struct sigaction.
dnl
dnl OPENSLP_STRUCT_SA_RESTORER
AC_DEFUN([OPENSLP_STRUCT_SA_RESTORER],
[AC_CACHE_CHECK([for sa_restorer in struct sigaction], 
ac_cv_struct_sa_restorer,
[AC_TRY_COMPILE([#include <sys/types.h>
#include <sys/signal.h>], [struct sigaction s; s.sa_restorer;],
ac_cv_struct_sa_restorer=yes, ac_cv_struct_sa_restorer=no)])
if test $ac_cv_struct_sa_restorer = yes; then
  AC_DEFINE(HAVE_SA_RESTORER, 1, [defined if struct sigaction has member sa_restorer])
fi
])

# SLP_ENABLE_DEFINE(variable, define variable, help message)
# Provides an --enable-foo option and a way to set an acconfig.h define switch
# e.g. SLP_ENABLE_DEFINE([memdebug], [MEM_DEBUG], [Memory Debugging])
#----------------------------------------------------------------
AC_DEFUN([SLP_ENABLE_DEFINE],
        [AC_MSG_CHECKING([config option $1 for setting $2])
        AC_ARG_ENABLE([$1], AC_HELP_STRING([--enable-$1], [$3]), , enableval=no)
        AC_MSG_RESULT($enableval)
        case "$1" in
	        yes) AC_DEFINE([$2], 1, [$3]) ;; 
	        no) ;; 
	        *) AC_MSG_ERROR(bad value ${1} for --enable-$1) ;;
	esac])

