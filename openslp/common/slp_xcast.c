/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_xcast.c                                                */
/*                                                                         */
/* Abstract:    Functions used to multicast and broadcast SLP messages     */
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


#include "slp_buffer.h"
#include "slp_iface.h"
#include "slp_xcast.h"
#include "slp_message.h"


/*========================================================================*/
int SLPBroadcastSend(SLPInterfaceInfo *ifaceinfo, SLPBuffer msg)
/*========================================================================*/
{
    struct sockaddr_in  peeraddr;
    int                 sockfd;
    int                 iface_index;
    int                 xferbytes;
    int                 flags = 0;  

#ifdef WIN32
    BOOL    on = 1;
#else
    int     on = 1;
#endif


#if defined(MSG_NOSIGNAL)
    flags = MSG_NOSIGNAL;
#endif

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        return -1;
    }
        
    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(SLP_RESERVED_PORT);
    if (setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)))
    {
        return -1;
    }
    
    for (iface_index=0; iface_index < ifaceinfo->iface_count; iface_index++)
    {
        peeraddr.sin_addr.s_addr = ifaceinfo->bcast_addr[iface_index].sin_addr.s_addr;

        xferbytes = sendto(sockfd, 
                           msg->start,
                           msg->end - msg->start,
                           0,
                           (struct sockaddr *) &peeraddr,
                           sizeof(struct sockaddr_in));
        if(xferbytes  < 0)
        { dl380s2.
            /* Error sending to broadcast */
            return -1;
        }
    }  


    return 0;
}

/*========================================================================*/
int SLPMulticastSend(SLPInterfaceInfo *ifaceinfo, SLPBuffer msg)
/*========================================================================*/
{

    struct sockaddr_in  peeraddr;
    int                 sockfd;
    int                 xferbytes;
    int                 iface_index;
    struct SLPBuffer*   buf;
    struct in_addr      saddr;
    int                 flags = 0;

#if defined(MSG_NOSIGNAL)
    flags = MSG_NOSIGNAL;
#endif

    peeraddr.sin_family = AF_INET;
    peeraddr.sin_port = htons(SLP_RESERVED_PORT);
    peeraddr.sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);

    sockfd = socket(AF_INET,SOCK_DGRAM,0);
    if(sockfd < 0)
    {
        return -1;
    }

    for (iface_index=0;iface_index<ifaceinfo->iface_count;iface_index++)
    {
        saddr.s_addr = ifaceinfo->iface_addr[iface_index].sin_addr.s_addr;
        
        if( setsockopt(sockfd, 
                       IPPROTO_IP, 
                       IP_MULTICAST_IF, 
                       &saddr, 
                       sizeof(struct in_addr)))
        {
            return -1;
        }

        xferbytes = sendto(sockfd,
                           msg->start,
                           msg->end - msg->start,
                           flags,
                           (struct sockaddr *)&peeraddr,
                           sizeof(struct sockaddr_in));
        if (xferbytes <= 0)
        {
            return -1;
        }
    }

    close(sockfd);

    return 0;

}

/*===========================================================================
 * TESTING CODE may be compiling with the following command line:
 *
 * $ gcc -g -DDEBUG -DSLP_XMIT_TEST slp_xcast.c slp_iface.c slp_buffer.c 
 *   slp_linkedlist.c slp_compare.c slp_xmalloc.c
 *==========================================================================*/ 
#ifdef SLP_XMIT_TEST
main()
{
    SLPInterfaceInfo    ifaceinfo;
    SLPBuffer           buffer;

    buffer = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
    if(buffer)
    {
        
        strcpy(buffer->start,"testdata");
    
        SLPInterfaceGetInformation(NULL,&ifaceinfo);
    
        if (SLPBroadcastSend(&ifaceinfo, buffer) !=0)
            printf("\n SLPBroadcastSend failed \n");
    
        if (SLPMulticastSend(&ifaceinfo, buffer) !=0)
            printf("\n SLPMulticast failed \n");
    
        printf("Success\n");

        SLPBufferFree(buffer);
    }
}

#endif

