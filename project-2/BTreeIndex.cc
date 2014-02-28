/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */
 
#include "BTreeIndex.h"
#include "BTreeNode.h"

static const PageId ROOT_PID = 0;

using namespace std;

/*
 * BTreeIndex constructor
 */
BTreeIndex::BTreeIndex()
{
    rootPid = INVALID_PID;
}

/*
 * Open the index file in read or write mode.
 * Under 'w' mode, the index file should be created if it does not exist.
 * @param indexname[IN] the name of the index file
 * @param mode[IN] 'r' for read, 'w' for write
 * @return error code. 0 if no error
 */
RC BTreeIndex::open(const string& indexname, char mode)
{
  RC rc;

  if((rc = pf.open(indexname, mode)) < 0) {
    rootPid = INVALID_PID;
    return rc;
  }

  rootPid = ROOT_PID;

  // If the index has not been initialized, write an empty root (leaf) node
  if(pf.endPid() <= 0) {
    BTLeafNode leaf;
    return leaf.write(rootPid, pf);
  }

  return 0;
}

/*
 * Close the index file.
 * @return error code. 0 if no error
 */
RC BTreeIndex::close()
{
  rootPid = INVALID_PID;
  return pf.close();
}

/*
 * Insert (key, RecordId) pair to the index.
 * @param key[IN] the key for the value inserted into the index
 * @param rid[IN] the RecordId for the record being inserted into the index
 * @return error code. 0 if no error
 */
RC BTreeIndex::insert(int key, const RecordId& rid)
{
  RC     rc;
  PageId siblingPid      = INVALID_PID;
  int    siblingFirstKey = INVALID_KEY;

  rc = insertRecursively(rootPid, key, rid, siblingPid, siblingFirstKey);

  // Bail on success or unknown error
  if(rc == 0 || rc != RC_INSERT_NEEDS_SPLIT)
    return rc;

  // The root node needs a split!
  const PageId  oldRootPid = pf.endPid();
  BTRawNonLeaf  oldRoot; // Since we are only reading and writing back data, node leafness doesn't matter
  BTNonLeafNode newRoot;

  // First we save the old root elsewhere on disk,
  // so the new root can be saved as the first page
  if((rc = oldRoot.read(rootPid, pf)) < 0)
    return rc;

  if((rc = oldRoot.write(oldRootPid, pf)) < 0)
    return rc;

  // Next we initialize the new root and save it on disk
  newRoot.initializeRoot(oldRootPid, siblingFirstKey, siblingPid);
  return newRoot.write(rootPid, pf);
}

/*
 * Find the leaf-node index entry whose key value is larger than or 
 * equal to searchKey, and output the location of the entry in IndexCursor.
 * IndexCursor is a "pointer" to a B+tree leaf-node entry consisting of
 * the PageId of the node and the SlotID of the index entry.
 * Note that, for range queries, we need to scan the B+tree leaf nodes.
 * For example, if the query is "key > 1000", we should scan the leaf
 * nodes starting with the key value 1000. For this reason,
 * it is better to return the location of the leaf node entry 
 * for a given searchKey, instead of returning the RecordId
 * associated with the searchKey directly.
 * Once the location of the index entry is identified and returned 
 * from this function, you should call readForward() to retrieve the
 * actual (key, rid) pair from the index.
 * @param key[IN] the key to find.
 * @param cursor[OUT] the cursor pointing to the first index entry
 *                    with the key value.
 * @return error code. 0 if no error.
 */
RC BTreeIndex::locate(int searchKey, IndexCursor& cursor) const
{
  RC rc = 0;
  cursor.pid = rootPid;
  BTRawNonLeaf rawNode; // Used to read in data and determine its type

  while(true) {
    if((rc = rawNode.read(cursor.pid, pf)) < 0)
      return rc;

    // Found a leaf, try to pull the key out
    if(rawNode.isLeaf()) {
      BTLeafNode leaf(rawNode, cursor.pid);
      return leaf.locate(searchKey, cursor.eid);
    } else { // Another non-leaf, keep traversing
      BTNonLeafNode node(rawNode, cursor.pid);
      if((rc = node.locateChildPtr(searchKey, cursor.pid)) < 0)
        return rc;
    }
  }

  return rc;
}

/**
 * Locates the very first entry in the B+tree
 * @param cursor[OUT] the cursor pointing to the first entry
 * @return 0 on success, or an error code
 */
RC BTreeIndex::locateFirstEntry(IndexCursor& cursor) const {
  RC rc = 0;
  BTRawNonLeaf rawNode; // Used to read in data and dtermine its type

  cursor.pid = rootPid;
  cursor.eid = 0;

  while(true) {
    if((rc = rawNode.read(cursor.pid, pf)) < 0)
      return rc;

    if(rawNode.isLeaf()) { // Found a leaf, return its pid or RC_END_OF_TREE if it is empty
      BTLeafNode leaf(rawNode, cursor.pid);
      return leaf.getKeyCount() > 0 ? 0 : RC_END_OF_TREE;
    } else { // Another node, grab its first page pointer
      int key; // Placeholder
      if((rc = rawNode.getPair(0, key, cursor.pid)) < 0)
        return rc;
    }
  }

  return rc;
}

/*
 * Read the (key, rid) pair at the location specified by the index cursor,
 * and move foward the cursor to the next entry.
 * @param cursor[IN/OUT] the cursor pointing to an leaf-node index entry in the b+tree
 * @param key[OUT] the key stored at the index cursor location.
 * @param rid[OUT] the RecordId stored at the index cursor location.
 * @return error code. 0 if no error
 */
RC BTreeIndex::readForward(IndexCursor& cursor, int& key, RecordId& rid) const
{
  RC rc = 0;
  BTLeafNode node;

  while(true) {
    // Bail on load errors
    if((rc = node.read(cursor.pid, pf)) < 0) {
      return rc == RC_WRONG_NODE_TYPE ? RC_INVALID_CURSOR : rc;
    }

    rc = node.readEntry(cursor.eid, key, rid);

    // Exit on success or bail on unknown errors
    if(rc == 0) {
      cursor.eid++; // Increment eid so it points to the next entry
      return 0;
    } else if(rc != RC_NO_SUCH_RECORD) {
      return RC_INVALID_CURSOR;
    }

    // Record doesn't exist in the current node, fetch the next!
    cursor.pid = node.getNextNodePtr();
    cursor.eid = 0;

    // Bail if no more nodes
    // otherwise loop again to read the value
    if(cursor.pid == INVALID_PID)
      return RC_END_OF_TREE;
  }

  return rc;
}

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
RC BTreeIndex::insertRecursively(const PageId& nodePid, const int key, const RecordId& rid, PageId& siblingPid, int& siblingFirstKey) {
  RC rc;
  PageId nextPid;
  int siblingKeyCopy; // see use below

  // Dynamically allocate and free to avoid cluttering the stack during recursive calls
  BTRawNonLeaf  *rawNode        = NULL;
  BTLeafNode    *leaf           = NULL;
  BTLeafNode    *leafSibling    = NULL;
  BTNonLeafNode *nonLeaf        = NULL;
  BTNonLeafNode *nonLeafSibling = NULL;

  if(! (rawNode = new BTRawNonLeaf) ) {
    rc = RC_OUT_OF_MEMORY;
    goto exit;
  }

  if((rc = rawNode->read(nodePid, pf)) < 0) {
    goto exit;
  }

  // Found a leaf, insert directly!
  if(rawNode->isLeaf()) {
    if(! (leaf = new BTLeafNode(*rawNode, nodePid)) ) {
      rc = RC_OUT_OF_MEMORY;
      goto exit;
    }

    rc = leaf->insert(key, rid);

    // Save the data on success, otherwise bail on unknown errors
    if(rc == 0) {
      rc = leaf->write(nodePid, pf);
      goto exit;
    } else if (rc != RC_NODE_FULL) {
      goto exit;
    }

    // Node is full, split!
    if(! (leafSibling = new BTLeafNode) ) {
      rc = RC_OUT_OF_MEMORY;
      goto exit;
    }

    if((rc = leaf->insertAndSplit(key, rid, *leafSibling, siblingFirstKey)) < 0) {
      goto exit;
    }

    // Save the leaf and its sibling on successful split
    siblingPid = pf.endPid();
    if((rc = leafSibling->write(siblingPid, pf)) < 0) {
      goto exit;
    }

    leaf->setNextNodePtr(siblingPid);
    if((rc = leaf->write(nodePid, pf)) < 0) {
      goto exit;
    }

    // Indicate to caller to create the appropriate parent node
    rc = RC_INSERT_NEEDS_SPLIT;
    goto exit;
  }

  // We have a non-leaf node, keep traversing
  if(! (nonLeaf = new BTNonLeafNode(*rawNode, nodePid)) ) {
    rc = RC_OUT_OF_MEMORY;
    goto exit;
  }

  if((rc = nonLeaf->locateChildPtr(key, nextPid)) < 0) {
    goto exit;
  }

  // Delete the in-memory buffers to lower out stack footprint while recursing
  // It is possible that we may have to refetch the nodes to update them if a split
  // occured down the tree, but it is very unlikely that we will have to cache
  // every single non-leaf node along the way down the stack. Thus we accept the possible
  // penalty of incurring an extra disk IO in the event that the node is no longer in the cache
  delete rawNode;
  rawNode = NULL;

  delete nonLeaf;
  nonLeaf = NULL;

  rc = insertRecursively(nextPid, key, rid, siblingPid, siblingFirstKey);
  if(rc == 0 || rc != RC_INSERT_NEEDS_SPLIT) {
    goto exit; // Success or unknown error
  }

  // Lower level split, reload and update the current node!
  if(! (nonLeaf = new BTNonLeafNode) ) {
    rc = RC_OUT_OF_MEMORY;
    goto exit;
  }

  if((rc = nonLeaf->read(nodePid, pf)) < 0) {
    goto exit;
  }

  rc = nonLeaf->insert(siblingFirstKey, siblingPid);

  // Save on success or bail on error
  if(rc == 0) {
    rc = nonLeaf->write(nodePid, pf);
    goto exit;
  } else if(rc != RC_NODE_FULL) {
    goto exit;
  }

  // Current node is full and needs a split!
  if(! (nonLeafSibling = new BTNonLeafNode)) {
    rc = RC_OUT_OF_MEMORY;
    goto exit;
  }

  // Copy the sibling key to avoid potential issues with using the value
  // from a non-const parameter passed by reference
  siblingKeyCopy = siblingFirstKey;
  if((rc = nonLeaf->insertAndSplit(key, siblingKeyCopy, *nonLeafSibling, siblingFirstKey)) < 0) {
    goto exit;
  }

  // Split successfull, save the node and its sibling
  if((rc = nonLeaf->write(nodePid, pf)) < 0) {
    goto exit;
  }

  siblingPid = pf.endPid();
  if((rc = nonLeafSibling->write(siblingPid, pf)) < 0) {
    goto exit;
  }

  // Indicate to caller to create the appropriate parent node
  rc = RC_INSERT_NEEDS_SPLIT;
  goto exit;

exit:
  delete rawNode;
  delete leaf;
  delete leafSibling;
  delete nonLeaf;
  delete nonLeafSibling;

  return rc;
}
