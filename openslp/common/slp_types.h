/*-------------------------------------------------------------------------
 * Copyright (C) 2005 Novell, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

/** Defines basic SLP types.
 *
 * C99 standard sized types are cool, but not very standard yet. Some
 * platforms/compilers support stdint.h, others support inttypes.h to
 * do the same thing (even though inttypes.h is clearly supposed to do
 * something different). Some platforms (like Windows) doesn't even 
 * support the C99 standard sized types yet. In this case, we define
 * them here on an as-needed basis.
 *
 * @file       slp_types.h
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_TYPES_H_INCLUDED
#define SLP_TYPES_H_INCLUDED

/*!@defgroup CommonCode Common Code 
 * @{
 */

#ifdef _WIN32

# pragma warning(disable: 4127)  /* conditional expression is constant */
# pragma warning(disable: 4706)  /* assignment within conditional */

/* system library includes */
# define WIN32_LEAN_AND_MEAN

/*
 * With Microsoft Platform SDK build environment (SETENV.CMD)
 * established, hard-coding NT 4.0 version value is an issue
 * because current Platform SDKs cannot target NT 4.0, which
 * then causes conflict with NTDDI_VERSION (which is set to
 * whatever SDK supported version has been selected by the
 * Platform SDK / SETENV.CMD).
 */

/*#if'd this out, because it was causing problems with different
 *versions of the platform SDK
 */
#if 0
# ifndef _WIN32_WINNT
#  define _WIN32_WINNT 0x0400
# endif /* ifndef _WIN32_WINNT */

# ifndef NTDDI_VERSION
#  define NTDDI_VERSION NTDDI_WINXP
# endif /* ifndef NTDDI_VERSION */
#endif

# include <windows.h>
# include <winsock2.h>
# include <ws2tcpip.h>

/* standard library includes */
# include <stdio.h>
# include <stdlib.h>
# include <stddef.h>
# include <stdarg.h>
# include <string.h>
# include <ctype.h>
# include <math.h>
# include <errno.h>
# include <time.h>

/* inttypes.h stuff for windows */
typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

/* windows -> posix system calls */
# define sleep Sleep

/* windows -> posix stdlib calls */
# define strncasecmp _strnicmp
# define strcasecmp _stricmp
# define snprintf _snprintf
# define strdup _strdup

/* file name paths */
# define SLPD_CONFFILE  "%WINDIR%\\slp.conf"
# define SLPD_SPIFILE   "%WINDIR%\\slp.spi"
# define SLPD_REGFILE   "%WINDIR%\\slp.reg"
# define SLPD_LOGFILE   "%WINDIR%\\slpd.log"
# define SLPD_PIDFILE   "%WINDIR%\\slpd.pid"

#else	/* ! _WIN32 */

# ifdef HAVE_CONFIG_H
#  include "config.h"
# endif

# define SLP_VERSION VERSION

# if STDC_HEADERS
#  include <stdio.h>
#  include <stdlib.h>
#  include <stddef.h>
#  include <stdarg.h>
#  include <string.h>
#  include <ctype.h>
#  include <math.h>
# else   /* ! STDC_HEADERS */
#  if HAVE_STDIO_H
#   include <stdio.h>
#  endif
#  if HAVE_STDLIB_H
#   include <stdlib.h>
#  endif
#  if HAVE_STDDEF_H
#   include <stddef.h>
#  endif
#  if HAVE_STDARG_H
#   include <stdarg.h>
#  endif
#  if HAVE_STRING_H
#   include <string.h>
#  else
#   if HAVE_STRINGS_H
#    include <strings.h>
#   endif
#  endif
#  if HAVE_CTYPE_H
#   include <ctype.h>
#  endif
#  if HAVE_MATH_H
#   include <math.h>
#  endif
#  if !HAVE_STRCHR
#   define strchr index
#   define strrchr rindex
#  endif
#  if !HAVE_MEMCPY
#   define memcpy(d,s,n) bcopy((s),(d),(n))
#   define memmove(d,s,n) bcopy((s),(d),(n))
#  endif
# endif  /* ! STDC_HEADERS */

# if HAVE_UNISTD_H
#  include <unistd.h>
# endif
# if HAVE_SYS_TYPES_H
#  include <sys/types.h>
# endif
# if HAVE_SIGNAL_H
#  include <signal.h>
# endif
# if HAVE_SYS_STAT_H
#  include <sys/stat.h>
# endif
# if TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else   /* ! TIME_WITH_SYS_TIME */
#  if HAVE_SYS_TIME_H
#   include <sys/time.h>
#  else
#   include <time.h>
#  endif
# endif  /* ! TIME_WITH_SYS_TIME */
# if HAVE_ERRNO_H
#  include <errno.h>
# endif
# if HAVE_SYS_SOCKET_H
#  include <sys/socket.h>
# endif
# if HAVE_ARPA_INET_H
#  include <arpa/inet.h>
# endif
# if HAVE_FCNTL_H
#  include <fcntl.h>
# endif
# if HAVE_PTHREAD_H
#  include <pthread.h>
# endif
# if HAVE_INTTYPES_H
#  include <inttypes.h>
# else
#  if HAVE_STDINT_H
#   include <stdint.h>
#  endif
# endif
# if HAVE_STDBOOL_H
#  include <stdbool.h>
# endif
# if HAVE_PWD_H
#  include <pwd.h>
# endif
# if HAVE_GRP_H
#  include <grp.h>
# endif
# if HAVE_POLL
#  include <sys/poll.h>
# endif

# define SLPD_CONFFILE  ETCDIR "/slp.conf"
# define SLPD_REGFILE   ETCDIR "/slp.reg"
# define SLPD_SPIFILE   ETCDIR "/slp.spi"
# define SLPD_LOGFILE   VARDIR "/log/slpd.log"
# define SLPD_PIDFILE   VARDIR "/run/slpd.pid"

#ifdef __hpux
/* HP-UX defines socklen_t, but gets it wrong */
# define socklen_t int
#endif

#endif /* ! _WIN32 */

/* default libslp global configuration file */
#ifndef LIBSLP_CONFFILE
# define LIBSLP_CONFFILE SLPD_CONFFILE
#endif

/* default libslp global spi file */
#ifndef LIBSLP_SPIFILE
# define LIBSLP_SPIFILE  SLPD_SPIFILE
#endif

#ifndef MAX_PATH
# define MAX_PATH 256
#endif

/* boolean type for C99 compliance */
#ifndef bool
# define bool int
# define false 0
# define true !false
#endif

/* expand environment strings or copy them, zero terminate, return d */
#ifdef _WIN32
# define strnenv(d,s,n) (ExpandEnvironmentStrings((s),(d),(n)-1),(d))
#else
# define strnenv(d,s,n) ((strncpy((d),(s),(n)-1)[(n)-1] = 0),(d))
#endif

/*! @} */

#endif   /* SLP_TYPES_H_INCLUDED */

/*=========================================================================*/
