#include "BlockBuffer.h"
#include<iostream>
#include <cstdlib>
#include <cstring>


BlockBuffer::BlockBuffer(int blockNum) {

  if (blockNum < 0 || blockNum >= DISK_BLOCKS)
		this->blockNum = E_DISKFULL;
	else
		this->blockNum = blockNum;
}


RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}


int BlockBuffer::getHeader(struct HeadInfo *head) {
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }
  unsigned char buffer[BLOCK_SIZE];
  Disk::readBlock(buffer,this->blockNum);
  memcpy(&head->numSlots, buffer + 24, 4);
  memcpy(&head->numEntries,buffer + 16, 4);
  memcpy(&head->numAttrs,buffer + 20, 4);
  memcpy(&head->rblock,buffer + 12, 4);
  memcpy(&head->lblock,buffer + 8, 4);

  return SUCCESS;
}

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }


    struct HeadInfo head;
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;

    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer, this->blockNum);

    int recordSize = attrCount*ATTR_SIZE;
    unsigned char *slotPointer = buffer + HEADER_SIZE + slotCount + (recordSize*slotNum);

    memcpy(rec, slotPointer, recordSize);
    return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum) {
    unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;   // return any errors that might have occured in the process
  }

    struct HeadInfo head;
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;
    
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer, this->blockNum);

    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = buffer + HEADER_SIZE + slotCount + (recordSize * slotNum);
   
   
    memcpy(slotPointer, rec, recordSize);
    Disk::writeBlock(buffer, blockNum);

    return SUCCESS;
}
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr) {
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

  if (bufferNum == E_BLOCKNOTINBUFFER) {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND) {
      return E_OUTOFBOUND;
    }

    Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
  }

  // store the pointer to this buffer (blocks[bufferNum]) in *buffPtr
  *buffPtr = StaticBuffer::blocks[bufferNum];

  return SUCCESS;
}


/* used to get the slotmap from a record block
NOTE: this function expects the caller to allocate memory for `*slotMap`
*/
int RecBuffer::getSlotMap(unsigned char *slotMap) {
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS) {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
    this->getHeader(&head);
  //int slotCount = /* number of slots in block from header */;
  int slotCount=head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);
  return SUCCESS;
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType) {
  return attrType == NUMBER ? 
		(attr1.nVal < attr2.nVal ? -1 : (attr1.nVal > attr2.nVal ? 1 : 0)) : 
		strcmp(attr1.sVal, attr2.sVal) ;
}