/*
 * implementation of class Scan for HeapFile project.
 * $Id: scan.C,v 1.1 1997/01/02 12:46:42 flisakow Exp $
 */

#include <stdio.h>
#include <stdlib.h>

#include "heapfile.h"
#include "scan.h"
#include "hfpage.h"
#include "buf.h"
#include "db.h"

// *******************************************
// The constructor pins the first page in the file
// and initializes its private data members from the private data members from hf
Scan::Scan(HeapFile *hf, Status &status)
{
  status = init(hf);
}

// *******************************************
// The deconstructor unpin all pages.
Scan::~Scan()
{
  // put your code here
  Status state;

  //cout<<numMINIBASE_BM->getNumBuffers();
  //state = MINIBASE_BM->unpinPage(dataPageId, FALSE, _hf->fileName);
  //state = MINIBASE_BM->unpinPage(dirPageId, FALSE, _hf->fileName);
}

// *******************************************
// Retrieve the next record in a sequential scan.
// Also returns the RID of the retrieved record.
Status Scan::getNext(RID &rid, char *recPtr, int &recLen)
{
  Status state;
  Page *t_page;
  char *t_recPtr;
  userRid.pageNo = dataPageId;

  // if slot is empty get first record else get the next record
  if (userRid.slotNo == -1)
  {
    state = dataPage->firstRecord(userRid);
  }
  else
  {
    state = dataPage->nextRecord(userRid, userRid);
  }

  // if sufficient space does not exist, we unpin the page and check for the next
  if (state == DONE)
  {
  //  state = MINIBASE_BM->unpinPage(dataPageId, FALSE, _hf->fileName);
    state = nextDataPage();
    if (state == DONE)
    {
      state = nextDirPage();
      if (state == DONE)
      {
       // state = MINIBASE_BM->unpinPage(dataPageId, FALSE, _hf->fileName);
        state = MINIBASE_BM->unpinPage(dirPageId, FALSE, _hf->fileName);
        return DONE;
      }
      else
      {
        firstDataPage();
      }
    }
    userRid.pageNo = dataPageId;

    state = dataPage->firstRecord(userRid);
  }

  state = dataPage->returnRecord(userRid, t_recPtr, recLen);
  memcpy(recPtr, t_recPtr, recLen);
  rid = userRid;
  return OK;
}

// *******************************************
// Do all the constructor work.
Status Scan::init(HeapFile *hf)
{
  // Initializing all the values

  Page *t_page;
  Status state;
  _hf = hf;
  dirPageId = hf->firstDirPageId;
  state = MINIBASE_BM->pinPage(dirPageId, t_page, 0, hf->fileName);
  dirPage = reinterpret_cast<HFPage *>(t_page);
  // state = MINIBASE_BM->unpinPage(dirPageId, FALSE, hf->fileName);
  dataPageId = -1;
  dataPageRid.pageNo = -1;
  dataPageRid.slotNo = -1;
  dataPage = NULL;
  state = firstDataPage();
  userRid.pageNo = -1;
  userRid.slotNo = -1;
  scanIsDone = -1;
  nxtUserStatus = -1;
  return OK;
}

// *******************************************
// Reset everything and unpin all pages.
Status Scan::reset()
{
  return OK;
}

// *******************************************
// Copy data about first page in the file.
Status Scan::firstDataPage()
{
  // scan for the first page and pin it
  Status state;
  struct DataPageInfo *temp;
  char *t_recPtr;
  Page *t_page;
  int len;
  dataPageRid.pageNo = dirPageId;
  state = dirPage->firstRecord(dataPageRid);
  if (state == DONE)
    return DONE;
  state = dirPage->returnRecord(dataPageRid, t_recPtr, len);
  temp = reinterpret_cast<struct DataPageInfo *>(t_recPtr);
  dataPageId = temp->pageId;
  state = MINIBASE_BM->pinPage(dataPageId, t_page, 0, _hf->fileName);
  dataPage = reinterpret_cast<HFPage *>(t_page);
  state = MINIBASE_BM->unpinPage(dataPageId, FALSE, _hf->fileName);
  return OK;
}

// *******************************************
// Retrieve the next data page.
Status Scan::nextDataPage()
{
  //scan for the next page and pin it

  Status state;
  struct DataPageInfo *temp;
  char *t_recPtr;
  Page *t_page;
  int len;

  state = dirPage->nextRecord(dataPageRid, dataPageRid);
  if (state == DONE)
    return DONE;
  state = dirPage->returnRecord(dataPageRid, t_recPtr, len);
  temp = reinterpret_cast<struct DataPageInfo *>(t_recPtr);
  dataPageId = temp->pageId;
  state = MINIBASE_BM->pinPage(dataPageId, t_page, 0, _hf->fileName);
  dataPage = reinterpret_cast<HFPage *>(t_page);
  state = MINIBASE_BM->unpinPage(dataPageId, FALSE, _hf->fileName);
  return OK;
}

// *******************************************
// Retrieve the next directory page.
Status Scan::nextDirPage()
{
  PageId next_id;
  Status state;
  Page *t_page;
  next_id = dirPage->getNextPage();
  if (next_id == -1)
    return DONE;
  state = MINIBASE_BM->unpinPage(dirPageId, FALSE, _hf->fileName);
  state = MINIBASE_BM->pinPage(next_id, t_page, 0, _hf->fileName);
  dirPage = reinterpret_cast<HFPage *>(t_page);
  dirPageId = next_id;
  return OK;
}
