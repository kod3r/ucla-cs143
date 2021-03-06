#include "BTreeNode.h"

using namespace std;

/**
 * Default constructor: initialize member variables
 */
BTLeafNode::BTLeafNode()
: dataPid(INVALID_PID)
{
  data.clearAll();
  data.setLeaf();
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::read(PageId pid, const PageFile& pf) {
  RC rc = data.read(pid, pf);
  dataPid = rc < 0 ? INVALID_PID : pid;
  return data.isLeaf() ? rc : RC_WRONG_NODE_TYPE;
}

/*
 * Write the content of the node to the page pid in the PageFile pf. Avoids a write
 * if no data has been changed and pid is the same as the pid from where the data
 * was previously read from or written to.
 * Note: if a read is issued from (pid1, pf1) and then a write is requested to (pid1, pf2)
 * without updating data, it is possible for no data to be written. It is *strongly discouraged*
 * to read from one page file and write to another.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::write(PageId pid, PageFile& pf) {
  RC rc;

  // If we are writing to the same page and no data has changed, avoid the extra write
  if(dataPid == pid && !data.isDirty())
    return 0;

  // Update associate the data with the (possibly new) pid
  if((rc = data.write(pid, pf)) == 0)
    dataPid = pid;

  return rc;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTLeafNode::getKeyCount() const {
  return data.getKeyCount();
}

/*
 * Insert a (key, rid) pair to the node.
 * @param key[IN] the key to insert
 * @param rid[IN] the RecordId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTLeafNode::insert(int key, const RecordId& rid)
{
  return data.insertPair(key, rid);
}

/*
 * Insert the (key, rid) pair to the node
 * and split the node half and half with sibling.
 * The first key of the sibling node is returned in siblingKey.
 * Caller must save both nodes to disk and set their next PageIds properly
 * @param key[IN] the key to insert.
 * @param rid[IN] the RecordId to insert.
 * @param sibling[IN] the sibling node to split with. This node MUST be EMPTY when this function is called.
 * @param siblingKey[OUT] the first key in the sibling node after split.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::insertAndSplit(int key, const RecordId& rid, 
                              BTLeafNode& sibling, int& siblingKey)
{
  RecordId placeHolder;
  return data.insertPairAndSplit(key, rid, sibling.data, siblingKey, placeHolder);
}

/*
 * Find the entry whose key value is larger than or equal to searchKey
 * and output the eid (entry number) whose key value >= searchKey.
 * Remeber that all keys inside a B+tree node should be kept sorted.
 * @param searchKey[IN] the key to search for
 * @param eid[OUT] the entry number that contains a key larger than or equalty to searchKey
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::locate(int searchKey, int& eid) const {
  int key;
  RecordId rid;

  for(eid = 0; (unsigned)eid < data.getKeyCount(); eid++) {
    data.getPair(eid, key, rid);
    if(key >= searchKey)
      return 0;
  }

  eid = -1;
  return RC_NO_SUCH_RECORD;
}

/*
 * Read the (key, rid) pair from the eid entry.
 * @param eid[IN] the entry number to read the (key, rid) pair from
 * @param key[OUT] the key from the entry
 * @param rid[OUT] the RecordId from the entry
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::readEntry(int eid, int& key, RecordId& rid) const {
  return data.getPair(eid, key, rid);
}

/*
 * Return the pid of the next slibling node.
 * @return the PageId of the next sibling node 
 */
PageId BTLeafNode::getNextNodePtr() const {
  return data.getNextPid();
}

/*
 * Set the pid of the next slibling node.
 * @param pid[IN] the PageId of the next sibling node 
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTLeafNode::setNextNodePtr(PageId pid) {
  data.setNextPid(pid);
  return 0;
}

/**
 * Default constructor: initialize member variables
 */
BTNonLeafNode::BTNonLeafNode()
: dataPid(INVALID_PID)
{
  data.clearAll();
  data.setNonLeaf();
}

/*
 * Read the content of the node from the page pid in the PageFile pf.
 * @param pid[IN] the PageId to read
 * @param pf[IN] PageFile to read from
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::read(PageId pid, const PageFile& pf) {
  RC rc = data.read(pid, pf);
  dataPid = rc < 0 ? INVALID_PID : pid;
  return data.isLeaf() ? RC_WRONG_NODE_TYPE : rc;
}
    
/*
 * Write the content of the node to the page pid in the PageFile pf. Avoids a write
 * if no data has been changed and pid is the same as the pid from where the data
 * was previously read from or written to.
 * Note: if a read is issued from (pid1, pf1) and then a write is requested to (pid1, pf2)
 * without updating data, it is possible for no data to be written. It is *strongly discouraged*
 * to read from one page file and write to another.
 * @param pid[IN] the PageId to write to
 * @param pf[IN] PageFile to write to
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::write(PageId pid, PageFile& pf) {
  RC rc;

  // If we are writing to the same page and no data has changed, avoid the extra write
  if(dataPid == pid && !data.isDirty())
    return 0;

  // Update associate the data with the (possibly new) pid
  if((rc = data.write(pid, pf)) == 0)
    dataPid = pid;

  return rc;
}

/*
 * Return the number of keys stored in the node.
 * @return the number of keys in the node
 */
int BTNonLeafNode::getKeyCount() const {
  return data.getKeyCount();
}


/*
 * Insert a (key, pid) pair to the node.
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @return 0 if successful. Return an error code if the node is full.
 */
RC BTNonLeafNode::insert(int key, PageId pid) { 
  RC rc;
  int      oldKey;
  PageId   origPid;
  unsigned index;

  // If the key is inserted at the end swap nextPid with pid to insert to keep the tree valid
  if(data.willBeInsertedAtEnd(key)) {
    if((rc = data.insertPair(key, data.getNextPid())) < 0)
      return rc;

    data.setNextPid(pid);
    return 0;
  }

  // Inserting into a nonLeaf node indicates a lower level split.
  // To properly update the tree, we have to insert the first key of the sibling
  // (i.e. the arg `key`) along with the pid of the original node (i.e. `origPid`),
  // since all keys in the old node are now less than `key`. We have to update the
  // held a pointer to the old node (i.e. a pair {`oldKey`, `origPid`}) so that it
  // now points to the sibling pid (i.e. becomes {`oldKey`, `pid`}), since all the
  // keys in the sibling are greater than or equal to `key` but less than `oldKey`.
  index = data.indexForInsert(key);

  if((rc = data.getPair(index, oldKey, origPid)) < 0)
    return rc;

  if((rc = data.updatePair(oldKey, pid)) < 0)
    return rc;

  return data.insertPair(key, origPid);
}

/*
 * Insert the (key, pid) pair to the node
 * and split the node half and half with sibling.
 * The middle key after the split is returned in midKey.
 * Caller must save both nodes to disk and set their next PageIds properly
 * @param key[IN] the key to insert
 * @param pid[IN] the PageId to insert
 * @param sibling[IN] the sibling node to split with. This node MUST be empty when this function is called.
 * @param midKey[OUT] the key in the middle after the split. This key should be inserted to the parent node.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::insertAndSplit(int key, PageId pid, BTNonLeafNode& sibling, int& midKey)
{
  RC rc;
  PageId midPid;

  // Make sure we properly update the nextPid if the new node will go at the end
  if(data.willBeInsertedAtEnd(key)) {
    rc = data.insertPairAndSplit(key, data.getNextPid(), sibling.data, midKey, midPid);
    sibling.data.setNextPid(pid);
  } else {
    rc = data.insertPairAndSplit(key, pid, sibling.data, midKey, midPid);
  }

  this->data.setNextPid(midPid);
  return rc;
}

/*
 * Given the searchKey, find the child-node pointer to follow and
 * output it in pid.
 * @param searchKey[IN] the searchKey that is being looked up.
 * @param pid[OUT] the pointer to the child node to follow.
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::locateChildPtr(int searchKey, PageId& pid) const
{
  RC rc;
  int key;

  for(unsigned eid = 0; eid < data.getKeyCount(); eid++) {
    // Bail on errors
    if((rc = data.getPair(eid, key, pid)) < 0)
      return rc;

    if(searchKey < key)
      return 0;
  }

  pid = data.getNextPid();
  return 0;
}

/*
 * Initialize the root node with (pid1, key, pid2).
 * @param pid1[IN] the first PageId to insert
 * @param key[IN] the key that should be inserted between the two PageIds
 * @param pid2[IN] the PageId to insert behind the key
 * @return 0 if successful. Return an error code if there is an error.
 */
RC BTNonLeafNode::initializeRoot(PageId pid1, int key, PageId pid2) {
  RC rc;

  data.clearAll();
  data.setNonLeaf();

  if((rc = data.insertPair(key, pid1)) < 0)
    return rc;

  data.setNextPid(pid2);
  return 0;
}
