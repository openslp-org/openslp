/***************************************************************************/
/*                                                                         */
/* Project:     OpenSLP - OpenSource implementation of Service Location    */
/*              Protocol                                                   */
/*                                                                         */
/* File:        slp_v1message.h                                            */
/*                                                                         */
/* Abstract:    Header file that defines prototypes for SLPv1 messages     */
/*                                                                         */
/*-------------------------------------------------------------------------*/
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

#if(!defined SLP_V1MESSAGE_H_INCLUDED)
#define SLP_V1MESSAGE_H_INCLUDED

/*=========================================================================*/
/* SLP language encodings for SLPv1 compatibility                          */
/*=========================================================================*/
#define SLP_CHAR_ASCII          3
#define SLP_CHAR_UTF8           106
#define SLP_CHAR_UNICODE16      1000
#define SLP_CHAR_UNICODE32      1001

/*=========================================================================*/
/* SLPv1 Flags                                                             */
/*=========================================================================*/
#define SLPv1_FLAG_OVERFLOW         0x80
#define SLPv1_FLAG_MONOLING         0x40
#define SLPv1_FLAG_URLAUTH          0x20
#define SLPv1_FLAG_ATTRAUTH         0x10
#define SLPv1_FLAG_FRESH            0x08

/*=========================================================================*/
/* Prototypes for SLPv1 functions                                          */
/*=========================================================================*/

extern int v1ParseHeader(SLPBuffer buffer, SLPHeader* header);

extern int SLPv1MessageParseBuffer(SLPBuffer buffer, SLPHeader *header,
				   SLPMessage message);

extern int SLPv1AsUTF8(int encoding, char *string, int *len);

extern int SLPv1ToEncoding(char *string, int *len, int encoding, 
						   const char *utfstring, int utflen);

#endif
