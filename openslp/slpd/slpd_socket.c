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

#include "slpd.h"

/*-------------------------------------------------------------------------*/
int AddMulticastMembership(int sockfd, struct in_addr* addr)
/* Sets the socket options to receive multicast traffic from the specified */
/* interface.                                                              */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    int             result;
    struct ip_mreq	mreq;
    struct in_addr  mcast_addr;

    /* join using the reserved SLP_MCAST_ADDRESS */
    mcast_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
    mreq.imr_multiaddr = mcast_addr;
    
    /* join with specified interface */
    memcpy(&mreq.imr_interface,addr,sizeof(addr));

    
    result = setsockopt(sockfd,
                        IPPROTO_IP,
                        IP_ADD_MEMBERSHIP,
                        (char*)&mreq,
                        sizeof(mreq));               
  
    return result;
}


/*-------------------------------------------------------------------------*/
int DropMulticastMembership(int sockfd, struct in_addr* addr)
/* Sets the socket options to not receive multicast traffic from the       */
/* specified interface.                                                    */
/*                                                                         */
/* sockfd   - the socket file descriptor to set the options on.            */
/*                                                                         */
/* addr     - pointer to the multicast address                             */
/*-------------------------------------------------------------------------*/
{
    struct ip_mreq	mreq;
	struct in_addr	mcast_addr;

     /* join using the reserved SLP_MCAST_ADDRESS */
    mcast_addr.s_addr = htonl(SLP_MCAST_ADDRESS);
    mreq.imr_multiaddr = mcast_addr;
    
    /* join with specified interface */
    memcpy(&mreq.imr_interface,addr,sizeof(addr));

    return setsockopt(sockfd, IPPROTO_IP, IP_DROP_MEMBERSHIP, (char*)&mreq,sizeof(mreq));               
}


/*-------------------------------------------------------------------------*/
int BindSocketToInetAddr(int sock, struct in_addr* addr)
/* Binds the specified socket to the SLP port and interface.               */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* addr     - the in_addr to bind to.                                      */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    int                 result;
    int                 lowat;
    struct sockaddr_in	mysockaddr;
    
    mysockaddr.sin_family = AF_INET;
    mysockaddr.sin_port = htons(SLP_RESERVED_PORT);
    mysockaddr.sin_addr = *addr;
    result = bind(sock,&mysockaddr,sizeof(mysockaddr));

    /* set the receive and send buffer low water mark to 18 bytes 
    (the length of the smallest slpv2 message) */
    lowat = 18;
    setsockopt(sock,SOL_SOCKET,SO_RCVLOWAT,&lowat,sizeof(lowat));
    setsockopt(sock,SOL_SOCKET,SO_SNDLOWAT,&lowat,sizeof(lowat));
    
    return result;
}


/*-------------------------------------------------------------------------*/
int BindSocketToLoopback(int sock)
/* Binds the specified socket to the specified port of the loopback        */
/* interface.                                                              */
/*                                                                         */
/* sock     - the socket to be bound                                       */
/*                                                                         */
/* Returns  - zero on success, -1 on error.                                */
/*-------------------------------------------------------------------------*/
{
    struct in_addr  loaddr;

    loaddr.s_addr = htonl(LOOPBACK_ADDRESS);
    return BindSocketToInetAddr(sock,&loaddr);
}

/*=========================================================================*/
SLPDSocket* SLPDSocketListLink(SLPDSocketList* list, SLPDSocket* sock)
/* Links the specified socket to the specified list                        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to link the socket to.          */
/*                                                                         */
/* sock     - pointer to the SLPSocket to link to the list                 */
/*                                                                         */
/* Returns  - pointer to the linked socket                                 */
/*=========================================================================*/

{
    sock->previous = 0;
    sock->next = list->head;
    if(list->head)
    {
        list->head->previous = sock;
    }
    
    list->head = sock;
    
    list->count++;

    return sock;
}


/*=========================================================================*/
SLPDSocket* SLPDSocketListUnlink(SLPDSocketList* list, SLPDSocket* sock)
/* Unlinks the specified socket from the specified list                    */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the unlinked socket                               */
/*=========================================================================*/
{
    if(sock->previous)
    {
        sock->previous->next = sock->next;
    }

    if(sock->next)
    {
        sock->next->previous = sock->previous;
    }

    if(sock == list->head)
    {
        list->head = sock->next;
    }

    list->count--;

    return sock;
}

/*=========================================================================*/
SLPDSocket* SLPDSocketListDestroy(SLPDSocketList* list, SLPDSocket* sock)
/* Unlinks and free()s the specified socket from the specified list        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - pointer to the unlinked socket                               */
/*=========================================================================*/
{
    SLPDSocket* del = sock;
    
    sock=sock->next;

    close(del->fd);
    SLPBufferFree(del->recvbuf);
    SLPBufferFree(del->sendbuf);
    free(SLPDSocketListUnlink(list,del));
    
    return sock;
}

/*=========================================================================*/
void SLPDSocketListDestroyAll(SLPDSocketList* list)
/* Destroys all of the sockets from the specified list                     */
/*                                                                         */
/* list     - pointer to the SLPSocketList to destroy                      */
/*                                                                         */
/* Returns  - none                                                         */
/*=========================================================================*/
{
    SLPDSocket* sock = list->head;
    while(sock) 
    {
        sock = SLPDSocketListDestroy(list,sock);
    }
    
    list->head = 0;
}


/*=========================================================================*/
void SLPDSocketInit(SLPDSocketList* list)
/* Adds SLPSockets (UDP and TCP) for all the interfaces and the loopback   */
/*                                                                         */
/* list     - pointer to SLPSocketList to which the SLPSockets will be     */
/*            added                                                        */
/*                                                                         */
/* Returns  - zero on success, -1 on failure.                              */
/*=========================================================================*/
{
    char*           begin;
    char*           end;
    int             finished;
    struct in_addr  myaddr;
    struct in_addr  mcastaddr;
    SLPDSocket*      slpsocket;
    
    SLPDSocketListDestroyAll(list);

    mcastaddr.s_addr = htonl(SLP_MCAST_ADDRESS);

    /*-----------------------------------------------------------------*/
    /* Create TCP_LISTEN socket for LOOPBACK for the library to talk to*/
    /*-----------------------------------------------------------------*/
    slpsocket = (SLPDSocket*) malloc(sizeof(SLPDSocket));
    if(slpsocket == 0)
    {
        /* out of memory */
        errno = ENOMEM;
        SLPFatal("Out of memory\n");
    }
    memset(slpsocket,0,sizeof(SLPDSocket));
    slpsocket->fd = socket(PF_INET, SOCK_STREAM, 0);
    if(slpsocket->fd >= 0)
    {          
        if(BindSocketToLoopback(slpsocket->fd) >= 0)
        {
            if(listen(slpsocket->fd,5) == 0)
            {
                slpsocket->type = TCP_LISTEN;
                SLPDSocketListLink(list,slpsocket);
                SLPLog("SLPLIB API socket listening\n");
            }
            else
            {
                /* could not listen(), close the socket*/
                close(slpsocket->fd);
                free(slpsocket);                
                SLPError("Could not listen on loopback\n");
                SLPError("No SLPLIB support will be available\n");
            }
        }
        else
        {
            /* could not bind, close the socket*/
            close(slpsocket->fd);
            free(slpsocket);
            SLPError("Could not bind loopback port 427.\n");
            SLPError("slpd may already be running\n");
        }
    }
    else
    {
        /* could not create the socket */
        SLPError("Could not create socket for loopback.\n");
        SLPError("No SLPLIB support will be available\n");
        free(slpsocket);
    }

                                                                                   
    /*--------------------------------------------------------------------*/
    /* Create UDP and TCP_LISTEN sockets for all of the interfaces in the */
    /* interfaces property                                                */
    /*--------------------------------------------------------------------*/
    begin = (char*)G_SlpdProperty.interfaces;
    end = begin;
    finished = 0;
    while(finished == 0)
    {
        while(*end && *end != ',') end ++;
        if(*end == 0) finished = 1;
        while(*end <=0x2f) 
        {
            *end = 0;
            end--;
        }

        /* begin now points to a null terminated ip address string */
        myaddr.s_addr = inet_addr(begin);

        /*----------------------------------------------*/
        /* Create socket that will handle multicast UDP */
        /*----------------------------------------------*/
        slpsocket = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(slpsocket == 0)
        {
            /* out of memory */
            SLPFatal("Out of memory\n");
        }
        slpsocket->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(BindSocketToInetAddr(slpsocket->fd, &mcastaddr) >= 0)
        {
            if(AddMulticastMembership(slpsocket->fd, &myaddr) == 0)
            {
                slpsocket->type = UDP_MCAST;
                slpsocket->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                slpsocket->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                SLPDSocketListLink(list,slpsocket);
                SLPLog("Multicast socket on %s ready\n",inet_ntoa(myaddr));
            }
        }
        else
        {
            /* could not bind(), close the socket*/
            close(slpsocket->fd);
            free(slpsocket);
        }

        /*--------------------------------------------*/
        /* Create socket that will handle unicast UDP */
        /*--------------------------------------------*/
        slpsocket = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(slpsocket == 0)
        {
            /* out of memory */
            /* out of memory */
            SLPFatal("Out of memory\n");
        }
        memset(slpsocket,0,sizeof(SLPDSocket)); 
        slpsocket->fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(slpsocket->fd >= 0)
        {          
            if(BindSocketToInetAddr(slpsocket->fd, &myaddr) >= 0)
            {
                slpsocket->type = UDP; 
                slpsocket->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                slpsocket->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
                SLPDSocketListLink(list,slpsocket);
                SLPLog("UDP socket on %s ready\n",inet_ntoa(myaddr));
            }
            else
            {
                /* could not bind(), close the socket*/
                close(slpsocket->fd);
                free(slpsocket);
            }
        }
        else
        {
            /* could not create the socket */
            free(slpsocket);
        }

        /*------------------------------------------------*/
        /* Create TCP_LISTEN that will handle unicast TCP */
        /*------------------------------------------------*/
        slpsocket = (SLPDSocket*)malloc(sizeof(SLPDSocket));
        if(slpsocket == 0)
        {
            /* out of memory */
            SLPFatal("Out of memory\n");
        }
        memset(slpsocket,0,sizeof(SLPDSocket));
        slpsocket->fd = socket(PF_INET, SOCK_STREAM, 0);
        if(slpsocket->fd >= 0)
        {          
            if(BindSocketToInetAddr(slpsocket->fd, &myaddr) >= 0)
            {
                if(listen(slpsocket->fd,2) == 0)
                {
                    slpsocket->type = TCP_LISTEN;
                    SLPDSocketListLink(list,slpsocket);
                    SLPLog("TCP socket on %s listening\n",inet_ntoa(myaddr));
                }
                else
                {
                    /* could not listen(), close the socket*/
                    close(slpsocket->fd);
                    free(slpsocket);                
                }
            }
            else
            {
                /* could not bind, close the socket*/
                close(slpsocket->fd);
                free(slpsocket);
            }
        }
        else
        {
            /* could not create the socket */
            free(slpsocket);
        }   

        begin = end + 1;
    }     
}



