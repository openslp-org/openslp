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

/** Internal Header file used by attribute parser.
 *
 * The internal structures that represent an attribute list. This is made 
 * available for slpd to speed its access to values for predicate evaluation. 
 *
 * @file       libslpattr_internal.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    LibSlpAttrCode
 */

#ifndef LIBSLPATTR_INTERNAL_H_INCLUDED
#define LIBSLPATTR_INTERNAL_H_INCLUDED

/*!@addtogroup LibSlpAttrCode
 * @{
 */

/******************************************************************************
 *
 *                              Individual values
 *
 * What is a value?
 *
 * In SLP an individual attribute can be associated with a list of values. A
 * value is the data associated with a tag. Depending on the type of
 * attribute, there can be zero, one, or many values associated with a single
 * tag. 
 *****************************************************************************/

typedef struct xx_value_t
{
    struct xx_value_t *next;
    int escaped_len;
    int unescaped_len;
    union
    {
        SLPBoolean va_bool;
        int va_int;
        char *va_str; /* This is used for keyword, string, and opaque. */
    } data; /* Stores the value of the variable. Note, any string must be copied into the struct. */

    /* Memory handling */
    struct xx_value_t *next_chunk; /* The next chunk of allocated memory in the value list. */
    struct xx_value_t *last_value_in_chunk; /* The last value in the chunk. Only set by the chunk head. */
} value_t;


/******************************************************************************
 *
 *                              Individual attributes (vars)
 *
 *  What is a var? 
 *
 *  A var is a variable tag that is associated with a list of values. Zero or
 *  more vars are kept in a single SLPAttributes object. Each value stored in
 *  a var is kept in a value struct. 
 *****************************************************************************/

/* An individual attribute in the struct. */
typedef struct xx_var_t
{
    struct xx_var_t *next; /* Pointer to the next variable. */
    SLPType type; /* The type of this variable. */
    const char *tag; /* The name of this variable. */
    unsigned int tag_len; /* The length of the tag. */
    value_t *list; /* The list of values. */
    int list_size; /* The number of values in the list. */
    SLPBoolean modified; /* Flag. Set to be true if the attribute should be included in the next freshen.  */
} var_t;


/******************************************************************************
 *
 *                             All the attributes. 
 *
 *****************************************************************************/

/* The opaque struct representing a SLPAttributes handle.
 */
struct xx_SLPAttributes
{
    SLPBoolean strict; /* Are we using strict typing? */
    char *lang; /* Language. */
    var_t *attrs; /* List of vars to be sent. */
    int attr_count; /* The number of attributes */
};



/* Finds a variable by its tag. */
var_t *attr_val_find_str(struct xx_SLPAttributes *slp_attr, const char *tag, int tag_len); 

/* Finds the type of an attribute. */
SLPError SLPAttrGetType_len(SLPAttributes attr_h, const char *tag, int tag_len, SLPType *type); 

/*! @} */

#endif   /* LIBSLPATTR_INTERNAL_H_INCLUDED */

/*=========================================================================*/
