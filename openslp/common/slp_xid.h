/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_xid.h                                                  */
/*                                                                         */
/* Abstract:    Header file that defines structures and constants and      */
/*              functions that are used to generate SLP transaction ids    */
/*                                                                         */
/* Author(s):   Matthew Peterson                                           */
/*                                                                         */
/***************************************************************************/ 

#if(!defined SLP_XID_H_INCLUDED)
#define SLP_XID_H_INCLUDED

/*=========================================================================*/
extern int G_Xid;
/* Global variable that is incremented before SLPGenerateXid() returns     */
/*=========================================================================*/
                                                                        

/*=========================================================================*/
void SLPXidSeed();
/* Seeds the XID generator.  Should only be called 1 time per process!     */
/* currently called when the first handle is opened.                       */
/*=========================================================================*/


/*=========================================================================*/
unsigned short SLPXidGenerate();
/*                                                                         */
/* Returns: A hopefully unique 16-bit value                                */
/*=========================================================================*/

#endif
