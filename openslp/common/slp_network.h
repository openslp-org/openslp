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
/* Copyright (c) 1995, 1999  Caldera Systems, Inc.                         */
/*                                                                         */
/* This program is free software; you can redistribute it and/or modify it */
/* under the terms of the GNU Lesser General Public License as published   */
/* by the Free Software Foundation; either version 2.1 of the License, or  */
/* (at your option) any later version.                                     */
/*                                                                         */
/*     This program is distributed in the hope that it will be useful,     */
/*     but WITHOUT ANY WARRANTY; without even the implied warranty of      */
/*     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       */
/*     GNU Lesser General Public License for more details.                 */
/*                                                                         */
/*     You should have received a copy of the GNU Lesser General Public    */
/*     License along with this program; see the file COPYING.  If not,     */
/*     please obtain a copy from http://www.gnu.org/copyleft/lesser.html   */
/*                                                                         */
/*-------------------------------------------------------------------------*/
/*                                                                         */
/*     Please submit patches to http://www.openslp.org                     */
/*                                                                         */
/***************************************************************************/

#if(!defined SLP_NETWORK_H_INCLUDED)
#define SLP_NETWORK_H_INCLUDED

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


#include <slp_buffer.h>
#include <slp_property.h>
#include <slp_message.h>

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
                          SLPBuffer buf,
                          struct timeval* timeout,
                          struct sockaddr_in* peeraddr);
/* Sends a message                                                         */
/*                                                                         */
/* Returns  -  zero on success non-zero on failure                         */
/*                                                                         */
/* errno         EPIPE error during write                                  */
/*               ETIME read timed out                                      */
/*=========================================================================*/ 


/*=========================================================================*/ 
int SLPNetworkRecvMessage(int sockfd,
                          SLPBuffer buf,
                          struct timeval* timeout,
                          struct sockaddr_in* peeraddr);
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
