/*
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 5/28/2008
 */

#ifndef BTNODE_H
#define BTNODE_H

#include "RecordFile.h"
#include "PageFile.h"

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define BT_NODE_RAW_DIRTY     (1<<0)
#define BT_NODE_RAW_LEAF      (1<<1)

static const PageId INVALID_PID = -1;

/**
 * A simple glorified struct for holding raw BTNode data.
 * Wrapper class helps maintain the clean/dirty state, prevents
 * memory access violations, and handles loading data from disk
 */
template <typename Key, typename Value, Key INVALID_KEY>
class BTRawNode {
  public:
    /**
     * Default constructor: initialize all values as empty
     */
    BTRawNode() {
      clearAll();
    }

    /**
     * Clears all internal flags and invalidates any stored keys
     */
    void clearAll() {
      flags     = 0;
      nextPid   = INVALID_PID;
      pairCount = 0;

      for(int i = 0; i < ARRAY_SIZE(keys); i++) {
        keys[i] = INVALID_KEY;
      }
    }

    /**
     * Loads in a page of data from disk.
     * Data considered "clean" until it is updated
     * @param pid[IN] the PageId to read
     * @param pf[IN] PageFile to read from
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC read(PageId pid, const PageFile& pf) {
      // By definition the data is clean until written again
      // regardless of the dirty bit upon the last disk write
      RC rc = pf.read(pid, this);
      flags &= ~BT_NODE_RAW_DIRTY;

      // Recalculate the number of valid entries just in case
      pairCount = 0;
      while(pairCount < ARRAY_SIZE(keys) && keys[pairCount] != INVALID_KEY)
        pairCount++;

      return rc;
    }

    /**
     * Write the node's data to the page pid in the PageFile pf.
     * Data is considered "clean" if write succeds.
     * @param pid[IN] the PageId to write to
     * @param pf[IN] PageFile to write to
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC write(PageId pid, PageFile& pf) {
      RC rc = pf.write(pid, this);

      if(rc == 0)
        flags &= ~BT_NODE_RAW_DIRTY;

      return rc;
    }

    /*************************************/
    /*************  Getters  *************/
    /*************************************/

    /**
     * Indicates if data in memory is dirty (differs from that on disk)
     * @return true if dirty, false if clean
     */
    bool isDirty() const { return flags & BT_NODE_RAW_DIRTY; }

    /**
     * Indicates if data represents a leaf node
     * @return true if leaf node, false if non-leaf
     */
    bool isLeaf()  const { return flags & BT_NODE_RAW_LEAF;  }

    /**
     * Retreives a key value pair from the given index
     * @param eid[IN] the entry index to retrieve
     * @param k[OUT] retrieved key
     * @param v[OUT] retrieved value
     * @return 0 on success, RC_NO_SUCH_RECORD on out of bounds eid or uninitialized entry
     */
    RC getPair(int eid, Key& k, Value& v) const {
      if(eid >= 0 && eid < MIN(pairCount, ARRAY_SIZE(keys))) {
        k = keys[eid];
        v = values[eid];
        return k == INVALID_KEY ? RC_NO_SUCH_RECORD : 0;
      }

      return RC_NO_SUCH_RECORD;
    }

    /**
     * Get the PageId of the next page, i.e. the right-most pointer of the node
     *
     * Specific version to set the nextPid, where the templated type
     * of Value IS PageId (i.e. this is a non-leaf node).
     * Since PageIds are inserted as a pair along with a key value, this
     * function will find the appropriate location to retrieve the value
     *
     * @return returns the PageId on success, INVALID_PID on internal error or uninitialized PageId
     */
    template <Key, PageId, Key>
    PageId getNextPid() const {
      // last most pid is stored outside of the values array
      if(pairCount < ARRAY_SIZE(keys) && keys[pairCount] != INVALID_KEY) { // sanity check
        return pairCount < ARRAY_SIZE(keys)-1 ? values[pairCount+1] : nextPid;
      }

      return INVALID_PID;
    }

    /**
     * Get the PageId of the next page.
     *
     * Generic version to set the nextPid, where the templated type
     * of Value is NOT PageId (i.e. this is probably a leaf node).
     * Only a single PageId can be stored for such nodes
     *
     * @return returns PageId
     */
    PageId getNextPid() const {
      return nextPid;
    }

    /**
     * Get number of valid keys inserted in node
     * @return entry count
     */
    int getKeyCount() const {
      return pairCount;
    }

    /**
     * Get the first (lowest) key in the node.
     * @return the key|-1 if non defined
     */
    Key getFirstKey() const {
      if (pairCount > 0)
        return key[0];
      else
        return -1;
    }

    /*************************************/
    /*************  Setters  *************/
    /*************************************/

    /**
     * Mark the node as a leaf node
     */
    void setLeaf() {
      if(isLeaf())
        return;

      flags |=  BT_NODE_RAW_DIRTY;
      flags |=  BT_NODE_RAW_LEAF;
    }

    /**
     * Mark the node as a non-leaf node
     */
    void setNonLeaf() {
      if(!isLeaf())
        return;

      flags |=  BT_NODE_RAW_DIRTY;
      flags &= ~BT_NODE_RAW_LEAF;
    }

    /**
     * Insert a new key value pair into the node
     * @param eid[OUT] the index of the newly inserted pair
     * @param k[IN] the key to insert
     * @param v[IN] the value to insert
     * @return 0 on success, RC_NODE_FULL if no room left in node
     */
    RC insertPair(int& eid, const Key& k, const Value& v) {
      // Avoid storing garbage
      if(k == INVALID_KEY)
        return 0;

      if(pairCount >= ARRAY_SIZE(keys))
        return RC_NODE_FULL;

      // Determine the proper location for this key
      int new_key_index = findIndexOfNewItem(k);

      // Are we inserting at the end or in the middle of the node?
      if(new_key_index < pairCount) {
        // If the new location isn't right before the end of the array, move everything back an index
        if (new_key_index < ARRAY_SIZE(keys) - 1)
          memmove((void*)keys[new_key_index + 1], (void*)keys[new_key_index], sizeof(Key)*(ARRAY_SIZE(keys)-new_key_index));
        else // Be extra paranoid to avoid segfaults
          return RC_NODE_FULL;
      }

      keys[new_key_index] = k;
      values[new_key_index] = v;

      pairCount++;
      flags |= BT_NODE_RAW_DIRTY;

      eid = pairCount;
      return 0;
    }

    /**
     * Insert a new key and value into the node, splitting appropriately.
     * @param key The key of the item to insert
     * @param value The value of the item to insert
     * @param sibling The sibling to insert any overflow items into on split
     * @param siblingKey[OUT] The key of the first item in the sibling
     */
    RC insertPairAndSplit(const Key& key, const Value& value, BTLeafNode& sibling, Key& siblingKey) {
     int pivot_location = findPivotIndexForSplit(pairCount);
     int new_item_index = findIndexOfNewItem(key);

     /**
      * We have three cases to handle.
      * 1. The new key we're adding is in this node.
      * 2. The new item is the pivot point, and will be the first item in the sibling.
      * 3. The new item is in the sibling, but is not the first key.
      */

      // This should handle (2) and (3) above.
      if (pivot_location <= new_item_index) {
        // Insert this item
        sibling.insertPair(key, value);

        // Now insert the rest
        for (int x = pivot_location; x < pairCount; x++) {
          sibling.insertPair(keys[x], values[x]);
          keys[x] = INVALID_KEY;
          values[x] = INVALID_KEY;
        }
        // Reset the new pair count
        pairCount = pivot_location;
      } else { // Otherwise, this new item is in the old list
        for (int x = pivot_location - 1; x < pairCount; x++) { 
          sibling.insertPair(keys[x], values[x]);
          keys[x] = INVALID_KEY;
          values[x] = INVALID_KEY;
        }
          pairCount = pivot_location - 1;
      }
      siblingKey = sibling.getFirstKey();
      return 0;
    }

    /**
     * Determine the pivot point for the current node. This takes into account the
     * additional item we're adding that's causing the split.
     */
    int findPivotIndexForSplit() {
      return (pairCount+1)/2;
    }

    /**
     * Determine the index that a supposed new key would hold. This could return
     * a value larger than the keys array.
     * @param key The potential key
     */
    int findIndexOfNewItem(const Key& key) {
      int new_position = 0;
      for (; new_position < pairCount; new_position++) {
        if (key < keys[new_position])
          break;
      }
      return new_position;
    }

    /**
     * Set the PageId of the next page.
     *
     * Specific version to set the nextPid, where the templated type
     * of Value IS PageId (i.e. this is a non-leaf node).
     * Since PageIds are inserted as a pair along with a key value, this
     * function will find the appropriate location to save the value
     *
     * @param pid[IN] pid to update
     * @return returns true on success, false on empty node or internal error
     */
    template <Key, PageId, Key>
    bool setNextPid(const PageId& pid) {
      // sanity check
      if(pairCount >= ARRAY_SIZE(keys) || keys[pairCount] == INVALID_KEY)
        return false;

      // Store pid in the next empty values slot
      if(pairCount < ARRAY_SIZE(keys)-1) {
        if(values[pairCount] != pid) {
          values[pairCount] = pid;
          flags |= BT_NODE_RAW_DIRTY;
        }
      } else if(nextPid != pid) { // if node is full, store pid in this->nextPid
        nextPid = pid;
        flags |= BT_NODE_RAW_DIRTY;
      }

      return true;
    }

    /**
     * Set the PageId of the next page.
     *
     * Generic version to set the nextPid, where the templated type
     * of Value is NOT PageId (i.e. this is probably a leaf node).
     * Only a single PageId can be stored for such nodes
     *
     * @param pid[IN] pid to update
     * @return returns true on success
     */
    bool setNextPid(const PageId& pid) {
      if(nextPid != pid) {
        nextPid = pid;
        flags |= BT_NODE_RAW_DIRTY;
      }

      return true;
    }

  protected:
    /**
     * A node of degree N will have N PageId pointers and N-1 keys, each being a word long.
     * An additional "word" can be used to set some internal flags such as the type of the node.
     * Thus we have n + n-1 + 1 = PageSize ==> 2n = PageSize => n = PageSize / 2.
     *
     * In the event that DEGREE is such that the max page size is not fully utilized, we pad
     * the remainder of the structure so that the two sizes will match up.
     *
     * To efficiently reuse code, we make this class a templated class. Employing the reasoning
     * commented above, we conservatively estimate the largest degree of keys and pointers that
     * can fit inside a page. Since both leaf and non-leaf nodes will *always* have one PageId
     * we hard code this value within the structure (nextPid).
     */
    static const int DEGREE = PageFile::PAGE_SIZE / (2 * MAX(sizeof(int), MAX(sizeof(PageId), MAX(sizeof(Key), sizeof(Value)))));

    /**
     * Note: if we used an int for flags, it is possible to not need any padding
     * as the maximum page size will be word aligned. Since a zero sized array is forbidden
     * we make our flags use an short and always use a few padded bytes.
     */
    Value   values[DEGREE - 1];
    Key     keys[DEGREE - 1];
    PageId  nextPid;
    unsigned short pairCount; // Cache the number of useful entries
    char    flags;

    char    padding[PageFile::PAGE_SIZE - (DEGREE-1)*(sizeof(Key) + sizeof(Value)) - sizeof(PageId) - sizeof(short) - sizeof(char)];
};

typedef BTRawNode<int, RecordId, INT_MAX> BTRawLeaf;
typedef BTRawNode<int, PageId,   INT_MAX> BTRawNonLeaf;

/**
 * BTLeafNode: The class representing a B+tree leaf node.
 */
class BTLeafNode {
  public:
    BTLeafNode();
    ~BTLeafNode();

   /**
    * Insert the (key, rid) pair to the node.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param rid[IN] the RecordId to insert
    * @return 0 if successful. Return an error code if the node is full.
    */
    RC insert(int key, const RecordId& rid);

   /**
    * Insert the (key, rid) pair to the node
    * and split the node half and half with sibling.
    * The first key of the sibling node is returned in siblingKey.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert.
    * @param rid[IN] the RecordId to insert.
    * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
    * @param siblingKey[OUT] the first key in the sibling node after split.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC insertAndSplit(int key, const RecordId& rid, BTLeafNode& sibling, int& siblingKey);

   /**
    * Find the index entry whose key value is larger than or equal to searchKey
    * and output the eid (entry id) whose key value &gt;= searchKey.
    * Remember that keys inside a B+tree node are sorted.
    * @param searchKey[IN] the key to search for.
    * @param eid[OUT] the entry number that contains a key larger              
    *                 than or equalty to searchKey.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC locate(int searchKey, int& eid);

   /**
    * Read the (key, rid) pair from the eid entry.
    * @param eid[IN] the entry number to read the (key, rid) pair from
    * @param key[OUT] the key from the slot
    * @param rid[OUT] the RecordId from the slot
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC readEntry(int eid, int& key, RecordId& rid);

   /**
    * Return the pid of the next slibling node.
    * @return the PageId of the next sibling node 
    */
    PageId getNextNodePtr();


   /**
    * Set the next slibling node PageId.
    * @param pid[IN] the PageId of the next sibling node 
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC setNextNodePtr(PageId pid);

   /**
    * Return the number of keys stored in the node.
    * @return the number of keys in the node
    */
    int getKeyCount();
 
   /**
    * Read the content of the node from the page pid in the PageFile pf.
    * @param pid[IN] the PageId to read
    * @param pf[IN] PageFile to read from
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC read(PageId pid, const PageFile& pf);
    
   /**
    * Write the content of the node to the page pid in the PageFile pf.
    * @param pid[IN] the PageId to write to
    * @param pf[IN] PageFile to write to
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC write(PageId pid, PageFile& pf);

  private:
   BTRawLeaf * const data;
   PageId dataPid;
}; 


/**
 * BTNonLeafNode: The class representing a B+tree nonleaf node.
 */
class BTNonLeafNode {
  public:
    BTNonLeafNode();
    ~BTNonLeafNode();

   /**
    * Insert a (key, pid) pair to the node.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param pid[IN] the PageId to insert
    * @return 0 if successful. Return an error code if the node is full.
    */
    RC insert(int key, PageId pid);

   /**
    * Insert the (key, pid) pair to the node
    * and split the node half and half with sibling.
    * The sibling node MUST be empty when this function is called.
    * The middle key after the split is returned in midKey.
    * Remember that all keys inside a B+tree node should be kept sorted.
    * @param key[IN] the key to insert
    * @param pid[IN] the PageId to insert
    * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
    * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey);

   /**
    * Given the searchKey, find the child-node pointer to follow and
    * output it in pid.
    * Remember that the keys inside a B+tree node are sorted.
    * @param searchKey[IN] the searchKey that is being looked up.
    * @param pid[OUT] the pointer to the child node to follow.
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC locateChildPtr(int searchKey, PageId& pid);

   /**
    * Initialize the root node with (pid1, key, pid2).
    * @param pid1[IN] the first PageId to insert
    * @param key[IN] the key that should be inserted between the two PageIds
    * @param pid2[IN] the PageId to insert behind the key
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC initializeRoot(PageId pid1, int key, PageId pid2);

   /**
    * Return the number of keys stored in the node.
    * @return the number of keys in the node
    */
    int getKeyCount();

   /**
    * Read the content of the node from the page pid in the PageFile pf.
    * @param pid[IN] the PageId to read
    * @param pf[IN] PageFile to read from
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC read(PageId pid, const PageFile& pf);
    
   /**
    * Write the content of the node to the page pid in the PageFile pf.
    * @param pid[IN] the PageId to write to
    * @param pf[IN] PageFile to write to
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC write(PageId pid, PageFile& pf);

  private:
   BTRawNonLeaf * const data;
   PageId dataPid;
}; 

#endif /* BTNODE_H */
