/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_property.h                                             */
/*                                                                         */
/* Abstract:    Internal declarations for SLP properties                   */
/*                                                                         */
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/

#if(!defined SLP_PROPERTY_H_INCLUDED)
#define SLP_PROPERTY_H_INCLUDED

#include <slp_linkedlist.h>

/*=========================================================================*/
typedef struct _SLPProperty
/*=========================================================================*/
{
    ListItem        listitem;
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

#endif 
