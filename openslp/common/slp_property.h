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
#endif 
