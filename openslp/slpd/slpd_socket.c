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
SLPDSocket* SLPDSocketListAdd(SLPDSocketList* list, int fd, int type)
/* Adds a free()s the specified socket from the specified list             */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* fd       - socket file descriptor                                       */
/*                                                                         */
/* type     - socket type                                                  */
/*                                                                         */
/* Returns  - pointer to the added socket or NULL on error                 */
/*=========================================================================*/
{
    SLPDSocket* newSocket;

    newSocket = (SLPDSocket*) malloc(sizeof(SLPDSocket));
    if(newSocket)
    {
        memset(newSocket,0,sizeof(SLPDSocket));
        newSocket->fd = fd;
        newSocket->type = type;
        newSocket->recvbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        newSocket->sendbuf = SLPBufferAlloc(SLP_MAX_DATAGRAM_SIZE);
        time(&(newSocket->timestamp));
        ListLink((PListItem*)&list->head,(PListItem)newSocket);
        list->count = list->count + 1;
    }

    return newSocket;
}

/*=========================================================================*/
void SLPDSocketListRemove(SLPDSocketList* list, SLPDSocket* sock)
/* Unlinks and free()s the specified socket from the specified list        */
/*                                                                         */
/* list     - pointer to the SLPSocketList to unlink the socket from.      */
/*                                                                         */
/* sock     - pointer to the SLPSocket to unlink to the list               */
/*                                                                         */
/* Returns  - none                                                         */
/*=========================================================================*/
{
    ListUnlink((PListItem*)&list->head,(PListItem)sock);
    close(sock->fd);
    SLPBufferFree(sock->recvbuf);
    SLPBufferFree(sock->sendbuf);                        
    free(sock);
    list->count = list->count - 1;
}


/*=========================================================================*/
void SLPDSocketListRemoveAll(SLPDSocketList* list)
/* Destroys all of the sockets from the specified list                     */
/*                                                                         */
/* list     - pointer to the SLPSocketList to destroy                      */
/*                                                                         */
/* Returns  - none                                                         */
/*=========================================================================*/

{
    SLPDSocket* sock = list->head;
    SLPDSocket* del;

    /*--------------------------*/
    /* Remove all registrations */
    /*--------------------------*/
    while(sock)
    {
        del = sock;
        sock = (SLPDSocket*)sock->listitem.next;
        SLPDSocketListRemove(list, sock);
    }
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
    int             fd;
    struct in_addr  myaddr;
    struct in_addr  mcastaddr;
    
    
    SLPDSocketListRemoveAll(list);

    mcastaddr.s_addr = htonl(SLP_MCAST_ADDRESS);

    /*-----------------------------------------------------------------*/
    /* Create TCP_LISTEN socket for LOOPBACK for the library to talk to*/
    /*-----------------------------------------------------------------*/
    fd = socket(PF_INET, SOCK_STREAM, 0);
    if(fd >= 0)
    {          
        if(BindSocketToLoopback(fd) >= 0)
        {
            if(listen(fd,5) == 0)
            {
                if(SLPDSocketListAdd(list,fd,TCP_LISTEN) == 0)
                {
                    SLPFatal("Out of memory");
                }

                SLPLog("SLPLIB API socket listening\n");
            }
            else
            {
                /* could not listen(), close the socket*/
                close(fd);
                SLPLog("ERROR: Could not listen on loopback\n");
                SLPLog("ERROR: No SLPLIB support will be available\n");
            }
        }
        else
        {
            /* could not bind, close the socket*/
            close(fd);
            SLPLog("ERROR: Could not bind loopback port 427.\n");
            SLPLog("ERROR: slpd may already be running\n");
        }
    }
    else
    {
        /* could not create the socket */
        SLPLog("ERROR: Could not create socket for loopback.\n");
        SLPLog("ERROR: No SLPLIB support will be available\n");
    }

                                                                                   
    /*--------------------------------------------------------------------*/
    /* Create UDP and TCP_LISTEN sockets for all of the interfaces in the */
    /* interfaces property                                                */
    /*--------------------------------------------------------------------*/
    begin = (char*)G_SlpdProperty.interfaces;
    end = begin;
    finished = 0;
    while( finished == 0)
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
        fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(fd >=0)
        {
            if(BindSocketToInetAddr(fd, &mcastaddr) >= 0)
            {
                if(AddMulticastMembership(fd, &myaddr) == 0)
                {
                    if(SLPDSocketListAdd(list,fd,UDP_MCAST) == 0)
                    {
                        SLPFatal("Out of memory");
                    }
                    SLPLog("Multicast socket on %s ready\n",inet_ntoa(myaddr));
                }
                else
                {
                    close(fd);
                }
            }
            else
            {
                /* could not bind(), close the socket*/
                close(fd);
            }
        }

        /*--------------------------------------------*/
        /* Create socket that will handle unicast UDP */
        /*--------------------------------------------*/
        fd = socket(PF_INET, SOCK_DGRAM, 0);
        if(fd >= 0)
        {          
            if(BindSocketToInetAddr(fd, &myaddr) >= 0)
            {
                if(SLPDSocketListAdd(list,fd,UDP) == 0)
                {
                    SLPFatal("Out of memory");
                } 
                SLPLog("UDP socket on %s ready\n",inet_ntoa(myaddr));
            }
            else
            {
                /* could not bind(), close the socket*/
                close(fd);
            }
        }

        /*------------------------------------------------*/
        /* Create TCP_LISTEN that will handle unicast TCP */
        /*------------------------------------------------*/
        fd = socket(PF_INET, SOCK_STREAM, 0);
        if(fd >= 0)
        {          
            if(BindSocketToInetAddr(fd, &myaddr) >= 0)
            {
                if(listen(fd,2) == 0)
                {
                    if(SLPDSocketListAdd(list,fd,TCP_LISTEN) == 0)
                    {
                        SLPFatal("Out of memory");
                    } 
                    
                    SLPLog("TCP socket on %s listening\n",inet_ntoa(myaddr));
                }
                else
                {
                    /* could not listen(), close the socket*/
                    close( fd );
                }
            }
            else
            {
                /* could not bind, close the socket*/
                close( fd );
            }
        }
        
        begin = end + 1;
    }     
}



