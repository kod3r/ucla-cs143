#include "BTreeNode.h"

using namespace std;

#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*a))

#define BT_NODE_RAW_DIRTY     (1<<0)
#define BT_NODE_RAW_LEAF      (1<<1)
#define BT_NODE_RAW_ROOT      (1<<2)

/**
 * A simple glorified struct for holding raw BTNode data.
 * Wrapper class helps maintain the clean/dirty state, prevents
 * memory access violations, and handles loading data from disk
 */
class BTRawNode {
public:
  BTRawNode() : flags(0) {
    for(int i = 0; i < ARRAY_SIZE(pids); i++) {
      pids[i] = -1; // A pid of -1 indicates an unset key value for the same index
    }
  }

  BTRawNode(void *buf, size_t len) {
    if(buf && buf != (void *)this) {
      memcpy(this, buf, MIN(len, sizeof(this)));
    }

    // By definition the data is clean until written again
    // regardless of the dirty bit upon the last disk write
    flags &= ~BT_NODE_RAW_DIRTY;
  }

  // Getters
  bool isDirty() const { return flags & BT_NODE_RAW_DIRTY; }
  bool isLeaf()  const { return flags & BT_NODE_RAW_LEAF;  }
  bool isRoot()  const { return flags & BT_NODE_RAW_ROOT;  }

  void const * data() const { return this; }

  unsigned maxKeyIndex() const { return ARRAY_SIZE(keys); }
  unsigned maxPidIndex() const { return ARRAY_SIZE(pids); }

  bool getKey(unsigned index, int& k) const {
    if(index < ARRAY_SIZE(keys)) {
      k = keys[index];
      return true;
    }

    return false;
  }

  bool getPid(unsigned index, PageId& p) const {
    if (index < ARRAY_SIZE(pids)) {
      p = pids[index];
      return true;
    }

    return false;
  }

  // Setters
  bool setKey(unsigned index, int k) {
    if(index < ARRAY_SIZE(keys)) {
      keys[index] = k;
      flags |= BT_NODE_RAW_DIRTY;
      return true;
    }

    return false;
  }

  bool setPid(unsigned index, PageId& p) {
    if(index < ARRAY_SIZE(pids)) {
      pids[index] = p;
      flags |= BT_NODE_RAW_DIRTY;
      return true;
    }

    return false;
  }

protected:
  /**
   * A node of degree N will have N PageId pointers and N-1 keys, each being a word long.
   * An additional "word" can be used to set some internal flags such as the type of the node.
   * Thus we have n + n-1 + 1 = PageSize ==> 2n = PageSize => n = PageSize / 2.
   *
   * In the event that DEGREE is such that the max page size is not fully utilized, we pad
   * the remainder of the structure so that the two sizes will match up.
   */
  static const int DEGREE = PageFile::PAGE_SIZE / (2 * MAX(sizeof(PageId), sizeof(int)));

  /**
   * Note: if we used an int for flags, it is possible to not need any padding
   * as the maximum page size will be word aligned. Since a zero sized array is forbidden
   * we make our flags use an short and always use a few padded bytes.
   */
  PageId  pids[DEGREE];
  int     keys[DEGREE - 1];
  short   flags;

  char    padding[PageFile::PAGE_SIZE - DEGREE*sizeof(PageId) - (DEGREE-1)*sizeof(int) - sizeof(short)];
};



/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf)
{ return 0; }
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf)
{ return 0; }

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount()
{ return 0; }

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{ return 0; }

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{ return 0; }

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid)
{ return 0; }

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid)
{ return 0; }

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr()
{ return 0; }

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid)
{ return 0; }

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf)
{ return 0; }
    
/*
 * Write the content of the node to the page pid in the PageFile pf.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf)
{ return 0; }

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount()
{ return 0; }


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid)
{ return 0; }

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{ return 0; }

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid)
{ return 0; }

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2)
{ return 0; }
