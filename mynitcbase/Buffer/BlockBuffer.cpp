#include "BlockBuffer.h"
#include <iostream>
#include <cstdlib>
#include <cstring>

BlockBuffer::BlockBuffer(int blockNum)
{

  if (blockNum < 0 || blockNum >= DISK_BLOCKS)
    this->blockNum = E_DISKFULL;
  else
    this->blockNum = blockNum;
}

BlockBuffer::BlockBuffer(char blockType)
{
  // allocate a block on the disk and a buffer in memory to hold the new block of
  // given type using getFreeBlock function and get the return error codes if any.
  int blocktype;
  if (blockType == 'R')
    blocktype = REC;
  else
    blocktype = UNUSED_BLK;
  int block_num = getFreeBlock(blocktype);
  if (block_num < 0 || block_num >= DISK_BLOCKS)
  {
    std::cout << "Error: Block is not available\n";
    this->blockNum = block_num;
    return;
  }

  this->blockNum = block_num;
  // set the block_numfield of the object to that of the allocated block
  // number if the method returned a valid block number,
  // otherwise set the error code returned as the block number.

  // (The caller must check if the constructor allocatted block successfully
  // by checking the value of block number field.)
}

int BlockBuffer::getHeader(struct HeadInfo *head)
{
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret; // return any errors that might have occured in the process
  }
  memcpy(&head->numSlots, bufferPtr + 24, 4);
  memcpy(&head->numEntries, bufferPtr + 16, 4);
  memcpy(&head->numAttrs, bufferPtr + 20, 4);
  memcpy(&head->rblock, bufferPtr + 12, 4);
  memcpy(&head->lblock, bufferPtr + 8, 4);

  return SUCCESS;
}
RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum) {}
RecBuffer::RecBuffer() : BlockBuffer('R') {}
// call parent non-default constructor with 'R' denoting record block.

// load the record at slotNum into the argument pointer
int RecBuffer::getRecord(union Attribute *rec, int slotNum)
{
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret; // return any errors that might have occured in the process
  }

  struct HeadInfo head;
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recordSize * slotNum);

  memcpy(rec, slotPointer, recordSize);
  return SUCCESS;
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum)
{
  unsigned char *bufferPtr;
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret; // return any errors that might have occured in the process
  }

  HeadInfo head;
  this->getHeader(&head);

  int attrCount = head.numAttrs;
  int slotCount = head.numSlots;
  if (slotNum > slotCount || slotNum < 0)
  {
    return E_OUTOFBOUND;
  }
  // if input slotNum is not in the permitted range return E_OUTOFBOUND.

  /* offset bufferPtr to point to the beginning of the record at required
     slot. the block contains the header, the slotmap, followed by all
     the records. so, for example,
     record at slot x will be at bufferPtr + HEADER_SIZE + (x*recordSize)
     copy the record from `rec` to buffer using memcpy
     (hint: a record will be of size ATTR_SIZE * numAttrs)
  */
  //  unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer, this->blockNum);

  int recordSize = attrCount * ATTR_SIZE;
  unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recordSize * slotNum);
  memcpy(slotPointer, rec, recordSize);
  // update dirty bit using setDirtyBit()
  int ret1 = StaticBuffer::setDirtyBit(this->blockNum);
  if (ret1 != SUCCESS)
  {
    std::cout << "something wrong with the setDirty function";
  }
  /* (the above function call should not fail since the block is already
     in buffer and the blockNum is valid. If the call does fail, there
     exists some other issue in the code) */

  // return SUCCESS

  return SUCCESS;
}
int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr)
{
  // check whether the block is already present in the buffer using StaticBuffer.getBufferNum()
  int bufferNum = StaticBuffer::getBufferNum(this->blockNum);
  // if present (!=E_BLOCKNOTINBUFFER),
  // set the timestamp of the corresponding buffer to 0 and increment the
  // timestamps of all other occupied buffers in BufferMetaInfo.
  if (bufferNum != E_BLOCKNOTINBUFFER)
  {

    for (int i = 0; i < 32; i++)
    {

      StaticBuffer::metainfo[i].timeStamp++;
    }
    StaticBuffer::metainfo[bufferNum].timeStamp = 0;
  }
  else
  {
    bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

    if (bufferNum == E_OUTOFBOUND)
    {
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
int RecBuffer::getSlotMap(unsigned char *slotMap)
{
  unsigned char *bufferPtr;

  // get the starting address of the buffer containing the block using loadBlockAndGetBufferPtr().
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
  {
    return ret;
  }

  struct HeadInfo head;
  // get the header of the block using getHeader() function
  this->getHeader(&head);
  // int slotCount = /* number of slots in block from header */;
  int slotCount = head.numSlots;

  // get a pointer to the beginning of the slotmap in memory by offsetting HEADER_SIZE
  unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

  // copy the values from `slotMapInBuffer` to `slotMap` (size is `slotCount`)
  memcpy(slotMap, slotMapInBuffer, slotCount);
  return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head)
{

  unsigned char *bufferPtr;
  // get the starting address of the buffer containing the block using
  // loadBlockAndGetBufferPtr(&bufferPtr).
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  if (ret != SUCCESS)
    return ret;

  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  // return the value returned by the call.

  // cast bufferPtr to type HeadInfo*
  struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;

  // copy the fields of the HeadInfo pointed to by head (except reserved) to
  // the header of the block (pointed to by bufferHeader)
  //(hint: bufferHeader->numSlots = head->numSlots )
  bufferHeader->blockType = head->blockType;
  bufferHeader->lblock = head->lblock;
  bufferHeader->numAttrs = head->numAttrs;
  bufferHeader->numEntries = head->numEntries;
  bufferHeader->numSlots = head->numSlots;
  bufferHeader->pblock = head->pblock;
  bufferHeader->rblock = head->rblock;
  StaticBuffer::setDirtyBit(this->blockNum);
  // update dirty bit by calling StaticBuffer::setDirtyBit()
  // if setDirtyBit() failed, return the error code

  return SUCCESS;
}

int BlockBuffer::setBlockType(int blockType)
{
  // printf("checking in blockbuffer checkpoint 1");
  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block
     using loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  // return the value returned by the call.
  if (ret != SUCCESS)
    return ret;
  // store the input block type in the first 4 bytes of the buffer.
  // (hint: cast bufferPtr to int32_t* and then assign it)
  // *((int32_t *)bufferPtr) = blockType;
  *((int32_t *)bufferPtr) = blockType;
  // update the StaticBuffer::blockAllocMap entry corresponding to the
  // object's block number to `blockType`.
  StaticBuffer::blockAllocMap[this->blockNum] = blockType;
  // update dirty bit by calling StaticBuffer::setDirtyBit()
  int db = StaticBuffer::setDirtyBit(this->blockNum);
  return db;
  // if setDirtyBit() failed
  // return the returned value from the call

  // return SUCCESS
}

int BlockBuffer::getFreeBlock(int blockType)
{
  // printf("checking in blockbuffer checkpoint 1");
  // iterate through the StaticBuffer::blockAllocMap and find the block number
  // of a free block in the disk.
  int blk_num;
  for (blk_num = 0; blk_num < DISK_BLOCKS; blk_num++)
  {
    if (StaticBuffer::blockAllocMap[blk_num] == UNUSED_BLK)
    {

      break;
    }
  }
  if (blk_num == DISK_BLOCKS)
    return E_DISKFULL;
  // if no block is free, return E_DISKFULL.
  this->blockNum = blk_num;
  // set the object's blockNum to the block number of the free block.

  // find a free buffer using StaticBuffer::getFreeBuffer() .
  int bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);
  if (bufferNum < 0 || bufferNum >= BUFFER_CAPACITY)
  {
    printf("Error:buffer is full\n");
    return bufferNum;
  }
  // initialize the header of the block passing a struct HeadInfo with values
  // pblock: -1, lblock: -1, rblock: -1, numEntries: 0, numAttrs: 0, numSlots: 0
  // to the setHeader() function.
  struct HeadInfo header;
  header.pblock = -1;
  header.lblock = -1;
  header.lblock = -1;
  header.numEntries = 0;
  header.numAttrs = 0;
  header.numSlots = 0;
  setHeader(&header);
  // printf("checking in blockbuffer checkpoint 1");
  // update the block type of the block to the input block type using setBlockType().
  setBlockType(blockType);
  return blk_num;
  // return block number of the free block.
}

int compareAttrs(union Attribute attr1, union Attribute attr2, int attrType)
{
  return attrType == NUMBER ? (attr1.nVal < attr2.nVal ? -1 : (attr1.nVal > attr2.nVal ? 1 : 0)) : strcmp(attr1.sVal, attr2.sVal);
}

int RecBuffer::setSlotMap(unsigned char *slotMap)
{
  unsigned char *bufferPtr;
  /* get the starting address of the buffer containing the block using
     loadBlockAndGetBufferPtr(&bufferPtr). */
  int ret = loadBlockAndGetBufferPtr(&bufferPtr);
  // if loadBlockAndGetBufferPtr(&bufferPtr) != SUCCESS
  if (ret != SUCCESS)
    return ret;
  // return the value returned by the call.

  // get the header of the block using the getHeader() function
  HeadInfo head;
  getHeader(&head);
  int numSlots = head.numSlots; /* the number of slots in the block */
  ;
  memcpy(bufferPtr + HEADER_SIZE, slotMap, numSlots);
  // the slotmap starts at bufferPtr + HEADER_SIZE. Copy the contents of the
  // argument `slotMap` to the buffer replacing the existing slotmap.
  // Note that size of slotmap is `numSlots`
  ret = StaticBuffer::setDirtyBit(this->blockNum);
  // update dirty bit using StaticBuffer::setDirtyBit
  // if setDirtyBit failed, return the value returned by the call
  // if(ret!=SUCCESS)
  // return ret;
  return SUCCESS;
}
int BlockBuffer::getBlockNum()
{
  return this->blockNum;
  // return corresponding block number.
}