/*-------------------------------------------------------------------------
 * Copyright (C) 2000 Caldera Systems, Inc
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 *    Neither the name of Caldera Systems nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * `AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE CALDERA
 * SYSTEMS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;  LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *-------------------------------------------------------------------------*/

#include "slptool.h"

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
            while(slider1 = strchr(slider2,','))
            {
                *slider1 = 0;
                printf("%s\n",slider2);
                slider1 ++;
                slider2 = slider1;
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
    int len = 0, callbackerr;

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
			SLP_LIFETIME_MAXIMUM,
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
int ParseCommandLine(int argc,char* argv[], SLPToolCommandLine* cmdline)
/* Returns  Zero on success.  Non-zero on error.
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
        if( strcasecmp(argv[i],"-s") == 0 ||
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
            
            /* attrids */
            i++;
            if(i < argc)
            {
                cmdline->cmdparam2 = argv[i];
            }
	    else
	    {
		return 1;
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
    printf("      -s (or --scope) followed by a comma separated list of scopes\n");
    printf("      -l (or --language) followed by a language tag\n");
    printf("   command-and-arguments may be:\n");
    printf("      findsrvs service-type [filter]\n");
    printf("      findattrs url [attrids]\n");
    printf("      findsrvtypes [authority]\n");
    printf("      findscopes\n");
    printf("      register url attrs\n");
    printf("      deregister url attrs\n");
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
        
        case FINDATTRS:
            FindAttrs(&cmdline);
            break;
        
        case FINDSRVTYPES:
            FindSrvTypes(&cmdline);
            break;
	
	case FINDSCOPES:
	    FindScopes(&cmdline);
	    break;
        
        case GETPROPERTY:
//            GetProperty(&cmdline);
            break;
        case REGISTER:
            Register(&cmdline);
            break;
        case DEREGISTER:
            Deregister(&cmdline);
            break;
        }
    }
    else
    {
        DisplayUsage();
        result = 1;
    }

    return 0;
}
