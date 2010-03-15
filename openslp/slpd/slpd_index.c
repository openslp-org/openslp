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

#define _GCC_SOURCE
#include <stdlib.h>
#include <string.h>

#include "slpd_index.h"

#ifdef DEBUG
#include <stdio.h>
#include "slpd_log.h"

#ifdef CHECKING
int check_tree(IndexTreeNode *root_node);
#endif

#endif

#define min(_a,_b)   (((_a) < (_b)) ? (_a) : (_b))
#define max(_a,_b)   (((_a) > (_b)) ? (_a) : (_b))

/* Returned in place of a valid depth value, which must be >=0 */
#define INSERT_FAILED   -1

static int compare_string(
   size_t len1,
   const char *str1,
   size_t len2,
   const char *str2)
{
   size_t len = min(len1, len2);
   int result = memcmp(str1, str2, len);
   if (result != 0)
      return result;
   return (int)len1 - (int)len2;
}

/** Compares the first string to the leading part of the second string
 *
 */
static int compare_string_leading(
   size_t len1,
   const char *str1,
   size_t len2,
   const char *str2)
{
   size_t len = min(len1, len2);
   int result = compare_string(len, str1, len, str2);
   if (result == 0)
      return (int)len1 - (int)len;
   return result;
}

/** Finds a location in a value set.
 *
 * Performs a linear scan of the list to match the given location.
 * 
 * @param[in] pvalueset - pointer to the value set to be searched.
 * @param[in] p - The "location" value to be matched.
 * 
 * @return Pointer to the value entry containing the location, or zero if the
 * location was not found.
 */
IndexTreeValue *find_in_value_set(IndexTreeValue *pvalueset, void *p)
{
   IndexTreeValue *pvalue;
   for (pvalue = pvalueset; pvalue; pvalue = pvalue->next)
   {
      if (pvalue->p == p)
         break;
   }
   return pvalue;
}

/** Removes an entry from a value set.
 *
 * Adjust the backward pointer of the following node.  If there is a preceding
 * node, adjust its forward pointer, otherwise update the valueset pointer.
 * 
 * @param[in] pvalueset - pointer to the value set to be searched.
 * @param[in] pvalue - the entry to be removed.
 * 
 * @return Pointer to the new valueset (possibly empty).
 */
static IndexTreeValue *remove_from_value_set(IndexTreeValue *pvalueset, IndexTreeValue *pvalue)
{
   if (pvalue->next)
      pvalue->next->prev = pvalue->prev;
   if (pvalue->prev)
      pvalue->prev->next = pvalue->next;
   else
      pvalueset = pvalue->next;
   return pvalueset;
}

/** Frees up a value entry that is no longer referenced.
 *
 * Frees the value entry.
 * 
 * @param[in] entry - a pointer to the entry to be freed.
 * 
 * @return None
 */
static void free_index_tree_value(IndexTreeValue * entry)
{
   free(entry);
}

/** Creates a new value entry for the given string value.
 *
 * Allocates and initialises a new value entry, and fills it in with the given
 * string value.
 * 
 * @param[in] value_str_len - length of the string to be added.
 * @param[in] value_str - A pointer to string to be added.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return Pointer to the new entry, or zero if there is an error creating
 *    the entry (memory allocation failure)
 */
static IndexTreeValue *create_index_tree_value(
   void *p)
{
   IndexTreeValue *new_value = (IndexTreeValue *)malloc(sizeof (IndexTreeValue));
   if (new_value)
   {
      new_value->next = (IndexTreeValue *)0;
      new_value->prev = (IndexTreeValue *)0;
      new_value->p = p;
   }
   return new_value;
}

#ifdef DEBUG
static int count_values(IndexTreeNode *node)
{
   int count = 0;
   IndexTreeValue *value;
   for (value = node->value; value; value = value->next)
      ++count;
   return count;
}
#endif

static int compare_with_tree_node(
   size_t value_str_len,
   const char *value_str,
   IndexTreeNode *pnode)
{
   return compare_string(value_str_len, value_str, pnode->value_str_len, pnode->value_str);
}

static int compare_with_tree_node_leading(
   size_t value_str_len,
   const char *value_str,
   IndexTreeNode *pnode)
{
   return compare_string_leading(value_str_len, value_str, pnode->value_str_len, pnode->value_str);
}

/** Creates a new index tree node for the given string value.
 *
 * Allocates and initialises a new tree node, and adds the single string value to it.
 * 
 * @param[in] parent_node - a pointer to the node that is to be the parent of the new node.
 * @param[in] value_str_len - length of the string to be added.
 * @param[in] value_str - A pointer to string to be added.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return Pointer to the new node, or zero if there is an error creating
 *    the node (memory allocation failure)
 */
static IndexTreeNode *create_index_tree_node(
   IndexTreeNode *parent_node,
   size_t value_str_len,
   const char *value_str,
   void *p)
{
   IndexTreeNode *new_node = (IndexTreeNode *)malloc(sizeof (IndexTreeNode) + value_str_len);
   if (new_node)
   {
      IndexTreeValue *new_value = create_index_tree_value(
         p);
      if (!new_value)
      {
         free(new_node);
         new_node = (IndexTreeNode *)0;
      }
      else
      {
         new_node->parent_node = parent_node;
         new_node->left_node = (IndexTreeNode *)0;
         new_node->left_depth = 0;
         new_node->value = new_value;
         new_node->right_node = (IndexTreeNode *)0;
         new_node->right_depth = 0;
         new_node->value_str_len = value_str_len;
         memcpy(new_node->value_str, value_str, value_str_len);
      }
   }
   return new_node;
}

/** Frees up a tree node that is no longer referenced.
 *
 * Frees the tree node structure.
 * 
 * @param[in] node - a pointer to the node to be freed.
 * 
 * @return None
 */
static void free_index_tree_node(IndexTreeNode * node)
{
   free(node);
}

/** Inserts a new record into the value set.
 *
 * Adds the string value into the value set for the root node.
 * 
 * @param[in] root_value - a pointer to the value set.
 * @param[in] value_str_len - length of the string to be added.
 * @param[in] value_str - A pointer to string to be added.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return New value set pointer, or zero if there is an error adding the
 *    string to the value set (memory allocation failure)
 */
static IndexTreeValue *index_value_insert(
   IndexTreeValue *root_value,
   void *p)
{
   IndexTreeValue *new_value = (IndexTreeValue *)malloc(sizeof (IndexTreeValue));
   if (new_value)
   {
      new_value->p = p;
      new_value->prev = (IndexTreeValue *)0;
      /* Add the new value at the start of the list */
      new_value->next = root_value;
      root_value->prev = new_value;
      return new_value;    /* new head of list */
   }
   return root_value;
}

/** Adds the value to the given node. 
 *
 * Adds the string value into the value set for the root node.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be added.
 * @param[in] value_str - A pointer to string to be added.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return New root value, zero if there is an error adding the string to the
 *    node (memory allocation failure)
 */
static IndexTreeValue *index_node_insert_value(
   IndexTreeNode *root_node,
   void *p)
{
   IndexTreeValue *root_value = index_value_insert(
      root_node->value,
      p);
   if (root_value)
   {
      root_node->value = root_value;
   }
   return root_value;
}

char *strdup_len(const char *str, size_t max_len)
{
   int len;
   const char *p;
   char *result;

   for (p = str; max_len && *p; --max_len, p++)
      ;
   len = p - str;
   result = (char *)malloc(len+1);
   if (result)
   {
      memcpy(result, str, len);
      result[len] = '\0';
   }
   return result;
}

/** Return the value string from this node.
 *
 * If the node pointer is null, return an error string.
 * 
 * @param[in] pnode - a pointer to the tree node.
 * 
 * @return malloc'd copy of string value (must be freed by caller)
 */
char *get_value_string(IndexTreeNode *pnode)
{
   if (!pnode)
      return strdup("!! ERROR: No node !!");
   return strdup_len(pnode->value_str, pnode->value_str_len);
}

/** Calculate the depth of a (possibly null) tree. 
 *
 * If the root node is null, the depth is zero, otherwise it is the larger
 * of the depths of the two subtrees, plus one for the node itself.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * 
 * @return Depth of the tree (>= 0)
 */
int tree_depth(IndexTreeNode *root_node)
{
   if (root_node)
   {
      return 1 + max(root_node->left_depth, root_node->right_depth);
   }
   return 0;
}

/** Re-balance the (sub-)tree, if necessary. 
 *
 * Checks whether the tree is balanced, and performs rotations as required to
 * re-balance if necessary.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * 
 * @return New root node of the tree
 */
static IndexTreeNode *rebalance_tree(
   IndexTreeNode *root_node)
{
   /* Check whether we need to re-balance the (sub-)tree */
   if (root_node->left_depth - root_node->right_depth == 2)
   {
      /* Need to perform a right-rotation */
      IndexTreeNode *parent_node;
      IndexTreeNode *pivot = root_node->left_node;
      if (tree_depth(pivot->right_node) > tree_depth(pivot->left_node))
      {
         /* Rotate the pivot tree left first, as the right subtree will be
          * going beneath the current root node which will be going beneath
          * the pivot node, so will increase the depth.  By rotating the
          * subtrees, we ensure that the shallower sub-tree has its depth
          * increased, to minimise the difference in depth
          */
         IndexTreeNode *new_pivot = pivot->right_node;
         parent_node = pivot->parent_node;
         pivot->right_node = new_pivot->left_node;
         if (pivot->right_node)
            pivot->right_node->parent_node = pivot;
         pivot->right_depth = new_pivot->left_depth;
         pivot->parent_node = new_pivot;
         new_pivot->left_node = pivot;
         new_pivot->left_depth = tree_depth(pivot);
         new_pivot->parent_node = parent_node;
         pivot = new_pivot;
      }
      /* Now perform the right rotation */
      parent_node = root_node->parent_node;
      root_node->left_node = pivot->right_node;
      if (root_node->left_node)
         root_node->left_node->parent_node = root_node;
      root_node->left_depth = pivot->right_depth;
      root_node->parent_node = pivot;
      pivot->right_node = root_node;
      pivot->right_depth = tree_depth(root_node);
      pivot->parent_node = parent_node;
      root_node = pivot;
   }
   else if (root_node->right_depth - root_node->left_depth == 2)
   {
      /* Need to perform a left-rotation */
      IndexTreeNode *parent_node;
      IndexTreeNode *pivot = root_node->right_node;
      if (tree_depth(pivot->left_node) > tree_depth(pivot->right_node))
      {
         /* Rotate the pivot tree right first, as the left subtree will be
          * going beneath the current root node which will be going beneath
          * the pivot node, so will increase the depth.  By rotating the
          * subtrees, we ensure that the shallower sub-tree has its depth
          * increased, to minimise the difference in depth
          */
         IndexTreeNode *new_pivot = pivot->left_node;
         parent_node = pivot->parent_node;
         pivot->left_node = new_pivot->right_node;
         if (pivot->left_node)
            pivot->left_node->parent_node = pivot;
         pivot->left_depth = new_pivot->right_depth;
         pivot->parent_node = new_pivot;
         new_pivot->right_node = pivot;
         new_pivot->right_depth = tree_depth(pivot);
         new_pivot->parent_node = parent_node;
         pivot = new_pivot;
      }
      /* Now perform the left rotation */
      parent_node = root_node->parent_node;
      root_node->right_node = pivot->left_node;
      if (root_node->right_node)
         root_node->right_node->parent_node = root_node;
      root_node->right_depth = pivot->left_depth;
      root_node->parent_node = pivot;
      pivot->left_node = root_node;
      pivot->left_depth = tree_depth(root_node);
      pivot->parent_node = parent_node;
      root_node = pivot;
   }
#if defined(DEBUG) && defined(CHECKING)
   if (!check_tree(root_node)) {
      SLPDLog("Index tree check fails after rebalancing tree %p\n", root_node);
   }
#endif
   return root_node;
}

/** Insert entry into the index tree. 
 *
 * Compares the value of the string with the value in the root node, then
 * either recursively inserts the value in the left or right sub-trees, or
 * adds the value to the root node.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be inserted.
 * @param[in] value_str - A pointer to string to be inserted.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return New root node of the tree, or null if there is an error inserting
 *    the string value (memory allocation failure)
 */
static IndexTreeNode *index_tree_insert(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   void *p)
{
   int cmp = compare_with_tree_node(value_str_len, value_str, root_node);
   if (cmp < 0)
   {
      /* Put it into the left-hand subtree */
      if (root_node->left_node)
      {
         IndexTreeNode *new_subtree_root = index_tree_insert(root_node->left_node, value_str_len, value_str, p);
         if (!new_subtree_root)
            /* Error return */
            return (IndexTreeNode *)0;
         root_node->left_node = new_subtree_root;
         root_node->left_depth = tree_depth(new_subtree_root);
      }
      else
      {
         IndexTreeNode *new_node = create_index_tree_node(root_node, value_str_len, value_str, p);
         if (!new_node)
            return (IndexTreeNode *)0;
         root_node->left_node = new_node;
         root_node->left_depth = 1;
      }
   }
   else if (cmp > 0)
   {
      /* Put it into the right-hand subtree */
      if (root_node->right_node)
      {
         IndexTreeNode *new_subtree_root = index_tree_insert(root_node->right_node, value_str_len, value_str, p);
         if (!new_subtree_root)
            /* Error return */
            return (IndexTreeNode *)0;
         root_node->right_node = new_subtree_root;
         root_node->right_depth = tree_depth(new_subtree_root);
      }
      else
      {
         IndexTreeNode *new_node = create_index_tree_node(root_node, value_str_len, value_str, p);
         if (!new_node)
            return (IndexTreeNode *)0;
         root_node->right_node = new_node;
         root_node->right_depth = 1;
      }
   }
   else
   {
      /* Add it to the current node */
      if (!index_node_insert_value(root_node, p))
         return (IndexTreeNode *)0;
   }

   /* Perform any re-balancing required */
   return rebalance_tree(root_node);
}

/** Adds an entry to the index tree.
 *
 * If the tree is empty, creates a single node for the entry, otherwise it
 * inserts the entry into the given tree
 * 
 * @param[in] index_root_node - a pointer to the root of the index tree.
 * @param[in] value_str_len - length of the string to be inserted.
 * @param[in] value_str - A pointer to string to be inserted.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return New root node of the tree, or null if there is an error inserting
 *    the string value (memory allocation failure)
 */
IndexTreeNode *add_to_index(
   IndexTreeNode *index_root_node,
   size_t value_str_len,
   const char *value_str,
   void *p)
{
   if (index_root_node)
      index_root_node = index_tree_insert(index_root_node, value_str_len, value_str, p);
   else
      index_root_node = create_index_tree_node(index_root_node, value_str_len, value_str, p);
#if defined(DEBUG) && defined(CHECKING)
   if (!check_tree(index_root_node))
   {
      char buffer[200];
      strncat(buffer, value_str, value_str_len);
      SLPDLog("Index tree check fails adding value %s\n", buffer);
   }
#endif
   return index_root_node;
}

/** Searches the index tree for the given string value.
 *
 * Compares the given value with the value in the given node, and returns the
 * value set for the current node if there is a match.  Otherwise it performs
 * a recursive search of the left or right sub-tree depending on whether the
 * value is less than or greater than the value in the node.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be inserted.
 * @param[in] value_str - A pointer to string to be inserted.
 * 
 * @return Pointer to the value set (list of locations) for the given value, or
 *    null if not found
 */
IndexTreeValue *find_in_index(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str)
{
   int cmp;
   
   if (!root_node)
      return (IndexTreeValue *)0;

   cmp = compare_with_tree_node(value_str_len, value_str, root_node);
   if (cmp < 0)
   {
      return find_in_index(root_node->left_node, value_str_len, value_str);
   }
   else if (cmp > 0)
   {
      return find_in_index(root_node->right_node, value_str_len, value_str);
   }
   else
   {
      return root_node->value;
   }
}

/** Extract the leftmost entry of the given sub-tree.
 *
 * First locate the leftmost entry.  If this is the root node, then the new
 * root node is the node's right sub-tree.  Otherwise, the parent's left node
 * becomes the leftmost node's right sub-tree, and we traverse the tree to the
 * root node, rebalancing as we go.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] leftmost_node - pointer to the location to store the leftmost node's pointer.
 * 
 * @return New root node of the (sub-)tree, or null if the tree is now empty
 */
static IndexTreeNode *extract_leftmost(IndexTreeNode *root_node, IndexTreeNode **leftmost_node)
{
   IndexTreeNode *parent_node, *current_node;
   *leftmost_node = root_node;
   while ((*leftmost_node)->left_node)
      *leftmost_node = (*leftmost_node)->left_node;
   /* Remove the leftmost node from the tree */
   if (*leftmost_node == root_node)
   {
      /* Just return the right sub-tree  - already balanced, or empty */
      return (*leftmost_node)->right_node;
   }
   else
   {
      /* Move the leftmost node's right sub-tree to the parent's left sub-tree */
      current_node = (*leftmost_node)->parent_node;
      current_node->left_node = (*leftmost_node)->right_node;
      if (current_node->left_node)
         current_node->left_node->parent_node = current_node;
      current_node->left_depth = tree_depth(current_node->left_node);
   }
   while (current_node != root_node)
   {
      parent_node = current_node->parent_node;
      parent_node->left_node = rebalance_tree(current_node);
      current_node = parent_node;
   }
   return rebalance_tree(current_node);
}

/** Extract the rightmost entry of the given sub-tree.
 *
 * First locate the rightmost entry.  If this is the root node, then the new
 * root node is the node's left sub-tree.  Otherwise, the parent's right node
 * becomes the rightmost node's left sub-tree, and we traverse the tree to the
 * root node, rebalancing as we go.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] rightmost_node - pointer to the location to store the rightmost node's pointer.
 * 
 * @return New root node of the (sub-)tree, or null if the tree is now empty
 */
static IndexTreeNode *extract_rightmost(IndexTreeNode *root_node, IndexTreeNode **rightmost_node)
{
   IndexTreeNode *parent_node, *current_node;
   *rightmost_node = root_node;
   while ((*rightmost_node)->right_node) {
      *rightmost_node = (*rightmost_node)->right_node;
   }
   /* Remove the rightmost node from the tree */
   if (*rightmost_node == root_node)
   {
      /* Just return the left sub-tree  - already balanced, or empty */
      return (*rightmost_node)->left_node;
   }
   else
   {
      /* Move the rightmost node's left sub-tree to the parent's right sub-tree */
      current_node = (*rightmost_node)->parent_node;
      current_node->right_node = (*rightmost_node)->left_node;
      if (current_node->right_node)
         current_node->right_node->parent_node = current_node;
      current_node->right_depth = tree_depth(current_node->right_node);
   }
   while (current_node != root_node)
   {
      parent_node = current_node->parent_node;
      parent_node->right_node = rebalance_tree(current_node);
      current_node = parent_node;
   }
   return rebalance_tree(current_node);
}

/** Delete entry from the index tree.
 *
 * Compares the value of the string with the value in the root node, then
 * either recursively deletes the value in the left or right sub-trees, or
 * deletes the value from the root node.
 * If the root_node needs to be deleted, then create a new tree from its
 * subtrees, re-balancing where necessary.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be removed.
 * @param[in] value_str - A pointer to string to be removed.
 * @param[in] p - The "location" value associated with the string.
 * 
 * @return New root node of the tree, or null if the tree is now empty
 */
IndexTreeNode *index_tree_delete(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   void *p)
{
   int cmp;
   
   if (!root_node)
      /* Shouldn't really happen, but nothing to do */
      return root_node;

   cmp = compare_with_tree_node(value_str_len, value_str, root_node);
   if (cmp < 0)
   {
      root_node->left_node = index_tree_delete(root_node->left_node, value_str_len, value_str, p);
      root_node->left_depth = tree_depth(root_node->left_node);
      if (root_node->left_node)
         root_node->left_node->parent_node = root_node;
   }
   else if (cmp > 0)
   {
      root_node->right_node = index_tree_delete(root_node->right_node, value_str_len, value_str, p);
      root_node->right_depth = tree_depth(root_node->right_node);
      if (root_node->right_node)
         root_node->right_node->parent_node = root_node;
   }
   else
   {
      IndexTreeNode *replacement_node;

      /* Find the given location in the value list */
      IndexTreeValue *entry = find_in_value_set(root_node->value, p);
      if (!entry)
         /* Shouldn't really happen, but nothing to do */
         return root_node;

      root_node->value = remove_from_value_set(root_node->value, entry);
      free_index_tree_value(entry);

      if (root_node->value)
         /* All we've done is remove one of the values in the set for this node,
          * so the tree structure does not need to be changed
          */
         return root_node;

      /* The node needs to be deleted, so replace this node by the leftmost
       * node of the right sub-tree, or the rightmost mode of the left
       * sub-tree, depending on which has the greater depth
       */
      if (root_node->left_depth > root_node->right_depth)
      {
         root_node->left_node = extract_rightmost(root_node->left_node, &replacement_node);
         root_node->left_depth = tree_depth(root_node->left_node);
      }
      else if (root_node->right_node)
      {
         root_node->right_node = extract_leftmost(root_node->right_node, &replacement_node);
         root_node->right_depth = tree_depth(root_node->right_node);
      }
      else
      {
         /* Both sub-trees are empty  ie. this is a leaf node */
         free_index_tree_node(root_node);
         return (IndexTreeNode *)0;
      }
      replacement_node->right_node = root_node->right_node;
      replacement_node->right_depth = root_node->right_depth;
      if (replacement_node->right_node)
         replacement_node->right_node->parent_node = replacement_node;
      replacement_node->left_node = root_node->left_node;
      replacement_node->left_depth = root_node->left_depth;
      if (replacement_node->left_node)
         replacement_node->left_node->parent_node = replacement_node;
      free_index_tree_node(root_node);
      root_node = replacement_node;
   }
   root_node = rebalance_tree(root_node);
#if defined(DEBUG) && defined(CHECKING)
   if (!check_tree(root_node))
   {
      SLPDLog("Index tree check fails deleting from tree %p\n", root_node);
   }
#endif
   return root_node;
}

/** Call a function for each of the values in the given value set.
 *
 * @param[in] tree_value - a pointer to the first item in the value set.
 * @param[in] callback - A pointer to the function to be called for matching entries.
 * @param[in] cookie - A pointer to the context needed by the callback.
 * 
 * @return Number of matching values (calls)
 */
size_t value_set_call(IndexTreeValue *tree_value, pIndexTreeCallback callback, void *cookie)
{
   size_t num_matches = 0;
   while (tree_value)
   {
      ++num_matches;
      (*callback)(cookie, tree_value->p);
      tree_value = tree_value->next;
   }
   return num_matches;
}

/** Search the index tree for entries matching the given string value
 *  and call a function for each of them, in order.
 *
 * Compares the given value with the value in the given node (full text match).
 * Performs the operation on the left sub-tree if <, calls the callback for
 * the current node's value list if ==, and performs the operation on the right
 * sub-tree if >.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be matched.
 * @param[in] value_str - A pointer to string to be matched.
 * @param[in] callback - A pointer to the function to be called for matching entries.
 * @param[in] cookie - A pointer to the context needed by the callback.
 * 
 * @return Number of matching values (calls)
 */
size_t find_and_call(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   pIndexTreeCallback callback,
   void *cookie)
{
   int cmp;
   size_t num_matches = 0;
   
   if (!root_node)
      return 0;

   cmp = compare_with_tree_node(value_str_len, value_str, root_node);
   if (cmp < 0)
      num_matches += find_and_call(root_node->left_node, value_str_len, value_str, callback, cookie);
   else if (cmp == 0)
      num_matches += value_set_call(root_node->value, callback, cookie);
   else
      /* cmp > 0 */
      num_matches += find_and_call(root_node->right_node, value_str_len, value_str, callback, cookie);
   return num_matches;
}

/** Search the index tree for entries starting with the given string value
 *  and call a function for each of them, in order.
 *
 * Compares the given value with the value in the given node (leading text match).
 * Performs the operation on the left sub-tree if <=, calls the callback for
 * the current node's value list if ==, and performs the operation on the right
 * sub-tree if >=.
 * Note that all three will be done if the current node matches.
 * 
 * @param[in] root_node - a pointer to the root of the (sub-)tree.
 * @param[in] value_str_len - length of the string to be matched.
 * @param[in] value_str - A pointer to string to be matched.
 * @param[in] callback - A pointer to the function to be called for matching entries.
 * @param[in] cookie - A pointer to the context needed by the callback.
 * 
 * @return Number of matching values (calls)
 */
size_t find_leading_and_call(
   IndexTreeNode *root_node,
   size_t value_str_len,
   const char *value_str,
   pIndexTreeCallback callback,
   void *cookie)
{
   int cmp;
   size_t num_matches = 0;
   
   if (!root_node)
      return 0;

   cmp = compare_with_tree_node_leading(value_str_len, value_str, root_node);
   if (cmp <= 0)
   {
      num_matches += find_leading_and_call(root_node->left_node, value_str_len, value_str, callback, cookie);
   }
   if (cmp == 0)
   {
      num_matches += value_set_call(root_node->value, callback, cookie);
   }
   if (cmp >= 0)
   {
      num_matches += find_leading_and_call(root_node->right_node, value_str_len, value_str, callback, cookie);
   }
   return num_matches;
}

#ifdef DEBUG
void print_tree(IndexTreeNode *root_node, unsigned depth)
{
   char *value_str;
   int i;
   char buffer[256];

   if (!root_node)
      return;
   print_tree(root_node->left_node, depth+1);
   for (i = 0; i < depth; i++)
      SLPDLog("   ");
   value_str = get_value_string(root_node);
   sprintf(buffer, "%03d %3d %s\n", tree_depth(root_node), count_values(root_node), value_str);
   SLPDLog(buffer);
   free(value_str);
   print_tree(root_node->right_node, depth+1);
}
#endif /* DEBUG */

#if defined(DEBUG) && defined(CHECKING)
int check_tree(IndexTreeNode *root_node)
{
   char buffer[256];

   if (!root_node)
      return 1;
   if (root_node->left_node) {
      if (root_node->left_node->parent_node != root_node) {
          SLPDLog("Invalid parent link in %p - parent is %p - should be %p\n", root_node->left_node, root_node->left_node->parent_node, root_node);
          return 0;
      }
   }
   if (!check_tree(root_node->left_node))
      return 0;
   if (root_node->right_node) {
      if (root_node->right_node->parent_node != root_node) {
          SLPDLog("Invalid parent link in %p - parent is %p - should be %p\n", root_node->right_node, root_node->right_node->parent_node, root_node);
          return 0;
      }
   }
   if (!check_tree(root_node->right_node))
      return 0;
   return 1;
}
#endif /* CHECKING */
