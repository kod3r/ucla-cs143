/**
 * Copyright (C) 2008 by The Regents of the University of California
 * Redistribution of this file is permitted under the terms of the GNU
 * Public License (GPL).
 *
 * @author Junghoo "John" Cho <cho AT cs.ucla.edu>
 * @date 3/24/2008
 */

#include <cstdio>
#include <iostream>
#include <fstream>
#include "Bruinbase.h"
#include "SqlEngine.h"
#include "BTreeIndex.h"

using namespace std;

// external functions and variables for load file and sql command parsing 
extern FILE* sqlin;
int sqlparse(void);


RC SqlEngine::run(FILE* commandline)
{
  fprintf(stdout, "Bruinbase> ");

  // set the command line input and start parsing user input
  sqlin = commandline;
  sqlparse();  // sqlparse() is defined in SqlParser.tab.c generated from
               // SqlParser.y by bison (bison is GNU equivalent of yacc)

  return 0;
}

RC SqlEngine::select(int attr, const string& table, const vector<SelCond>& cond)
{
  RecordFile rf;   // RecordFile containing the table
  RecordId   rid;  // record cursor for table scanning

  BTreeIndex  index;  // Handle to the table's index
  IndexCursor cursor; // index cursor for table scanning

  RC     rc;
  int    key;     
  string value;
  int    count;

  bool hasIndex   = true;
  bool finishScan = false;

  vector<SelCond> indexConds; // Conditions only on key, can get directly from index
  vector<SelCond> tableConds; // Conditions on value, requires reading table

  if((rc = processConditions(cond, indexConds, tableConds)) < 0) {
    fprintf(stderr, "Error processing conditions");
    return rc;
  }

  // open the table file
  if ((rc = rf.open(table + ".tbl", 'r')) < 0) {
    fprintf(stderr, "Error: table %s does not exist\n", table.c_str());
    return rc;
  }

  /**
   * We do not need to consult the index in the following conditions:
   *
   * (1) select VALUE with no KEY constraints
   * (2) count(*) with only VALUE constraints (no constraints means we can just count up the index entries)
   * (3) select * with no KEY constraints
   */
  if(  (attr == 2 && indexConds.empty()                       ) // (1)
    || (attr == 3 && indexConds.empty() && !tableConds.empty()) // (2)
    || (attr == 4 &&                       !tableConds.empty()) // (3)
  ) {
    hasIndex = false;
  }

  // open the table index
  if (hasIndex && (rc = index.open(table + ".idx", 'r')) < 0) {
    hasIndex = false;
  }

  // no index, go directly to the table
  if(!hasIndex) {
    tableConds.insert(tableConds.begin(), indexConds.begin(), indexConds.end());
    indexConds.clear();
  }

  // init the cursor at an appropriate position
  rid.pid = rid.sid = 0;
  if(hasIndex) {
    if(indexConds.size() > 0) {
      switch(indexConds[0].comp) {
        case SelCond::EQ:
        case SelCond::GT:
        case SelCond::GE:
          rc = index.locate(atoi(indexConds[0].value), cursor);
          break;
        case SelCond::LT:
        case SelCond::LE:
        case SelCond::NE:
        default:
          rc = index.locateFirstEntry(cursor);
          break;
      }
    } else { // no KEY conditions
      rc = index.locateFirstEntry(cursor);
    }

    // Fetch the first tuple from the index
    if(rc < 0 || (rc = index.readForward(cursor, key, rid)) < 0) {
      fprintf(stderr, "Error while reading from index for table %s\n", table.c_str());
      goto exit_select;
    }
  }

  count = 0;
  while (!finishScan) {
    // check the index conditions on the tuple
    value = "";
    for(unsigned i = 0; i < indexConds.size(); i++) {
      if(!matchesCondition(indexConds[i], key, value, finishScan))
        goto next_tuple;
    }

    // grab the key value pair from the table if no index is available,
    // or if we need to check or select on values
    if(!hasIndex || !tableConds.empty() || (hasIndex && (attr == 2 || attr == 3))) {
      if ((rc = rf.read(rid, key, value)) < 0) {
        fprintf(stderr, "Error: while reading a tuple from table %s\n", table.c_str());
        goto exit_select;
      }
    }

    // check the table conditions on the tuple
    for (unsigned i = 0; i < tableConds.size(); i++) {
      if(!matchesCondition(tableConds[i], key, value, finishScan))
        goto next_tuple;
    }

    // the condition is met for the tuple. 
    // increase matching tuple counter
    count++;

    // print the tuple 
    switch (attr) {
    case 1:  // SELECT key
      fprintf(stdout, "%d\n", key);
      break;
    case 2:  // SELECT value
      fprintf(stdout, "%s\n", value.c_str());
      break;
    case 3:  // SELECT *
      fprintf(stdout, "%d '%s'\n", key, value.c_str());
      break;
    }

    // move to the next tuple
    next_tuple:

    if(hasIndex) {
      // otherwise continue reading
      rc = index.readForward(cursor, key, rid);

      // exit on end of tree or unknown errors
      if(rc == RC_END_OF_TREE) {
        break;
      } else if(rc < 0) {
        fprintf(stderr, "Error while reading from index for table %s\n", table.c_str());
        goto exit_select;
      }
    } else { // no index, read from table directly
      finishScan = ++rid >= rf.endRid();
    }
  }

  // print matching tuple count if "select count(*)"
  if (attr == 4) {
    fprintf(stdout, "%d\n", count);
  }
  rc = 0;

  // close the table file and return
  exit_select:
  rf.close();
  index.close();
  return rc;
}

RC SqlEngine::load(const string& table, const string& loadfile, bool index)
{
  // Status variables
  RC          rc = 0;
  RC          rfCloseStatus;
  RC          indexCloseStatus;

  // File handles
  ifstream    lfs;
  RecordFile  rf;

  // Buffer for reading from loadfile
  string      line;

  // Values for inserting into the table
  int         key;
  string      value;
  RecordId    rid;

  // Index handle
  BTreeIndex  dbIndex;

  try {
    lfs.open(loadfile.c_str(), std::ifstream::in);
  } catch(...) {
    return RC_FILE_OPEN_FAILED;
  }

  if(index && (rc = dbIndex.open((table + ".idx").c_str(), 'w')) < 0)
    return rc;

  if((rc = rf.open((table + ".tbl").c_str(), 'w')) < 0)
    return rc;

  while(!lfs.eof()) {
    getline(lfs, line);

    if((rc = parseLoadLine(line, key, value)) < 0)
      break;

    if((rc = rf.append(key, value, rid)) < 0)
      break;

    if(index && (rc = dbIndex.insert(key, rid)) < 0)
      break;
  }

  try {
    lfs.close();
  } catch(...) {
    rc = RC_FILE_CLOSE_FAILED;
  }

  if((rfCloseStatus = rf.close()) < 0)
    return rfCloseStatus;

  if(index && (indexCloseStatus = dbIndex.close()) < 0)
    return indexCloseStatus;

  return rc;
}

RC SqlEngine::parseLoadLine(const string& line, int& key, string& value)
{
    const char *s;
    char        c;
    string::size_type loc;
    
    // ignore beginning white spaces
    c = *(s = line.c_str());
    while (c == ' ' || c == '\t') { c = *++s; }

    // get the integer key value
    key = atoi(s);

    // look for comma
    s = strchr(s, ',');
    if (s == NULL) { return RC_INVALID_FILE_FORMAT; }

    // ignore white spaces
    do { c = *++s; } while (c == ' ' || c == '\t');
    
    // if there is nothing left, set the value to empty string
    if (c == 0) { 
        value.erase();
        return 0;
    }

    // is the value field delimited by ' or "?
    if (c == '\'' || c == '"') {
        s++;
    } else {
        c = '\n';
    }

    // get the value string
    value.assign(s);
    loc = value.find(c, 0);
    if (loc != string::npos) { value.erase(loc); }

    return 0;
}

/**
 * Filters out conditions into two types: those that can be resolved
 *    using only an index, and those that require reading the table itself
 * @param conds[IN] the mixed conditions
 * @param indexConds[OUT] conditions that can be resolved from the index only
 * @param tableConds[OUT] conditions that require reading the table in order to resolve
 * @return 0 on success, an error code otherwise
 */
RC SqlEngine::processConditions(const std::vector<SelCond>& conds, std::vector<SelCond>& indexConds, std::vector<SelCond>& tableConds) {
  indexConds.clear();
  tableConds.clear();

  for(unsigned i = 0; i < conds.size(); i++) {
    if(conds[i].attr == 1)
      indexConds.push_back(conds[i]);
    else
      tableConds.push_back(conds[i]);
  }

  return 0;
}

/**
 * Determines if a key and value pair satisfy a given condition
 * @param cond[IN] the condition to check against
 * @param key[IN] the key to check
 * @param value[IN] the value to check
 * @param terminate[OUT] indicates a possible early termination (i.e. if this condition is applied to the index)
 * @return true if the condition is matched, false otherwise
 */
bool SqlEngine::matchesCondition(const SelCond &cond, const int key, const string& value, bool& terminate) {
  int diff;
  bool match = false;

  terminate = false;

  // compute the difference between the tuple value and the condition value
  switch (cond.attr) {
  case 1:
    diff = key - atoi(cond.value);
    break;
  case 2: diff = strcmp(value.c_str(), cond.value);
    break;
  default:
    return false;
    break;
  }

  // skip the tuple if any condition is not met
  switch (cond.comp) {
    case SelCond::EQ: match = diff == 0; terminate = !match; break;
    case SelCond::NE: match = diff != 0; terminate =  false; break;
    case SelCond::GT: match = diff >  0; terminate =  false; break;
    case SelCond::LT: match = diff <  0; terminate = !match; break;
    case SelCond::GE: match = diff >= 0; terminate =  false; break;
    case SelCond::LE: match = diff <= 0; terminate = !match; break;
  }

  return match;
}
