/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_database.c                                            */
/*                                                                         */
/* Abstract:    Implements database abstraction.  Currently a simple double*/
/*              linked list is used for the underlying storage.            */
/*                                                                         */
/* WARNING:     NOT thread safe!                                           */
//*-------------------------------------------------------------------------*/
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
#include <ctype.h>

/*-------------------------------------------------------------------------*/
char* TrimWhitespace(char* str)
/*-------------------------------------------------------------------------*/
{
    char* end;

    end=str+strlen(str)-1;

    while(*str && *str <= 0x20)
    {
        str++;
    }

    while(end >= str)
    {
        if(*end > 0x20)
        {
            break;
        }
        
        *end = 0;

        end--;
    }
    
    return str;
}

/*-------------------------------------------------------------------------*/
char* RegFileReadLine(FILE* fd, char* line, int linesize)
/*-------------------------------------------------------------------------*/
{
    while(1)
    {
        if(fgets(line,linesize,fd) == 0)
        {
            return 0;
        }
        if(*line == 0x0d || *line == 0x0a)
        {
            break;    
        }
        while(*line && *line <= 0x20) line++;

        if(*line != 0 && *line != '#' && *line != ';')
        {
            break;
        }
    }

    return line;
}

/*=========================================================================*/
SLPDDatabaseEntry* SLPDRegFileReadEntry(FILE* fd, SLPDDatabaseEntry** entry)
/* A really big and nasty function that reads an entry SLPDDatabase entry  */
/* from a file. Don't look at this too hard or you'll be sick              */
/*                                                                         */
/* fd       (IN) file to read from                                         */
/*                                                                         */
/* entry    (OUT) Address of a pointer that will be set to the location of */
/*                a dynamically allocated SLPDDatabase entry.  The entry   */
/*                must be freed                                            */
/*                                                                         */
/* Returns  *entry or null on error.                                       */
/*=========================================================================*/
{
    char*   slider1;
    char*   slider2;
    char    line[4096];

    /* give the out param a value */
    *entry = 0;

    /*----------------------------------------------------------*/
    /* read the next non-white non-comment line from the stream */
    /*----------------------------------------------------------*/
    do
    {
        slider1 = RegFileReadLine(fd,line,4096);
        if(slider1 == 0)
        {
            /* read through the whole file and found no entries */
            return 0;
        }
    }while(*slider1 == 0x0d ||  *slider1 == 0x0a);

    /*---------------------------*/
    /* Allocate a database entry */
    /*---------------------------*/
    *entry = SLPDDatabaseEntryAlloc();
    if(entry == 0)
    {
        SLPFatal("Out of memory!\n");
        return 0;
    }

    /* entries read from the .reg file are always local */
    (*entry)->islocal = 1;

    /*---------------------*/
    /* Parse the url-props */
    /*---------------------*/
    slider2 = strchr(slider1,',');
    if(slider2) 
    {
        /* srvurl */
        *slider2 = 0; /* squash comma to null terminate srvurl */
        (*entry)->url = strdup(TrimWhitespace(slider1));
        if((*entry)->url == 0)
        {
            SLPLog("Out of memory reading srvurl from regfile line ->%s",line);
            goto SLPD_ERROR;
        }
        (*entry)->urllen = strlen((*entry)->url);
        
        /* derive srvtype from srvurl */
        (*entry)->srvtype = strstr(slider1,"://");
        if((*entry)->srvtype == 0)
        {
            SLPLog("Looks like a bad url on regfile line ->%s",line);
            goto SLPD_ERROR;   
        }
        *(*entry)->srvtype = 0;
        (*entry)->srvtype=strdup(TrimWhitespace(slider1));
        (*entry)->srvtypelen = strlen((*entry)->srvtype);
        slider1 = slider2 + 1;

        /*lang*/
        slider2 = strchr(slider1,',');
        if(slider2)
        {
            *slider2 = 0; /* squash comma to null terminate lang */
            (*entry)->langtag = strdup(TrimWhitespace(slider1)); 
            if((*entry)->langtag == 0)
            {
                SLPLog("Out of memory reading langtag from regfile line ->%s",line);
                goto SLPD_ERROR;
            }            (*entry)->langtaglen = strlen((*entry)->langtag);     
            slider1 = slider2 + 1;                                  
        }
        else
        {
            SLPLog("Expected language tag near regfile line ->%s\n",line);
            goto SLPD_ERROR;
        }
             
        /* ltime */
        slider2 = strchr(slider1,',');
        if(slider2)                      
        {
            *slider2 = 0; /* squash comma to null terminate ltime */
            (*entry)->lifetime = atoi(slider1);
            slider1 = slider2 + 1;
        }                                  
        else
        {
            (*entry)->lifetime = atoi(slider1);
            slider1 = slider2;
        }
        if((*entry)->lifetime < 1 || (*entry)->lifetime > 0xffff)
        {
            SLPLog("Invalid lifetime near regfile line ->%s\n",line);
            goto SLPD_ERROR;
        }
        
        /* get the srvtype if one was not derived by the srvurl*/
        if((*entry)->srvtype == 0)
        {
            (*entry)->srvtype = strdup(TrimWhitespace(slider1));
            if((*entry)->srvtype == 0)
            {
                SLPLog("Out of memory reading srvtype from regfile line ->%s",line);
                goto SLPD_ERROR;
            }
            (*entry)->srvtypelen = strlen((*entry)->srvtype);
            if((*entry)->srvtypelen == 0)
            {
                SLPLog("Expected to derive service-type near regfile line -> %s\n",line);
                goto SLPD_ERROR;
            }
        }   

    }
    else
    {
        SLPLog("Expected to find srv-url near regfile line -> %s\n",line);
        goto SLPD_ERROR;
    }
    
    /*-------------------------------------------------*/
    /* Read all the attributes including the scopelist */
    /*-------------------------------------------------*/
    *line=0;
    while(1)
    {
        if(RegFileReadLine(fd,line,4096) == 0)
        {
            break;
        }         
        if(*line == 0x0d || *line == 0x0a)
        {
            break;
        }

        /* Check to see if it is the scopes line */
		/* FIXME We can collapse the scope stuff into the value getting and 
		 * just make it a special case (do strcmp on the tag as opposed to the 
		 * line) of attribute getting. 
		 */
        if(strncasecmp(line,"scopes",6) == 0)
        {
            /* found scopes line */
            slider1 = line;
            slider2 = strchr(slider1,'=');
            if(slider2)
            {
                slider2++;
                if(*slider2)
                {
                    /* just in case some idiot puts multiple scopes lines */
                    if((*entry)->scopelist)
                    {
                        SLPLog("scopes already defined previous to regfile line ->%s",line);
                        goto SLPD_ERROR;
                    }

                    (*entry)->scopelist=strdup(TrimWhitespace(slider2));
                    if((*entry)->scopelist == 0)
                    {
                        SLPLog("Out of memory adding scopes from regfile line ->%s",line);
                        goto SLPD_ERROR;
                    }
                    (*entry)->scopelistlen = strlen((*entry)->scopelist);
                }
            }
        }
        else
        {
            #ifdef USE_PREDICATES
            char *tag; /* Will point to the start of the tag. */
			char *val; /* Will point to the start of the value. */
			char *end;
			char *tag_end;

			tag = line;
			/*** Elide ws. ***/
			while (isspace(*tag)) 
			{
				tag++;
			}
			tag_end = tag;

			/*** Find tag end. ***/
			while (*tag_end && (!isspace(*tag_end)) && (*tag_end != '=')) {
				tag_end++;
			}
			while (*tag_end && *tag_end != '=') {
				tag_end++;
			}
			*tag_end = 0;

			/*** Find value start. ***/
			val = tag_end + 1;
			/*** Elide ws. ***/
			while (isspace(*val)) 
			{
				val++;
			}

			/*** Elide trailing ws. ***/
			end = val;

			/** Find tag end. **/
			while (*end != 0) {
				end++;
			}

			/*** Back up over trailing whitespace. ***/
			end--;
			while(isspace(*end)) {
				*end = 0; /* Overwrite ws. */
				end--;
			}           
			
            SLPAttrSet_guess((*entry)->attr, tag, val, SLP_ADD);

            #else
            
            /* line contains an attribute (slow but it works)*/
            /* TODO Fix this so we do not have to realloc memory each time! */
            TrimWhitespace(line); 
            (*entry)->attrlistlen += strlen(line) + 2;
            
            if((*entry)->attrlist == 0)
            {
                (*entry)->attrlist = malloc((*entry)->attrlistlen + 1);
                *(*entry)->attrlist = 0;
            }
            else
            {
                (*entry)->attrlist = realloc((*entry)->attrlist,
                                             (*entry)->attrlistlen + 1);
            }
            
            if((*entry)->attrlist == 0)
            {
                SLPLog("Out of memory adding DEFAULT scope\n");
                goto SLPD_ERROR;
            }

            strcat((*entry)->attrlist,"(");
            strcat((*entry)->attrlist,line);
            strcat((*entry)->attrlist,")");

            #endif
        }
    }

    /* Set the scope set in properties if not is set */
    if((*entry)->scopelist == 0)
    {
        (*entry)->scopelist=strdup(G_SlpdProperty.useScopes);
        if((*entry)->scopelist == 0)
        {
            SLPLog("Out of memory adding DEFAULT scope\n");
            goto SLPD_ERROR;
        }
        (*entry)->scopelistlen = G_SlpdProperty.useScopesLen;
    }

    return *entry;

SLPD_ERROR:
    if(*entry)
    {
        SLPDDatabaseEntryFree(*entry);
		*entry = 0;
    }

    return 0;
}
