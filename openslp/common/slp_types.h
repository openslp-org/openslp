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

/*!@addtogroup CommonCode
 * @{
 */

#ifdef _WIN32

# pragma warning(disable: 4127)  /* conditional expression is constant */
# pragma warning(disable: 4706)  /* assignment within conditional */

# define WIN32_LEAN_AND_MEAN
# include <winsock2.h>           /* must include before windows.h */
# include <windows.h>            /* included for intptr_t */
# include <stddef.h>

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed long int32_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long uint32_t;

# define strncasecmp _strnicmp
# define strcasecmp _stricmp

#else

# include <pthread.h>
# ifdef HAVE_STDINT_H
#  include <stdint.h>
# else
#  include <inttypes.h>
# endif

#endif

#ifndef MAX_PATH
# define MAX_PATH 256
#endif

#ifndef bool
# define bool int
# define false 0
# define true !false
#endif

/*! @} */

#endif   /* SLP_TYPES_H_INCLUDED */

/*=========================================================================*/
