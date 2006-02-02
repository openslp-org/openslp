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

/** Header file used by attribute parser.
 *
 * @file       libslpattr.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSlpAttrCode
 */

#ifndef LIBSLPATTR_H_INCLUDED
#define LIBSLPATTR_H_INCLUDED

/*!@defgroup LibSlpAttrCode Attribute Parsing */

/*!@addtogroup LibSlpAttrCode
 * @{
 */

#include "../common/slp_types.h"
#include "../common/slp_compare.h"
#include "../common/slp_net.h"
#include "../libslp/slp.h"

#define SLP_TAG_BAD 300
#define SLP_TAG_ERROR 400

/* The type for SLP attributes. An opaque type that acts as a handle to an 
 * attribute bundle.
 */
typedef void * SLPAttributes;
typedef void * SLPAttrIterator;
typedef void * SLPTemplate; 

/* The callback for receiving attributes from a SLPFindAttrObj(). */
typedef SLPBoolean SLPAttrObjCallback(SLPHandle hslp,
      const SLPAttributes attr, SLPError errcode, void * cookie);

/* A datatype to encapsulate opaque data.*/
typedef struct
{
   size_t len;
   void * data;
} SLPOpaque;

/*****************************************************************************
 *
 * Datatype to represent the types of known attributes. 
 * 
 ****************************************************************************/
typedef int SLPType;
#define SLP_BOOLEAN  ((SLPType)1)
#define SLP_INTEGER  ((SLPType)2)
#define SLP_KEYWORD  ((SLPType)4)
#define SLP_STRING   ((SLPType)8)
#define SLP_OPAQUE   ((SLPType)16)

/*****************************************************************************
 *
 * Datatype to represent modes of attribute modification.
 * 
 ****************************************************************************/
typedef enum
{
   SLP_ADD = 1,      /* Appends to the attribute list.*/
   SLP_REPLACE = 2   /* Replaces attribute. */
} SLPInsertionPolicy;

/*****************************************************************************
 *
 * Functions for the attribute struct. One could almost call them methods. 
 * 
 ****************************************************************************/

SLPError SLPAttrAlloc(const char * lang, const FILE * template_h,
      const SLPBoolean strict, SLPAttributes * slp_attr);

SLPError SLPAttrAllocStr(const char * lang, const FILE * template_h,
      SLPBoolean strict, SLPAttributes * slp_attr, const char * str);

void SLPAttrFree(SLPAttributes attr_h);

/* Attribute manipulation. */
SLPError SLPAttrSet_bool(SLPAttributes attr_h, const char * attribute_tag,
      SLPBoolean val);

SLPError SLPAttrSet_str(SLPAttributes attr_h, const char * tag,
      const char * val, SLPInsertionPolicy);

SLPError SLPAttrSet_keyw(SLPAttributes attr_h, const char * attribute_tag);

SLPError SLPAttrSet_int(SLPAttributes attr_h, const char * tag, int val,
      SLPInsertionPolicy policy); 

SLPError SLPAttrSet_opaque(SLPAttributes attr_h, const char * tag,
      const char * val, size_t len, SLPInsertionPolicy policy);

SLPError SLPAttrSet_guess(SLPAttributes attr_h, const char * tag,
      const char * val, SLPInsertionPolicy policy);

/* Attribute Querying. */
SLPError SLPAttrGet_bool(SLPAttributes attr_h, const char * tag,
      SLPBoolean * val);

SLPError SLPAttrGet_keyw(SLPAttributes attr_h, const char * tag);

SLPError SLPAttrGet_int(SLPAttributes attr_h, const char * tag, int * val[],
      size_t * size);

SLPError SLPAttrGet_str(SLPAttributes attr_h, const char * tag, char *** val,
      size_t * size); 

SLPError SLPAttrGet_opaque(SLPAttributes attr_h, const char * tag,
      SLPOpaque *** val, size_t * size);

/* Misc. */
SLPError SLPAttrGetType(SLPAttributes attr_h, const char * tag, 
      SLPType * type);

SLPError SLPAttrSerialize(SLPAttributes attr_h,
      const char * tags /* NULL terminated */, 
      char ** buffer,
      size_t bufferlen, /* Size of buffer. */
      size_t * count,   /* Bytes needed/written. */
      SLPBoolean find_delta);

SLPError SLPAttrFreshen(SLPAttributes attr_h, const char * new_attrs);

/* Functions. */
SLPError SLPRegAttr(SLPHandle slp_h, const char * srvurl,
      unsigned short lifetime, const char * srvtype, SLPAttributes attr_h,
      SLPBoolean fresh, SLPRegReport callback, void * cookie);

SLPError SLPFindAttrObj(SLPHandle hslp, const char * srvurlorsrvtype,
      const char * scopelist, const char * attrids,
      SLPAttrObjCallback * callback, void * cookie);

/*****************************************************************************
 *
 * Functions for the iterator struct
 * 
 ****************************************************************************/

SLPError SLPAttrIteratorAlloc(SLPAttributes attr, SLPAttrIterator * iter);
      void SLPAttrIteratorFree(SLPAttrIterator iter);

SLPBoolean SLPAttrIterNext(SLPAttrIterator iter_h, char const * *tag,
      SLPType * type);

/*! @} */

#endif   /* LIBSLPATTR_H_INCLUDED */

/*=========================================================================*/
