/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_outgoing.h                                            */
/*                                                                         */
/* Abstract:    Handles "outgoing" network conversations requests made by  */
/*              other agents to slpd. (slpd_incoming.c handles reqests     */
/*              made by other agents to slpd)                              */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/* Copyright (C) 2000 Caldera Systems, Inc                                 */
/* All rights reserved.                                                    */
/*                                                                         */
/* Redistribution and use in source and binary forms, with or without      */
/* modification, are permitted provided that the following conditions are  */
/* met:                                                                    */ 
/*                                                                         */
/*      Redistributions of source code must retain the above copyright     */
/*      notice, this list of conditions and the following disclaimer.      */
/*                                                                         */
/*      Redistributions in binary form must reproduce the above copyright  */
/*      notice, this list of conditions and the following disclaimer in    */
/*      the documentation and/or other materials provided with the         */
/*      distribution.                                                      */
/*                                                                         */
/*      Neither the name of Caldera Systems nor the names of its           */
/*      contributors may be used to endorse or promote products derived    */
/*      from this software without specific prior written permission.      */
/*                                                                         */
/* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS     */
/* `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT      */
/* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR   */
/* A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA      */
/* SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, */
/* SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT        */
/* LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,  */
/* DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON       */
/* ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT */
/* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE   */
/* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    */
/*                                                                         */
/***************************************************************************/

#ifndef SLPD_OUTGOING_H_INCLUDED


#include "slpd.h"

/*=========================================================================*/
/* slpd includes                                                           */
/*=========================================================================*/
#include "slpd_socket.h"


/*=========================================================================*/
extern SLPList G_OutgoingSocketList;
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingAge(time_t seconds);
/* Age the sockets in the outgoing list by the specified number of seconds.*/
/*                                                                         */
/* seconds (IN) seconds to age each entry of the list                      */
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingHandler(int* fdcount,
                         fd_set* readfds,
                         fd_set* writefds);

/* Handles all incoming requests that are pending on the specified file    */
/* discriptors                                                             */
/*                                                                         */
/* fdcount  (IN/OUT) number of file descriptors marked in fd_sets          */
/*                                                                         */
/* readfds  (IN) file descriptors with pending read IO                     */
/*                                                                         */
/* writefds  (IN) file descriptors with pending read IO                    */
/*=========================================================================*/


/*=========================================================================*/
void SLPDOutgoingDatagramWrite(SLPDSocket* sock);
/* Add a ready to write outgoing datagram socket to the outgoing list.     */
/* The datagram will be written then sit in the list until it ages out     */
/* (after  net.slp.unicastMaximumWait)                                     */
/*                                                                         */
/* sock (IN) the socket that will belong on the outgoing list              */
/*=========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDOutgoingConnect(struct in_addr* addr);
/* Get a pointer to a connected socket that is associated with the         */
/* outgoing socket list.  If a connected socket already exists on the      */
/* outgoing list, a pointer to it is returned, otherwise a new connection  */
/* is made and added to the outgoing list                                  */
/*                                                                         */
/* addr (IN) the address of the peer a connection is desired for           */
/*                                                                         */
/* returns: pointer to socket or null on error                             */
/*=========================================================================*/


/*=========================================================================*/
int SLPDOutgoingInit();
/* Initialize outgoing socket list to have appropriate sockets for all     */
/* network interfaces                                                      */
/*                                                                         */
/* Returns  Zero on success non-zero on error                              */
/*=========================================================================*/


/*=========================================================================*/
int SLPDOutgoingDeinit(int graceful);
/* Deinitialize incoming socket list to have appropriate sockets for all   */
/* network interfaces                                                      */
/*                                                                         */
/* graceful (IN) Do not close sockets with pending writes                  */
/*                                                                         */
/* Returns  Zero on success non-zero when pending writes remain            */
/*=========================================================================*/

/*=========================================================================*/
void SLPDOutgoingSocketDump();
/*=========================================================================*/


#endif
