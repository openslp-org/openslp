/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_property.h                                             */
/*                                                                         */
/* Abstract:    Internal declarations for SLP properties                   */
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

#if(!defined SLP_PROPERTY_H_INCLUDED)
    #define SLP_PROPERTY_H_INCLUDED

    #include <slp_linkedlist.h>

/*=========================================================================*/
typedef struct _SLPProperty
/*=========================================================================*/
{
    SLPListItem        listitem;
    char*           propertyName;
    char*           propertyValue;
}SLPProperty;


/*=========================================================================*/
const char* SLPPropertyGet(const char* pcName); 
/*                                                                         */
/* Returns the value of the corresponding SLP property name.  The returned */
/* string is owned by the library and MUST NOT be freed.                   */
/*                                                                         */
/* pcName   Null terminated string with the property name, from            */
/*          Section 2.1 of RFC 2614.                                       */
/*                                                                         */
/* Returns: If no error, returns a pointer to a character buffer containing*/
/*          the property value.  If the property was not set, returns the  */
/*          default value.  If an error occurs, returns NULL. The returned */
/*          string MUST NOT be freed.                                      */
/*=========================================================================*/


/*=========================================================================*/
int SLPPropertySet(const char *pcName,                                     
                   const char *pcValue);
/*                                                                         */
/* Sets the value of the SLP property to the new value.  The pcValue       */
/* parameter should be the property value as a string.                     */
/*                                                                         */
/* pcName   Null terminated string with the property name, from Section 2.1*/
/*          of RFC 2614.                                                   */
/*                                                                         */
/* pcValue  Null terminated string with the property value, in UTF-8       */
/*          character encoding.                                            */
/*=========================================================================*/


/*=========================================================================*/
int SLPPropertyReadFile(const char* conffile);                             
/* Reads and sets properties from the specified configuration file         */
/*                                                                         */
/* conffile     (IN) the path of the config file to read.                  */
/*                                                                         */
/* Returns  -   zero on success. non-zero on error.  Properties will be set*/
/*              to default on error.                                       */
/*=========================================================================*/


/*=========================================================================*/
int SLPPropertyAsBoolean(const char* property);
/*=========================================================================*/


/*=========================================================================*/
int SLPPropertyAsInteger(const char* property);
/*=========================================================================*/


/*=========================================================================*/
int SLPPropertyAsIntegerVector(const char* property, 
                               int* vector, 
                               int vectorsize);
/*=========================================================================*/

    #ifdef DEBUG


/*=========================================================================*/
void SLPPropertyFreeAll();
/*=========================================================================*/
    #endif

#endif 
