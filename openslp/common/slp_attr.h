/*******************************************************************
 *  Description: encode/decode attribute lists
 *
 *  Originated: 03-07-2000 
 *	Original Author: Mike Day - md@soft-hackle.net
 *  Project: 
 *
 *  $Header$
 *
 *  Copyright (C) Michael Day, 1999-2001 
 *
 *  This program is free software; you can redistribute it and/or 
 *  modify it under the terms of the GNU General Public License 
 *  as published by the Free Software Foundation; either version 2 
 *  of the License, or (at your option) any later version. 
 *
 *  This program is distributed in the hope that it will be useful, 
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
 *  GNU General Public License for more details. 
 *
 *  You should have received a copy of the GNU General Public License 
 *  along with this program; if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. 

 *******************************************************************/
#ifndef SLP_ATTR_H_INCLUDED
#define SLP_ATTR_H_INCLUDED

typedef enum attrTypes
{
    head = -1,
    string,
    integer,
    boolean,
    opaque,
    tag
}lslpTypes;

typedef union lslp_attr_value 
{
    char *stringVal;
    unsigned long intVal;
    int boolVal;
    void *opaqueVal;
}lslpAttrVal;

typedef struct lslp_attr_list 
{
    struct lslp_attr_list *next;
    struct lslp_attr_list *prev;
    int isHead;
    unsigned char *name;
    lslpTypes type;
    lslpAttrVal val;
}lslpAttrList;

lslpAttrList *lslpAllocAttr(char *name, lslpTypes type, void *val, int len);
lslpAttrList *lslpAllocAttrList(void);
void lslpFreeAttr(lslpAttrList *attr);
void lslpFreeAttrList(lslpAttrList *list);
lslpAttrList *lslpDecodeAttrString(char *s);

#endif /* SLP_ATTR_H_INCLUDED */



