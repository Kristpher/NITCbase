#include "StaticBuffer.h"
#include<iostream>
// the declarations for this class can be found at "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];
StaticBuffer::StaticBuffer() {
  int blockalloc=0;
  for(int i=0;i<4;i++){
    unsigned char buffer[BLOCK_SIZE];
    Disk::readBlock(buffer,i);
    for(int slot=0;slot<BLOCK_SIZE;slot++,blockalloc++){
      blockAllocMap[blockalloc]=buffer[slot];
    }
  }
  // initialise all blocks as free
  for (int bufferIndex=0;bufferIndex<32;bufferIndex++) {
    metainfo[bufferIndex].free = true;
    metainfo[bufferIndex].dirty=false;
    metainfo[bufferIndex].timeStamp=-1;
    metainfo[bufferIndex].blockNum=-1;
  }
}

/*
At this stage, we are not writing back from the buffer to the disk since we are
not modifying the buffer. So, we will define an empty destructor for now. In
subsequent stages, we will implement the write-back functionality here.
*/
StaticBuffer::~StaticBuffer() {
    int blockalloc=0;
  for(int i=0;i<4;i++){
    unsigned char buffer[BLOCK_SIZE];
    for(int slot=0;slot<BLOCK_SIZE;slot++,blockalloc++){
     buffer[slot]= blockAllocMap[blockalloc];
    }
       Disk::writeBlock(buffer, i);
  }
  for (int bufferIndex = 0; bufferIndex < BUFFER_CAPACITY; bufferIndex++) {
    if(metainfo[bufferIndex].free==false && metainfo[bufferIndex].dirty==true){
      Disk::writeBlock(blocks[bufferIndex],metainfo[bufferIndex].blockNum);
    }
  }
}

int StaticBuffer::getFreeBuffer(int blockNum) {
     // Check if blockNum is valid (non zero and less than DISK_BLOCKS)
    // and return E_OUTOFBOUND if not valid.
    if(blockNum<0 ||blockNum>DISK_BLOCKS)
    return E_OUTOFBOUND;
    int allocatedBuffer=-1;
    int timestamp=0;
    int maxim=0;
    for(int bufferIndex=0;bufferIndex<BUFFER_CAPACITY;bufferIndex++){
      if(metainfo[bufferIndex].timeStamp>timestamp){
        timestamp=metainfo[bufferIndex].timeStamp;
        maxim=bufferIndex;
      }
      if(metainfo[bufferIndex].free)
      {
        allocatedBuffer=bufferIndex;
        break;
      }

    }
       if(allocatedBuffer==-1){
      if(metainfo[maxim].dirty==true){
       Disk::writeBlock(blocks[maxim],metainfo[maxim].blockNum);
       allocatedBuffer=maxim;
      }
        }

    metainfo[allocatedBuffer].blockNum=blockNum;
    metainfo[allocatedBuffer].dirty=false;
    metainfo[allocatedBuffer].timeStamp=0;
    metainfo[allocatedBuffer].free=false;


 
    // increase the timeStamp in metaInfo of all occupied buffers.

    // let bufferNum be used to store the buffer number of the free/freed buffer.
   
   // iterate through metainfo and check if there is any buffer free

   // if a free buffer is available, set bufferNum = index of that free buffer.

   // if a free buffer is not available,
    //     find the buffer with the largest timestamp
    //     IF IT IS DIRTY, write back to the disk using Disk::writeBlock()
    //     set bufferNum = index of this buffer

    // update the metaInfo entry corresponding to bufferNum with
    // free:false, dirty:false, blockNum:the input block number, timeStamp:0.

    // return the bufferNum.
    return allocatedBuffer;
}

/* Get the buffer index where a particular block is stored
   or E_BLOCKNOTINBUFFER otherwise
*/
int StaticBuffer::getBufferNum(int blockNum) {
  // Check if blockNum is valid (between zero and DISK_BLOCKS)
  // and return E_OUTOFBOUND if not valid.
  if(blockNum<0 || blockNum>8091){
    return E_OUTOFBOUND;
  }
  for(int bufferIndex=0;bufferIndex<32;bufferIndex++){
   if(metainfo[bufferIndex].blockNum==blockNum &&metainfo[bufferIndex].free==false)
   return bufferIndex;
  }
  
  // find and return the bufferIndex which corresponds to blockNum (check metainfo)

  // if block is not in the buffer
  return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){
    // find the buffer index corresponding to the block using getBufferNum().
 
    int bufferNum=getBufferNum(blockNum);
    if(bufferNum == E_BLOCKNOTINBUFFER)
    return E_BLOCKNOTINBUFFER;
    
    if(blockNum==E_OUTOFBOUND)
    return E_OUTOFBOUND;
    else{
    metainfo[bufferNum].dirty=true;
    }
    // if block is not present in the buffer (bufferNum = E_BLOCKNOTINBUFFER)
    //     return E_BLOCKNOTINBUFFER
    
    // if blockNum is out of bound (bufferNum = E_OUTOFBOUND)
    //     return E_OUTOFBOUND

    // else
    //     (the bufferNum is valid)
    //     set the dirty bit of that buffer to true in metainfo

     return SUCCESS;
}