/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_network.h                                              */
/*                                                                         */
/* Abstract:    Implementation for functions that are related              */
/*              network (and ipc) communication.                           */
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

#if(!defined SLP_NETWORK_H_INCLUDED)
#define SLP_NETWORK_H_INCLUDED

#ifdef __WIN32__
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#include <errno.h>
#if(_WIN32_WINNT >= 0x0400 && _WIN32_WINNT < 0x0500) 
#include <ws2tcpip.h> 
#endif
#define ETIMEDOUT 110
#define ENOTCONN  107
#else
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <arpa/inet.h> 
#include <netdb.h> 
#include <fcntl.h> 
#include <errno.h>
#endif

#include "slp_buffer.h"
#include "slp_property.h"
#include "slp_message.h"
#include "slp_xid.h"


/*=========================================================================*/ 
int SLPNetworkConnectStream(struct sockaddr_in* peeraddr,
                            struct timeval* timeout);  
/* Connect a TCP stream to the specified peer                              */
/*                                                                         */
/* peeraddr (IN) pointer to the peer to connect to                         */
/*                                                                         */
/* timeout  (IN) pointer to the maximum time to spend connecting           */
/*                                                                         */
/* returns: a connected socket or -1                                       */
/*=========================================================================*/


/*=========================================================================*/
int SLPNetworkConnectToMulticast(struct sockaddr_in* peeraddr, int ttl); 
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* ttl              (IN)  ttl for the mcast socket                         */
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/


/*=========================================================================*/
int SLPNetworkConnectToBroadcast(struct sockaddr_in* peeraddr);                                                        
/* Creates a socket and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* peeraddrlen      (IN/OUT) Size of the peeraddr structure                */
/*                                                                         */
/* Returns          Valid socket or -1 if no DA connection can be made     */
/*=========================================================================*/


/*=========================================================================*/
int SLPNetworkSendMessage(int sockfd,
                          int socktype,
                          SLPBuffer buf,
                          struct sockaddr_in* peeraddr,
                          struct timeval* timeout);  
/* Sends a message                                                         */
/*                                                                         */
/* Returns  -  zero on success non-zero on failure                         */
/*                                                                         */
/* errno         EPIPE error during write                                  */
/*               ETIME read timed out                                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPNetworkRecvMessage(int sockfd,
                          int socktype,
                          SLPBuffer* buf,
                          struct sockaddr_in* peeraddr,
                          struct timeval* timeout); 
/* Receives a message                                                      */
/*                                                                         */
/* Returns  -    zero on success, non-zero on failure                      */
/*                                                                         */
/* errno         ENOTCONN error during read                                */
/*               ETIME read timed out                                      */
/*               ENOMEM out of memory                                      */
/*               EINVAL parse error                                        */
/*=========================================================================*/

#endif
