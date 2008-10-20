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

/** Includes platform-specific sockets headers.
 *
 * This header file isolates differences between sockets implementations. 
 * The most significant of these is Win32 vs. Unix. However there are also
 * minor differences between BSD sockets implementations among various Unix
 * flavors.
 *
 * @file       slp_socket.h
 * @author     John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_SOCKET_H_INCLUDED
#define SLP_SOCKET_H_INCLUDED

/*!@addtogroup CommonCode
 * @{
 */

#include "slp_types.h"

#ifdef _WIN32

# pragma warning(disable: 4127)  /* conditional expression is constant */
# pragma warning(disable: 4706)  /* assignment within conditional */
# define WIN32_LEAN_AND_MEAN
# include <winsock2.h>
# include <ws2tcpip.h>
# include <iptypes.h>
# define so_bool_t char
# define sockfd_t SOCKET
# define ssize_t int
# define inet_aton(opt, bind) ((bind)->s_addr = inet_addr(opt))
# define SLP_INVALID_SOCKET INVALID_SOCKET
# define ETIMEDOUT 110
# define ENOTCONN  107
# define EAFNOSUPPORT -1
# define inet_aton(opt, bind) ((bind)->s_addr = inet_addr(opt))

/*
 * Windows Vista SDK and later provide inet_pton and inet_ntop.
 * For now, continue using OpenSLP internal versions on all
 * Windows platforms.
 */

# if ( NTDDI_VERSION >= NTDDI_LONGHORN )
#define inet_pton openslp_inet_pton
#define inet_ntop openslp_inet_ntop
# endif /* if ( NTDDI_VERSION >= NTDDI_LONGHORN ) */

int inet_pton(int af, const char * src, void * dst);
const char * inet_ntop(int af, const void * src, char * dst, size_t size);

#else

/* @todo: This was copied from openslp 1.2.1 for solaris support. Is it needed?*/
#ifdef SOLARIS
#include <sys/sockio.h>
#endif

# include <sys/types.h>
# include <sys/socket.h>
# include <sys/ioctl.h>
# include <net/if.h>
# include <net/if_arp.h>
# include <netdb.h>

/** Portability definitions
 * @todo Move to slp_types.h
 */
# define so_bool_t int
# define sockfd_t int
# define SLP_INVALID_SOCKET -1
# define closesocket close

#endif

/*! @} */

#endif   /* SLP_SOCKET_H_INCLUDED */

/*=========================================================================*/
