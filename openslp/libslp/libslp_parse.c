/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slplib_parse.c                                             */
/*                                                                         */
/* Abstract:    Implementation for SLPParseSrvUrl(), SLPEscape(),          */
/*              SLPUnescape() and SLPFree() calls.                         */
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

#include "slp.h"
#include "libslp.h"

#ifdef WIN32 /* on Win32 strncasecmp is named strnicmp, but behaves the same */
    #define strncasecmp(String1, String2, Num) strnicmp(String1, String2, Num)
#endif

/*=========================================================================*/
void SLPFree(void* pvMem)                                                  
/*                                                                         */
/* Frees memory returned from SLPParseSrvURL(), SLPFindScopes(),           */
/* SLPEscape(), and SLPUnescape().                                         */
/*                                                                         */
/* pvMem    A pointer to the storage allocated by the SLPParseSrvURL(),    */
/*          SLPEscape(), SLPUnescape(), or SLPFindScopes() function.       */
/*          Ignored if NULL.                                               */
/*=========================================================================*/
{
    if(pvMem)
    {
        xfree(pvMem);
    }
}


/*=========================================================================*/
SLPError SLPParseSrvURL(const char *pcSrvURL,
                        SLPSrvURL** ppSrvURL)
/*                                                                         */
/* Parses the URL passed in as the argument into a service URL structure   */
/* and returns it in the ppSrvURL pointer.  If a parse error occurs,       */
/* returns SLP_PARSE_ERROR. The input buffer pcSrvURL is destructively     */
/* modified during the parse and used to fill in the fields of the         */
/* return structure.  The structure returned in ppSrvURL should be freed   */
/* with SLPFreeURL().  If the URL has no service part, the s_pcSrvPart     */
/* string is the empty string, "", i.e.  not NULL. If pcSrvURL is not a    */
/* service:  URL, then the s_pcSrvType field in the returned data          */
/* structure is the URL's scheme, which might not be the same as the       */
/* service type under which the URL was registered.  If the transport is   */
/* IP, the s_pcTransport field is the empty string.  If the transport is   */
/* not IP or there is no port number, the s_iPort field is zero.           */
/*                                                                         */
/* pcSrvURL A pointer to a character buffer containing the null terminated */
/*          URL string to parse.                                           */
/*                                                                         */
/* ppSrvURL A pointer to a pointer for the SLPSrvURL structure to receive  */
/*          the parsed URL. The memory should be freed by a call to        */
/*          SLPFree() when no longer needed.                               */
/*                                                                         */
/* Returns: If no error occurs, the return value is SLP_OK. Otherwise, the */
/*          appropriate error code is returned.                            */
/*=========================================================================*/
{
    char*   empty;   /* always points to an empty string */
    char*   slider1; /* points to location in the SLPSrvURL buffer */
    char*   slider2;
    char*   slider3;

    /* Check for bad parameters */
    if(pcSrvURL == 0 ||
       ppSrvURL == 0)
    {
        return SLP_PARAMETER_BAD;
    }

    /* Allocate memory and set up sliders */
    *ppSrvURL = (SLPSrvURL*)xmalloc(strlen(pcSrvURL) + sizeof(SLPSrvURL) + 5);
    /* +5 ensures space for 5 null terminations */
    if(*ppSrvURL == 0)
    {
        return SLP_MEMORY_ALLOC_FAILED;
    }
    memset(*ppSrvURL,0,strlen(pcSrvURL) + sizeof(SLPSrvURL) + 5);    
    slider1 = ((char*)*ppSrvURL) + sizeof(SLPSrvURL);
    slider2 = slider3 = (char*)pcSrvURL;
    
    /* Set empty */
    empty = slider1;
    slider1 ++;
     
    /* parse out the service type */
    slider3 = (char*)strstr(slider2,":/");
    if(slider3 == 0)
    {
        xfree(*ppSrvURL);
        *ppSrvURL = 0;
        return SLP_PARSE_ERROR;
    }    
    memcpy(slider1,slider2,slider3-slider2);
    (*ppSrvURL)->s_pcSrvType = slider1;
    slider1 += (slider3 - slider2) + 1;

    /* parse out the host */
    slider3 = slider2 = slider3 + 3; /* + 3 skips the "://" */
    while(*slider3 && *slider3 != '/' && *slider3 != ':') slider3++;
    if(slider3-slider2 < 1)
    {
        /* no host part (this is okay according to RFC2609) */
        (*ppSrvURL)->s_pcHost = empty;
    }
    else
    {
        memcpy(slider1,slider2,slider3-slider2);
        (*ppSrvURL)->s_pcHost = slider1;
        slider1 += (slider3 - slider2) + 1;
    }

    /* parse out the port */
    if(*slider3 == ':')
    {
        slider3 = slider2 = slider3 + 1; /* + 1 skips the ":" */
        while(*slider3 && (*slider3 != '/' && *slider3 != ';')) slider3++;
        memcpy(slider1,slider2,slider3-slider2);
        (*ppSrvURL)->s_iPort = atoi(slider1);
        slider1 += (slider3-slider2) + 1;
    }

    /* parse out the remainder of the url */
    if(*slider3)
    {
        slider2 = slider3;
        while(*slider3) slider3 ++;
        memcpy(slider1,slider2,slider3-slider2);
        (*ppSrvURL)->s_pcSrvPart = slider1;
        slider1 += (slider3 - slider2) + 1;
    }
    else
    {
        /* no remainder portion */
        (*ppSrvURL)->s_pcSrvPart = empty;
    }

    /* set  the net family to always be an empty string for IP */
    (*ppSrvURL)->s_pcNetFamily = empty;

    return SLP_OK;
}

#define ATTRIBUTE_RESERVE_STRING	"(),\\!<=>~"
#define ATTRIBUTE_BAD_TAG			"\r\n\t_"
#define ESCAPE_CHARACTER			'\\'
#define ESCAPE_CHARACTER_STRING		"\\"
/*=========================================================================*/
SLPError SLPEscape(const char* pcInbuf,
                   char** ppcOutBuf,
                   SLPBoolean isTag)
/*                                                                         */
/* Process the input string in pcInbuf and escape any SLP reserved         */
/* characters.  If the isTag parameter is SLPTrue, then look for bad tag   */
/* characters and signal an error if any are found by returning the        */
/* SLP_PARSE_ERROR code.  The results are put into a buffer allocated by   */
/* the API library and returned in the ppcOutBuf parameter.  This buffer   */
/* should be deallocated using SLPFree() when the memory is no longer      */
/* needed.                                                                 */
/*                                                                         */
/* pcInbuf      Pointer to he input buffer to process for escape           */
/*              characters.                                                */
/*                                                                         */
/* ppcOutBuf    Pointer to a pointer for the output buffer with the SLP    */
/*              reserved characters escaped.  Must be freed using          */
/*              SLPFree()when the memory is no longer needed.              */ 
/*                                                                         */
/* isTag        When true, the input buffer is checked for bad tag         */
/*              characters.                                                */
/*                                                                         */
/* Returns:     Return SLP_PARSE_ERROR if any characters are bad tag       */
/*              characters and the isTag flag is true, otherwise SLP_OK,   */
/*              or the appropriate error code if another error occurs.     */
/*=========================================================================*/
{
    char        *current_inbuf, *current_outBuf;
    int         amount_of_escape_characters;
    char        hex_digit;

    /* Ensure that the parameters are good. */
    if((pcInbuf == NULL) || ((isTag != SLP_TRUE) && (isTag != SLP_FALSE)))
        return(SLP_PARAMETER_BAD);

    /* 
     * Loop thru the string, counting the number of reserved characters 
     * and checking for bad tags when required.  This is also used to 
     * calculate the size of the new string to create.
     * ASSUME: that pcInbuf is a NULL terminated string. 
     */
    current_inbuf = (char *) pcInbuf;
    amount_of_escape_characters = 0;

    while(*current_inbuf != '\0')
    {
        /* Ensure that there are no bad tags when it is a tag. */
        if((isTag) && strchr(ATTRIBUTE_BAD_TAG, *current_inbuf))
            return(SLP_PARSE_ERROR);

        if(strchr(ATTRIBUTE_RESERVE_STRING, *current_inbuf))
            amount_of_escape_characters++;

        current_inbuf++;
    } /* End While. */

    /* Allocate the string. */
    *ppcOutBuf = (char *) xmalloc(
                                sizeof(char) * 
                                (strlen(pcInbuf) + (amount_of_escape_characters * 2) + 1));

    if(ppcOutBuf == NULL)
        return(SLP_MEMORY_ALLOC_FAILED);

    /*
     * Go over it, again.  Replace each of the escape characters with their 
     * \hex equivalent.
     */
    current_inbuf = (char *) pcInbuf;
    current_outBuf = *ppcOutBuf;
    while(*current_inbuf != '\0')
    {
        /* Check to see if it is an escape character. */
        if((strchr(ATTRIBUTE_RESERVE_STRING, *current_inbuf)) || 
           ((*current_inbuf >= 0x00) && (*current_inbuf <= 0x1F)) ||
           (*current_inbuf == 0x7F)
          )
        {
            /* Insert the escape character. */
            *current_outBuf = ESCAPE_CHARACTER;
            current_outBuf++;

            /* Do the first digit. */
            hex_digit = (*current_inbuf & 0xF0)/0x0F;
            if((hex_digit >= 0) && (hex_digit <= 9))
                *current_outBuf = hex_digit + '0';
            else
                *current_outBuf = hex_digit + 'A' - 0x0A;

            current_outBuf++;

            /* Do the last digit. */
            hex_digit = *current_inbuf & 0x0F;
            if((hex_digit >= 0) && (hex_digit <= 9))
                *current_outBuf = hex_digit + '0';
            else
                *current_outBuf = hex_digit + 'A' - 0x0A;
        }
        else
        {
            *current_outBuf = *current_inbuf;
        } /* End If, Else. */

        current_outBuf += sizeof(char);
        current_inbuf += sizeof(char);
    } /* End While. */

    /* Make sure that the string is properly terminated. */
    *current_outBuf = '\0';

    return(SLP_OK);
}

/*=========================================================================*/
SLPError SLPUnescape(const char* pcInbuf,
                     char** ppcOutBuf,
                     SLPBoolean isTag)
/*                                                                         */
/* Process the input string in pcInbuf and unescape any SLP reserved       */
/* characters.  If the isTag parameter is SLPTrue, then look for bad tag   */
/* characters and signal an error if any are found with the                */
/* SLP_PARSE_ERROR code.  No transformation is performed if the input      */
/* string is an opaque.  The results are put into a buffer allocated by    */
/* the API library and returned in the ppcOutBuf parameter.  This buffer   */
/* should be deallocated using SLPFree() when the memory is no longer      */
/* needed.                                                                 */
/*                                                                         */
/* pcInbuf      Pointer to he input buffer to process for escape           */
/*              characters.                                                */
/*                                                                         */
/* ppcOutBuf    Pointer to a pointer for the output buffer with the SLP    */
/*              reserved characters escaped.  Must be freed using          */
/*              SLPFree() when the memory is no longer needed.             */
/*                                                                         */
/* isTag        When true, the input buffer is checked for bad tag         */
/*              characters.                                                */
/*                                                                         */
/* Returns:     Return SLP_PARSE_ERROR if any characters are bad tag       */
/*              characters and the isTag flag is true, otherwise SLP_OK,   */
/*              or the appropriate error code if another error occurs.     */
/*=========================================================================*/
{
    int     output_buffer_size;
    char    *current_Inbuf, *current_OutBuf;
    char    escaped_digit[2];

    /* Ensure that the parameters are good. */
    if((pcInbuf == NULL) || ((isTag != SLP_TRUE) && (isTag != SLP_FALSE)))
        return(SLP_PARAMETER_BAD);

    /* 
     * Loop thru the string, counting the number of escape characters 
     * and checking for bad tags when required.  This is also used to 
     * calculate the size of the new string to create.
     * ASSUME: that pcInbuf is a NULL terminated string. 
     */
    current_Inbuf = (char *) pcInbuf;
    output_buffer_size = strlen(pcInbuf);

    while(*current_Inbuf != '\0')
    {
        /* Ensure that there are no bad tags when it is a tag. */
        if((isTag) && strchr(ATTRIBUTE_BAD_TAG, *current_Inbuf))
            return(SLP_PARSE_ERROR);

        if(strchr(ESCAPE_CHARACTER_STRING, *current_Inbuf))
            output_buffer_size-=2;

        current_Inbuf++;
    } /* End While. */

    /* Allocate the string. */
    *ppcOutBuf = (char *) xmalloc((sizeof(char) * output_buffer_size) + 1);

    if(ppcOutBuf == NULL)
        return(SLP_MEMORY_ALLOC_FAILED);

    current_Inbuf = (char *) pcInbuf;
    current_OutBuf = *ppcOutBuf;

    while(*current_Inbuf != '\0')
    {
        /* Check to see if it is an escape character. */
        if(strchr(ESCAPE_CHARACTER_STRING, *current_Inbuf))
        {
            /* Insert the real character based on the escaped character. */
            escaped_digit[0] = *(current_Inbuf + sizeof(char));
            escaped_digit[1] = *(current_Inbuf + (sizeof(char) * 2));

            if((escaped_digit[0] >= 'A') && (escaped_digit[0] <= 'F'))
                escaped_digit[0] = escaped_digit[0] - 'A' + 0x0A;
            else if((escaped_digit[0] >= '0') && (escaped_digit[0] <= '9'))
                escaped_digit[0] = escaped_digit[0] - '0';
            else
                return(SLP_PARSE_ERROR);

            if((escaped_digit[1] >= 'A') && (escaped_digit[1] <= 'F'))
                escaped_digit[1] = escaped_digit[1] - 'A' + 0x0A;
            else if((escaped_digit[1] >= '0') && (escaped_digit[1] <= '9'))
                escaped_digit[1] = escaped_digit[1] - '0';
            else
                return(SLP_PARSE_ERROR);

            *current_OutBuf = escaped_digit[1] + (escaped_digit[0] * 0x10);
            current_Inbuf = (char *) current_Inbuf + (sizeof(char) * 2);
        }
        else
        {
            *current_OutBuf = *current_Inbuf;
        } /* End If, Else. */

        /* Move to the next character. */
        current_OutBuf++;
        current_Inbuf++;
    } /* End While. */

    /* Make sure we terminate the string properly. */
    *current_OutBuf = '\0';

    return(SLP_OK);
}


/*=========================================================================*/
SLPError SLPParseAttrs(const char* pcAttrList,
                       const char *pcAttrId,
                       char** ppcAttrVal)
/*                                                                         */
/* Used to get individual attribute values from an attribute string that   */
/* is passed to the SLPAttrCallback                                        */
/*                                                                         */
/* pcAttrList (IN) A character buffer containing a comma separated, null   */
/*                 terminated list of attribute id/value assignments, in   */
/*                 SLP wire format; i.e.  "(attr-id=attr-value-list)"      */
/*                                                                         */
/* pcAttrId (IN)   The string indicating which attribute value to return.  */
/*                 MUST not be null.  MUST not be the empty string ("").   */
/*                                                                         */
/* ppcAttrVal (IN) A pointer to a pointer to the buffer to receive         */
/*                 attribute value.  The memory should be freed by a call  */
/*                 to SLPFree() when no longer needed.                     */
/*                                                                         */
/* Returns: Returns SLP_PARSE_ERROR if an attribute of the specified id    */
/*          was not found otherwise SLP_OK                                 */
/*=========================================================================*/
{
    const char* slider1;
    const char* slider2;
    size_t      attridlen;

    /* Check for bad parameters */
    if(pcAttrList == 0 ||
       pcAttrId == 0   ||
       ppcAttrVal == 0)
    {
        return SLP_PARAMETER_BAD;
    }

    attridlen = strlen(pcAttrId);
    slider1 = pcAttrList;
    while(1)
    {
        while(*slider1 != '(')
        {
            if(*slider1 == 0)
            {
                return SLP_PARSE_ERROR;
            }
            slider1++;
        }
        slider1++;
        slider2=slider1;

        while(*slider2 && *slider2 != '=' && *slider2 !=')') slider2++;

        if(attridlen == slider2-slider1 && 
           strncasecmp(slider1, pcAttrId, slider2 - slider1) == 0)
        {
            /* found the attribute id */
            slider1 = slider2;
            if(*slider1 == '=') slider1++;
            while(*slider2 && *slider2 !=')') slider2++;
            
            *ppcAttrVal = (char*)xmalloc((slider2 - slider1) + 1);
            if(*ppcAttrVal == 0)
            {
                return SLP_MEMORY_ALLOC_FAILED;
            }

            memcpy(*ppcAttrVal,slider1,slider2-slider1);
            (*ppcAttrVal)[slider2-slider1] = 0;
                
            return SLP_OK;
        }
    }

    /* attrid does not exist */
    return SLP_PARSE_ERROR;
}
