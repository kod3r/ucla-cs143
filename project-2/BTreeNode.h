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

const PageId INVALID_PID = -1;
const int    INVALID_KEY = INT_MIN;

// @todo: move this error definition to Bruinbase.h
const int RC_WRONG_NODE_TYPE     = -1015;

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
     * Copy constructor
     * @param node[IN] data to copy
     */
    template <typename k2, typename v2, k2 m2>
    BTRawNode(const BTRawNode<k2, v2, m2>& node) {
      memcpy(this, &node, sizeof(*this));
      countValidKeys(); // Recount keys since data was reinterpreted
    }

    /**
     * Clears all internal flags and invalidates any stored keys
     */
    void clearAll() {
      flags     = 0;
      nextPid   = INVALID_PID;
      pairCount = 0;

      invalidateStartingAtIndex(0);
      memset(padding, '\0', sizeof(padding));
    }

    /**
     * Loads in a page of data from disk.
     * Data considered "clean" until it is updated
     * @param pid[IN] the PageId to read
     * @param pf[IN] PageFile to read from
     * @return 0 if successful. Return an error code if there is an error.
     */
    RC read(PageId pid, const PageFile& pf) {
      clearAll();

      // By definition the data is clean until written again
      // regardless of the dirty bit upon the last disk write
      RC rc = pf.read(pid, this);
      flags &= ~BT_NODE_RAW_DIRTY;

      // Recalculate the number of valid entries just in case
      countValidKeys();

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
    RC getPair(unsigned eid, Key& k, Value& v) const {
      if(eid >= 0 && eid < MIN(pairCount, ARRAY_SIZE(keys))) {
        k = keys[eid];
        v = values[eid];
        return k == INVALID_KEY ? RC_NO_SUCH_RECORD : 0;
      }

      return RC_NO_SUCH_RECORD;
    }

    /**
     * Get the PageId of the next page.
     * @return returns PageId
     */
    PageId getNextPid() const {
      return nextPid;
    }

    /**
     * Get number of valid keys inserted in node
     * @return entry count
     */
    unsigned getKeyCount() const {
      return pairCount;
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
    RC insertPair(const Key& k, const Value& v) {
      const PageId oldPid = getNextPid();

      // Avoid storing garbage
      if(k == INVALID_KEY)
        return 0;

      if(pairCount >= ARRAY_SIZE(keys))
        return RC_NODE_FULL;

      // Determine the proper location for this key
      unsigned new_key_index = indexForInsert(k);

      // Are we inserting at the end or in the middle of the node?
      // If the new location isn't right after the last pair, move the entires foward an index
      if(new_key_index < pairCount) {
        unsigned keysToMove   = MIN(pairCount, ARRAY_SIZE(keys)  ) - MIN(new_key_index, ARRAY_SIZE(keys)  );
        unsigned valuesToMove = MIN(pairCount, ARRAY_SIZE(values)) - MIN(new_key_index, ARRAY_SIZE(values));

        // Make sure we don't try to move more than the array handles!
        if(keysToMove < ARRAY_SIZE(keys) && valuesToMove < ARRAY_SIZE(values)) {
          memmove(keys   + new_key_index+1, keys   + new_key_index, keysToMove   * sizeof(Key)  );
          memmove(values + new_key_index+1, values + new_key_index, valuesToMove * sizeof(Value));
        } else { // Graceful sanity check
          return RC_NODE_FULL;
        }
      }

      keys[new_key_index] = k;
      values[new_key_index] = v;

      pairCount++;
      flags |= BT_NODE_RAW_DIRTY;
      setNextPid(oldPid);
      return 0;
    }

    /**
     * Insert a new key and value into the node, splitting appropriately.
     * Note: caller is responsible for writing both nodes to disk and
     * setting the nextPid pointers appropriately.
     * @param key The key of the item to insert
     * @param value The value of the item to insert
     * @param sibling The sibling to insert any overflow items into on split
     *        sibling must be different than *this. Behavior is undefined if sibling == *this
     * @param siblingKey[OUT] The key of the first item in the sibling
     */
    RC insertPairAndSplit(const Key& key, const Value& value, BTRawNode<Key, Value, INVALID_KEY>& sibling, Key& siblingKey) {
      RC     rc;
      PageId siblingPid;
      Value  placeHolder;

      // Variables to hold the overflow pair
      Key   lastKey;
      Value lastValue;

      // Pivot is the middle of the array, favoring a (possibly) larger portion for the sibling
      const int lastItem  = MIN(pairCount, ARRAY_SIZE(keys));
      const int pivot     = lastItem / 2;
      const int newItem   = indexForInsert(key);
      const int numToMove = lastItem - pivot;

      // If the new value will NOT be the last value in the array
      // remove and save the last item, then insert the current value
      if(newItem < lastItem) {
        if((rc = this->getPair(lastItem-1, lastKey, lastValue)) < 0) {
          return rc;
        }

        pairCount--;
        if((rc = this->insertPair(key, value)) < 0) {
          return rc;
        }
      } else { // Current value will end up being last, save it as such
        lastKey = key;
        lastValue = value;
      }

      // Copy the node type (and that its dirty) to the sibling
      this->flags |= BT_NODE_RAW_DIRTY;

      sibling.clearAll();
      sibling.flags = this->flags;

      // Split the nodes
      memmove(sibling.keys,   this->keys   + pivot, numToMove*sizeof(Key)  );
      memmove(sibling.values, this->values + pivot, numToMove*sizeof(Value));

      sibling.pairCount = numToMove;

      // Insert the overflow node into the sibling
      if((rc = sibling.insertPair(lastKey, lastValue)) < 0) {
        return rc;
      }

      sibling.setNextPid(this->getNextPid());
      this->invalidateStartingAtIndex(pivot);

      if((rc = sibling.getPair(0, siblingKey, placeHolder)) < 0) {
        return rc;
      }

      return 0;
    }

    /**
     * Set the PageId of the next page.
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

    /**
     * Invalidates key-value pairs starting at a given index, as well as nextPid
     * @param index[IN] invalidate keys after this index, inclusive
     */
    void invalidateStartingAtIndex(unsigned index) {
      if(index == 0)
       pairCount = ARRAY_SIZE(keys);

      const unsigned oldCount = pairCount;

      if(index < pairCount && index < ARRAY_SIZE(keys))
        pairCount = index;
      else
        return;

      nextPid = INVALID_PID;
      for(unsigned i = index; i < MIN(oldCount, ARRAY_SIZE(keys)); i++) {
        keys[i] = INVALID_KEY;
      }

      memset(values+index, '\0', sizeof(Value) * (oldCount - pairCount));

      if(!isLeaf())
        setNextPid(INVALID_PID);
    }

    /**
     * Determines if key to be inserted will be at the end of the node
     * @param key[IN] key to potentially insert
     * @return true if key will be inserted at the end, false if otherwise
     */
    bool willBeInsertedAtEnd(const Key& key) const {
      return indexForInsert(key) == MIN(pairCount, ARRAY_SIZE(keys));
    }

  protected:
    /**
     * Determine the index that a supposed new key would hold. This could return
     * a value larger than the keys array.
     * @param key The potential key
     * @return the index where the key should be inserted. If this value is greater
     *         than ARRAY_SIZE(keys) the key should be inserted in a new node.
     */
    unsigned indexForInsert(const Key& key) const {
      unsigned lo = 0;
      unsigned hi = MIN(pairCount, ARRAY_SIZE(keys)) - 1;
      unsigned cur = lo;

      while(lo < hi) {
        if(key < keys[lo]) {
          cur = lo;
          break;
        } else if(key >= keys[hi]) {
          cur = hi;
          break;
        } else {
          cur = (hi + lo) / 2;
        }

        if(key == keys[cur])
          break;
        else if(key < keys[cur])
          hi = --cur;
        else // key > keys[cur]
          lo = ++cur;
      }

      if(keys[cur] != INVALID_KEY && key >= keys[cur])
        cur++;

      return cur;
    }

    /**
     * Resets pairCount based on the number of valid keys
     */
    void countValidKeys() {
      pairCount = 0;
      while(pairCount < ARRAY_SIZE(keys) && keys[pairCount] != INVALID_KEY)
        pairCount++;
    }

  private:
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
    static const unsigned DEGREE = PageFile::PAGE_SIZE / (2 * MAX(sizeof(int), MAX(sizeof(PageId), MAX(sizeof(Key), sizeof(Value)))));

    /**
     * Note: if we used an int for flags, it is possible to not need any padding
     * as the maximum page size will be word aligned. Since a zero sized array is forbidden
     * we make our flags use an short and always use a few padded bytes.
     */
    Value   values[DEGREE - 1];
    Key     keys[DEGREE - 1];

    //              Max structure size          sizeof(keys) + sizeof(values)           nextPid        pairCount         flags
    char    padding[PageFile::PAGE_SIZE - (DEGREE-1)*(sizeof(Key) + sizeof(Value)) - sizeof(PageId) - sizeof(short) - sizeof(short)];

    // Shared entries should always be after the padding so that
    // they are always aligned between different node types
    PageId  nextPid;
    unsigned short pairCount; // Cache the number of useful entries
    unsigned short flags;
};

typedef BTRawNode<int, RecordId, INVALID_KEY> BTRawLeaf;
typedef BTRawNode<int, PageId,   INVALID_KEY> BTRawNonLeaf;

/**
 * BTLeafNode: The class representing a B+tree leaf node.
 */
class BTLeafNode {
  public:
    BTLeafNode();

    /**
     * Constructor for creating a BTLeafNode with raw data already in memory.
     * A copy of node is made, thus the caller must manage its memory.
     * Data *MUST* be of leaf type, otherwise behavior is undefined.
     * @param node[IN] raw data in memory to use
     * @param pid[IN] PageId with which the data should be associated
     */
    template<typename k, typename v, k m>
    BTLeafNode(const BTRawNode<k, v, m>& node, const PageId& pid) : data(node), dataPid(pid)
    {}

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
    RC locate(int searchKey, int& eid) const;

   /**
    * Read the (key, rid) pair from the eid entry.
    * @param eid[IN] the entry number to read the (key, rid) pair from
    * @param key[OUT] the key from the slot
    * @param rid[OUT] the RecordId from the slot
    * @return 0 if successful. Return an error code if there is an error.
    */
    RC readEntry(int eid, int& key, RecordId& rid) const;

   /**
    * Return the pid of the next slibling node.
    * @return the PageId of the next sibling node 
    */
    PageId getNextNodePtr() const;


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
    int getKeyCount() const;
 
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
   BTRawLeaf data;
   PageId dataPid;
}; 


/**
 * BTNonLeafNode: The class representing a B+tree nonleaf node.
 */
class BTNonLeafNode {
  public:
    BTNonLeafNode();

    /**
     * Constructor for creating a BTNonLeafNode with raw data already in memory.
     * A copy of node is made, thus the caller must manage its memory.
     * Data *MUST* be of non-leaf type, otherwise behavior is undefined.
     * @param node[IN] raw data in memory to use
     * @param pid[IN] PageId with which the data should be associated
     */
    template <typename k, typename v, k m>
    BTNonLeafNode(const BTRawNode<k, v, m>& node, const PageId& pid) : data(node), dataPid(pid)
    {}

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
    RC locateChildPtr(int searchKey, PageId& pid) const;

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
    int getKeyCount() const;

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
   BTRawNonLeaf data;
   PageId dataPid;
}; 

#endif /* BTNODE_H */
