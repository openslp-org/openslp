/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_socket.c                                              */
/*                                                                         */
/* Abstract:    Socket specific functions implementation                   */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
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

#ifndef SLPD_SOCKET_H_INCLUDED
#define SLPD_SOCKET_H_INCLUDED

#include "slpd.h"


/*=========================================================================*/
/* common code includes                                                    */
/*=========================================================================*/
#include "slp_buffer.h"

/*=========================================================================*/
/* Misc constants                                                          */
/*=========================================================================*/
#define SLPD_SMALLEST_MESSAGE       18   /* 18 bytes is smallest SLPv2 msg */


/*=========================================================================*/
/* Values representing a type or state of a socket                         */
/*=========================================================================*/
#define    SOCKET_PENDING_IO       100
#define    SOCKET_LISTEN           0
#define    SOCKET_CLOSE            1
#define    DATAGRAM_UNICAST        2
#define    DATAGRAM_MULTICAST      3
#define    DATAGRAM_BROADCAST      4
#define    STREAM_CONNECT_IDLE     5
#define    STREAM_CONNECT_BLOCK    6    + SOCKET_PENDING_IO
#define    STREAM_CONNECT_CLOSE    7    + SOCKET_PENDING_IO
#define    STREAM_READ             8    + SOCKET_PENDING_IO
#define    STREAM_READ_FIRST       9    + SOCKET_PENDING_IO
#define    STREAM_WRITE            10   + SOCKET_PENDING_IO
#define    STREAM_WRITE_FIRST      11   + SOCKET_PENDING_IO
#define    STREAM_WRITE_WAIT       12   + SOCKET_PENDING_IO

#ifdef WIN32
    #define CloseSocket(Arg) closesocket(Arg)
#else
    #define CloseSocket(Arg) close(Arg)
#endif

/*=========================================================================*/
typedef struct _SLPDSocket
/* Structure representing a socket                                         */
/*=========================================================================*/
{
    SLPListItem         listitem;    
    int                 fd;
    time_t              age;    /* in seconds */
    int                 state;
    struct sockaddr_in  peeraddr;

    /* Incoming socket stuff */
    SLPBuffer           recvbuf;
    SLPBuffer           sendbuf;

    /* Outgoing socket stuff */
    int                 reconns;
    SLPList             sendlist;
}SLPDSocket;


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateConnected(struct in_addr* addr);
/*                                                                          */
/* addr - (IN) the address of the peer to connect to                        */
/*                                                                          */
/* Returns: A connected socket or a socket in the process of being connected*/
/*          if the socket was connected the SLPDSocket->state will be set   */
/*          to writable.  If the connect would block, SLPDSocket->state will*/
/*          be set to connect.  Return NULL on error                        */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateListen(struct in_addr* peeraddr);
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A listening socket. SLPDSocket->state will be set to            */
/*          SOCKET_LISTEN.   Returns NULL on error                          */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateDatagram(struct in_addr* peeraddr, int type); 
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type - (IN) the type of socket to create DATAGRAM_UNICAST,               */
/*             DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST                    */
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/


/*==========================================================================*/
SLPDSocket* SLPDSocketCreateBoundDatagram(struct in_addr* myaddr,
                                          struct in_addr* peeraddr,
                                          int type);                                                                          
/* myaddr - (IN) the address of the interface to join mcast on              */
/*                                                                          */
/* peeraddr - (IN) the address of the peer to connect to                    */
/*                                                                          */
/* type (IN) DATAGRAM_UNICAST, DATAGRAM_MULTICAST, DATAGRAM_BROADCAST       */
/*                                                                          */
/* Returns: A datagram socket SLPDSocket->state will be set to              */
/*          DATAGRAM_UNICAST, DATAGRAM_MULTICAST, or DATAGRAM_BROADCAST     */
/*==========================================================================*/


/*=========================================================================*/
SLPDSocket* SLPDSocketAlloc();
/* Allocate memory for a new SLPDSocket.                                   */
/*                                                                         */
/* Returns: pointer to a newly allocated SLPDSocket, or NULL if out of     */
/*          memory.                                                        */
/*=========================================================================*/


/*=========================================================================*/
void SLPDSocketFree(SLPDSocket* sock);
/* Frees memory associated with the specified SLPDSocket                   */
/*                                                                         */
/* sock (IN) pointer to the socket to free                                 */
/*=========================================================================*/

#endif
