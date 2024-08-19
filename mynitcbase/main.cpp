#include "Buffer/StaticBuffer.h"
#include "Cache/OpenRelTable.h"
#include "Disk_Class/Disk.h"
#include "FrontendInterface/FrontendInterface.h"
#include <iostream>
#include <cstdio>
#include <cstring>

void stage2_ex1(){
   // StaticBuffer buffer;
  // OpenRelTable cache;
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;
  HeadInfo storePointer;
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);
 

  
  for (int i = 0; i < relCatHeader.numEntries; i++) {
    int flag=0;
    Attribute relCatRecord[RELCAT_NO_ATTRS];
    relCatBuffer.getRecord(relCatRecord, i);

    printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Process all attribute blocks for the current relation
    int attrCatBlock = ATTRCAT_BLOCK;
    while (attrCatBlock!=-1) {
      attrCatBuffer = RecBuffer(attrCatBlock);
      attrCatBuffer.getHeader(&attrCatHeader);
      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, relCatRecord[RELCAT_REL_NAME_INDEX].sVal) == 0) {
          const char *attrType = attrCatRecord[ATTRCAT_ATTR_TYPE_INDEX].nVal == NUMBER ? "NUM" : "STR";
          printf("  %s: %s\n", attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, attrType);
        }
      }
      attrCatBlock = attrCatHeader.rblock;
    }

    // Reset the attribute catalog header to the initial state
    printf("\n");
  }

}

void stage2_ex2(const char*relname,const char*attriname,const char* attr2name){
  RecBuffer relCatBuffer(RELCAT_BLOCK);
  RecBuffer attrCatBuffer(ATTRCAT_BLOCK);

  HeadInfo relCatHeader;
  HeadInfo attrCatHeader;
  HeadInfo storePointer;
  relCatBuffer.getHeader(&relCatHeader);
  attrCatBuffer.getHeader(&attrCatHeader);
 

  
  // for (int i = 0; i < relCatHeader.numEntries; i++) {
  //   int flag=0;
  //   Attribute relCatRecord[RELCAT_NO_ATTRS];
  //   relCatBuffer.getRecord(relCatRecord, i);

  //   printf("Relation: %s\n", relCatRecord[RELCAT_REL_NAME_INDEX].sVal);

    // Process all attribute blocks for the current relation
    int attrCatBlock = ATTRCAT_BLOCK;
    while (attrCatBlock!=-1) {
      attrCatBuffer = RecBuffer(attrCatBlock);
      attrCatBuffer.getHeader(&attrCatHeader);
      for (int j = 0; j < attrCatHeader.numEntries; j++) {
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBuffer.getRecord(attrCatRecord, j);
      if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,relname) == 0) {
          if (strcmp(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attriname) == 0) 
         {
          strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal,attr2name);
          attrCatBuffer.setRecord(attrCatRecord,j);
          return;
         }
        }
      }
      attrCatBlock = attrCatHeader.rblock;
    }

    // Reset the attribute catalog header to the initial state
  
  // }

}
void STAGE3_READING_FROM_CACHE(){
    for(int rel_Id= RELCAT_RELID;rel_Id<=2;++rel_Id){
      RelCatEntry relcatbuf;
      RelCacheTable::getRelCatEntry(rel_Id,&relcatbuf);
      printf("Relation: %s\n", relcatbuf.relName);

      for (int j = 0; j<relcatbuf.numAttrs;j++){
          AttrCatEntry attrcatbuf;
          AttrCacheTable::getAttrCatEntry(rel_Id, j,&attrcatbuf);//attribute offset j
          printf("  %s: %d\n", attrcatbuf.attrName, attrcatbuf.attrType);
      }
  
    }
}

int main(int argc, char *argv[]) {
  /* Initialize the Run Copy of Disk */
  Disk disk_run;
  StaticBuffer buffer;
  OpenRelTable cache;
  //  unsigned char buffer[BLOCK_SIZE];
  // Disk::readBlock(buffer, 7000);
  // char message[] = "hello";
  // memcpy(buffer + 20, message, 6);
  // Disk::writeBlock(buffer, 7000);

  // unsigned char buffer2[BLOCK_SIZE];
  // char message2[6];
  // Disk::readBlock(buffer2, 7000);
  // memcpy(message2, buffer2 + 20, 6);
  // std::cout << message2;
  // stage2_ex1();
  // stage2_ex2("Students","Class","Batch");
  //stage3_p1();
  //STAGE3_READING_FROM_CACHE();
  return FrontendInterface::handleFrontend(argc, argv);
  return 0;
}

