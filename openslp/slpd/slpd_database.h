/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol Version 2                                         */
/*                                                                         */
/* File:        slpd_database.h                                            */
/*                                                                         */
/* Abstract:    Implements database abstraction.  Currently a simple       */
/*              double linked list (common/slp_database.c) is used for the */
/*              underlying storage.                                        */
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

#ifndef SLPD_DATABASE_H_INCLUDED
#define SLPD_DATABASE_H_INCLUDED

#include "slpd.h"

/*=========================================================================*/
/* Common code includes                                                    */
/*=========================================================================*/
#include "../common/slp_database.h"


#define SLPDDATABASE_INITIAL_URLCOUNT           256
#define SLPDDATABASE_INITIAL_SRVTYPELISTLEN     2048

/*=========================================================================*/
typedef struct _SLPDDatabase
/*=========================================================================*/
{
    SLPDatabase database;
    int         urlcount;
    int         srvtypelistlen;
}SLPDDatabase;


/*=========================================================================*/
typedef struct _SLPDDatabaseSrvRqstResult
/*=========================================================================*/
{
    void*             reserved;
    SLPUrlEntry**     urlarray;
    int               urlcount;
}SLPDDatabaseSrvRqstResult;


/*=========================================================================*/
typedef struct _SLPDDatabaseSrvTypeRqstResult
/*=========================================================================*/
{
    void*           reserved;
    char*           srvtypelist;
    int             srvtypelistlen;
}SLPDDatabaseSrvTypeRqstResult;       


/*=========================================================================*/
typedef struct _SLPDDatabaseAttrRqstResult
/*=========================================================================*/
{
    void*           reserved;
    char*           attrlist;
    int             attrlistlen;
    SLPAuthBlock**  autharray;
    int             authcount;
}SLPDDatabaseAttrRqstResult;       


/*=========================================================================*/
void SLPDDatabaseAge(int seconds, int ageall);
/* Ages the database entries and clears new and deleted entry lists        */
/*                                                                         */
/* seconds  (IN) the number of seconds to age each entry by                */
/*																		   */
/* ageall   (IN) age even entries with SLP_LIFETIME_MAXIMUM                */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseReg(SLPMessage msg, SLPBuffer buf);
/* Add a service registration to the database                              */
/*                                                                         */
/* msg          (IN) SLPMessage of a SrvReg message as returned by         */
/*                   SLPMessageParse()                                     */
/*                                                                         */
/* buf          (IN) Otherwise unreferenced buffer interpreted by the msg  */
/*                   structure                                             */
/*                                                                         */
/* Returns  -   Zero on success.  Nonzero on error                         */
/*                                                                         */
/* NOTE:        All registrations are treated as fresh                     */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseDeReg(SLPMessage msg);
/* Remove a service registration from the database                         */
/*                                                                         */
/* msg  - (IN) message interpreting an SrvDereg message                    */
/*                                                                         */
/* Returns  -   Zero on success.  Non-zero on failure                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseSrvRqstStart(SLPMessage msg,
                             SLPDDatabaseSrvRqstResult** result);
/* Find services in the database                                           */
/*                                                                         */
/* msg      (IN) the SrvRqst to find.                                      */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseSrvRqstEnd() to free   */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseSrvRqstEnd(SLPDDatabaseSrvRqstResult* result);
/* Release resources used to find services in the database                 */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvRqstStart                                  */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseSrvTypeRqstStart(SLPMessage msg,
                                 SLPDDatabaseSrvTypeRqstResult** result);
/* Find service types in the database                                      */
/*                                                                         */
/* msg      (IN) the SrvTypRqst to find.                                   */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseSrvtypeRqstEnd() to    */
/*          free                                                           */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseSrvTypeRqstEnd(SLPDDatabaseSrvTypeRqstResult* result);
/* Release resources used to find service types in the database            */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvTypeRqstStart                              */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseAttrRqstStart(SLPMessage msg,
                              SLPDDatabaseAttrRqstResult** result);
/* Find attributes in the database                                         */
/*                                                                         */
/* msg      (IN) the AttrRqst to find.                                     */
/*                                                                         */
/* result   (OUT) pointer result structure                                 */
/*                                                                         */
/* Returns  - Zero on success. Non-zero on failure                         */
/*                                                                         */
/* Note:    Caller must pass *result to SLPDDatabaseAttrRqstEnd() to       */
/*          free                                                           */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseAttrRqstEnd(SLPDDatabaseAttrRqstResult* result);
/* Release resources used to find attributes in the database               */
/*                                                                         */
/* result   (IN) pointer result structure previously passed to             */
/*               SLPDDatabaseSrvTypeRqstStart                              */
/*                                                                         */
/* Returns  - None                                                         */
/*=========================================================================*/


/*=========================================================================*/
void* SLPDDatabaseEnumStart();
/* Start an enumeration of the entire database                             */
/*                                                                         */
/* Returns: An enumeration handle that is passed to subsequent calls to    */
/*          SLPDDatabaseEnum().  Returns NULL on failure.  Returned        */
/*          enumeration handle (if not NULL) must be passed to             */
/*          SLPDDatabaseEnumEnd() when you are done with it.               */
/*=========================================================================*/


/*=========================================================================*/
SLPMessage SLPDDatabaseEnum(void* eh, SLPMessage* msg, SLPBuffer* buf);
/* Enumerate through all entries of the database                           */
/*                                                                         */
/* eh (IN) pointer to opaque data that is used to maintain                 */
/*         enumerate entries.  Pass in a pointer to NULL to start          */
/*         enumeration.                                                    */
/*                                                                         */
/* msg (OUT) pointer to the SrvReg message that discribes buf              */
/*                                                                         */
/* buf (OUT) pointer to the SrvReg message buffer                          */
/*                                                                         */
/* returns: Pointer to enumerated entry or NULL if end of enumeration      */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseEnumEnd(void* eh);
/* End an enumeration started by SLPDDatabaseEnumStart()                   */
/*                                                                         */
/* Parameters:  eh (IN) The enumeration handle returned by                 */
/*              SLPDDatabaseEnumStart()                                    */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseIsEmpty();
/* Returns an boolean value indicating whether the database is empty       */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseInit(const char* regfile);
/* Initialize the database with registrations from a regfile.              */
/*                                                                         */
/* regfile  (IN)    the regfile to register.  Pass in NULL for no regfile  */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/


/*=========================================================================*/
int SLPDDatabaseReInit(const char* regfile);
/* Re-initialize the database with changed registrations from a regfile.   */
/*                                                                         */
/* regfile  (IN)    the regfile to register.                               */
/*                                                                         */
/* Returns  - zero on success or non-zero on error.                        */
/*=========================================================================*/

#ifdef DEBUG
/*=========================================================================*/
void SLPDDatabaseDeinit(void);
/* Cleans up all resources used by the database                            */
/*=========================================================================*/


/*=========================================================================*/
void SLPDDatabaseDump(void);
/* Dumps currently valid service registrations present with slpd           */
/*=========================================================================*/
#endif


#endif /* not defined SLPD_DATABASE_H_INCLUDED */
