/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_property.c                                            */
/*                                                                         */
/* Abstract:    Defines the data structures for global SLP properties      */
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

/*=========================================================================*/
SLPDProperty G_SlpdProperty;
/*=========================================================================*/

/*-------------------------------------------------------------------------*/
int UseAllInterfaces()
/*-------------------------------------------------------------------------*/
{
    int             result = -1; /* assume error */
    struct in_addr  ifaddr;
    struct utsname  myname;
    struct hostent* myhostent;
    int             i;

    /* put in all interfaces if none specified */
    if(uname(&myname) >= 0)
    {
         myhostent = gethostbyname(myname.nodename);
         if(myhostent != 0)
         {
             if(myhostent->h_addrtype == AF_INET)
             {
                 /* count the interfaces */
                 for(i=0; myhostent->h_addr_list[i];i++);
                                
                 /* allocate memory 16 bytes per interface*/
                 G_SlpdProperty.interfaces = (char*)malloc((i * 16) + 1);
                 if(G_SlpdProperty.interfaces == 0)
                 {
                     SLPFatal("slpd is out of memory!\n");
                 }
                 *(char*)G_SlpdProperty.interfaces = 0; /* null terminate */

                 for(i=0; myhostent->h_addr_list[i];i++)
                 {
                     memcpy(&ifaddr,myhostent->h_addr_list[i],sizeof(ifaddr));
                     if(i)
                     {
                         strcat((char*)G_SlpdProperty.interfaces,",");
                     }
                     strcat((char*)G_SlpdProperty.interfaces,inet_ntoa(ifaddr));
                 }

                 result = 0;
             }
         }
    }
    
    G_SlpdProperty.interfacesLen = strlen(G_SlpdProperty.interfaces);

    return 0;
}

/*-------------------------------------------------------------------------*/
int UseDefaultScope()
/*-------------------------------------------------------------------------*/
{
    G_SlpdProperty.useScopes = (const char*)strdup("DEFAULT");
    if( G_SlpdProperty.useScopes == 0)
    {
        return -1;
    }
    return 0;
}


/*-------------------------------------------------------------------------*/
int AsBoolean(const char* str)
/*-------------------------------------------------------------------------*/
{
    if(*str == 'T' ||
       *str == 't' ||
       *str == 'Y' ||
       *str == 'y')
    {
        return 1;
    }

    return 0;
}

/*=========================================================================*/
void SLPDPropertyInit(const char* conffile)
/*=========================================================================*/
{
    
    SLPPropertyReadFile(conffile);

    G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
    if(*G_SlpdProperty.interfaces == 0)
    {
        UseAllInterfaces();
    }
    G_SlpdProperty.DAAddressesLen = strlen(G_SlpdProperty.interfaces);

    G_SlpdProperty.useScopes = SLPPropertyGet("net.slp.useScopes");
    if(*G_SlpdProperty.useScopes == 0)
    {
        UseDefaultScope();
    }
    G_SlpdProperty.useScopesLen = strlen(G_SlpdProperty.useScopes);

    G_SlpdProperty.DAAddresses = SLPPropertyGet("net.slp.DAAddresses");
    G_SlpdProperty.DAAddressesLen = strlen(G_SlpdProperty.DAAddresses);

    G_SlpdProperty.isBroadcastOnly = AsBoolean(SLPPropertyGet("net.slp.isBroadcastOnly"));               
    G_SlpdProperty.passiveDADetection = AsBoolean(SLPPropertyGet("net.slp.passiveDADetection"));               
    G_SlpdProperty.activeDADetection = AsBoolean(SLPPropertyGet("net.slp.activeDADetection"));               
    G_SlpdProperty.multicastTTL = atoi(SLPPropertyGet("net.slp.multicastTTL"));
    G_SlpdProperty.multicastMaximumWait = atoi(SLPPropertyGet("net.slp.multicastMaximumWait"));
    G_SlpdProperty.unicastMaximumWait = atoi(SLPPropertyGet("net.slp.unicastMaximumWait"));
    G_SlpdProperty.randomWaitBound = atoi(SLPPropertyGet("net.slp.randomWaitBound"));
}



