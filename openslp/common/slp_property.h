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

/** Types and prototypes for SLP configuration property access.
 *
 * @file       slp_property.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeProp
 */

#ifndef SLP_PROPERTY_H_INCLUDED
#define SLP_PROPERTY_H_INCLUDED

/*!@defgroup CommonCodeProp Property Access
 * @ingroup CommonCode
 * @{
 */

/** @name SLPPropertySet Property Attributes */
/*@{*/
#define SLP_PA_USERSET     0x0001   /*!< Only users may set these values */
#define SLP_PA_READONLY    0x0002   /*!< Once set, these are immutable */
/*@}*/

char * SLPPropertyXDup(const char * name);
const char * SLPPropertyGet(const char * name, char * buffer, size_t * bufsz);
int SLPPropertySet(const char * name, const char * value, unsigned attrs);

bool SLPPropertyAsBoolean(const char * name);
int SLPPropertyAsInteger(const char * name);
int SLPPropertyAsIntegerVector(const char * name, 
      int * ivector, int ivectorsz);

int SLPPropertySetAppConfFile(const char * aconffile);

int SLPPropertyReinit(void);
int SLPPropertyInit(const char * gconffile);
void SLPPropertyExit(void);

/*! @} */

#endif   /* SLP_PROPERTY_H_INCLUDED */

/*=========================================================================*/
