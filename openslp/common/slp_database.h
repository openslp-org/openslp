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

/** Header file SLP message database functions. 
 *
 * The SLP message database holds actual SLP "wire" message buffers as well 
 * as structures that interpret the message buffer. The database exposes an 
 * interface suitable for a linked-list-based implementation.
 *
 * @file       slp_database.h
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCodeDB
 */

#ifndef SLP_DATABASE_H_INCLUDED
#define SLP_DATABASE_H_INCLUDED

/*!@defgroup CommonCodeDB Database
 * @ingroup CommonCode
 * @{
 */

#include "slp_message.h"
#include "slp_buffer.h"
#include "slp_linkedlist.h"

/** A database entry */
typedef struct _SLPDatabaseEntry
{
   SLPListItem listitem;
   SLPMessage * msg;
   SLPBuffer buf;
} SLPDatabaseEntry;

/** The database is just a list in this implementation. */
typedef SLPList SLPDatabase;

/** A database handle is the database and a cursor. */
typedef struct _SLPDatabaseHandle
{
   SLPDatabase * database;
   SLPDatabaseEntry * current;
} * SLPDatabaseHandle;

int SLPDatabaseInit(SLPDatabase* database);

void SLPDatabaseDeinit(SLPDatabase * database);

SLPDatabaseEntry * SLPDatabaseEntryCreate(SLPMessage * msg, SLPBuffer buf);

void SLPDatabaseEntryDestroy(SLPDatabaseEntry * entry);

SLPDatabaseHandle SLPDatabaseOpen(SLPDatabase * database);

SLPDatabaseEntry * SLPDatabaseEnum(SLPDatabaseHandle dh);

void SLPDatabaseRewind(SLPDatabaseHandle dh);

void SLPDatabaseClose(SLPDatabaseHandle dh);

void SLPDatabaseRemove(SLPDatabaseHandle dh, SLPDatabaseEntry * entry);

void SLPDatabaseAdd(SLPDatabaseHandle dh, SLPDatabaseEntry * entry);

int SLPDatabaseCount(SLPDatabaseHandle dh);

/*! @} */

#endif   /* SLP_DATABASE_H_INCLUDED */

/*=========================================================================*/
