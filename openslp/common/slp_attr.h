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

/** Header file for common attribute list management code.
 *
 * @file       slp_attr.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#ifndef SLP_ATTR_H_INCLUDED
#define SLP_ATTR_H_INCLUDED

/*!@defgroup CommonCode Common Code */
/*!@defgroup CommonCodeAttrs Attribute */

/*!@addtogroup CommonCodeAttrs
 * @ingroup CommonCode
 * @{
 */

typedef enum attrTypes
{
   head           = -1,
   string,
   integer,
   boolean,
   opaque,
   tag
} SLPTypes;

typedef union SLP_attr_value
{
   char * stringVal;
   unsigned long intVal;
   int boolVal;
   void * opaqueVal;
} SLPAttrVal;

typedef struct SLP_attr_list
{
   struct SLP_attr_list * next;
   struct SLP_attr_list * prev;
   int isHead;
   char * name;
   SLPTypes type;
   SLPAttrVal val;
} SLPAttrList;

SLPAttrList * SLPAllocAttr(char * name, SLPTypes type, void * val, int len);
SLPAttrList * SLPAllocAttrList(void);
void SLPFreeAttr(SLPAttrList * attr);
void SLPFreeAttrList(SLPAttrList * list, int staticFlag);

SLPAttrList * SLPDecodeAttrString(char * s);

/*! @} */

#endif   /* SLP_ATTR_H_INCLUDED */

/*=========================================================================*/
