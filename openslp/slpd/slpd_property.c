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
char* GetHostname()
/* Returns a string represting this host (the FDN) or null. Caller must    */
/* free returned string                                                    */
/*-------------------------------------------------------------------------*/
{
    char*   host    = 0;
    int     hostlen;

    /* TODO find a better constant for MAX_HOSTNAME */
    #define MAX_HOST_NAME 1024    
    host = (char*)malloc(MAX_HOST_NAME);
    if(host == 0)
    {
        return host;  
    }
    memset(host,0,MAX_HOST_NAME);
    if (gethostname(host, MAX_HOST_NAME) != 0)
    {
        free(host);
        host = 0;
    }

    /* fix for hostname stupidity.  Looks like hostname is not FQDN on  */
    /* some systems */

    if(strchr(host,'.') == 0)
    {
        #ifdef HAVE_GETDOMAINNAME
        hostlen = strlen(host);
        if(getdomainname(host + hostlen + 1,
                         MAX_HOST_NAME - hostlen - 1) == 0)
        {
            host[hostlen] = '.'; /* slip in the dot */
        }
        #endif
    }

    return host;
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
    if (ioctl(fd,SIOCGIFCONF,&ifc) == 0) 
    {
        /* allocate 16 bytes per interface */        
        result = (char*)malloc((MAX_INTERFACE * 16) + 1);
        if(result)
        {
            result[0] = 0; /* null terminate for strcat */
            for (i = 0; i < ifc.ifc_len/sizeof(struct ifreq); i++) 
            {
                sa = (struct sockaddr *)&(iflist[i].ifr_addr);
                if (sa->sa_family == AF_INET) 
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
void SLPDPropertyInit(const char* conffile)
/*=========================================================================*/
{
    char* myname = 0;

    SLPPropertyReadFile(conffile);
   
    memset(&G_SlpdProperty,0,sizeof(G_SlpdProperty));
   
    /*-------------------------------------------------------------*/
    /* Set the properties with out hard defaults                   */
    /*-------------------------------------------------------------*/
    G_SlpdProperty.isDA = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.isDA"));
    if(G_SlpdProperty.isDA)
    {
        /* DAs do not do active DA Detection */
        G_SlpdProperty.activeDADetection = 0;
        G_SlpdProperty.passiveDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.passiveDADetection"));                   
    }
    else
    {
        /* SAs do not do passiveDADetection */
        G_SlpdProperty.passiveDADetection = 0;
        G_SlpdProperty.activeDADetection = SLPPropertyAsBoolean(SLPPropertyGet("net.slp.activeDADetection"));               
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
    

    /*-------------------------------------*/
    /* Set the net.slp.interfaces property */
    /*-------------------------------------*/
    G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
    if(*G_SlpdProperty.interfaces == 0)
    {
        G_SlpdProperty.interfaces = GetInterfaceList();
        if(G_SlpdProperty.interfaces == 0)
        {
            G_SlpdProperty.interfaces = SLPPropertyGet("net.slp.interfaces");
        }
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
}



