/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP command line UA wrapper                            */
/*                                                                         */
/* File:        slptool.c                                                  */
/*                                                                         */
/* Abstract:    Command line wrapper for OpenSLP                           */
/*                                                                         */
/* Requires:    OpenSLP installation                                       */
/*                                                                         */
/* Author(s):   Matt Peterson <mpeterson@caldera.com>                      */
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
/*     Please submit patches to maintainer of http://www.openslp.org       */
/*                                                                         */
/***************************************************************************/

#include "slptool.h"
#ifndef MI_NOT_SUPPORTED
#include "libslp.h"
#else
#ifndef UNICAST_NOT_SUPPORTED
#include "libslp.h"
#endif
#endif /* MI_NOT_SUPPORTED */


#ifdef WIN32
#define strncasecmp(String1, String2, Num) strnicmp(String1, String2, Num)
#define strcasecmp(String1, String2) stricmp(String1, String2)
#define inet_aton(opt,bind) ((bind)->s_addr = inet_addr(opt))
#else
#ifndef HAVE_STRNCASECMP
int
strncasecmp(const char *s1, const char *s2, size_t len);
#endif
#ifndef HAVE_STRCASECMP
int
strcasecmp(const char *s1, const char *s2);
#endif
#endif 

#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
SLPError SLPAssociateIFList( SLPHandle hSLP,
                         SLPToolCommandLine* cmdline)
/*=========================================================================*/
{

    PSLPHandleInfo      handle;

    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(hSLP            == 0 ||
       *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
       cmdline->cmdparam3 == 0 ||
       *cmdline->cmdparam3 == 0)  /* interface list can't be empty string */
    {
        return SLP_PARAMETER_BAD;
    }

    handle = (PSLPHandleInfo)hSLP;

#ifdef DEBUG
    fprintf(stderr, "SLPAssociateIFList(): cmdline->cmdparam3 = %s\n", cmdline->cmdparam3);
#endif

    if ((cmdline->cmd == FINDSRVSUSINGIFLIST) || 
        (cmdline->cmd ==  FINDATTRSUSINGIFLIST) || 
        (cmdline->cmd == FINDSRVTYPESUSINGIFLIST))
    { 
        handle->McastIFList = cmdline->cmdparam3;
    } else {
        return SLP_PARAMETER_BAD;
    }

    return SLP_OK;
}
#endif /* MI_NOT_SUPPORTED */


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
SLPError SLPAssociateIP( SLPHandle hSLP,
                         SLPToolCommandLine* cmdline)
/*=========================================================================*/
{

    PSLPHandleInfo      handle;

    /*------------------------------*/
    /* check for invalid parameters */
    /*------------------------------*/
    if(hSLP            == 0 ||
       *(unsigned int*)hSLP != SLP_HANDLE_SIG ||
       cmdline->cmdparam3 == 0 ||
       *cmdline->cmdparam3 == 0)  /* unicast address not specified */
    {
        return SLP_PARAMETER_BAD;
    }

    handle = (PSLPHandleInfo)hSLP;

#ifdef DEBUG
    fprintf(stderr, "SLPAssociateIP(): cmdline->cmdparam3 = %s\n", cmdline->cmdparam3);
#endif
    handle->dounicast = 1;
    handle->unicastaddr.sin_family = AF_INET;
    if (inet_aton(cmdline->cmdparam3, ((struct in_addr *)(&handle->unicastaddr.sin_addr))) == 0 ) {
        return SLP_PARAMETER_BAD;
    }
    handle->unicastaddr.sin_port = htons(SLP_RESERVED_PORT);

    return SLP_OK;
}
#endif


/*=========================================================================*/
SLPBoolean mySrvTypeCallback( SLPHandle hslp, 
                              const char* srvtypes, 
                              SLPError errcode, 
                              void* cookie ) 
/*=========================================================================*/
{
    char* cpy;
    char* slider1;
    char* slider2;

    if(errcode == SLP_OK && *srvtypes)
    {
        cpy = strdup(srvtypes);
        if(cpy)
        {
            slider1 = slider2 = cpy;
            slider1 = strchr(slider2,',');
	    while(slider1)
            {
                *slider1 = 0;
                printf("%s\n",slider2);
                slider1 ++;
                slider2 = slider1;
	        slider1 = strchr(slider2,',');
            }

            /* print the final itam */
            printf("%s\n",slider2);

            free(cpy);
        }
        
    }

    return SLP_TRUE;
}


/*=========================================================================*/
void FindSrvTypes(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
        if(cmdline->cmdparam1)
        {
            result = SLPFindSrvTypes(hslp,
				     cmdline->cmdparam1,
				     cmdline->scopes,
				     mySrvTypeCallback,
				     0);
	}
        else
        {
	    result = SLPFindSrvTypes(hslp,
				     "*",
				     cmdline->scopes,
				     mySrvTypeCallback,
				     0);
	}
       
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
       
        SLPClose(hslp);
    }
}


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindSrvTypes(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK) {
        if((result = SLPAssociateIP(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        if(cmdline->cmdparam1)
        {
            result = SLPFindSrvTypes(hslp,
                                     cmdline->cmdparam1,
                                     cmdline->scopes,
                                     mySrvTypeCallback,
                                     0);
        }
        else
        {
            result = SLPFindSrvTypes(hslp,
                                     "*",
                                     cmdline->scopes,
                                     mySrvTypeCallback,
                                     0);
        }

        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }
}
#endif


#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
void FindSrvTypesUsingIFList(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
	if((result = SLPAssociateIFList(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        if(cmdline->cmdparam1)
        {
            result = SLPFindSrvTypes(hslp,
                                     cmdline->cmdparam1,
                                     cmdline->scopes,
                                     mySrvTypeCallback,
                                     0);
	}
        else
        {
	    result = SLPFindSrvTypes(hslp,
                                     "*",
                                     cmdline->scopes,
                                     mySrvTypeCallback,
                                     0);
	}
       
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
       
        SLPClose(hslp);
    }
}
#endif /* MI_NOT_SUPPORTED */

/*=========================================================================*/
SLPBoolean myAttrCallback(SLPHandle hslp, 
                        const char* attrlist, 
                        SLPError errcode, 
                        void* cookie )
/*=========================================================================*/
{
    if(errcode == SLP_OK)
    {
        printf("%s\n",attrlist);
    }
    
    return SLP_TRUE;
}


/*=========================================================================*/
void FindAttrs(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {

        result = SLPFindAttrs(hslp,
                             cmdline->cmdparam1,
                             cmdline->scopes,
                             cmdline->cmdparam2,
                             myAttrCallback,
                             0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindAttrs(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK) {
        if((result = SLPAssociateIP(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        result = SLPFindAttrs(hslp,
                             cmdline->cmdparam1,
                             cmdline->scopes,
                             cmdline->cmdparam2,
                             myAttrCallback,
                             0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }
}
#endif


#ifndef MI_NOT_SUPPORTED 
/*=========================================================================*/
void FindAttrsUsingIFList(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
	if((result = SLPAssociateIFList(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        result = SLPFindAttrs(hslp,
                              cmdline->cmdparam1,
                              cmdline->scopes,
                              cmdline->cmdparam2,
                              myAttrCallback,
                              0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}
#endif /* MI_NOT_SUPPORTED */

 
/*=========================================================================*/
SLPBoolean mySrvUrlCallback( SLPHandle hslp, 
                             const char* srvurl, 
                             unsigned short lifetime, 
                             SLPError errcode, 
                             void* cookie ) 
/*=========================================================================*/
{
    if(errcode == SLP_OK)
    {
        printf("%s,%i\n",srvurl,lifetime);
    }
    
    return SLP_TRUE;
}


/*=========================================================================*/
void FindSrvs(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
        result = SLPFindSrvs(hslp,
                             cmdline->cmdparam1,
                             cmdline->scopes,
                             cmdline->cmdparam2,
                             mySrvUrlCallback,
                             0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}


#ifndef MI_NOT_SUPPORTED
/*=========================================================================*/
void FindSrvsUsingIFList(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
	if((result = SLPAssociateIFList(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        result = SLPFindSrvs(hslp,
                             cmdline->cmdparam1,
                             cmdline->scopes,
                             cmdline->cmdparam2,
                             mySrvUrlCallback,
                             0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}
#endif /* MI_NOT_SUPPORTED */


#ifndef UNICAST_NOT_SUPPORTED
/*=========================================================================*/
void UnicastFindSrvs(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK) {
	if((result = SLPAssociateIP(hslp, cmdline)) != SLP_OK)
        {
            printf("errorcode: %i\n",result);
            SLPClose(hslp);
            return;
        }

        result = SLPFindSrvs(hslp,
                             cmdline->cmdparam1,
                             cmdline->scopes,
			     cmdline->cmdparam2,
			     mySrvUrlCallback,
                             0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }
}
#endif


/*=========================================================================*/
void FindScopes(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;
    char*       scopes;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {
        result = SLPFindScopes(hslp,&scopes);
        if(result == SLP_OK)
	{
	   printf("%s\n",scopes);
	   SLPFree(scopes);
	}
       
        SLPClose(hslp);
    }               
}

/*-------------------------------------------------------------------------*/
void mySLPRegReport(SLPHandle hslp, SLPError errcode, void* cookie)
{
    if (errcode)
    printf("(de)registration errorcode %d\n", errcode);
}


/*=========================================================================*/
void Register(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;
    char srvtype[80] = "", *s;
    int len = 0;

    if (strncasecmp(cmdline->cmdparam1, "service:", 8) == 0)
	len = 8;

    s = strchr(cmdline->cmdparam1 + len, ':');
    if (!s)
    {
	printf("Invalid URL: %s\n", cmdline->cmdparam1);
	return;
    }
    len = s - cmdline->cmdparam1;
    strncpy(srvtype, cmdline->cmdparam1, len);
    srvtype[len] = 0;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {

        result = SLPReg(hslp,
                        cmdline->cmdparam1,
                        SLP_LIFETIME_DEFAULT,
                        srvtype,
                        cmdline->cmdparam2,
                        SLP_TRUE,
                        mySLPRegReport,
                        0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}

/*=========================================================================*/
void Deregister(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
    SLPError    result;
    SLPHandle   hslp;

    if(SLPOpen(cmdline->lang,SLP_FALSE,&hslp) == SLP_OK)
    {

        result = SLPDereg(hslp,
			  cmdline->cmdparam1,
			  mySLPRegReport,
			  0);
        if(result != SLP_OK)
        {
            printf("errorcode: %i\n",result);
        }
        SLPClose(hslp);
    }               
}

/*=========================================================================*/
void PrintVersion(SLPToolCommandLine* cmdline)
/*=========================================================================*/
{
#ifdef WIN32
    printf("slptool version = %s\n",SLP_VERSION);
#else
	printf("slptool version = %s\n",VERSION);
#endif
    printf("libslp version = %s\n", 
           SLPGetProperty("net.slp.OpenSLPVersion"));

    printf("libslp configuration file = %s\n",
           SLPGetProperty("net.slp.OpenSLPConfigFile"));
}

/*=========================================================================*/
void GetProperty(SLPToolCommandLine* cmdline)
/*=========================================================================*/  
{
    const char* propertyValue;
    
    propertyValue = SLPGetProperty(cmdline->cmdparam1);
    printf("%s = %s\n", 
	   cmdline->cmdparam1,
	   propertyValue == 0 ? "" : propertyValue);
}

/*=========================================================================*/
int ParseCommandLine(int argc,char* argv[], SLPToolCommandLine* cmdline)
/* Returns  Zero on success.  Non-zero on error.                           */
/*=========================================================================*/
{
    int i;

    if(argc < 2)
    {
        /* not enough arguments */
        return 1;
    }

    for (i=1;i<argc;i++)
    {
        if( strcasecmp(argv[i],"-v") == 0 ||
            strcasecmp(argv[i],"--version") == 0 )
        {
            if(i < argc)
            {
                cmdline->cmd = PRINT_VERSION;
		return 0;
            }
            else
            {
                return 1;
            }
        }
        else if( strcasecmp(argv[i],"-s") == 0 ||
		 strcasecmp(argv[i],"--scopes") == 0 )
        {
            i++;
            if(i < argc)
            {
                cmdline->scopes = argv[i];
            }
            else
            {
                return 1;
            }
        }
        else if( strcasecmp(argv[i],"-l") == 0 ||
                 strcasecmp(argv[i],"--lang") == 0 )
        {
            i++;
            if(i < argc)
            {
                cmdline->lang = argv[i];
            }
            else
            {
                return 1;
            }
        }
        else if(strcasecmp(argv[i],"findsrvs") == 0)
        {
            cmdline->cmd = FINDSRVS;
            
            /* service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
            else
            {
                return 1;
            }
            
            /* (optional) filter */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }

            break;
        }
        #ifndef MI_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"findsrvsusingiflist") == 0)
        {
            cmdline->cmd = FINDSRVSUSINGIFLIST;

	    /* (required) IFList */
	    i++;
	    if(i < argc)
	    {
                cmdline->cmdparam3 = argv[i];
	    }
	    else
	    {
                return 1;
	    }
            
            /* service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
            else
            {
                return 1;
            }

            /* (optional) filter */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }

            break;
        }
        #endif /* MI_NOT_SUPPORTED */
        #ifndef UNICAST_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"unicastfindsrvs") == 0)
        {
            cmdline->cmd = UNICASTFINDSRVS;
            
            i++;
            if(i < argc)
            {
                cmdline->cmdparam3 = argv[i];
            }
            else
            {
                return 1;
            }

            /* service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            } else {
		return 1;
	    }
	
		/* optional filter */
	    i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            } 

            break;
        }
        #endif
        else if(strcasecmp(argv[i],"findattrs") == 0)
        {
            cmdline->cmd = FINDATTRS;     
                  
            /* url or service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
            else
            {
                return 1;
            }
            
            /* (optional) attrids */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }
        }
        #ifndef UNICAST_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"unicastfindattrs") == 0)
        {
            cmdline->cmd = UNICASTFINDATTRS;

            /* unicast IP address */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam3 = argv[i];
            }
            else
            {
                return 1;
            }

            /* url or service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            } else {
                return 1;
            }

                /* optional filter */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }

            break;
        }
        #endif
#ifndef MI_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"findattrsusingiflist") == 0)
        {
            cmdline->cmd = FINDATTRSUSINGIFLIST;     
                  
	    /* (required) IFList */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam3 = argv[i];
            }
            else
            {
                return 1;
            }
            
            /* url or service type */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
            else
            {
                return 1;
            }

            /* (optional) attrids */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }
        }
#endif /* MI_NOT_SUPPORTED */
        else if(strcasecmp(argv[i],"findsrvtypes") == 0)
        {
            cmdline->cmd = FINDSRVTYPES;

            /* (optional) naming authority */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }                             
        }
        #ifndef UNICAST_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"unicastfindsrvtypes") == 0)
        {
            cmdline->cmd = UNICASTFINDSRVTYPES;

            /* unicast IP address */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam3 = argv[i];
            }
            else
            {
                return 1;
            }
            /* (optional) naming authority */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
        }
        #endif
#ifndef MI_NOT_SUPPORTED
        else if(strcasecmp(argv[i],"findsrvtypesusingiflist") == 0)
        {
            cmdline->cmd = FINDSRVTYPESUSINGIFLIST;

            /* (required) IFList */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam3 = argv[i];
            }
            else
            {
                return 1;
            }

            /* (optional) naming authority */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }                             
        }
#endif /* MI_NOT_SUPPORTED */
        else if(strcasecmp(argv[i],"findscopes") == 0)
        {
	    cmdline->cmd = FINDSCOPES;
	}
        else if(strcasecmp(argv[i],"register") == 0)
        {
            cmdline->cmd = REGISTER;
            
            /* url */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
            else
            {
                return 1;
            }
            
            /* Optional attrids */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }
	    else
	    {
	        cmdline->cmdparam2 = cmdline->cmdparam1 + strlen(cmdline->cmdparam1);
            }
	   
            break;
        }
        else if(strcasecmp(argv[i],"deregister") == 0)
        {
            cmdline->cmd = DEREGISTER;

            /* url */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam1 = argv[i];
            }
	    else
	    {
		return 1;
	    }
        }
        else if(strcasecmp(argv[i],"getproperty") == 0)
        {
	    cmdline->cmd = GETPROPERTY;
	    i++;
	    if(i < argc)
	    {
	        cmdline->cmdparam1 = argv[i];
	    }
	    else
	    {
	        return 1;
	    }
	}
        else
        {
            return 1;
        }    
    }

    return 0;
}


/*=========================================================================*/
void DisplayUsage()
/*=========================================================================*/
{
    printf("Usage: slptool [options] command-and-arguments \n");
    printf("   options may be:\n");
    printf("      -v (or --version) displays version of slptool and OpenSLP\n");
    printf("      -s (or --scope) followed by a comma separated list of scopes\n");
    printf("      -l (or --language) followed by a language tag\n");
    printf("   command-and-arguments may be:\n");
    printf("      findsrvs service-type [filter]\n");
#ifndef MI_NOT_SUPPORTED
    printf("      findsrvsusingiflist interface-list service-type [filter]\n");
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
    printf("      unicastfindsrvs ip-address service-type [filter]\n");
#endif
    printf("      findattrs url [attrids]\n");
#ifndef UNICAST_NOT_SUPPORTED
    printf("      unicastfindattrs ip-address url [attrids]\n");
#endif
#ifndef MI_NOT_SUPPORTED
    printf("      findattrsusingiflist interface-list url [attrids]\n");
#endif /* MI_NOT_SUPPORTED */
    printf("      findsrvtypes [authority]\n");
#ifndef UNICAST_NOT_SUPPORTED
    printf("      unicastfindsrvtypes [authority]\n");
#endif
#ifndef MI_NOT_SUPPORTED
    printf("      findsrvtypesusingiflist interface-list [authority]\n");
#endif /* MI_NOT_SUPPORTED */
    printf("      findscopes\n");
    printf("      register url [attrs]\n");
    printf("      deregister url\n");
    printf("      getproperty propertyname\n");
    printf("Examples:\n");
    printf("   slptool register service:myserv.x://myhost.com \"(attr1=val1),(attr2=val2)\"\n");
    printf("   slptool findsrvs service:myserv.x\n");
    printf("   slptool findsrvs service:myserv.x \"(attr1=val1)\"\n");
#ifndef MI_NOT_SUPPORTED
    printf("   slptool findsrvsusingiflist 10.77.13.240,192.168.250.240 service:myserv.x\n");
    printf("   slptool findsrvsusingiflist 10.77.13.243 service:myserv.x \"(attr1=val1)\"\n");
#endif /* MI_NOT_SUPPORTED */
#ifndef UNICAST_NOT_SUPPORTED
    printf("   slptool unicastfindsrvs 10.77.13.237 service:myserv.x\n");
    printf("   slptool unicastfindsrvs 10.77.13.237 service:myserv.x \"(attr1=val1)\"\n");
#endif
    printf("   slptool findattrs service:myserv.x://myhost.com\n");
    printf("   slptool findattrs service:myserv.x://myhost.com attr1\n");
#ifndef UNICAST_NOT_SUPPORTED
    printf("   slptool unicastfindattrs 10.77.13.237 service:myserv.x\n");
    printf("   slptool unicastfindattrs 10.77.13.237 service:myserv.x://myhost.com attr1 \n");
#endif
#ifndef MI_NOT_SUPPORTED
    printf("   slptool findattrsusingiflist 10.77.13.240,192.168.250.240 service:myserv.x://myhost.com\n");
    printf("   slptool findattrsusingiflist 10.77.13.243 service:myserv.x://myhost.com attr1\n");
#endif /* MI_NOT_SUPPORTED */
    printf("   slptool deregister service:myserv.x://myhost.com\n");
    printf("   slptool getproperty net.slp.useScopes\n");
}


/*=========================================================================*/
int main(int argc, char* argv[])
/*=========================================================================*/
{
    int                 result;
    SLPToolCommandLine  cmdline;

    /* zero out the cmdline */
    memset(&cmdline,0,sizeof(cmdline));
    
    /* Parse the command line */
    if(ParseCommandLine(argc,argv,&cmdline) == 0)
    {
        switch(cmdline.cmd)
        {
        case FINDSRVS:
            FindSrvs(&cmdline);
            break;
        
        #ifndef UNICAST_NOT_SUPPORTED
        case UNICASTFINDSRVS:
	    UnicastFindSrvs(&cmdline);
            break;
	#endif

        case FINDATTRS:
            FindAttrs(&cmdline);
            break;
        
        #ifndef UNICAST_NOT_SUPPORTED
        case UNICASTFINDATTRS:
	    UnicastFindAttrs(&cmdline);
            break;
	#endif

        case FINDSRVTYPES:
            FindSrvTypes(&cmdline);
            break;
	
        #ifndef UNICAST_NOT_SUPPORTED
        case UNICASTFINDSRVTYPES:
	    UnicastFindSrvTypes(&cmdline);
            break;
	#endif

	case FINDSCOPES:
	    FindScopes(&cmdline);
	    break;
	   
        case GETPROPERTY:
            GetProperty(&cmdline);
            break;

	 case REGISTER:
            Register(&cmdline);
            break;

	 case DEREGISTER:
            Deregister(&cmdline);
            break;

#ifndef MI_NOT_SUPPORTED
	 case FINDSRVSUSINGIFLIST:
            FindSrvsUsingIFList(&cmdline);
            break;

        case FINDATTRSUSINGIFLIST:
            FindAttrsUsingIFList(&cmdline);
            break;
        
        case FINDSRVTYPESUSINGIFLIST:
            FindSrvTypesUsingIFList(&cmdline);
            break;
#endif /* MI_NOT_SUPPORTED */

	 case PRINT_VERSION:
	    PrintVersion(&cmdline);
        }
    }
    else
    {
        DisplayUsage();
        result = 1;
    }

    return 0;
}
