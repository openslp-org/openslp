/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        libslp_network.c                                           */
/*                                                                         */
/* Abstract:    Implementation for functions that are related to INTERNAL  */
/*              library network (and ipc) communication.                   */
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

#include "slp.h"
#include "libslp.h"

/*=========================================================================*/ 
SLPDAEntry* G_KnownDAListHead = 0;
time_t G_LastMulticast = 0;
/*=========================================================================*/ 


/*=========================================================================*/ 
int NetworkConnectToDA(const char* scopelist,
                       int scopelistlen,
                       struct sockaddr_in* peeraddr)
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* scopelist        (IN) Scope that must be supported by DA. Pass in NULL  */
/*                       for any scope                                     */
/*                                                                         */
/* scopelistlen     (IN) Length of the scope list in bytes.  Ignored if    */
/*                       scopelist is NULL                                 */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    struct timeval      timeout;
    time_t              curtime;
    int                 interval;
    int                 result  = -1;
    SLPDAEntry*         entry   = 0;

    timeout.tv_sec = atoi(SLPGetProperty("net.slp.multicastMaximumWait"));
    timeout.tv_usec = timeout.tv_sec % 1000;
    timeout.tv_sec = timeout.tv_sec / 1000;
    interval = atoi(SLPGetProperty("net.slp.DAActiveDiscoveryInterval"));

    memset(peeraddr,0,sizeof(struct sockaddr_in));
    peeraddr->sin_family = AF_INET;
    peeraddr->sin_port = htons(SLP_RESERVED_PORT);

    entry = G_KnownDAListHead;
    while(entry)
    {
        peeraddr->sin_addr = entry->daaddr;
        result = SLPNetworkConnectStream(peeraddr,&timeout);
        if(result >= 0)
        {
            /* hurray we connected */
            break; 
        }
        else
        {
            /* remove DAs that we can't connect to */
            ListUnlink((PListItem*)&G_KnownDAListHead,(PListItem)entry);
            SLPDAEntryFree(entry);
        }
    }

    if(result < 0)
    {
        time(&curtime);
        if(interval && curtime - G_LastMulticast > interval) 
        {
            G_LastMulticast = curtime;
            if(SLPDiscoverDAs(&G_KnownDAListHead,
                              scopelistlen,
                              scopelist,
                              0x00000003,
                              SLPGetProperty("net.slp.DAAddresses"),
                              &timeout))
            {
                entry = G_KnownDAListHead;
                while(entry)
                {
                    peeraddr->sin_addr = entry->daaddr;
                    result = SLPNetworkConnectStream(peeraddr,&timeout);
                    if(result >= 0)
                    {
                        /* hurray we connected */
                        break; 
                    }
                    else
                    {
                        /* remove DAs that we can't connect to */
                        ListUnlink((PListItem*)&G_KnownDAListHead,(PListItem)entry);
                        SLPDAEntryFree(entry);
                    }
                }
            }
        }
    }

    return result;
}


/*=========================================================================*/ 
int NetworkConnectToSlpd(struct sockaddr_in* peeraddr)       
/* Connects to slpd and provides a peeraddr to send to                     */
/*                                                                         */
/* peeraddr         (OUT) pointer to receive the connected DA's address    */
/*                                                                         */
/* Returns          Connected socket or -1 if no DA connection can be made */
/*=========================================================================*/
{
    int lowat;
    int result;
     
    result = socket(AF_INET,SOCK_STREAM,0);
    if(result >= 0)
    {
        peeraddr->sin_family      = AF_INET;
        peeraddr->sin_port        = htons(SLP_RESERVED_PORT);
        peeraddr->sin_addr.s_addr = htonl(LOOPBACK_ADDRESS);
        if(connect(result,
                   (struct sockaddr*)peeraddr,
                   sizeof(struct sockaddr_in)) == 0)
        {
             /* set the receive and send buffer low water mark to 18 bytes 
            (the length of the smallest slpv2 message) */
            lowat = 18;
            setsockopt(result,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
            setsockopt(result,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
        }
        else
        {
              /* Could not connect to the slpd through the loopback */
              close(result);
        }
    }    
    
    return result;
}

