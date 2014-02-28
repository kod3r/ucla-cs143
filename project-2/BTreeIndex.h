/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#ifndef BTREEINDEX_H
#define BTREEINDEX_H

#include "Bruinbase.h"
#include "PageFile.h"
#include "RecordFile.h"

// @todo: move this error definition to Bruinbase.h
const int RC_INSERT_NEEDS_SPLIT  = -1016;
const int RC_OUT_OF_MEMORY       = -1017;
             
/**
 * The data structure to point to a particular entry at a b+tree leaf node.
 * An IndexCursor consists of pid (PageId of the leaf node) and 
 * eid (the location of the index entry inside the node).
 * IndexCursor is used for index lookup and traversal.
 */
typedef struct {
  // PageId of the index entry
  PageId  pid;  
  // The entry number inside the node
  int     eid;  
} IndexCursor;

/**
 * Implements a B-Tree index for bruinbase.
 * 
 */
class BTreeIndex {
 public:
  BTreeIndex();

  /**
   * Open the index file in read or write mode.
   * Under 'w' mode, the index file should be created if it does not exist.
   * @param indexname[IN] the name of the index file
   * @param mode[IN] 'r' for read, 'w' for write
   * @return error code. 0 if no error
   */
  RC open(const std::string& indexname, char mode);

  /**
   * Close the index file.
   * @return error code. 0 if no error
   */
  RC close();
    
  /**
   * Insert (key, RecordId) pair to the index.
   * @param key[IN] the key for the value inserted into the index
   * @param rid[IN] the RecordId for the record being inserted into the index
   * @return error code. 0 if no error
   */
  RC insert(int key, const RecordId& rid);

  /**
   * Find the leaf-node index entry whose key value is larger than or
   * equal to searchKey and output its location (i.e., the page id of the node
   * and the entry number in the node) as "IndexCursor."
   * IndexCursor consists of pid (page id of the node that contains the 
   * searchKey) and eid (the entry number inside the node)
   * to indicate the location of a particular index entry in the B+tree.
   * Note that, for range queries, we need to scan the B+tree leaf nodes.
   * For example, if the query is "key > 1000", we should scan the leaf
   * nodes starting with the key value 1000. For this reason,
   * this function returns the location of the leaf node entry
   * for a given searchKey, instead of returning the RecordId
   * associated with the searchKey.
   * Using the returned "IndexCursor", you will have to call readForward()
   * to retrieve the actual (key, rid) pair from the index.
   * @param key[IN] the key to find
   * @param cursor[OUT] the cursor pointing to the first index entry
   * with the key value
   * @return error code. 0 if no error.
   */
  RC locate(int searchKey, IndexCursor& cursor) const;

  /**
   * Locates the very first entry in the B+tree
   * @param cursor[OUT] the cursor pointing to the first entry
   * @return 0 on success, or an error code
   */
  RC locateFirstEntry(IndexCursor& cursor) const;

  /**
   * Read the (key, rid) pair at the location specified by the index cursor,
   * and move foward the cursor to the next entry.
   * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
   * @param key[OUT] the key stored at the index cursor location
   * @param rid[OUT] the RecordId stored at the index cursor location
   * @return error code. 0 if no error
   */
  RC readForward(IndexCursor& cursor, int& key, RecordId& rid) const;
  
 private:
  PageFile pf;         /// the PageFile used to store the actual b+tree in disk
  PageId   rootPid;    /// the PageId of the root node

  /**
   * Traverses the B+tree recursively and creates any appropriate nodes along the way.
   * If an insert succeeds without the need for a split, the data will be written on
   * disk. If a split is needed, both the current and sibling nodes will be written.
   * It is the caller's duty to create the proper parent node to hold both.
   * @param nodePid[IN] the PageId of the node at which to start traversal
   * @param key[IN] key to insert
   * @param rid[IN] RecordId to insert
   * @param siblingPid[OUT] PageId of a newly created sibling which needs insertion
   * @param siblingFirstKey[OUT] the key from the newly created sibling which should be inserted
            into the parent node
   * @return 0 on success, RC_INSERT_NEEDS_SPLIT if a sibling node was created, or another RC error
   */
  RC insertRecursively(const PageId& nodePid, const int key, const RecordId& rid, PageId& siblingPid, int& siblingFirstKey);
};

#endif /* BTREEINDEX_H */
