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

#include "slpd.h"

/*=========================================================================*/
SLPDProperty G_SlpdProperty;
/*=========================================================================*/


/*-------------------------------------------------------------------------*/
char* GetHostname()
/* Returns a string represting this host (the FDN) or null. Caller must    */
/* free returned string                                                    */
/*-------------------------------------------------------------------------*/
{
    /* TODO find a better constant for MAX_HOSTNAME */
#define MAX_HOST_NAME 256

    char            host[MAX_HOST_NAME];
    char*           hostfdn = 0;
    struct hostent* he;

    if(gethostname(host, MAX_HOST_NAME) == 0)
    {
        he = gethostbyname(host);
        if(he)
        {
            hostfdn = strdup(he->h_name);
        }
    }

    return hostfdn;
}


/*-------------------------------------------------------------------------*/
char* GetInterfaceList()
/* Returns a comma delimited string of interface addresses or null. Caller */
/* is responsible for freeing buffer returned buffer                       */
/*-------------------------------------------------------------------------*/
{
    char*               result = 0;

#ifdef LINUX
    /*------------------------------------------------------------------*/
    /* Use Linux ioctl. We will return only list of up to 10 interfaces */
    /*------------------------------------------------------------------*/
#define MAX_INTERFACE 10
    struct sockaddr     *sa;
    struct sockaddr_in  *sin;
    struct ifreq        iflist[MAX_INTERFACE];
    struct ifconf       ifc;
    int                 i;
    int                 fd;

    ifc.ifc_len = sizeof(struct ifreq) * 10;
    ifc.ifc_req = iflist;
    fd = socket(PF_INET,SOCK_STREAM,0);
    if(ioctl(fd,SIOCGIFCONF,&ifc) == 0)
    {
        /* allocate 16 bytes per interface */
        result = (char*)malloc((MAX_INTERFACE * 16) + 1);
        if(result)
        {
            result[0] = 0; /* null terminate for strcat */
            for(i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++)
            {
                sa = (struct sockaddr *)&(iflist[i].ifr_addr);
                if(sa->sa_family == AF_INET)
                {
                    sin = (struct sockaddr_in*) sa;
                    /* exclude localhost */
                    if((ntohl(sin->sin_addr.s_addr) & 0xff000000) != 0x7f000000)
                    {
                        if(*result)
                        {
                            strcat(result,",");
                        }
                        strcat(result,inet_ntoa(sin->sin_addr)); 
                    }
                }
            }
        }
    }
#else
    /*---------------------------------------------------*/
    /* Use gethostbyname(). Not necessarily the best way */
    /*---------------------------------------------------*/
    struct hostent* myhostent;
    char* myname;
    struct in_addr ifaddr;
    int i;

    myname = GetHostname();
    if(myname)
    {
        myhostent = gethostbyname(myname);
        if(myhostent != 0)
        {
            if(myhostent->h_addrtype == AF_INET)
            {
                /* count the interfaces */
                for(i=0; myhostent->h_addr_list[i];i++);

                /* allocate memory 16 bytes per interface*/
                result = (char*)malloc((i * 16) + 1);
                if(result)
                {
                    result[0] = 0; /* null terminate */
                    for(i=0; myhostent->h_addr_list[i];i++)
                    {
                        memcpy(&ifaddr,myhostent->h_addr_list[i],sizeof(ifaddr));
                        if(*result)
                        {
                            strcat(result,",");
                        }
                        strcat(result,inet_ntoa(ifaddr));
                    }    
                }
            }

            free(myname);    
        }
    }
#endif

    return result;
}


/*=========================================================================*/
int SLPDPropertyInit(const char* conffile)
/*=========================================================================*/
{
    char* myname = 0;

    SLPPropertyReadFile(conffile);

    memset(&G_SlpdProperty,0,sizeof(G_SlpdProperty));

    /*-------------------------------------------------------------*/
    /* Set the properties with out hard defaults                   */
    /*-------------------------------------------------------------*/
    G_SlpdProperty.isDA = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isDA"));
    G_SlpdProperty.activeDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.activeDADetection"));               
    if(G_SlpdProperty.isDA)
    {
        /* DAs do perform passiveDADetection */
        G_SlpdProperty.passiveDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.passiveDADetection"));                   
    }
    else
    {
        /* SAs do not do passiveDADetection */
        G_SlpdProperty.passiveDADetection = 0;
    }

    G_SlpdProperty.isBroadcastOnly = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isBroadcastOnly"));
    G_SlpdProperty.multicastTTL = atoi(SLPPropertyGet("net.slp.multicastTTL"));
    G_SlpdProperty.multicastMaximumWait = atoi(SLPPropertyGet("net.slp.multicastMaximumWait"));
    G_SlpdProperty.unicastMaximumWait = atoi(SLPPropertyGet("net.slp.unicastMaximumWait"));
    G_SlpdProperty.randomWaitBound = atoi(SLPPropertyGet("net.slp.randomWaitBound"));
    G_SlpdProperty.maxResults = atoi(SLPPropertyGet("net.slp.maxResults"));
    G_SlpdProperty.traceMsg = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceMsg"));
    G_SlpdProperty.traceReg = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceReg"));
    G_SlpdProperty.traceDrop = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceDrop"));
    G_SlpdProperty.traceDATraffic = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.traceDATraffic"));
    G_SlpdProperty.DAAddresses = SLPPropertyGet("net.slp.DAAddresses");
    G_SlpdProperty.DAAddressesLen = strlen(G_SlpdProperty.DAAddresses);
    /* TODO make sure that we are using scopes correctly.  What about DHCP, etc*/
    G_SlpdProperty.useScopes = SLPPropertyGet("net.slp.useScopes");
    G_SlpdProperty.useScopesLen = strlen(G_SlpdProperty.useScopes);
    G_SlpdProperty.locale = SLPPropertyGet("net.slp.locale");
    G_SlpdProperty.localeLen = strlen(G_SlpdProperty.locale);
    G_SlpdProperty.securityEnabled = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.securityEnabled"));


    /*-------------------------------------*/
    /* Set the net.slp.interfaces property */
    /*-------------------------------------*/
    G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
    if(*G_SlpdProperty.interfaces == 0)
    {
        SLPPropertySet("net.slp.interfaces", GetInterfaceList());
        G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
    }
    G_SlpdProperty.interfacesLen = strlen(G_SlpdProperty.interfaces);

    /*---------------------------------------------------------*/
    /* Set the value used internally as the url for this agent */
    /*---------------------------------------------------------*/
    /* 27 is the size of "service:directory-agent://(NULL)" */
    myname = GetHostname();    
    if(myname)
    {
        G_SlpdProperty.myUrl = (const char*)malloc(27 + strlen(myname));
        if(G_SlpdProperty.isDA)
        {
            strcpy((char*)G_SlpdProperty.myUrl,"service:directory-agent://");
        }
        else
        {
            strcpy((char*)G_SlpdProperty.myUrl,"service:service-agent://");
        }

        strcat((char*)G_SlpdProperty.myUrl,myname);
        G_SlpdProperty.myUrlLen = strlen(G_SlpdProperty.myUrl);

        free(myname);
    }

    /*----------------------------------*/
    /* Set other values used internally */
    /*----------------------------------*/
    G_SlpdProperty.DATimestamp = 1;  /* DATimestamp must start at 1 */
    G_SlpdProperty.activeDiscoveryXmits = 3;
    G_SlpdProperty.nextPassiveDAAdvert = 0;
    G_SlpdProperty.nextActiveDiscovery = 0;

    return 0;
}


#ifdef DEBUG
/*=========================================================================*/
void SLPDPropertyDeinit()
/*=========================================================================*/
{
    SLPPropertyFreeAll();     
}
#endif
