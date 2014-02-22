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
#define BT_NODE_RAW_ROOT      (1<<2)

static const PageId INVALID_PID = -1;

/**
 * A simple glorified struct for holding raw BTNode data.
 * Wrapper class helps maintain the clean/dirty state, prevents
 * memory access violations, and handles loading data from disk
 */
template <typename Key, typename Value, Key INVALID_KEY>
class BTRawNode {
  public:
    BTRawNode() {
      clearAll();
    }

    void clearAll() {
      flags     = 0;
      nextPid   = INVALID_PID;
      pairCount = 0;

      for(int i = 0; i < ARRAY_SIZE(keys); i++) {
        keys[i] = INVALID_KEY;
      }
    }

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

    RC write(PageId pid, PageFile& pf) {
      RC rc = pf.write(pid, this);

      if(rc == 0)
        flags &= ~BT_NODE_RAW_DIRTY;

      return rc;
    }

    // Getters
    bool isDirty() const { return flags & BT_NODE_RAW_DIRTY; }
    bool isLeaf()  const { return flags & BT_NODE_RAW_LEAF;  }
    bool isRoot()  const { return flags & BT_NODE_RAW_ROOT;  }

    RC getPair(int eid, Key& k, Value& v) const {
      if(eid < MIN(pairCount, ARRAY_SIZE(keys))) {
        k = keys[eid];
        v = values[eid];
        return 0;
      }

      return RC_NO_SUCH_RECORD;
    }

    // PageId template, i.e. non-leaf
    template <Key, PageId, Key>
    PageId getNextPid() const {
      // last most pid is stored outside of the values array
      if(pairCount < ARRAY_SIZE(keys) && keys[pairCount] != INVALID_KEY) { // sanity check
        return pairCount < ARRAY_SIZE(keys)-1 ? values[pairCount+1] : nextPid;
      }

      return INVALID_KEY;
    }

    // Non PageId template, i.e. leaf node
    PageId getNextPid() const {
      return nextPid;
    }

    int getKeyCount() const {
      return pairCount;
    }

    // Setters
    void setRoot() {
      if(isRoot())
        return;

      flags |=  BT_NODE_RAW_DIRTY;
      flags |=  BT_NODE_RAW_ROOT;
      flags &= ~BT_NODE_RAW_LEAF;
    }

    void setLeaf() {
      if(isLeaf())
        return;

      flags |=  BT_NODE_RAW_DIRTY;
      flags &= ~BT_NODE_RAW_ROOT;
      flags |=  BT_NODE_RAW_LEAF;
    }

    void setNonLeaf() {
      if(!isLeaf())
        return;

      flags |=  BT_NODE_RAW_DIRTY;
      flags &= ~BT_NODE_RAW_ROOT;
      flags &= ~BT_NODE_RAW_LEAF;
    }

    RC insertPair(int& eid, const Key& k, const Value& v) {
      if(pairCount >= ARRAY_SIZE(keys))
        return RC_NODE_FULL;

      keys[pairCount] = k;
      values[pairCount] = v;

      pairCount++;
      flags |= BT_NODE_RAW_DIRTY;

      eid = pairCount;
      return 0;
    }

    // PageId template, i.e. non-leaf
    template <Key, PageId, Key>
    bool setNextPid(const PageId& pid) {
      // sanity check
      if(pairCount >= ARRAY_SIZE(keys) || keys[pairCount] == INVALID_KEY)
        return false;

      if(pairCount < ARRAY_SIZE(keys)-1)
        values[pairCount] = pid;
      else
        nextPid = pid;

      return true;
    }

    // Non PageId template, i.e. leaf node
    bool setNextPid(const PageId& pid) {
      nextPid = pid;
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
