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

/** Header file for database abstraction.
 *
 * @file       slpd_database.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    SlpdCode
 */

#ifndef SLPD_DATABASE_H_INCLUDED
#define SLPD_DATABASE_H_INCLUDED

/*!@defgroup SlpdCodeDatabase Database */

/*!@addtogroup SlpdCodeDatabase
 * @ingroup SlpdCode
 * @{
 */

#include "slpd.h"
#include "slp_database.h"

#define SLPDDATABASE_INITIAL_URLCOUNT           256
#define SLPDDATABASE_INITIAL_SRVTYPELISTLEN     2048

typedef struct _SLPDDatabase
{
   SLPDatabase database;
   int urlcount;
   size_t srvtypelistlen;
} SLPDDatabase;

typedef struct _SLPDDatabaseSrvRqstResult
{
   void * reserved;
   SLPUrlEntry ** urlarray;
   int urlcount;
} SLPDDatabaseSrvRqstResult;

typedef struct _SLPDDatabaseSrvTypeRqstResult
{
   void * reserved;
   char * srvtypelist;
   size_t srvtypelistlen;
} SLPDDatabaseSrvTypeRqstResult;       

typedef struct _SLPDDatabaseAttrRqstResult
{
   void * reserved;
   char * attrlist;
   size_t attrlistlen;
   SLPAuthBlock * autharray;
   int authcount;
   int ispartial;
} SLPDDatabaseAttrRqstResult;       

void SLPDDatabaseAge(int seconds, int ageall);
int SLPDDatabaseReg(SLPMessage msg, SLPBuffer buf);
int SLPDDatabaseDeReg(SLPMessage msg);
int SLPDDatabaseSrvRqstStart(SLPMessage msg, 
      SLPDDatabaseSrvRqstResult ** result);
void SLPDDatabaseSrvRqstEnd(SLPDDatabaseSrvRqstResult * result);
int SLPDDatabaseSrvTypeRqstStart(SLPMessage msg, 
      SLPDDatabaseSrvTypeRqstResult ** result);
void SLPDDatabaseSrvTypeRqstEnd(SLPDDatabaseSrvTypeRqstResult * result);
int SLPDDatabaseAttrRqstStart(SLPMessage msg, 
      SLPDDatabaseAttrRqstResult ** result);
void SLPDDatabaseAttrRqstEnd(SLPDDatabaseAttrRqstResult * result);
void * SLPDDatabaseEnumStart(void);
SLPMessage SLPDDatabaseEnum(void * eh, SLPMessage * msg, SLPBuffer * buf);
void SLPDDatabaseEnumEnd(void * eh);
int SLPDDatabaseIsEmpty(void);
int SLPDDatabaseInit(const char * regfile);
int SLPDDatabaseReInit(const char * regfile);

#ifdef DEBUG
void SLPDDatabaseDeinit(void);
void SLPDDatabaseDump(void);
#endif

/*! @} */

#endif   /* SLPD_DATABASE_H_INCLUDED */

/*=========================================================================*/
