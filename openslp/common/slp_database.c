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

/** An SLP message database. 
 *
 * The SLP message database holds actual SLP "wire" message buffers, as 
 * well as structures that interpret the message buffer. The database 
 * exposes an interface suitable linked-list based implementation.
 *
 * @file       slp_database.c
 * @author     Matthew Peterson, John Calcote (jcalcote@novell.com)
 * @attention  Please submit patches to http://www.openslp.org
 * @ingroup    CommonCode
 */

#include "slp_database.h"
#include "slp_xmalloc.h"

/** Initialize an SLPDatabase.
 *
 * @param[in] database - The database to be initialized.
 * 
 * @return Zero on success, or a non-zero error code.
 */
int SLPDatabaseInit(SLPDatabase* database)
{
    if(database && database->head)
    {
        SLPDatabaseDeinit(database);
    }
    return 0;
}

/** Deinitialize an SLPDatabase.
 *
 * @param[in] database - The database to be deinitialized.
 */
void SLPDatabaseDeinit(SLPDatabase* database)
{
    while(database->count)
    {
        SLPDatabaseEntryDestroy((SLPDatabaseEntry*)SLPListUnlink(database,database->head));
    }
    memset(database,0,sizeof(SLPDatabase));
}

/** Create a new SLP database entry.
 *
 * @param[in] msg - The interpreting message structure for @p buf.
 * @param[in] buf - The message buffer to be converted to an entry.
 * 
 * @return A valid SLP database entry pointer on success; 0 on failure.
 *
 * @note VERY IMPORTANT. The @p msg and especially the @p buf are owned
 *    by the returned SLPDatabaseEntry and MUST NOT be freed by the caller
 *    via SLPMessageFree or SLPBufferFree! Instead, the caller should
 *    use SLPDatabaseEntryDestroy only to free memory.
 */
SLPDatabaseEntry* SLPDatabaseEntryCreate(SLPMessage msg,
                                         SLPBuffer buf)
{
    SLPDatabaseEntry* result;

    result = (SLPDatabaseEntry*)xmalloc(sizeof(SLPDatabaseEntry));
    if(result)
    {
        result->msg = msg;
        result->buf = buf;
    }

    return result;
}

/** Frees resources associated with the specified entry.
 *
 * @param[in] entry - The entry to be destroyed.
 */
void SLPDatabaseEntryDestroy(SLPDatabaseEntry* entry)
{
    SLPMessageFree(entry->msg);
    SLPBufferFree(entry->buf);
    xfree(entry);
}

/** Open a datbase handle.
 *
 * Database handles are used with subsequent calls to SLPDatabaseEnum,
 * SLPDatabaseAdd and SLPDatabaseRemove.
 *
 * @param[in] database - the database object to be opened.
 * 
 * @return A valid database handle, or 0 on error.
 *
 * @remarks It is important to make sure that handles returned by this
 *    function are used and closed as quickly as possible. Future 
 *    versions may use handles to ensure synchronized access to the 
 *    database in multi-threaded environments.
 */
SLPDatabaseHandle SLPDatabaseOpen(SLPDatabase* database)
{
    SLPDatabaseHandle result;
    result = (SLPDatabaseHandle) xmalloc(sizeof(struct _SLPDatabaseHandle));
    if(result)
    {
        result->database = database;
        result->current = (SLPDatabaseEntry*)database->head;
    }
    
    return result;
}

/** Enumerates entries of an SLP database object.
 *
 * @param[in] dh - The database handle to be enumerated.
 * 
 * @return A pointer to an SLP database entry, or 0 if end-of-enumeration
 *    has been reached.
 */
SLPDatabaseEntry* SLPDatabaseEnum(SLPDatabaseHandle dh)
{
    SLPDatabaseEntry* result;    

    result = dh->current;
    if(result)
    {
        dh->current = (SLPDatabaseEntry*)((SLPListItem*)(dh->current))->next;
    }

    return result;
}

/** Reset handle so SLPDatabaseEnum starts at the beginning again.
 *
 * @param[in] dh - The database handle to rewind.
 */
void SLPDatabaseRewind(SLPDatabaseHandle dh)
{
    dh->current = (SLPDatabaseEntry*) dh->database->head;    
}

/** Closes an SLP database handle.
 *
 * @param[in] dh - the database handle to close.
 */
void SLPDatabaseClose(SLPDatabaseHandle dh)
{
    xfree(dh);
    dh = 0;
}

/** Removes the specified entry.
 *
 * @param[in] dh - The database handle to use.
 * @param[in] entry - The entry to be removed.
 * 
 * @remarks During removal, SLPDatabaseEntryDestroy is called on 
 *    @p entry. This means that you must NOT use the entry after it's
 *    been removed.
 */
void SLPDatabaseRemove(SLPDatabaseHandle dh, 
                       SLPDatabaseEntry* entry)
{
    SLPDatabaseEntryDestroy((SLPDatabaseEntry*)(SLPListUnlink(dh->database,(SLPListItem*)entry)));
}

/** Add the specified entry
 *
 * @param[in] dh - The handle to the database.
 * @param[in] entry - The entry to add to @p database.
 * 
 * @remarks Do NOT call SLPDatabaseEntryDestroy on an entry that has 
 *    been added to the database. Instead call SLPDatabaseDeinit or 
 *    SLPDatabaseRemove to free resources associated with an added entry.
 */
void SLPDatabaseAdd(SLPDatabaseHandle dh,
                    SLPDatabaseEntry* entry)
{
    SLPListLinkTail(dh->database, (SLPListItem*)entry);
}

/** Count the number of entries in an SLP database.
 *
 * @param[in] dh - The handle to the database to be counted.
 * 
 * @return The number of entries that are in the database.
 */
int SLPDatabaseCount(SLPDatabaseHandle dh)
{
    return dh->database->count;
}

#ifdef TEST_SLP_DATABASE_TEST
int main(int argc, char* argv[])
{
    SLPDatabase         testdb;
    SLPDatabaseEntry*   testentry;
    
    memset(&testdb, 0, sizeof(testdb));
    SLPDatabaseInit(&testdb);
    SLPDatabaseDeinit(&testdb);
}
#endif

/*=========================================================================*/
