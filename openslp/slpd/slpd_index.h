/*-------------------------------------------------------------------------
 * Copyright (C) 2009 Thales Underwater Systems Ltd
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
 *    Neither the name of Thales Underwater Systems nor the names of its
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

#ifndef SLPD_INDEX_H_INCLUDED
#define SLPD_INDEX_H_INCLUDED

#include "slpd.h"

/******************************************************************************
 *
 *                             Individual values
 *  A value is a string that is associated with a specific object
 *  A list of values is maintained for each node, being the set of objects with
 *  identical strings.
 *****************************************************************************/

typedef struct _IndexTreeValue
{
   struct _IndexTreeValue *next;    /* pointer to the next value in the list */
   struct _IndexTreeValue *prev;    /* pointer to the previous value in the list */
   void *p;                         /* pointer to the object */
} IndexTreeValue;

/******************************************************************************
 *
 *                             Individual tree nodes
 *  A tree node is a node in a balanced binary tree.
 *  Each node is linked with its parent node, and with the root nodes of
 *  subtrees which contain objects with strings lexically less than (left) or
 *  greater than (right) the string associated with this node.  In order for the
 *  tree to be balanced, the node also maintains the depth of each of its
 *  subtrees, and the insertion/deletion operations maintain the balance by
 *  ensuring that the difference between the left and right depths is no more
 *  than 1.
 *  Operations on the tree generally involve standard recursive tree traversal.
 *****************************************************************************/

typedef struct _IndexTreeNode
{
   struct _IndexTreeNode *parent_node; /* Pointer to the node's parent - null if this is the root node */
   struct _IndexTreeNode *left_node;   /* Pointer to the root node of the left sub-tree, if any */
   unsigned left_depth;                /* Depth of the left sub-tree */
   IndexTreeValue *value;              /* Pointer to the first in the list of objects associated with this node */
   struct _IndexTreeNode *right_node;  /* Pointer to the root node of the right sub-tree, if any */
   unsigned right_depth;               /* Depth of the right sub-tree */
   size_t value_str_len;               /* Length of the string */
   char value_str[1];                  /* Copy of the string */
} IndexTreeNode;

int tree_depth(IndexTreeNode *root_node);
char *get_value_string(IndexTreeNode *pnode);

typedef void (*pIndexTreeCallback)(void *cookie, void *p);

IndexTreeNode *add_to_index(
   IndexTreeNode *index_root_node,
   size_t value_str_len,
   const char *value_str,
   void *p);

IndexTreeValue *find_in_index(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str);

IndexTreeNode *index_tree_delete(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   void *p);

size_t find_and_call(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   pIndexTreeCallback callback,
   void *cookie);

size_t find_leading_and_call(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   pIndexTreeCallback callback,
   void *cookie);

#ifdef DEBUG
void print_tree(IndexTreeNode *root_node, unsigned depth);
#endif

#endif   /* SLPD_INDEX_H_INCLUDED */

