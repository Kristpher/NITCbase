#include "OpenRelTable.h"
#include <iostream>
#include <stdlib.h>
#include <cstring>

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];
OpenRelTable::OpenRelTable()
{

  // initialize relCache and attrCache with nullptr
  for (int i = 0; i < MAX_OPEN; ++i)
  {
    RelCacheTable::relCache[i] = nullptr;
    AttrCacheTable::attrCache[i] = nullptr;
    OpenRelTable::tableMetaInfo[i].free = true;
  }

  /************ Setting up Relation Cache entries ************/
  // (we need to populate relation cache with entries for the relation catalog
  //  and attribute catalog.)

  /**** setting up Relation Catalog relation in the Relation Cache Table****/
  RecBuffer relCatBlock(RELCAT_BLOCK);

  Attribute relCatRecord[RELCAT_NO_ATTRS];
  relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

  struct RelCacheEntry relCacheEntry;
  RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
  relCacheEntry.recId.block = RELCAT_BLOCK;
  relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[RELCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;
  OpenRelTable::tableMetaInfo[RELCAT_RELID].free = false;
  strcpy(OpenRelTable::tableMetaInfo[RELCAT_RELID].relName, RelCacheTable::relCache[RELCAT_RELID]->relCatEntry.relName);

  /**** setting up Attribute Catalog relation in the Relation Cache Table ****/
  RecBuffer relCatBlock1(RELCAT_BLOCK);

  Attribute relCatRecord1[RELCAT_NO_ATTRS];
  relCatBlock1.getRecord(relCatRecord1, RELCAT_SLOTNUM_FOR_ATTRCAT);

  struct RelCacheEntry relCacheEntry1;
  RelCacheTable::recordToRelCatEntry(relCatRecord1, &relCacheEntry1.relCatEntry);
  relCacheEntry1.recId.block = RELCAT_BLOCK;
  relCacheEntry1.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

  // allocate this on the heap because we want it to persist outside this function
  RelCacheTable::relCache[ATTRCAT_RELID] = (struct RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  *(RelCacheTable::relCache[ATTRCAT_RELID]) = relCacheEntry1;
  OpenRelTable::tableMetaInfo[ATTRCAT_RELID].free = false;
  strcpy(OpenRelTable::tableMetaInfo[ATTRCAT_RELID].relName, RelCacheTable::relCache[ATTRCAT_RELID]->relCatEntry.relName);
  // set up the relation cache entry for the attribute catalog similarly
  // from the record at RELCAT_SLOTNUM_FOR_ATTRCAT

  // set the value at RelCacheTable::relCache[ATTRCAT_RELID]

  //   /************ Setting up Attribute cache entries ************/
  //   // (we need to populate attribute cache with entries for the relation catalog
  //   //  and attribute catalog.)

  //   /**** setting up Relation Catalog relation in the Attribute Cache Table ****/

  RecBuffer attrCatBlock1(ATTRCAT_BLOCK);
  Attribute attrCatRecord1[ATTRCAT_NO_ATTRS];
  struct AttrCacheEntry *attrCacheEntry1;
  struct AttrCacheEntry *attrCacheEntryTemp1 = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry));

  for (int ATTRCATRELIDX = 0; ATTRCATRELIDX <= 5; ATTRCATRELIDX++)
  {
    attrCatBlock1.getRecord(attrCatRecord1, ATTRCATRELIDX);
    attrCacheEntryTemp1->next = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord1, &(attrCacheEntryTemp1->next->attrCatEntry));
    if (ATTRCATRELIDX == 0)
    {
      attrCacheEntry1 = attrCacheEntryTemp1;
    }
    attrCacheEntryTemp1->next->recId.block = ATTRCAT_BLOCK;
    attrCacheEntryTemp1->next->recId.slot = ATTRCATRELIDX;
    attrCacheEntryTemp1 = attrCacheEntryTemp1->next;
  }
  attrCacheEntryTemp1->next = nullptr;
  AttrCacheTable::attrCache[RELCAT_RELID] = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
  *(AttrCacheTable::attrCache[RELCAT_RELID]) = *(attrCacheEntry1->next);
  attrCacheEntryTemp1 = attrCacheEntry1;
  attrCacheEntry1 = attrCacheEntry1->next;
  free(attrCacheEntryTemp1);

  //   // iterate through all the attributes of the relation catalog and create a linked
  //   // list of AttrCacheEntry (slots 0 to 5)
  //   // for each of the entries, set
  //   //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //   //    attrCacheEntry.recId.slot = i   (0 to 5)
  //   //    and attrCacheEntry.next appropriately
  //   // NOTE: allocate each entry dynamically using malloc

  //   // set the next field in the last entry to nullptr

  // RecBuffer attrCatBlockRel(ATTRCAT_BLOCK);

  //   Attribute attrCatRecordRel[ATTRCAT_NO_ATTRS];

  //   // iterate through all the attributes of the relation catalog and create a linked
  //   // list of AttrCacheEntry (slots 0 to 5)
  //   // for each of the entries, set
  //   //    attrCacheEntry.recId.block = ATTRCAT_BLOCK;
  //   //    attrCacheEntry.recId.slot = i   (0 to 5)
  //   //    and attrCacheEntry.next appropriately
  //   // NOTE: allocate each entry dynamically using malloc

  RecBuffer attrCatBlock2(ATTRCAT_BLOCK);

  Attribute attrCatRecord2[ATTRCAT_NO_ATTRS];
  struct AttrCacheEntry *attrCacheEntry2;
  struct AttrCacheEntry *attrCacheEntryTemp2 = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry));

  for (int ATTRCATRELIDX = 6; ATTRCATRELIDX <= 11; ATTRCATRELIDX++)
  {
    attrCatBlock2.getRecord(attrCatRecord2, ATTRCATRELIDX);
    attrCacheEntryTemp2->next = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry));
    AttrCacheTable::recordToAttrCatEntry(attrCatRecord2, &(attrCacheEntryTemp2->next->attrCatEntry));
    if (ATTRCATRELIDX == 6)
    {
      attrCacheEntry2 = attrCacheEntryTemp2;
    }
    attrCacheEntryTemp2->next->recId.block = ATTRCAT_BLOCK;
    attrCacheEntryTemp2->next->recId.slot = ATTRCATRELIDX;
    attrCacheEntryTemp2 = attrCacheEntryTemp2->next;
  }
  attrCacheEntryTemp2->next = nullptr;

  AttrCacheTable::attrCache[ATTRCAT_RELID] = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));
  *(AttrCacheTable::attrCache[ATTRCAT_RELID]) = *(attrCacheEntry2->next);
  attrCacheEntryTemp2 = attrCacheEntry2;
  attrCacheEntry2 = attrCacheEntry2->next;
  free(attrCacheEntryTemp2);

  //   // AttrCacheTable::attrCache[RELCAT_RELID] = /* head of the linked list */;

  //   /**** setting up Attribute Catalog relation in the Attribute Cache Table ****/

  //   // set up the attributes of the attribute cache similarly.
  //   // read slots 6-11 from attrCatBlock and initialise recId appropriately

  //   // set the value at AttrCacheTable::attrCache[ATTRCAT_RELID]
  // }

  // void freell(AttrCacheEntry* head) {
  //     AttrCacheEntry* current = head;
  //     AttrCacheEntry* nextNode;

  //     while (current != nullptr) {
  //         nextNode = current->next; //Save the next node
  //         free(current); //Free the current node
  //         current = nextNode; //Move to the next node
  //     }

  //     head = nullptr; // Optional: set head to NULL
  // }
  // OpenRelTable::~OpenRelTable() {
  //   for(int i=0;i<MAX_OPEN;i++){
  //     if(RelCacheTable::relCache[i]!=nullptr){
  //       free(RelCacheTable::relCache[i]);
  //     }
  //   }
  //     for(int i=0;i<MAX_OPEN;i++){
  //     if(AttrCacheTable::attrCache[i]!=nullptr){
  //       freell(AttrCacheTable::attrCache[i]);
  //     }
  //   }
  //   // free all the memory that you allocated in the constructor
}

void freeLinkedList(AttrCacheEntry *head)
{
  AttrCacheEntry *current = head;
  AttrCacheEntry *nextNode;

  while (current != nullptr)
  {
    nextNode = current->next; // Save the next node
    free(current);            // Free the current node
    current = nextNode;       // Move to the next node
  }

  head = nullptr; // Optional: set head to NULL
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE])
{

  // if relname is RELCAT_RELNAME, return RELCAT_RELID
  // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (!tableMetaInfo[i].free && strcmp(relName, RelCacheTable::relCache[i]->relCatEntry.relName) == 0)
      return i;
  }
  return E_RELNOTOPEN;
}

OpenRelTable::~OpenRelTable()
{
  
  // close all open relations (from rel-id = 2 onwards. Why?)
  for (int i = 2; i < MAX_OPEN; ++i)
  {
    if (!tableMetaInfo[i].free)
    {
      OpenRelTable::closeRel(i); // we will implement this function later
    }
  }
  
  // free all the memory that you allocated in the constructor
  // relcache free karo
  for (int i = 0; i < MAX_OPEN; ++i)
  {
    if (RelCacheTable::relCache[i] != nullptr)
    {
      free(RelCacheTable::relCache[i]);
    }
  }
  
  // attrcache free karo

    for(int i=0;i< MAX_OPEN;++i){
    if (AttrCacheTable::attrCache[i]!=nullptr){
      freeLinkedList(AttrCacheTable::attrCache[i]);
    }
  }
}

  // free the memory allocated for rel-id 0 and 1 in the caches


/* This function will open a relation having name `relName`.
Since we are currently only working with the relation and attribute catalog, we
will just hardcode it. In subsequent stages, we will loop through all the relations
and open the appropriate one.
*/
int OpenRelTable::getFreeOpenRelTableEntry()
{
  for (int i = 0; i < MAX_OPEN; i++)
  {
    if (tableMetaInfo[i].free)
      return i;
  }
  return E_CACHEFULL;
  /* traverse through the tableMetaInfo array,
    find a free entry in the Open Relation Table.*/

  // if found return the relation id, else return E_CACHEFULL.
 }

int OpenRelTable::openRel(char relName[ATTR_SIZE])
{
  /* the relation `relName` already has an entry in the Open Relation Table */
  int relId = OpenRelTable::getRelId(relName);
 
  if (relId >= 0)
  {
    // (checked using OpenRelTable::getRelId())
    return relId;
    // return that relation id;
  }

  relId = OpenRelTable::getFreeOpenRelTableEntry();
  /* find a free slot in the Open Relation Table
     using OpenRelTable::getFreeOpenRelTableEntry(). */
  

  if (relId == E_CACHEFULL)
  {
    return E_CACHEFULL;
  }

  // let relId be used to store the free slot.

  /****** Setting up Relation Cache entry for the relation ******/

  /* search for the entry with relation name, relName, in the Relation Catalog using
      BlockAccess::linearSearch().

      Care should be taken to reset the searchIndex of the relation RELCAT_RELID
      before calling linearSearch().*/

  // relcatRecId stores the rec-id of the relation `relName` in the Relation Catalog.
  RecId relcatRecId;

  Attribute attrVal;
  strcpy(attrVal.sVal, relName);
  // std::cout<<attrVal.sVal<<"Open:285";
  RelCacheTable::resetSearchIndex(RELCAT_RELID);
  relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, attrVal, EQ);
  
  if (relcatRecId.block == -1 && relcatRecId.slot == -1)
  {
    // (the relation is not found in the Relation Catalog.)
    return E_RELNOTEXIST;
  }
  RecBuffer BufferBlock(relcatRecId.block);
  Attribute rec[RELCAT_NO_ATTRS];
  BufferBlock.getRecord(rec, relcatRecId.slot);
  RelCacheEntry *relCacheBuffer =nullptr;
  //* NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  relCacheBuffer = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
  RelCacheTable::recordToRelCatEntry(rec, &(relCacheBuffer->relCatEntry));

  // update the recId field of this Relation Cache entry to relcatRecId.
  relCacheBuffer->recId.block = relcatRecId.block;
  relCacheBuffer->recId.slot = relcatRecId.slot;

  // use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
  RelCacheTable::relCache[relId] = relCacheBuffer;
  /* read the record entry corresponding to relcatRecId and create a relCacheEntry
      on it using RecBuffer::getRecord() and RelCacheTable::recordToRelCatEntry().
      update the recId field of this Relation Cache entry to relcatRecId.
      use the Relation Cache entry to set the relId-th entry of the RelCacheTable.
    NOTE: make sure to allocate memory for the RelCacheEntry using malloc()
  */

  /****** Setting up Attribute Cache entry for the relation ******/

  // let listHead be used to hold the head of the linked list of attrCache entries.
  AttrCacheEntry *listHead;
  RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

  /*iterate over all the entries in the Attribute Catalog corresponding to each
  attribute of the relation relName by multiple calls of BlockAccess::linearSearch()
  care should be taken to reset the searchIndex of the relation, ATTRCAT_RELID,
  corresponding to Attribute Catalog before the first call to linearSearch().*/

  struct AttrCacheEntry *attrCatEntryorg = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry));
  listHead = attrCatEntryorg;

  int block = ATTRCAT_BLOCK;

  while (1)
  {
    /* let attrcatRecId store a valid record id an entry of the relation, relName,
    in the Attribute Catalog.*/
    RecId attrcatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, RELCAT_ATTR_RELNAME, attrVal, EQ);
   
    if (attrcatRecId.block == -1 and attrcatRecId.slot==-1) break;
    RecBuffer BlockBuffer(attrcatRecId.block);
    Attribute rec[ATTRCAT_NO_ATTRS];
    BlockBuffer.getRecord(rec, attrcatRecId.slot);
    attrCatEntryorg->next = (struct AttrCacheEntry *)malloc(sizeof(struct AttrCacheEntry ));
    AttrCacheTable::recordToAttrCatEntry(rec, &(attrCatEntryorg->next->attrCatEntry));
  
    attrCatEntryorg->recId.block = attrcatRecId.block;
    attrCatEntryorg->recId.slot = attrcatRecId.slot;
    attrCatEntryorg = attrCatEntryorg->next;

    /* read the record entry corresponding to attrcatRecId and create an
    Attribute Cache entry on it using RecBuffer::getRecord() and
    AttrCacheTable::recordToAttrCatEntry().
    update the recId field of this Attribute Cache entry to attrcatRecId.
    add the Attribute Cache entry to the linked list of listHead .*/
    // NOTE: make sure to allocate memory for the AttrCacheEntry using malloc()
  }
  attrCatEntryorg->next = nullptr;
  AttrCacheTable::attrCache[relId] = listHead->next;
  attrCatEntryorg = listHead;
  listHead = listHead->next;
  delete attrCatEntryorg;
  tableMetaInfo[relId].free = false;
  strcpy(tableMetaInfo[relId].relName, relName);
  // set the relIdth entry of the AttrCacheTable to listHead.

  /****** Setting up metadata in the Open Relation Table for the relation******/

  // update the relIdth entry of the tableMetaInfo with free as false and
  // relName as the input.
  return relId;
}
int OpenRelTable::closeRel(int relId)
{
  if (relId == 0 || relId == 1)
  {
    return E_NOTPERMITTED;
  }
  if (relId < 0 || relId >=MAX_OPEN)
  {
    return E_OUTOFBOUND;
  }
  if (tableMetaInfo[relId].free)
  {
    return E_RELNOTOPEN;
  }
 
  if(RelCacheTable::relCache[relId]->dirty==true)
  {  
      Attribute record[RELCAT_NO_ATTRS];
      RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry),record);
      RecId recId =RelCacheTable::relCache[relId]->recId;
      RecBuffer relCatBlock(recId.block);
      relCatBlock.setRecord(record, RelCacheTable::relCache[relId]->recId.slot);
  }
  free(RelCacheTable::relCache[relId]);
  AttrCacheEntry *head = AttrCacheTable::attrCache[relId];
  AttrCacheEntry *next = head->next;
  while (next)
  {
    free(head);
    head = next;
    next = next->next;
  }
  free(head);
  // free the memory allocated in the relation and attribute caches which was
  // allocated in the OpenRelTable::openRel() function
  tableMetaInfo[relId].free=true;
  RelCacheTable::relCache[relId] = nullptr;
  AttrCacheTable::attrCache[relId] = nullptr;
  // update `tableMetaInfo` to set `relId` as a free slot
  // update `relCache` and `attrCache` to set the entry at `relId` to nullptr
  return SUCCESS;
}
