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

/** Header file for all declarations used by slpd. 
 *
 * @file       slpd.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_H_INCLUDED
#define SLPD_H_INCLUDED

/*!@defgroup SlpdCode Service & Directory Agent */
/*!@defgroup SlpdCodeSlpd Internal */

/*!@addtogroup SlpdCodeSlpd
 * @ingroup SlpdCode
 * @{
 */

/* Misc "adjustable" constants (I would not adjust them if I were you) */

/** Maximum number tcp of reconnects to complete an outgoing transaction.
 */
#define SLPD_CONFIG_MAX_RECONN 2 
                                         
/** Maximum number of sockets.
 */
#define SLPD_MAX_SOCKETS 128

/** A "comfortable" number of sockets.
 *
 * Exceeding this number will indicate a busy.
 */
#define SLPD_COMFORT_SOCKETS 64

/** Maximum idle time (60 min) when not busy.
 */
#define SLPD_CONFIG_CLOSE_CONN 900
                                         
/** Maximum idle time (30 sec) when busy.
 */
#define SLPD_CONFIG_BUSY_CLOSE_CONN 30
                                         
/** Minimum delay between active discovery requests (15 min).
 */
#define SLPD_CONFIG_DA_FIND 900

/** Age every 15 seconds.
 */
#define SLPD_AGE_INTERVAL 15

/*! @} */

#endif   /* SLPD_H_INCLUDED */

/*=========================================================================*/
