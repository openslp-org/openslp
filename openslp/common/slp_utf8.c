/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slp_unicode.c                                              */
/*                                                                         */
/* Abstract:    Do conversions between UTF-8 and other character encodings */
/*                                                                         */
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


#include <sys/types.h>

#include "slp_message.h"
#include "slp_v1message.h"

/* The following two routines are adapted from Ken Thompson's fss-utf.c.
 * See ftp://ftp.informatik.uni-erlangen.de/pub/doc/ISO/charsets/utf-8.c 
 */

typedef struct
{
    int     cmask;
    int     cval;
    int     shift;
    long    lmask;
    long    lval;
} Tab;

static Tab tab[] =
{
{ 0x80, 0x00, 0*6,       0x7F,         0}, /* 1 byte sequence */
{ 0xE0, 0xC0, 1*6,      0x7FF,      0x80}, /* 2 byte sequence */
{ 0xF0, 0xE0, 2*6,     0xFFFF,     0x800}, /* 3 byte sequence */
{ 0xF8, 0xF0, 3*6,   0x1FFFFF,   0x10000}, /* 4 byte sequence */
{ 0xFC, 0xF8, 4*6,  0x3FFFFFF,  0x200000}, /* 5 byte sequence */
{ 0xFE, 0xFC, 5*6, 0x7FFFFFFF, 0x4000000}, /* 6 byte sequence */
{ 0,       0,   0,          0,         0}  /* end of table    */
};


static int
utftouni(unsigned *p, const char *s, size_t n)
{
    long l;
    int c0, c, nc;
    Tab *t;

    if(s == 0)
        return 0;

    nc = 0;
    if(n <= nc)
        return -1;
    c0 = *s & 0xff;
    l = c0;
    for(t = tab; t->cmask; t++)
    {
        nc++;
        if((c0 & t->cmask) == t->cval)
        {
            l &= t->lmask;
            if(l < t->lval)
                return -1;
            *p = l;
            return nc;
        }
        if(n <= nc)
            return -1;
        s++;
        c = (*s ^ 0x80) & 0xFF;
        if(c & 0xC0)
            return -1;
        l = (l << 6) | c;
    }
    return -1;
}

static int
unitoutf(char *s, unsigned wc)
{
    long l;
    int c, nc;
    Tab *t;

    if(s == 0)
        return 0;

    l = wc;
    nc = 0;
    for(t=tab; t->cmask; t++)
    {
        nc++;
        if(l <= t->lmask)
        {
            c = t->shift;
            *s = t->cval | ( l >> c );
            while(c > 0)
            {
                c -= 6;
                s++;
                *s = 0x80 | (( l >> c) & 0x3F);
            }
            return nc;
        }
    }
    return -1;
}

/*=========================================================================*/
int SLPv1AsUTF8(int encoding, char *string, int *len) 
/* Converts a SLPv1 encoded string to a UTF-8 character string in          */
/* place. If string does not have enough space to hold the encoded string  */
/* we are dead.                                                            */
/*                                                                         */
/* encoding - (IN) unicode encoding of the string passed in                */
/*                                                                         */
/* string   - (INOUT) IN - pointer to SLPv1 encoded string                 */
/*                    OUT - pointer to converted UTF-8 string.             */
/*                                                                         */
/* len      - (INOUT) IN - length of SLPv1 encoded string (in bytes)       */
/*                    OUT - length of UTF-8 string (in bytes)              */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  string and len   */
/*            invalid if return is not successful.                         */
/*                                                                         */
/*=========================================================================*/
{
    int nc;
    unsigned uni;
    char utfchar[6];        /* UTF-8 chars are at most 6 bytes */
    char *utfstring = string, *unistring = string;

    if(encoding == SLP_CHAR_ASCII || encoding == SLP_CHAR_UTF8)
        return 0;

    if(encoding != SLP_CHAR_UNICODE16 && encoding != SLP_CHAR_UNICODE32)
        return SLP_ERROR_INTERNAL_ERROR;

    while(*len)
    {
        if(encoding == SLP_CHAR_UNICODE16)
        {
            uni = AsUINT16(unistring);
            unistring += 2;
            *len -= 2;
        }
        else
        {
            uni = AsUINT32(unistring);
            unistring += 4;
            *len -= 4;
        }
        if(*len < 0)
            return SLP_ERROR_INTERNAL_ERROR;

        nc = unitoutf(utfchar, uni);

        /* Take care not to overwrite. */
        if(nc < 0 || utfstring + nc > unistring)
            return SLP_ERROR_INTERNAL_ERROR;

        memcpy(utfstring, utfchar, nc);
        utfstring += nc;
    }
    *len = utfstring - string;
    return 0;
}


/*=========================================================================*/
int SLPv1ToEncoding(char *string, int *len, int encoding, 
                    const char *utfstring, int utflen) 
/* Converts a UTF-8 character string to a SLPv1 encoded string.            */
/* When called with string set to null returns number of bytes needed      */
/* in string.                                                              */
/*                                                                         */
/* string - (OUT) SLPv1 encoded string.                                    */
/*                                                                         */
/* len    - (INOUT) IN - bytes available in string                         */
/*                  OUT - bytes used up in string                          */
/*                                                                         */
/* encoding  - (IN) encoding of the string passed in                       */
/*                                                                         */
/* utfstring - (IN) pointer to UTF-8 string                                */
/*                                                                         */
/* utflen    - (IN) length of UTF-8 string                                 */
/*                                                                         */
/* Returns  - Zero on success, SLP_ERROR_PARSE_ERROR, or                   */
/*            SLP_ERROR_INTERNAL_ERROR if out of memory.  string and len   */
/*            invalid if return is not successful.                         */
/*                                                                         */
/*=========================================================================*/
{
    unsigned uni;
    int nc, total = 0;

    if(encoding == SLP_CHAR_ASCII || encoding == SLP_CHAR_UTF8)
    {
        if(*len < utflen)
            return SLP_ERROR_INTERNAL_ERROR;
        *len = utflen;
        if(string)
            memcpy(string, utfstring, utflen);
        return 0;
    }
    if(encoding != SLP_CHAR_UNICODE16 && encoding != SLP_CHAR_UNICODE32)
        return SLP_ERROR_INTERNAL_ERROR;
    while(utflen)
    {
        nc = utftouni(&uni, utfstring, utflen);
        utflen -= nc;
        if(nc < 0 || utflen < 0)
            return SLP_ERROR_INTERNAL_ERROR;
        utfstring += nc;
        if(encoding == SLP_CHAR_UNICODE16)
        {
            if(string)
            {
                ToUINT16(string, uni);
                string += 2;
            }
            total += 2;
        }
        else
        {
            if(string)
            {
                ToUINT32(string, uni);
                string += 4;
            }
            total += 4;
        }
        if(total > *len)
            return SLP_ERROR_INTERNAL_ERROR;
    }
    *len = total;
    return 0;
}
