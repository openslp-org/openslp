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


/*=========================================================================*/
void SLPDPropertyInit(const char* conffile)
/*=========================================================================*/
{
    struct in_addr  ifaddr;
    struct utsname  myname;
    struct hostent* myhostent;
    int             i;

    SLPPropertyReadFile(conffile);
   
    memset(&G_SlpdProperty,0,sizeof(G_SlpdProperty));
   
    /*-------------------------------------------------------------*/
    /* Set the properties with out hard defaults                   */
    /*-------------------------------------------------------------*/
    G_SlpdProperty.isBroadcastOnly = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isBroadcastOnly"));
    G_SlpdProperty.passiveDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.passiveDADetection"));               
    G_SlpdProperty.activeDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.activeDADetection"));               
    G_SlpdProperty.activeDiscoveryAttempts = G_SlpdProperty.activeDADetection * 3;
    G_SlpdProperty.multicastTTL = atoi(SLPPropertyGet("net.slp.multicastTTL"));
    G_SlpdProperty.multicastMaximumWait = atoi(SLPPropertyGet("net.slp.multicastMaximumWait"));
    G_SlpdProperty.unicastMaximumWait = atoi(SLPPropertyGet("net.slp.unicastMaximumWait"));
    G_SlpdProperty.randomWaitBound = atoi(SLPPropertyGet("net.slp.randomWaitBound"));
    G_SlpdProperty.traceMsg = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceMsg"));
    G_SlpdProperty.traceReg = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceReg"));
    G_SlpdProperty.traceDrop = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceDrop"));
    G_SlpdProperty.traceDATraffic = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceDATraffic"));
    G_SlpdProperty.DAAddresses = SLPPropertyGet("net.slp.DAAddresses");
    G_SlpdProperty.DAAddressesLen = strlen(G_SlpdProperty.DAAddresses);
    G_SlpdProperty.useScopes = SLPPropertyGet("net.slp.useScopes");
    G_SlpdProperty.useScopesLen = strlen(G_SlpdProperty.useScopes);
    G_SlpdProperty.isDA = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isDA"));
    G_SlpdProperty.locale = SLPPropertyGet("net.slp.locale");
    G_SlpdProperty.localeLen = strlen(G_SlpdProperty.locale);
    




    /*-------------------------------------*/
    /* Set the net.slp.interfaces property */
    /*-------------------------------------*/
    G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
    if(*G_SlpdProperty.interfaces == 0)
    {
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
		        }
            }
        }
    }
    if(G_SlpdProperty.interfaces)
    {
        G_SlpdProperty.interfacesLen = strlen(G_SlpdProperty.interfaces);
    }
   
    /*---------------------------------------------------------*/
    /* Set the value used internally as the url for this agent */
    /*---------------------------------------------------------*/      
    G_SlpdProperty.myUrl = (const char*)malloc(25 + strlen(myname.nodename));
    if(G_SlpdProperty.myUrl == 0)
    {
       SLPFatal("slpd is out of memory!\n");
    }
    if(G_SlpdProperty.isDA)
    {
        strcpy((char*)G_SlpdProperty.myUrl,"service:directory-agent://");
    }
    else
    {
        strcpy((char*)G_SlpdProperty.myUrl,"service:service-agent://");
    }
    if(uname(&myname) >= 0)
    {
        /* 25 is the length of "service:directory-agent://" */
    	strcat((char*)G_SlpdProperty.myUrl,myname.nodename);
    }   
    G_SlpdProperty.myUrlLen = strlen(G_SlpdProperty.myUrl);
}



