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

/** Parse URL.
 *
 * Implementation for SLPParseSrvURL(), SLPEscape(), SLPUnescape() and 
 * SLPFree() calls.
 *
 * @file       libslp_parse.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSLPCode
 */

#include "slp.h"
#include "libslp.h"
#include "slp_net.h"
#include "slp_xmalloc.h"
#include "slp_parse.h"
#include "slp_message.h"

/** Put a 16-bit-length-preceeded string into a buffer.
 * 
 * Write 2 bytes of length in Big-Endian format followed by the specified
 * number of bytes of string data. Update the pointer on return to point to
 * the location following the last byte written.
 * 
 * @param[in,out] cpp - The address of a buffer pointer.
 * @param[in] str - The string to write.
 * @param[in] strsz - The length of @p str.
 */
void PutL16String(uint8_t ** cpp, const char * str, size_t strsz)
{
   PutUINT16(cpp, strsz);
   memcpy(*cpp, str, strsz);
   *cpp += strsz;
}

/** Calculate the size of a URL Entry.
 * 
 * @param[in] urllen - the length of the URL.
 * @param[in] urlauthlen - the length of the URL authentication block.
 *
 * @remarks Currently OpenSLP only handles a single authentication 
 *    block. To handle more than this, this routine would have to take
 *    a sized array of @p urlauthlen values.
 *
 * @return A number of bytes representing the total URL Entry size.
 */
size_t SizeofURLEntry(size_t urllen, size_t urlauthlen) 
{
   /* reserved(1) + lifetime(2) + urllen(2) + url + #auths(2) + authlen */
   return 1 + 2 + 2 + urllen + 2 + urlauthlen;
}

/** Write a URL Entry to a network buffer.
 *
 * This routine inserts all the fields of a URL entry (as defined by 
 * RFC 2608, Section 4.3 URL Entries) into a buffer starting at the
 * location specified by the address stored in @p cpp, returning an
 * updated address value in @p cpp, such that on exit it points to 
 * the next location in the buffer after the last byte of the URL 
 * entry.
 *
 * @param[in,out] cpp - The address of a buffer pointer.
 * @param[in] url - The URL to write.
 * @param[in] urllen - The length of @p url.
 * @param[in] urlauth - The URL authentication block to write.
 * @param[in] urlauthlen - The length of @p urlauth.
 *
 * @remarks The pointer contained in @p cpp is updated to reflect the
 *    next buffer position after the URL Entry written on exit.
 *
 * @remarks Currently OpenSLP only handles a single authentication 
 *    block. To handle more than this, PutURLEntry would have to take
 *    arrays of @p urlauth and @p urlauthlen values.
 */
void PutURLEntry(uint8_t ** cpp, const char * url, size_t urllen, 
      const uint8_t * urlauth, size_t urlauthlen)
{
   uint8_t * curpos = *cpp;

/*  0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |   Reserved    |          Lifetime             |   URL Length  |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |URL len, contd.|            URL (variable length)              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |# of URL auths |            Auth. blocks (if any)              \
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ */

   /* Reserved */
   *curpos++ = 0;

   /* Lifetime */
   PutUINT16(&curpos, 0);

   /* URL Length */
   PutUINT16(&curpos, urllen);

   /* URL */
   memcpy(curpos, url, urllen);
   curpos += urllen;

   /* @todo Enhance OpenSLP to take multiple auth blocks per request. */

   /* # of URL auths / Auth. blocks */
   *curpos++ = urlauth? 1: 0;
   memcpy(curpos, urlauth, urlauthlen);
   curpos += urlauthlen;

   *cpp = curpos;
}

/** Free a block of memory acquired through the OpenSLP interface.
 *
 * Frees memory returned from SLPParseSrvURL, SLPFindScopes, SLPEscape, 
 * and SLPUnescape.
 *
 * @param[in] pvMem - A pointer to the storage allocated by the 
 *    SLPParseSrvURL, SLPEscape, SLPUnescape, or SLPFindScopes functions.
 *    Ignored if NULL.
 */
void SLPAPI SLPFree(void * pvMem)                                                  
{
   if (pvMem)
      xfree(pvMem);
}

/** Parse a service URL into constituent parts.
 *
 * Parses the URL passed in as the argument into a service URL structure
 * and returns it in the @p ppSrvURL pointer. If a parse error occurs, 
 * returns SLP_PARSE_ERROR. The input buffer @p pcSrvURL is not modified.
 * Rather, portions of it are copied into a buffer at the end of the
 * structure allocated for return. Note that this is contrary to the RFC, 
 * which indicates that it is destructively modified during the parse and 
 * used to fill in the fields of the return structure. The structure 
 * returned in @p ppSrvURL should be freed with SLPFree. Again, note that
 * the RFC refers to a non-existent routine named SLPFreeURL. If the URL 
 * has no service part, the s_pcSrvPart string is the empty string, ""
 * (ie, not NULL). If @p pcSrvURL is not a service: URL, then the 
 * s_pcSrvType field in the returned data structure is the URL's scheme, 
 * which might not be the same as the service type under which the URL was 
 * registered. If the transport is IP, the s_pcNetFamily field is the empty 
 * string. Note that the RFC refers to a non-existent s_pcTransport field
 * here. If the transport is not IP or there is no port number, the s_iPort 
 * field is zero.
 *
 * @param[in] pcSrvURL - A pointer to a character buffer containing the 
 *    null terminated URL string to parse. Note that RFC 2614 makes this 
 *    parameter non-const, and the routine destructive, but this is 
 *    unecessary from an API perspective, as the memory for the output 
 *    parameter is allocated.
 * @param[in] ppSrvURL - A pointer to a pointer for the SLPSrvURL structure 
 *    to receive the parsed URL. The memory should be freed by a call to
 *    SLPFree when no longer needed.
 *
 * @return If no error occurs, the return value is SLP_OK. Otherwise, the
 *    appropriate error code is returned.
 */
SLPEXP SLPError SLPAPI SLPParseSrvURL(
      const char * pcSrvURL,
      SLPSrvURL ** ppSrvURL)
{
   int result = SLPParseSrvUrl(strlen(pcSrvURL), pcSrvURL, 
         (SLPParsedSrvUrl **) ppSrvURL); 
   switch(result)
   {
      case ENOMEM:
         return SLP_MEMORY_ALLOC_FAILED;

      case EINVAL:
         return SLP_PARSE_ERROR;
   } 
   return SLP_OK;
}

#define ATTRIBUTE_RESERVE_STRING "(),\\!<=>~"
#define ATTRIBUTE_BAD_TAG        "\r\n\t_"
#define ESCAPE_CHARACTER         '\\'
#define ESCAPE_CHARACTER_STRING  "\\"

/** Escape an SLP string.
 *
 * Process the input string in pcInbuf and escape any SLP reserved
 * characters. If the isTag parameter is SLPTrue, then look for bad tag
 * characters and signal an error if any are found by returning the
 * SLP_PARSE_ERROR code. The results are put into a buffer allocated by
 * the API library and returned in the ppcOutBuf parameter. This buffer
 * should be deallocated using SLPFree when the memory is no longer
 * needed.
 *
 * @param[in] pcInbuf - Pointer to he input buffer to process for escape 
 *    characters.
 * @param[out] ppcOutBuf - Pointer to a pointer for the output buffer with 
 *    the SLP reserved characters escaped. Must be freed using SLPFree
 *    when the memory is no longer needed.
 * @param[in] isTag - When true, the input buffer is checked for bad tag 
 *    characters.
 *
 * @return Return SLP_PARSE_ERROR if any characters are bad tag characters 
 *    and the isTag flag is true, otherwise SLP_OK, or the appropriate error
 *    code if another error occurs.
 */
SLPEXP SLPError SLPAPI SLPEscape(
      const char * pcInbuf,
      char ** ppcOutBuf,
      SLPBoolean isTag)
{
   char * current_inbuf, * current_outBuf;
   int amount_of_escape_characters;
   char hex_digit;

   /* Ensure that the parameters are good. */
   if (!pcInbuf || (isTag != SLP_TRUE && isTag != SLP_FALSE))
      return SLP_PARAMETER_BAD;

   /* Loop thru the string, counting the number of reserved characters 
    * and checking for bad tags when required.  This is also used to 
    * calculate the size of the new string to create.
    * ASSUME: that pcInbuf is a NULL terminated string. 
    */
   current_inbuf = (char *)pcInbuf;
   amount_of_escape_characters = 0;

   while (*current_inbuf)
   {
      /* Ensure that there are no bad tags when it is a tag. */
      if (isTag && strchr(ATTRIBUTE_BAD_TAG, *current_inbuf))
         return SLP_PARSE_ERROR;
      if (strchr(ATTRIBUTE_RESERVE_STRING, *current_inbuf))
         amount_of_escape_characters++;
      current_inbuf++;
   }

   /* Allocate the string. */
   *ppcOutBuf = xmalloc(sizeof(char) * (strlen(pcInbuf) 
         + amount_of_escape_characters * 2 + 1));

   if (!ppcOutBuf)
      return SLP_MEMORY_ALLOC_FAILED;

   /* Go over it, again. Replace each of the escape characters with their 
    * hex equivalent. 
    */
   current_inbuf = (char *)pcInbuf;
   current_outBuf = *ppcOutBuf;
   while (*current_inbuf)
   {
      /* Check to see if it is an escape character. */
      if ((strchr(ATTRIBUTE_RESERVE_STRING, *current_inbuf)) 
            || ((*current_inbuf >= 0x00) && (*current_inbuf <= 0x1F)) 
            || (*current_inbuf == 0x7F))
      {
         /* Insert the escape character. */
         *current_outBuf = ESCAPE_CHARACTER;
         current_outBuf++;

         /* Do the first digit. */
         hex_digit = (*current_inbuf & 0xF0) / 0x0F;
         if (hex_digit >= 0 && hex_digit <= 9)
            *current_outBuf = hex_digit + '0';
         else
            *current_outBuf = hex_digit + 'A' - 0x0A;

         current_outBuf++;

         /* Do the last digit. */
         hex_digit = *current_inbuf & 0x0F;
         if (hex_digit >= 0 && hex_digit <= 9)
            *current_outBuf = hex_digit + '0';
         else
            *current_outBuf = hex_digit + 'A' - 0x0A;
      }
      else
         *current_outBuf = *current_inbuf;

      current_outBuf += sizeof(char);
      current_inbuf += sizeof(char);
   }
   *current_outBuf = 0;
   return SLP_OK;
}

/** Unescape an SLP string.
 *
 * Process the input string in pcInbuf and unescape any SLP reserved
 * characters.  If the isTag parameter is SLPTrue, then look for bad tag
 * characters and signal an error if any are found with the
 * SLP_PARSE_ERROR code.  No transformation is performed if the input
 * string is an opaque.  The results are put into a buffer allocated by
 * the API library and returned in the ppcOutBuf parameter.  This buffer
 * should be deallocated using SLPFree() when the memory is no longer
 * needed.
 *
 * @param[in] pcInbuf - Pointer to he input buffer to process for escape 
 *    characters.
 * @param[out] ppcOutBuf - Pointer to a pointer for the output buffer with 
 *    the SLP reserved characters escaped. Must be freed using SLPFree
 *    when the memory is no longer needed.
 * @param[in] isTag - When true, the input buffer is checked for bad tag 
 *    characters.
 *
 * @return Return SLP_PARSE_ERROR if any characters are bad tag characters 
 *    and the isTag flag is true, otherwise SLP_OK, or the appropriate 
 *    error code if another error occurs.
 */
SLPEXP SLPError SLPAPI SLPUnescape(
      const char * pcInbuf,
      char ** ppcOutBuf,
      SLPBoolean isTag)
{
   size_t output_buffer_size;
   char * current_Inbuf, * current_OutBuf;
   char escaped_digit[2];

   /* Ensure that the parameters are good. */
   if (!pcInbuf || (isTag != SLP_TRUE && isTag != SLP_FALSE))
      return SLP_PARAMETER_BAD;

   /* Loop thru the string, counting the number of escape characters 
    * and checking for bad tags when required.  This is also used to 
    * calculate the size of the new string to create.
    * ASSUME: that pcInbuf is a NULL terminated string. 
    */
   current_Inbuf = (char *)pcInbuf;
   output_buffer_size = strlen(pcInbuf);

   while(*current_Inbuf)
   {
      /* Ensure that there are no bad tags when it is a tag. */
      if (isTag && strchr(ATTRIBUTE_BAD_TAG, *current_Inbuf))
         return SLP_PARSE_ERROR;

      if (strchr(ESCAPE_CHARACTER_STRING, *current_Inbuf))
         output_buffer_size-=2;

      current_Inbuf++;
   }

   /* Allocate the string. */
   *ppcOutBuf = (char *)xmalloc((sizeof(char) * output_buffer_size) + 1);

   if (!ppcOutBuf)
      return SLP_MEMORY_ALLOC_FAILED;

   current_Inbuf = (char *)pcInbuf;
   current_OutBuf = *ppcOutBuf;

   while (*current_Inbuf)
   {
      /* Check to see if it is an escape character. */
      if (strchr(ESCAPE_CHARACTER_STRING, *current_Inbuf))
      {
         /* Insert the real character based on the escaped character. */
         escaped_digit[0] = *(current_Inbuf + sizeof(char));
         escaped_digit[1] = *(current_Inbuf + (sizeof(char) * 2));

         if (escaped_digit[0] >= 'A' && escaped_digit[0] <= 'F')
            escaped_digit[0] = escaped_digit[0] - 'A' + 0x0A;
         else if (escaped_digit[0] >= '0' && escaped_digit[0] <= '9')
            escaped_digit[0] = escaped_digit[0] - '0';
         else
            return SLP_PARSE_ERROR;

         if (escaped_digit[1] >= 'A' && escaped_digit[1] <= 'F')
            escaped_digit[1] = escaped_digit[1] - 'A' + 0x0A;
         else if (escaped_digit[1] >= '0' && escaped_digit[1] <= '9')
            escaped_digit[1] = escaped_digit[1] - '0';
         else
            return SLP_PARSE_ERROR;

         *current_OutBuf = escaped_digit[1] + (escaped_digit[0] * 0x10);
         current_Inbuf = (char *)current_Inbuf + (sizeof(char) * 2);
      }
      else
         *current_OutBuf = *current_Inbuf;

      /* Move to the next character. */
      current_OutBuf++;
      current_Inbuf++;
   }
   *current_OutBuf = 0;
   return SLP_OK;
}

/** Parse an SLP attribute buffer.
 *
 * Used to get individual attribute values from an attribute string that
 * is passed to the SLPAttrCallback.
 *
 * @param[in] pcAttrList - A character buffer containing a comma separated, 
 *    null terminated list of attribute id/value assignments, in SLP wire 
 *    format; i.e. "(attr-id=attr-value-list)".
 * @param[in] pcAttrId - The string indicating which attribute value to 
 *    return. MUST not be null. MUST not be the empty string ("").
 * @param[out] ppcAttrVal - A pointer to a pointer to the buffer to receive 
 *    the attribute value. The memory should be freed by a call to SLPFree
 *    when no longer needed.
 *
 * @return Returns SLP_PARSE_ERROR if an attribute of the specified id
 *    was not found otherwise SLP_OK.
 */
SLPEXP SLPError SLPAPI SLPParseAttrs(
      const char * pcAttrList,
      const char * pcAttrId,
      char ** ppcAttrVal)
{
   const char* slider1;
   const char* slider2;
   size_t      attridlen;

   /* Check for bad parameters. */
   if (!pcAttrList || !pcAttrId || !ppcAttrVal)
      return SLP_PARAMETER_BAD;

   attridlen = strlen(pcAttrId);
   slider1 = pcAttrList;
   while (1)
   {
      while (*slider1 != '(')
      {
         if (!*slider1)
            return SLP_PARSE_ERROR;
         slider1++;
      }
      slider1++;
      slider2 = slider1;

      while (*slider2 && *slider2 != '=' && *slider2 !=')')
         slider2++;

      if (attridlen == (unsigned)(slider2 - slider1) 
            && strncasecmp(slider1, pcAttrId, slider2 - slider1) == 0)
      {
         /* Found the attribute id. */
         slider1 = slider2;
         if (*slider1 == '=') 
            slider1++;
         while (*slider2 && *slider2 !=')') 
            slider2++;

         *ppcAttrVal = xmalloc((slider2 - slider1) + 1);
         if (!*ppcAttrVal)
            return SLP_MEMORY_ALLOC_FAILED;

         memcpy(*ppcAttrVal, slider1, slider2 - slider1);
         (*ppcAttrVal)[slider2-slider1] = 0;

         return SLP_OK;
      }
   }

   /* The attribute id does not exist. */
   return SLP_PARSE_ERROR;
}

/*=========================================================================*/
