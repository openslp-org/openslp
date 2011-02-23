/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
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

/** Header file OpenSLP command line User Agent wrapper.
 *
 * @file       slptool.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpToolCode
 */

#ifndef SLPTOOL_H_INCLUDED
#define SLPTOOL_H_INCLUDED

/*!@defgroup SlpToolCode Command Line Tool */

/*!@addtogroup SlpToolCode
 * @{
 */

#include <slp.h>

#ifdef _WIN32

# include <string.h>
# include <stdio.h>
# include <stdlib.h>

# define strncasecmp _strnicmp
# define strcasecmp _stricmp
# define inet_aton(opt, bind) ((bind)->s_addr = inet_addr(opt))
# define strdup _strdup

#else /* ! _WIN32 */

# if HAVE_CONFIG_H
#  include "config.h"
# endif
# if HAVE_STDIO_H
#  include <stdio.h>
# endif
# if HAVE_STDLIB_H
#  include <stdlib.h>
# endif
# if HAVE_STRING_H
#  include <string.h>
# else
#  if HAVE_STRINGS_H
#   include <strings.h>
#  endif
# endif

# define SLP_VERSION VERSION

#endif /* ! _WIN32 */

typedef enum _SLPToolCommand
{
   FINDSRVS = 1,
   FINDATTRS,
   FINDSRVTYPES,
   FINDSCOPES,
   GETPROPERTY,
   REGISTER,
   DEREGISTER,
   PRINT_VERSION,
   DUMMY
} SLPToolCommand;

typedef struct _SLPToolCommandLine
{
   SLPToolCommand cmd;
   const char * lang;
   const char * time;
   const char * interfaces;
   const char * unicastifc;
   const char * scopes;
   const char * cmdparam1;
   const char * cmdparam2;
   const char * cmdparam3;
} SLPToolCommandLine;

void FindSrvs(SLPToolCommandLine * cmdline);
void FindAttrs(SLPToolCommandLine * cmdline);
void FindSrvTypes(SLPToolCommandLine * cmdline);
void GetProperty(SLPToolCommandLine * cmdline);
void Register(SLPToolCommandLine * cmdline);
void Deregister(SLPToolCommandLine * cmdline);

/*! @} */

#endif   /* SLPTOOL_H_INCLUDED */

/*=========================================================================*/
