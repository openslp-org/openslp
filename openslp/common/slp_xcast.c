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


#include "slp_xcast.h"
#include "slp_message.h"


/*========================================================================*/
int SLPBroadcastSend(const SLPInterfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks)
/* Description:
 *    Broadcast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPInterfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 *   socks      (OUT) Sockets used broadcast multicast.  May be used to 
 *                    recv() responses.  MUST be close by caller using 
 *                    SLPXcastSocketsClose() 
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/
{
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

    for (socks->sock_count = 0; 
         socks->sock_count < ifaceinfo->iface_count; 
         socks->sock_count++)
    {
        socks->sock[socks->sock_count] = socket(AF_INET, SOCK_DGRAM, 0);
        if (socks->sock[socks->sock_count] < 0)
        {
            /* error creating socket */
            return -1;
        }
        
        if( (setsockopt(socks->sock[socks->sock_count],
                        SOL_SOCKET, 
                        SO_BROADCAST, 
                        &on, 
                        sizeof(on))) )
        {
            /* Error setting socket option */
            return -1;
        }

        socks->peeraddr[socks->sock_count].sin_family = AF_INET;
        socks->peeraddr[socks->sock_count].sin_port = htons(SLP_RESERVED_PORT);
        socks->peeraddr[socks->sock_count].sin_addr.s_addr = ifaceinfo->bcast_addr[socks->sock_count].sin_addr.s_addr;

        xferbytes = sendto(socks->sock[socks->sock_count], 
                           msg->start,
                           msg->end - msg->start,
                           0,
                           (struct sockaddr *) &(socks->peeraddr[socks->sock_count]),
                           sizeof(struct sockaddr_in));
        if(xferbytes  < 0)
        { 
            /* Error sending to broadcast */
            return -1;
        }
    }  

    return 0;
}


/*========================================================================*/
int SLPMulticastSend(const SLPInterfaceInfo* ifaceinfo, 
                     SLPBuffer msg,
                     SLPXcastSockets* socks)
/* Description:
 *    Multicast a message.
 *
 * Parameters:
 *    ifaceinfo (IN) Pointer to the SLPInterfaceInfo structure that contains
 *                   information about the interfaces to send on
 *    msg       (IN) Buffer to send
 *
 *   socks      (OUT) Sockets used to multicast.  May be used to recv() 
 *                    responses.  MUST be close by caller using 
 *                    SLPXcastSocketsClose() 
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/
{
    int             flags = 0;
    int             xferbytes;
    struct in_addr  saddr;


#if defined(MSG_NOSIGNAL)
    flags = MSG_NOSIGNAL;
#endif

    for (socks->sock_count = 0;
         socks->sock_count < ifaceinfo->iface_count;
         socks->sock_count++)
    {
        socks->sock[socks->sock_count] = socket(AF_INET, SOCK_DGRAM, 0);
        if (socks->sock[socks->sock_count] < 0)
        {
            /* error creating socket */
            return -1;
        }
        
        saddr.s_addr = ifaceinfo->iface_addr[socks->sock_count].sin_addr.s_addr;
        if( setsockopt(socks->sock[socks->sock_count], 
                       IPPROTO_IP, 
                       IP_MULTICAST_IF, 
                       &saddr, 
                       sizeof(struct in_addr)))
        {
            /* error setting socket option */
            return -1;
        }

        socks->peeraddr[socks->sock_count].sin_family = AF_INET;
        socks->peeraddr[socks->sock_count].sin_port = htons(SLP_RESERVED_PORT);
        socks->peeraddr[socks->sock_count].sin_addr.s_addr = htonl(SLP_MCAST_ADDRESS);

        xferbytes = sendto(socks->sock[socks->sock_count],
                           msg->start,
                           msg->end - msg->start,
                           flags,
                           (struct sockaddr *) &(socks->peeraddr[socks->sock_count]),
                           sizeof(struct sockaddr_in));
        if (xferbytes <= 0)
        {
            /* error sending */
            return -1;
        }
    }
    
    return 0;

}


/*========================================================================*/
int SLPXcastSocketsClose(SLPXcastSockets* socks)
/* Description:
 *    Closes sockets that were opened by calls to SLPMulticastSend() and
 *    SLPBroadcastSend()
 *
 * Parameters:
 *    socks (IN) Pointer to the SLPXcastSockets structure being close
 *
 * Returns:
 *    Zero on sucess.  Non-zero with errno set on error
 *========================================================================*/
{
    while(socks->sock_count)
    {
        socks->sock_count = socks->sock_count - 1;
        close(socks->sock[socks->sock_count]);
    }

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
    SLPXcastSockets      socks;
    SLPBuffer           buffer;

    buffer = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
    if(buffer)
    {
        
        strcpy(buffer->start,"testdata");
    
        SLPInterfaceGetInformation(NULL,&ifaceinfo);
    
        if (SLPBroadcastSend(&ifaceinfo, buffer,&socks) !=0)
            printf("\n SLPBroadcastSend failed \n");
        SLPXcastSocketsClose(&socks);
    
        if (SLPMulticastSend(&ifaceinfo, buffer, &socks) !=0)
            printf("\n SLPMulticast failed \n");
        SLPXcastSocketsClose(&socks);

        printf("Success\n");

        SLPBufferFree(buffer);
    }
}

#endif

