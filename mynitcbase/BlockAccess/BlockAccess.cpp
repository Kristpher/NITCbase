#include "BlockAccess.h"
#include <cstdlib>
#include <cstring>
#include <stdio.h>
#include <iostream>

RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
{
    // get the previous search index of the relation relId from the relation cache
    // (use RelCacheTable::getSearchIndex() function)
    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId, &prevRecId);
    // let block and slot denote the record id of the record being currently checked
    int block = -1, slot = -1;
    // if the current search index record is invalid(i.e. both block and slot = -1)
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (no hits from previous search; search should start from the
        // first record itself)

        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
        RelCatEntry firstnode;
        RelCacheTable::getRelCatEntry(relId, &firstnode);
        // block = first record block of the relation
        block = firstnode.firstBlk;
        slot = 0;
        // slot = 0
    }
    else
    {
        // (there is a hit from previous search; search should start from
        // the record next to the search index record)

        // block = search index's block
        // slot = search index's slot + 1
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }
    RelCatEntry relCatBuffer;
    RelCacheTable::getRelCatEntry(relId, &relCatBuffer);
    /* The following code searches for the next record in the relation
       that satisfies the given condition
       We start from the record id (block, slot) and iterate over the remaining
       records of the relation
    */

    while (block != -1)
    {
        /* create a RecBuffer object for block (use RecBuffer Constructor for
           existing block) */
        RecBuffer Buffer(block);
        HeadInfo header;
        Attribute CatRecord[RELCAT_NO_ATTRS];
        // get the record with id (block, slot) using RecBuffer::getRecord()
        Buffer.getRecord(CatRecord, slot);
        // get header of the block using RecBuffer::getHeader() function
        Buffer.getHeader(&header);
        // get slot map of the block using RecBuffer::getSlotMap() function
        unsigned char *slotMap = (unsigned char *)malloc(sizeof(unsigned char) * header.numSlots);
        Buffer.getSlotMap(slotMap);
        // If slot >= the number of slots per block(i.e. no more slots in this block)
        if (slot >= relCatBuffer.numSlotsPerBlk)
        {
            block = header.rblock;
            slot = 0;
            // update block = right block of block
            // update slot = 0
            continue; // continue to the beginning of this while loop
        }

        // if slot is free skip the loop
        // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
        if (slotMap[slot] == SLOT_UNOCCUPIED)
        {
            slot++;
            continue;
            // increment slot and continue to the next record slot
        }

        // compare record's attribute value to the the given attrVal as below:
        /*
            firstly get the attribute offset for the attrName attribute
            from the attribute cache entry of the relation using
            AttrCacheTable::getAttrCatEntry()
        */
        AttrCatEntry attrcatbuff;
        AttrCacheTable::getAttrCatEntry(relId, attrName, &attrcatbuff);
        Attribute *record = (Attribute *)malloc(sizeof(Attribute) * header.numAttrs);
        Buffer.getRecord(record, slot);
        int attr_offset = attrcatbuff.offset;
        /* use the attribute offset to get the value of the attribute from
           current record */

        int cmpVal;
        cmpVal = compareAttrs(record[attr_offset], attrVal, attrcatbuff.attrType);
        // will store the difference between the attributes
        // set cmpVal using compareAttrs()

        /* Next task is to check whether this record satisfies the given condition.
           It is determined based on the output of previous comparison and
           the op value received.
           The following code sets the cond variable if the condition is satisfied.
        */
        if (
            (op == NE && cmpVal != 0) || // if op is "not equal to"
            (op == LT && cmpVal < 0) ||  // if op is "less than"
            (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) || // if op is "equal to"
            (op == GT && cmpVal > 0) ||  // if op is "greater than"
            (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
        )
        {
            /*
            set the search index in the relation cache as
            the record id of the record that satisfies the given condition
            (use RelCacheTable::setSearchIndex function)
            */
            RecId newIndex;
            newIndex.block = block;
            newIndex.slot = slot;
            RelCacheTable::setSearchIndex(relId, &newIndex);
            return RecId{block, slot};
        }

        slot++;
    }

    // no record in the relation with Id relid satisfies the given condition
    return RecId{-1, -1};
}

int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute newRelationName; // set newRelationName with newName
    strcpy(newRelationName.sVal, newName);
    RecId recid;
    recid = linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, newRelationName, EQ);
    if (recid.block != -1 && recid.slot != -1)
        return E_RELEXIST;
    // search the relation catalog for an entry with "RelName" = newRelationName

    // If relation with name newName already exists (result of linearSearch
    //                                               is not {-1, -1})
    //    return E_RELEXIST;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */

    Attribute oldRelationName; // set oldRelationName with oldName
    strcpy(oldRelationName.sVal, oldName);

    // search the relation catalog for an entry with "RelName" = oldRelationName
    recid = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    if (recid.block == -1 && recid.slot == -1)
    {
        return E_RELNOTEXIST;
    }

    // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
    //    return E_RELNOTEXIST;
    RecBuffer getRelation(recid.block);
    Attribute changeRel[RELCAT_NO_ATTRS];
    getRelation.getRecord(changeRel, recid.slot);
    strcpy(changeRel[RELCAT_REL_NAME_INDEX].sVal, newName);
    getRelation.setRecord(changeRel, recid.slot);
    /* get the relation catalog record of the relation to rename using a RecBuffer
       on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
    */
    /* update the relation name attribute in the record with newName.
       (use RELCAT_REL_NAME_INDEX) */
    // set back the record value using RecBuffer.setRecord

    /*

    update all the attribute catalog entries in the attribute catalog corresponding
    to the relation with relation name oldName to the relation name newName
    */
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for (int i = 0; i < changeRel[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++)
    {
        recid = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
        RecBuffer getRelation1(recid.block);
        Attribute changeAttr[ATTRCAT_NO_ATTRS];
        getRelation1.getRecord(changeAttr, recid.slot);
        strcpy(changeAttr[ATTRCAT_REL_NAME_INDEX].sVal, newName);
        getRelation1.setRecord(changeAttr, recid.slot);
    }

    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */

    // for i = 0 to numberOfAttributes :
    //     linearSearch on the attribute catalog for relName = oldRelationName
    //     get the record using RecBuffer.getRecord
    //
    //     update the relName field in the record to newName
    //     set back the record using RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
{

    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
    RelCacheTable::resetSearchIndex(RELCAT_RELID);
    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);
    RecId recid; // set relNameAttr to relName
    recid = BlockAccess::linearSearch(RELCAT_RELID, RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if (recid.block == -1 && recid.slot == -1)
        return E_RELNOTEXIST;

    // Search for the relation with name relName in relation catalog using linearSearch()
    // If relation with name relName does not exist (search returns {-1,-1})
    //    return E_RELNOTEXIST;
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    /* reset the searchIndex of the attribute catalog using
       RelCacheTable::resetSearchIndex() */

    /* declare variable attrToRenameRecId used to store the attr-cat recId
    of the attribute to rename */
    RecId attrToRenameRecId{-1, -1};

    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    /* iterate over all Attribute Catalog Entry record corresponding to the
       relation to find the required attribute */
    while (true)
    {
        // linear search on the attribute catalog for RelName = relNameAttr
        RecId attrRec = BlockAccess::linearSearch(ATTRCAT_RELID, ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if (attrRec.block == -1 && attrRec.slot != -1)
            break;

        // if there are no more attributes left to check (linearSearch returned {-1,-1})
        //     break;
        RecBuffer changeAttr(attrRec.block);
        changeAttr.getRecord(attrCatEntryRecord, attrRec.slot);
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
        {
            attrToRenameRecId.block = attrRec.block;
            attrToRenameRecId.slot = attrRec.slot;
            break;
        }
        if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
            return E_ATTREXIST;

        /* Get the record from the attribute catalog using RecBuffer.getRecord
          into attrCatEntryRecord */

        // if attrCatEntryRecord.attrName = oldName
        //     attrToRenameRecId = block and slot of this record

        // if attrCatEntryRecord.attrName = newName
        //     return E_ATTREXIST;
    }

    // if attrToRenameRecId == {-1, -1}
    //     return E_ATTRNOTEXIST;
    if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
        {printf("blockaccess renameAttribute\n");
            return E_ATTRNOTEXIST;
        }
    RecBuffer changeAttribute(attrToRenameRecId.block);
    Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
    changeAttribute.getRecord(attrCatRecord, attrToRenameRecId.slot);
    strcpy(attrCatRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
    changeAttribute.setRecord(attrCatRecord, attrToRenameRecId.slot);

    // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
    /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
         attrToRenameRecId.slot */
    //   update the AttrName of the record with newName
    //   set back the record with RecBuffer.setRecord

    return SUCCESS;
}

int BlockAccess::insert(int relId, Attribute *record)
{
    // get the relation catalog entry from relation cache
    RelCatEntry relcatentry;
    RelCacheTable::getRelCatEntry(relId, &relcatentry);
    // ( use RelCacheTable::getRelCatEntry() of Cache Layer)

    int blockNum = relcatentry.firstBlk; /* first record block of the relation (from the rel-cat entry)*/
    // std::cout<<blockNum<<std::endl;
    // rec_id will be used to store where the new record will be inserted
    RecId recId = {-1, -1};

    int numOfSlots = relcatentry.numSlotsPerBlk; /* number of slots per record block */
    int numOfAttributes = relcatentry.numAttrs; /* number of attributes of the relation */

    int prevBlockNum = -1; /* block number of the last element in the linked list = -1 */
    

    /*
        Traversing the linked list of existing record blocks of the relation
        until a free slot is found OR
        until the end of the list is reached
    */
    // printf("checking in blockaccess checkpoint 1\n");
    while (blockNum != -1)
    {
        // create a RecBuffer object for blockNum (using appropriate constructor!)
        RecBuffer recBuffer(blockNum);
        HeadInfo header;
        recBuffer.getHeader(&header);
        // get header of block(blockNum) using RecBuffer::getHeader() function
        unsigned char slotMap[header.numSlots];
        recBuffer.getSlotMap(slotMap);
        // get slot map of block(blockNum) using RecBuffer::getSlotMap() function

        // search for free slot in the block 'blockNum' and store it's rec-id in rec_id
        // (Free slot can be found by iterating over the slot map of the block)
        /* slot map stores SLOT_UNOCCUPIED if slot is free and
           SLOT_OCCUPIED if slot is occupied) */
        // printf("checking in blockaccess checkpoint 2\n");
        for (int i = 0; i < header.numSlots; i++)
        {
            if (slotMap[i] == SLOT_UNOCCUPIED)
            {
                recId.block = blockNum;
                recId.slot = i;
                break;
            }
        }
        // printf("checking in blockaccess checkpoint 3\n");
        /* if a free slot is found, set rec_id and discontinue the traversal
           of the linked list of record blocks (break from the loop) */

        /* otherwise, continue to check the next block by updating the
           block numbers as follows:
              update prevBlockNum = blockNum
              update blockNum = header.rblock (next element in the linked
                                               list of record blocks)
        */
        if (recId.slot != -1 && recId.block != -1)
            break;
        prevBlockNum = blockNum;
        blockNum = header.rblock;
        // std::cout<<"bloknum is "<<blockNum<<std::endl;
    }
    // printf("checking in blockaccess checkpoint 4\n");
    //  if no free slot is found in existing record blocks (recId = {-1, -1})
    if (recId.block == -1 and recId.slot == -1)
    {
        {
            if (relId == RELCAT_RELID)
                return E_MAXRELATIONS;
            // if relation is RELCAT, do not allocate any more blocks
            //     return E_MAXRELATIONS;

            // Otherwise,
            // get a new record block (using the appropriate RecBuffer constructor!)
            // get the block number of the newly allocated block
            // (use BlockBuffer::getBlockNum() function)
            // let ret be the return value of getBlockNum() function call
            RecBuffer blockBuffer;
            int ret = blockBuffer.getBlockNum();
            // printf("block num is %d\n",ret);
            if (ret == E_DISKFULL)
            {
                printf("blockaccess insert problem");
                return E_DISKFULL;
            }

            // Assign recId.block = new block number(i.e. ret) and recId.slot = 0
            recId.block = ret;
            recId.slot = 0;
            /*
                set the header of the new record block such that it links with
                existing record blocks of the relation
                set the block's header as follows:
                blockType: REC, pblock: -1
                lblock
                      = -1 (if linked list of existing record blocks was empty
                             i.e this is the first insertion into the relation)
                      = prevBlockNum (otherwise),
                rblock: -1, numEntries: 0,
                numSlots: numOfSlots, numAttrs: numOfAttributes
                (use BlockBuffer::setHeader() function)
            */
            HeadInfo header_block;
            header_block.blockType = REC;
            header_block.lblock = prevBlockNum;
            header_block.rblock = -1;
            header_block.pblock = -1;
            header_block.numAttrs = relcatentry.numAttrs;
            header_block.numEntries = 0;
            header_block.numSlots = relcatentry.numSlotsPerBlk;
            blockBuffer.setHeader(&header_block);
            // printf("checking in blockaccess checkpoint 5\n");
            /*
                set block's slot map with all slots marked as free
                (i.e. store SLOT_UNOCCUPIED for all the entries)
                (use RecBuffer::setSlotMap() function)
            */
            unsigned char slotMap[relcatentry.numSlotsPerBlk];
            for (int slot = 0; slot < relcatentry.numSlotsPerBlk; slot++)
            {
                slotMap[slot] = SLOT_UNOCCUPIED;
            }
            blockBuffer.setSlotMap(slotMap);
            // if prevBlockNum != -1
            if (prevBlockNum != -1)
            {
                RecBuffer newbuf(prevBlockNum);
                HeadInfo prevheader;
                newbuf.getHeader(&prevheader);
                prevheader.rblock = recId.block;
                newbuf.setHeader(&prevheader);
                // create a RecBuffer object for prevBlockNum
                // get the header of the block prevBlockNum and
                // update the rblock field of the header to the new block
                // number i.e. recId.block
                // (use BlockBuffer::setHeader() function)
            }
            else
            {
                relcatentry.firstBlk = recId.block;
                RelCacheTable::setRelCatEntry(relId, &relcatentry);
                // update first block field in the relation catalog entry to the
                // new block (using RelCacheTable::setRelCatEntry() function)
            }
            relcatentry.lastBlk = recId.block;
            RelCacheTable::setRelCatEntry(relId, &relcatentry);
            // update last block field in the relation catalog entry to the
            // new block (using RelCacheTable::setRelCatEntry() function)
        }
    }
    // create a RecBuffer object for recId.block
    // insert the record into recId'th slot using RecBuffer.setRecord())
    RecBuffer rec(recId.block);
    int out = rec.setRecord(record, recId.slot);
    if (out != SUCCESS)
    {
        printf("Record not saved successfully.\n");
        return out;
    }

    /* update the slot map of the block by marking entry of the slot to
       which record was inserted as occupied) */
    // (ie store SLOT_OCCUPIED in free_slot'th entry of slot map)
    // (use RecBuffer::getSlotMap() and RecBuffer::setSlotMap() functions)
    unsigned char slotMap[relcatentry.numSlotsPerBlk];
    rec.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_OCCUPIED;
    rec.setSlotMap(slotMap);
    // increment the numEntries field in the header of the block to
    // which record was inserted
    // (use BlockBuffer::getHeader() and BlockBuffer::setHeader() functions)
    HeadInfo hd;
    rec.getHeader(&hd);
    hd.numEntries++;
    rec.setHeader(&hd);
    // Increment the number of records field in the relation cache entry for
    // the relation. (use RelCacheTable::setRelCatEntry function)
    relcatentry.numRecs++;
    // printf("checking in blockaccess checkpoint 6\n");
    RelCacheTable::setRelCatEntry(relId, &relcatentry);
    //stage 11

       int flag = SUCCESS;
    // Iterate over all the attributes of the relation
    // (let attrOffset be iterator ranging from 0 to numOfAttributes-1)
    for(int attrOffset =0; attrOffset<numOfAttributes; attrOffset++)
    {
        // get the attribute catalog entry for the attribute from the attribute cache
        // (use AttrCacheTable::getAttrCatEntry() with args relId and attrOffset)
        AttrCatEntry attrCatEntry;
        AttrCacheTable::getAttrCatEntry(relId, attrOffset, &attrCatEntry);

        // get the root block field from the attribute catalog entry

        // if index exists for the attribute(i.e. rootBlock != -1)
        if(attrCatEntry.rootBlock != -1)
        {
            /* insert the new record into the attribute's bplus tree using
             BPlusTree::bPlusInsert()*/
            int retVal = BPlusTree::bPlusInsert(relId, attrCatEntry.attrName,
                                                record[attrOffset], recId);

            if (retVal == E_DISKFULL) {
                //(index for this attribute has been destroyed)
                // flag = E_INDEX_BLOCKS_RELEASED
                flag = E_INDEX_BLOCKS_RELEASED;
                BPlusTree::bPlusDestroy(attrCatEntry.rootBlock);
            }
        }
    }
  return flag;

}

/*
NOTE: This function will copy the result of the search to the `record` argument.
      The caller should ensure that space is allocated for `record` array
      based on the number of attributes in the relation.
*/





// int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
//     // Declare a variable called recid to store the searched record
//     RecId recId;

//     /* get the attribute catalog entry from the attribute cache corresponding
//     to the relation with Id=relid and with attribute_name=attrName  */
//     AttrCatEntry attrRelcatEntry;
//     int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrRelcatEntry);
//     if(ret!=SUCCESS)
//     return ret;

//     // if this call returns an error, return the appropriate error code
//     int rootBlock=attrRelcatEntry.rootBlock;
//     // get rootBlock from the attribute catalog entry
//     /* if Index does not exist for the attribute (check rootBlock == -1) */ 
//                     if(rootBlock!=-1){
                    
//                     recId=linearSearch(relId,attrName,attrVal,op);
//         /* search for the record id (recid) corresponding to the attribute with
//            attribute name attrName, with value attrval and satisfying the
//            condition op using linearSearch()
//         */
//     }

//      else  {
//         // (index exists for the attribute)

//         /* search for the record id (recid) correspoding to the attribute with
//         attribute name attrName and with value attrval and satisfying the
//         condition op using BPlusTree::bPlusSearch() */
//         recId=BPlusTree::bPlusSearch(relId,attrName,attrVal,op);
//     }
//     if(recId.block==-1 && recId.slot==-1)
//     {
//         return E_NOTFOUND;
//     }

//     // if there's no record satisfying the given condition (recId = {-1, -1})
//     //     return E_NOTFOUND;
    
//     /* Copy the record with record id (recId) to the record buffer (record).
//        For this, instantiate a RecBuffer class object by passing the recId and
//        call the appropriate method to fetch the record
//     */
//     RecBuffer recbuf(recId.block);
//     ret=recbuf.getRecord(record,recId.slot);
//     if(ret!=SUCCESS)
//     return ret;
//     return SUCCESS;
// }


int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op) {
    // Declare a variable called recid to store the searched record
    RecId recId;


    //shit that was before this

    // /* search for the record id (recid) corresponding to the attribute with
    // attribute name attrName, with value attrval and satisfying the condition op
    // using linearSearch() */
    // recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);

    //end of shit



    /* get the attribute catalog entry from the attribute cache corresponding
    to the relation with Id=relid and with attribute_name=attrName  */
    // if this call returns an error, return the appropriate error code
    AttrCatEntry attrcatbuf;
    int ret=AttrCacheTable::getAttrCatEntry(relId,attrName,&attrcatbuf);
	if(ret!=SUCCESS){
		return ret;
	}

    // // get rootBlock from the attribute catalog entry
    // /* if Index does not exist for the attribute (check rootBlock == -1) */ {

    //     /* search for the record id (recid) corresponding to the attribute with
    //        attribute name attrName, with value attrval and satisfying the
    //        condition op using linearSearch()
    //     */
    // }
    if(attrcatbuf.rootBlock==-1){
        recId = BlockAccess::linearSearch(relId, attrName, attrVal, op);
    }

    //     /* search for the record id (recid) correspoding to the attribute with
    //     attribute name attrName and with value attrval and satisfying the
    //     condition op using BPlusTree::bPlusSearch() */
    // }
    else{
        recId = BPlusTree::bPlusSearch(relId,attrName,attrVal,op);
    }

   // if there's no record satisfying the given condition (recId = {-1, -1})
    //    return E_NOTFOUND;
    if (recId.block == -1 and recId.slot == -1){
      return E_NOTFOUND;
    }
    /* Copy the record with record id (recId) to the record buffer (record)
       For this Instantiate a RecBuffer class object using recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBuffer(recId.block);
    ret = recBuffer.getRecord(record, recId.slot);
    if (ret != SUCCESS){
      return ret;
    }
    return SUCCESS;
}


int BlockAccess::deleteRelation(char relName[ATTR_SIZE]) {
    // if the relation to delete is either Relation Catalog or Attribute Catalog,
    //     return E_NOTPERMITTED
     if(strcmp(relName,RELCAT_RELNAME)==0 ||strcmp(relName,ATTRCAT_RELNAME)==0)
     return E_NOTPERMITTED;

        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_NAME and ATTRCAT_NAME)
      RelCacheTable::resetSearchIndex(RELCAT_RELID);
    /* reset the searchIndex of the relation catalog using
       RelCacheTable::resetSearchIndex() */
        
    Attribute relNameAttr; // (stores relName as type union Attribute)
    // assign relNameAttr.sVal = relName
       strcpy(relNameAttr.sVal,relName);
       RecId recId;
       recId=linearSearch(RELCAT_RELID,RELCAT_ATTR_RELNAME,relNameAttr,EQ);
    //  line arSearch on the relation catalog for RelName = relNameAttr
      if(recId.block ==  -1 || recId.slot == -1)
      return E_RELNOTEXIST;
    // if the relation does not exist (linearSearch returned {-1, -1})
    //     return E_RELNOTEXIST

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    /* store the relation catalog record corresponding to the relation in
       relCatEntryRecord using RecBuffer.getRecord */
       RecBuffer recBlk(recId.block);
       recBlk.getRecord(relCatEntryRecord,recId.slot);
    /* get the first record block of the relation (firstBlock) using the
       relation catalog entry record */
      int firstBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
	int numAttributes = relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
    /* get the number of attributes corresponding to the relation (numAttrs)
       using the relation catalog entry record */

    /*
     Delete all the record blocks of the relation
    */
   int currnum=firstBlock;
     while(currnum!=-1){
      RecBuffer currBlock(currnum);
      HeadInfo head;
      currBlock.getHeader(&head);
      currnum=head.rblock;
      currBlock.releaseBlock();
     }
    // for each record block of the relation:
    //     get block header using BlockBuffer.getHeader
    //     get the next block from the header (rblock)
    //     release the block using BlockBuffer.releaseBlock
    //
    //     Hint: to know if we reached the end, check if nextBlock = -1


    /***
        Deleting attribute catalog entries corresponding the relation and index
        blocks corresponding to the relation with relName on its attributes
    ***/

    // reset the searchIndex of the attribute catalog
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numberOfAttributesDeleted = 0;

    while(true) {
        RecId attrCatRecId;
        // attrCatRecId = linearSearch on attribute catalog for RelName = relNameAttr
           Attribute attrRec;
           attrCatRecId=linearSearch(ATTRCAT_RELID,RELCAT_ATTR_RELNAME,relNameAttr,EQ);
           if(attrCatRecId.block==-1 || attrCatRecId.slot==-1)
           break;
        // if no more attributes to iterate over (attrCatRecId == {-1, -1})
        //     break;

        numberOfAttributesDeleted++;
        RecBuffer recbuf(attrCatRecId.block);
        HeadInfo header;
        recbuf.getHeader(&header);
      
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        recbuf.getRecord(attrCatRecord,attrCatRecId.slot); 
        
        // create a RecBuffer for attrCatRecId.block
        // get the header of t he block
        // get the record corresponding to attrCatRecId.slot
     
        // declare variable rootBlock which will be used to store the root
        // block field from the attribute catalog record.
         int rootBlock = attrCatRecord[ATTRCAT_ROOT_BLOCK_INDEX].nVal; 
        // (This will be used later to delete any indexes if it exists)
        unsigned char slot_var[header.numSlots];
        recbuf.getSlotMap(slot_var);
        slot_var[attrCatRecId.slot]=SLOT_UNOCCUPIED;
        recbuf.setSlotMap(slot_var);
        // Update the Slotmap for the block by setting the slot as SLOT_UNOCCUPIED
        // Hint: use RecBuffer.getSlotMap and RecBuffer.setSlotMap
        header.numEntries--;
      
         
        
        recbuf.setHeader(&header);
        /* Decrement the numEntries in the header of the block corresponding to
           the attribute catalog entry and then set back the header
           using RecBuffer.setHeader */
          
        /* If number of entries become 0, releaseBlock is called after fixing
           the linked list.
        */
        if (!header.numEntries) {
            /* Standard Linked List Delete for a Block
               Get the header of the left block and set it's rblock to this
               block's rblock
            */
           RecBuffer prevblock(header.lblock);
           HeadInfo lf;
           prevblock.getHeader(&lf);
           lf.rblock=header.rblock;
           prevblock.setHeader(&lf);
           // create a RecBuffer for lblock and call appropriate methods
        if (header.rblock!=INVALID_BLOCKNUM) {
                /* Get the header of the right block and set it's lblock to
                   this block's lblock */
                   RecBuffer nextblock(header.rblock);
                   HeadInfo rf;
                   nextblock.getHeader(&rf);
                   rf.lblock=header.lblock;
                    prevblock.setHeader(&rf);
                // create a RecBuffer for rblock and call appropriate methods

            } else {
                // (the block being released is the "Last Block" of the relation.)
                /* update the Relation Catalog entry's LastBlock field for this
                   relation with the block number of the previous block. */
                   RelCatEntry Relcatbuff;

				
				RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &Relcatbuff);

				Relcatbuff.lastBlk = header.lblock;
            }

            // (Since the attribute catalog will never be empty(why?), we do not
            //  need to handle the case of the linked list becoming empty - i.e
            //  every block of the attribute catalog gets released.)
            recbuf.releaseBlock();
            // call releaseBlock()
        }

        // (the following part is only relevant once indexing has been implemented)
    // if index exists for the attribute (rootBlock != -1), call bplus destroy
        // if (rootBlock != -1) {
        //     // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
        // }
          if (rootBlock != -1) {
            // delete the bplus tree rooted at rootBlock using BPlusTree::bPlusDestroy()
            BPlusTree::bPlusDestroy(rootBlock);
        }
    }

    /*** Delete the entry corresponding to the relation from relation catalog ***/
    // Fetch the header of Relcat block
     HeadInfo relheader;
     recBlk.getHeader(&relheader);
     relheader.numEntries--;
     recBlk.setHeader(&relheader);
     
    /* Decrement the numEntries in the header of the block corresponding to the
       relation catalog entry and set it back */
    int numslots=relheader.numSlots;
      unsigned char slotmap[numslots];
      recBlk.getSlotMap(slotmap);
      slotmap[recId.slot]=SLOT_UNOCCUPIED;
      recBlk.setSlotMap(slotmap);
    /* Get the slotmap in relation catalog, update it by marking the slot as
       free(SLOT_UNOCCUPIED) and set it back. */
    
 
    /*** Updating the Relation Cache Table ***/
    /** Update relation catalog record entry (number of records in relation
        catalog is decreased by 1) **/
    // Get the entry corresponding to relation catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCatEntry relCatEntryBuffer;
	RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntryBuffer);

	relCatEntryBuffer.numRecs--;
	RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntryBuffer);
    /** Update attribute catalog entry (number of records in attribute catalog
        is decreased by numberOfAttributesDeleted) **/
    // i.e., #Records = #Records - numberOfAttributesDeleted
     
    // Get the entry corresponding to attribute catalog from the relation
    // cache and update the number of records and set it back
    // (using RelCacheTable::setRelCatEntry() function)
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);
	relCatEntryBuffer.numRecs -= numberOfAttributesDeleted;
	RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntryBuffer);
    return SUCCESS;
}


/*
NOTE: the caller is expected to allocate space for the argument `record` based
      on the size of the relation. This function will only copy the result of
      the projection onto the array pointed to by the argument.
*/

int BlockAccess::project(int relId, Attribute *record) {
    // get the previous search index of the relation relId from the relation
    // cache (use RelCacheTable::getSearchIndex() function)

    RecId prevRecId;
    RelCacheTable::getSearchIndex(relId,&prevRecId);
    // declare block and slot which will be used to store the record id of the
    // slot we need to check.
    int block, slot;

    /* if the current search index record is invalid(i.e. = {-1, -1})
       (this only happens when the caller reset the search index)
    */
    if (prevRecId.block == -1 && prevRecId.slot == -1)
    {
        // (new project operation. start from beginning)
          RelCatEntry relCatEntry;
          RelCacheTable::getRelCatEntry(relId,&relCatEntry);
          block=relCatEntry.firstBlk;
          slot=0;
        // get the first record block of the relation from the relation cache
        // (use RelCacheTable::getRelCatEntry() function of Cache Layer)

        // block = first record block of the relation
        // slot = 0
    }
    else
    {

        // (a project/search operation is already in progress)
        block=prevRecId.block;
        slot=prevRecId.slot+1;
        // block = previous search index's block
        // slot = previous search index's slot + 1
    }


    // The following code finds the next record of the relation
    /* Start from the record id (block, slot) and iterate over the remaining
       records of the relation */
    while (block != -1)
    {
        // create a RecBuffer object for block (using appropriate constructor!)
          RecBuffer recBuffer(block);
          HeadInfo recHeader;
          recBuffer.getHeader(&recHeader);
          unsigned char slotMap[recHeader.numSlots];
          recBuffer.getSlotMap(slotMap);
        // get header of the block using RecBuffer::getHeader() function
        // get slot map of the block using RecBuffer::getSlotMap() function

        if(slot>=recHeader.numSlots)
        {   block=recHeader.rblock;
            slot=0;
            // (no more slots in this block)
            // update block = right block of block
            // update slot = 0
            // (NOTE: if this is the last block, rblock would be -1. this would
            //        set block = -1 and fail the loop condition )
        }
        else if (slotMap[slot]==SLOT_UNOCCUPIED)
        { // (i.e slot-th entry in slotMap contains SLOT_UNOCCUPIED)
               slot++;
            // increment slot
        }
        else {
            // (the next occupied slot / record has been found)
            break;
        }
    }

    if (block == -1){
        // (a record was not found. all records exhausted)
        return E_NOTFOUND;
    }

    // declare nextRecId to store the RecId of the record found
    RecId nextRecId{block, slot};

    // set the search index to nextRecId using RelCacheTable::setSearchIndex
      RelCacheTable::setSearchIndex(relId,&nextRecId);
    /* Copy the record with record id (nextRecId) to the record buffer (record)
       For this Instantiate a RecBuffer class object by passing the recId and
       call the appropriate method to fetch the record
    */
    RecBuffer recBuffer2(nextRecId.block);
    int ret=recBuffer2.getRecord(record,nextRecId.slot);
    if(ret!=SUCCESS){

    std::cout<<"record not found!!"<<std::endl;
    return ret;
    }

    return SUCCESS;
}


// #include "BlockAccess.h"
// #include "../Buffer/BlockBuffer.h"
// #include <cstring>
// #include <iostream>
// using namespace std;

// RecId BlockAccess::linearSearch(int relId, char attrName[ATTR_SIZE], union Attribute attrVal, int op)
// {
//     // TODO: No error handling is done in this function. Should add error handling code.
//     // get the previous search index of the relation relId from the relation cache
//     // (use RelCacheTable::getSearchIndex() function)
//     RecId prevRecId;
//     RelCacheTable::getSearchIndex(relId, &prevRecId);

//     int block, slot;

//     // if the current search index record is invalid(i.e. both block and slot = -1)
//     if (prevRecId.block == -1 && prevRecId.slot == -1)
//     {
//         // (no hits from previous search; search should start from the
//         // first record itself)

//         // get the first record block of the relation from the relation cache
//         // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
//         RelCatEntry relCatEntry;
//         RelCacheTable::getRelCatEntry(relId, &relCatEntry);

//         block = relCatEntry.firstBlk;
//         slot = 0;

//         // block = first record block of the relation
//         // slot = 0
//     }
//     else
//     {
//         // (there is a hit from previous search; search should start from
//         // the record next to the search index record)

//         // TODO: What if the previous hit is the last record of the relation?
//         // How exactly should I move the block and slot pointers in that case?
//         // block = search index's block
//         // slot = search index's slot + 1
//         block = prevRecId.block;
//         slot = prevRecId.slot + 1;
//     }

//     /* The following code searches for the next record in the relation
//        that satisfies the given condition
//        We start from the record id (block, slot) and iterate over the remaining
//        records of the relation
//     */
//     while (block != -1)
//     {
//         /* create a RecBuffer object for block (use RecBuffer Constructor for
//            existing block) */
//         RecBuffer recBuffer(block);

//         // get the record with id (block, slot) using RecBuffer::getRecord()
//         HeadInfo head;
//         recBuffer.getHeader(&head);
//         unsigned char slotMap[head.numSlots];
//         recBuffer.getSlotMap(slotMap);

//         // get header of the block using RecBuffer::getHeader() function
//         // get slot map of the block using RecBuffer::getSlotMap() function

//         // If slot >= the number of slots per block(i.e. no more slots in this block)
//         if (slot >= head.numSlots)
//         {
//             // update block = right block of block
//             // (use the block header to get the right block of the current block)
//             block = head.rblock;
//             slot = 0;
//             // update slot = 0
//             continue; // continue to the beginning of this while loop
//         }

//         int numAttrs = head.numAttrs;

//         // if slot is free skip the loop
//         // (i.e. check if slot'th entry in slot map of block contains SLOT_UNOCCUPIED)
//         Attribute record[numAttrs];
//         recBuffer.getRecord(record, slot);

//         if (slotMap[slot] == SLOT_UNOCCUPIED)
//         {
//             // increment slot and continue to the next record slot
//             slot++;
//             continue;
//         }
//         // compare record's attribute value to the the given attrVal as below:
//         /*
//             firstly get the attribute offset for the attrName attribute
//             from the attribute cache entry of the relation using
//             AttrCacheTable::getAttrCatEntry()
//         */
//         AttrCatEntry attrCatEntry;
//         AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
//         /* use the attribute offset to get the value of the attribute from
//            current record */
//         Attribute recordAttrVal = record[attrCatEntry.offset];
//         int cmpVal; // will store the difference between the attributes
//         // set cmpVal using compareAttrs()
//         cmpVal = compareAttrs(recordAttrVal, attrVal, attrCatEntry.attrType);

//         /* Next task is to check whether this record satisfies the given condition.
//            It is determined based on the output of previous comparison and
//            the op value received.
//            The following code sets the cond variable if the condition is satisfied.
//         */
//         if (
//             (op == NE && cmpVal != 0) || // if op is "not equal to"
//             (op == LT && cmpVal < 0) ||  // if op is "less than"
//             (op == LE && cmpVal <= 0) || // if op is "less than or equal to"
//             (op == EQ && cmpVal == 0) || // if op is "equal to"
//             (op == GT && cmpVal > 0) ||  // if op is "greater than"
//             (op == GE && cmpVal >= 0)    // if op is "greater than or equal to"
//         )
//         {
//             /*
//             set the search index in the relation cache as
//             the record id of the record that satisfies the given condition
//             (use RelCacheTable::setSearchIndex function)
//             */
//             prevRecId = RecId{block, slot};
//             RelCacheTable::setSearchIndex(relId, &prevRecId);
//             return RecId{block, slot};
//         }

//         slot++;
//     }

//     // no record in the relation with Id relid satisfies the given condition
//     return RecId{-1, -1};
// }

// int BlockAccess::renameRelation(char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
// {
//     /* reset the searchIndex of the relation catalog using
//        RelCacheTable::resetSearchIndex() */
//     RelCacheTable::resetSearchIndex(RELCAT_RELID);

//     Attribute newRelationName; // set newRelationName with newName
//     strcpy(newRelationName.sVal, newName);

//     // search the relation catalog for an entry with "RelName" = newRelationName
//     // TODO : Get this dynamically
//     RecId recId = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, newRelationName, EQ);

//     // If relation with name newName already exists (result of linearSearch
//     //                                               is not {-1, -1})
//     //    return E_RELEXIST;

//     if (recId.block != -1 && recId.slot != -1)
//     {
//         return E_RELEXIST;
//     }

//     /* reset the searchIndex of the relation catalog using
//        RelCacheTable::resetSearchIndex() */
//     RelCacheTable::resetSearchIndex(RELCAT_RELID);

//     Attribute oldRelationName; // set oldRelationName with oldName

//     strcpy(oldRelationName.sVal, oldName);
//     // search the relation catalog for an entry with "RelName" = oldRelationName
//     recId = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, oldRelationName, EQ);

//     // If relation with name oldName does not exist (result of linearSearch is {-1, -1})
//     //    return E_RELNOTEXIST;
//     if (recId.block == -1 && recId.slot == -1)
//     {
//         return E_RELNOTEXIST;
//     }

//     /* get the relation catalog record of the relation to rename using a RecBuffer
//        on the relation catalog [RELCAT_BLOCK] and RecBuffer.getRecord function
//     */
//     RecBuffer recBuffer(RELCAT_BLOCK);
//     HeadInfo head;
//     recBuffer.getHeader(&head);
//     Attribute record[head.numAttrs];
//     recBuffer.getRecord(record, recId.slot);

//     /* update the relation name attribute in the record with newName.
//        (use RELCAT_REL_NAME_INDEX) */
//     // set back the record value using RecBuffer.setRecord
//     strcpy(record[RELCAT_REL_NAME_INDEX].sVal, newName);
//     recBuffer.setRecord(record, recId.slot);

//     /*
//     update all the attribute catalog entries in the attribute catalog corresponding
//     to the relation with relation name oldName to the relation name newName
//     */

//     /* reset the searchIndex of the attribute catalog using
//        RelCacheTable::resetSearchIndex() */
//     RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

//     // for i = 0 to numberOfAttributes :
//     //     linearSearch on the attribute catalog for relName = oldRelationName
//     //     get the record using RecBuffer.getRecord
//     //
//     //     update the relName field in the record to newName
//     //     set back the record using RecBuffer.setRecord
//     // TODO : Get this dynamically
//     int numAttrs = record[RELCAT_NO_ATTRIBUTES_INDEX].nVal;
//     for (int i = 0; i < numAttrs; i++)
//     {
//         recId = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
//         RecBuffer recBuffer(recId.block);
//         recBuffer.getRecord(record, recId.slot);
//         strcpy(record[ATTRCAT_REL_NAME_INDEX].sVal, newName);
//         recBuffer.setRecord(record, recId.slot);
//     }

//     return SUCCESS;
// }

// int BlockAccess::renameAttribute(char relName[ATTR_SIZE], char oldName[ATTR_SIZE], char newName[ATTR_SIZE])
// {

//     /* reset the searchIndex of the relation catalog using
//        RelCacheTable::resetSearchIndex() */
//     RelCacheTable::resetSearchIndex(RELCAT_RELID);

//     Attribute relNameAttr; // set relNameAttr to relName
//     strcpy(relNameAttr.sVal, relName);

//     // Search for the relation with name relName in relation catalog using linearSearch()
//     // If relation with name relName does not exist (search returns {-1,-1})
//     //    return E_RELNOTEXIST;
//     RecId recId = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

//     if (recId.block == -1 && recId.slot == -1)
//     {
//         return E_RELNOTEXIST;
//     }

//     /* reset the searchIndex of the attribute catalog using
//        RelCacheTable::resetSearchIndex() */
//     RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

//     /* declare variable attrToRenameRecId used to store the attr-cat recId
//     of the attribute to rename */
//     RecId attrToRenameRecId{-1, -1};
//     Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

//     /* iterate over all Attribute Catalog Entry record corresponding to the
//        relation to find the required attribute */
//     while (true)
//     {
//         // linear search on the attribute catalog for RelName = relNameAttr
//         // get the record using RecBuffer.getRecord
//         // if the attribute name is oldName, set attrToRenameRecId to block and slot of this record

//         RecId recId = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);

//         // if there are no more attributes left to check (linearSearch returned {-1,-1})
//         //     break;
//         if (recId.block == -1 && recId.slot == -1)
//         {
//             break;
//         }

//         /* Get the record from the attribute catalog using RecBuffer.getRecord
//           into attrCatEntryRecord */
//         RecBuffer recBuffer(recId.block);
//         recBuffer.getRecord(attrCatEntryRecord, recId.slot);

//         // if attrCatEntryRecord.attrName = oldName
//         //     attrToRenameRecId = block and slot of this record
//         if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0)
//         {
//             attrToRenameRecId = recId;
//             break;
//         }

//         // if attrCatEntryRecord.attrName = newName
//         //     return E_ATTREXIST;
//         if (strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0)
//         {
//             return E_ATTREXIST;
//         }
//     }

//     // if attrToRenameRecId == {-1, -1}
//     //     return E_ATTRNOTEXIST;
//     if (attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
//     {
//         return E_ATTRNOTEXIST;
//     }

//     // Update the entry corresponding to the attribute in the Attribute Catalog Relation.
//     /*   declare a RecBuffer for attrToRenameRecId.block and get the record at
//          attrToRenameRecId.slot */
//     //   update the AttrName of the record with newName
//     //   set back the record with RecBuffer.setRecord
//     RecBuffer recBuffer(attrToRenameRecId.block);
//     recBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);
//     strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);
//     recBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);
//     return SUCCESS;
// }

// int BlockAccess::insert(int relId, Attribute *record)
// {
//     RelCatEntry relCatBuf;
//     int ret = RelCacheTable::getRelCatEntry(relId, &relCatBuf);

//     if (ret != SUCCESS)
//     {
//         return ret;
//     }

//     int blockNum = relCatBuf.firstBlk;

//     RecId recId = {-1, -1};

//     int numSlots = relCatBuf.numSlotsPerBlk;
//     int numAttrs = relCatBuf.numAttrs;

//     int prevBlockNum = -1;

//     while (blockNum != -1)
//     {
//         RecBuffer currentBlock(blockNum);

//         HeadInfo currentHeader;
//         currentBlock.getHeader(&currentHeader);

//         unsigned char slotMap[numSlots];
//         currentBlock.getSlotMap(slotMap);

//         int freeSlot = -1;
//         for (int i = 0; i < numSlots; i++)
//         {
//             if (slotMap[i] == SLOT_UNOCCUPIED)
//             {
//                 freeSlot = i;
//                 break;
//             }
//         }

//         if (freeSlot != -1)
//         {
//             recId.block = blockNum;
//             recId.slot = freeSlot;
//             break;
//         }

//         prevBlockNum = blockNum;
//         blockNum = currentHeader.rblock;
//     }

//     if (recId.block == -1 || recId.slot == -1)
//     {
//         if (relId == RELCAT_RELID)
//         {
//             return E_MAXRELATIONS;
//         }

//         RecBuffer newBlock;

//         int newBlockNum = newBlock.getBlockNum();

//         if (newBlockNum == E_DISKFULL)
//         {
//             return E_DISKFULL;
//         }

//         recId.block = newBlockNum;
//         recId.slot = 0;

//         HeadInfo newBlockHeader;
//         newBlock.getHeader(&newBlockHeader);
//         newBlockHeader.lblock = prevBlockNum;
//         newBlockHeader.numAttrs = numAttrs;
//         newBlockHeader.numSlots = numSlots;
//         newBlock.setHeader(&newBlockHeader);

//         unsigned char newBlockSlotMap[numSlots];
//         newBlock.getSlotMap(newBlockSlotMap);
//         for (int i = 0; i < numSlots; i++)
//             newBlockSlotMap[i] = SLOT_UNOCCUPIED;
//         newBlock.setSlotMap(newBlockSlotMap);

//         if (prevBlockNum != -1)
//         {
//             RecBuffer prevBlock(prevBlockNum);

//             HeadInfo prevBlockHeader;
//             prevBlock.getHeader(&prevBlockHeader);
//             prevBlockHeader.rblock = recId.block;
//             prevBlock.setHeader(&prevBlockHeader);
//         }
//         else
//         {
//             relCatBuf.firstBlk = recId.block;
//             RelCacheTable::setRelCatEntry(relId, &relCatBuf);
//         }
//         relCatBuf.lastBlk = recId.block;
//     }

//     RecBuffer blockToInsert(recId.block);
//     blockToInsert.setRecord(record, recId.slot);

//     unsigned char slotMapToInsert[numSlots];
//     blockToInsert.getSlotMap(slotMapToInsert);
//     slotMapToInsert[recId.slot] = SLOT_OCCUPIED;
//     blockToInsert.setSlotMap(slotMapToInsert);

//     HeadInfo headerToInsert;
//     blockToInsert.getHeader(&headerToInsert);
//     headerToInsert.numEntries++;
//     blockToInsert.setHeader(&headerToInsert);

//     relCatBuf.numRecs++;
//     RelCacheTable::setRelCatEntry(relId, &relCatBuf);

//     int flag = SUCCESS;
//     for (int attrOffset = 0; attrOffset < numAttrs; attrOffset++)
//     {
//         AttrCatEntry attrCatBuf;
//         AttrCacheTable::getAttrCatEntry(relId, attrOffset, &attrCatBuf);
//         if (attrCatBuf.rootBlock == -1)
//         {
//             continue;
//         }

//         int ret = BPlusTree::bPlusInsert(relId, attrCatBuf.attrName, record[attrOffset], recId);
//         if (ret == E_DISKFULL)
//         {
//             flag = E_INDEX_BLOCKS_RELEASED;
//             BPlusTree::bPlusDestroy(attrCatBuf.rootBlock);
//         }
//     }

//     return flag;
// }

// /*
// NOTE: This function will copy the result of the search to the `record` argument.
//       The caller should ensure that space is allocated for `record` array
//       based on the number of attributes in the relation.
// */
// int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op)
// {
//     // Declare a variable called recid to store the searched record
//     RecId recId;

//     /* get the attribute catalog entry from the attribute cache corresponding
//     to the relation with Id=relid and with attribute_name=attrName  */
//     AttrCatEntry attrCatEntry;
//     int ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);

//     // if this call returns an error, return the appropriate error code
//     if (ret != SUCCESS)
//     {
//         return ret;
//     }

//     // get rootBlock from the attribute catalog entry
//     int rootBlock = attrCatEntry.rootBlock;
//     /* if Index does not exist for the attribute (check rootBlock == -1) */
//     if (rootBlock == -1)
//     {

//         /* search for the record id (recid) corresponding to the attribute with
//            attribute name attrName, with value attrval and satisfying the
//            condition op using linearSearch()
//         */
//         recId = linearSearch(relId, attrName, attrVal, op);
//         // resetting the search index will be handled by linear search
//     }

//     /* else */
//     else
//     {
//         // (index exists for the attribute)

//         /* search for the record id (recid) correspoding to the attribute with
//         attribute name attrName and with value attrval and satisfying the
//         condition op using BPlusTree::bPlusSearch() */
//         recId = BPlusTree::bPlusSearch(relId, attrName, attrVal, op);
//     }

//     // if there's no record satisfying the given condition (recId = {-1, -1})
//     //     return E_NOTFOUND;
//     if (recId.block == -1 && recId.slot == -1)
//     {
//         return E_NOTFOUND;
//     }

//     /* Copy the record with record id (recId) to the record buffer (record).
//        For this, instantiate a RecBuffer class object by passing the recId and
//        call the appropriate method to fetch the record
//     */
//     RecBuffer recBuffer(recId.block);
//     recBuffer.getRecord(record, recId.slot);

//     return SUCCESS;
// }

// int BlockAccess::deleteRelation(char relName[ATTR_SIZE])
// {
//     if (
//         strcmp(relName, (char *)RELCAT_RELNAME) == 0 ||
//         strcmp(relName, (char *)ATTRCAT_RELNAME) == 0)
//     {
//         return E_NOTPERMITTED;
//     }

//     RelCacheTable::resetSearchIndex(RELCAT_RELID);

//     Attribute relNameAttribute;
//     strcpy(relNameAttribute.sVal, relName);

//     RecId recId = linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttribute, EQ);

//     if (recId.block == -1 || recId.slot == -1)
//     {
//         return E_RELNOTEXIST;
//     }

//     Attribute relCatEntryRecord[RELCAT_NO_ATTRS];

//     RecBuffer recBuffer(recId.block);
//     recBuffer.getRecord(relCatEntryRecord, recId.slot);

//     int currentBlock = relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;

//     while (currentBlock != -1)
//     {
//         RecBuffer currentBlockBuffer(currentBlock);
//         HeadInfo currentBlockHeader;
//         currentBlockBuffer.getHeader(&currentBlockHeader);

//         int nextBlock = currentBlockHeader.rblock;

//         currentBlockBuffer.releaseBlock();
//         currentBlock = nextBlock;
//     }

//     int numAttrsDeleted = 0;
//     RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
//     while (true)
//     {
//         RecId attrCatRecId = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttribute, EQ);

//         if (attrCatRecId.slot == -1 || attrCatRecId.block == -1)
//         {
//             break;
//         }

//         numAttrsDeleted++;

//         RecBuffer currentBlock(attrCatRecId.block);

//         HeadInfo currentBlockHeader;
//         currentBlock.getHeader(&currentBlockHeader);

//         Attribute record[ATTRCAT_NO_ATTRS];
//         currentBlock.getRecord(record, attrCatRecId.slot);

//         int rootBlock = record[ATTRCAT_ROOT_BLOCK_INDEX].nVal;

//         unsigned char slotMap[currentBlockHeader.numSlots];

//         currentBlock.getSlotMap(slotMap);
//         slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
//         currentBlock.setSlotMap(slotMap);

//         currentBlockHeader.numEntries--;
//         currentBlock.setHeader(&currentBlockHeader);

//         if (currentBlockHeader.numEntries == 0)
//         {
//             int leftBlock = currentBlockHeader.lblock;
//             int rightBlock = currentBlockHeader.rblock;

//             if (leftBlock != -1)
//             {
//                 RecBuffer prevBlock(leftBlock);
//                 HeadInfo prevBlockHeader;

//                 prevBlock.getHeader(&prevBlockHeader);
//                 prevBlockHeader.rblock = rightBlock;
//                 prevBlock.setHeader(&prevBlockHeader);
//             }

//             if (rightBlock != -1)
//             {
//                 RecBuffer nextBlock(rightBlock);
//                 HeadInfo nextBlockHeader;

//                 nextBlock.getHeader(&nextBlockHeader);
//                 nextBlockHeader.lblock = leftBlock;
//                 nextBlock.setHeader(&nextBlockHeader);
//             }

//             currentBlock.releaseBlock();
//         }

//         // condition to handle b+ trees
//         if (rootBlock != -1)
//         {
//             BPlusTree::bPlusDestroy(rootBlock);
//         }
//     }

//     HeadInfo relCatHeader;
//     recBuffer.getHeader(&relCatHeader);

//     unsigned char recSlotMap[relCatHeader.numSlots];

//     recBuffer.getSlotMap(recSlotMap);
//     recSlotMap[recId.slot] = SLOT_UNOCCUPIED;
//     recBuffer.setSlotMap(recSlotMap);

//     relCatHeader.numEntries--;
//     recBuffer.setHeader(&relCatHeader);

//     RelCatEntry relCatBuf;
//     RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatBuf);
//     relCatBuf.numRecs--;
//     RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatBuf);

//     RelCatEntry attrCatBuf;
//     RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrCatBuf);
//     attrCatBuf.numRecs -= numAttrsDeleted;
//     RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrCatBuf);

//     return SUCCESS;
// }

// /*
// NOTE: the caller is expected to allocate space for the argument `record` based
//       on the size of the relation. This function will only copy the result of
//       the projection onto the array pointed to by the argument.
// */
// int BlockAccess::project(int relId, Attribute *record)
// {
//     // get the previous search index of the relation relId from the relation
//     // cache (use RelCacheTable::getSearchIndex() function)
//     RecId prevRecId;
//     RelCacheTable::getSearchIndex(relId, &prevRecId);

//     // declare block and slot which will be used to store the record id of the
//     // slot we need to check.
//     int block, slot;

//     /* if the current search index record is invalid(i.e. = {-1, -1})
//        (this only happens when the caller reset the search index)
//     */
//     if (prevRecId.block == -1 && prevRecId.slot == -1)
//     {
//         // (new project operation. start from beginning)

//         // get the first record block of the relation from the relation cache
//         // (use RelCacheTable::getRelCatEntry() function of Cache Layer)
//         RelCatEntry relCatEntry;
//         RelCacheTable::getRelCatEntry(relId, &relCatEntry);

//         // block = first record block of the relation
//         // slot = 0
//         block = relCatEntry.firstBlk;
//         slot = 0;
//     }
//     else
//     {
//         // (a project/search operation is already in progress)

//         // block = previous search index's block
//         // slot = previous search index's slot + 1
//         block = prevRecId.block;
//         slot = prevRecId.slot + 1;
//     }

//     // The following code finds the next record of the relation
//     /* Start from the record id (block, slot) and iterate over the remaining
//        records of the relation */
//     while (block != -1)
//     {
//         // create a RecBuffer object for block (using appropriate constructor!)
//         RecBuffer recBlock(block);

//         // get header of the block using RecBuffer::getHeader() function
//         // get slot map of the block using RecBuffer::getSlotMap() function
//         struct HeadInfo header;
//         recBlock.getHeader(&header);
//         unsigned char slotMap[header.numSlots];
//         recBlock.getSlotMap(slotMap);

//         if (slot >= header.numSlots)
//         {
//             // (no more slots in this block)
//             // update block = right block of block
//             // update slot = 0
//             // (NOTE: if this is the last block, rblock would be -1. this would
//             //        set block = -1 and fail the loop condition )
//             block = header.rblock;
//             slot = 0;
//             continue; // continue to the beginning of this while loop
//         }
//         else if (slotMap[slot] == SLOT_UNOCCUPIED)
//         {
//             // (slot is free)
//             // increment slot and continue to the next record slot
//             slot++;
//             continue;
//         }
//         else
//         {
//             // (the next occupied slot / record has been found)
//             // declare nextRecId to store the RecId of the record found
//             break;
//         }
//     }

//     if (block == -1)
//     {
//         // (a record was not found. all records exhausted)
//         return E_NOTFOUND;
//     }

//     // declare nextRecId to store the RecId of the record found
//     RecId nextRecId{block, slot};

//     // set the search index to nextRecId using RelCacheTable::setSearchIndex
//     RelCacheTable::setSearchIndex(relId, &nextRecId);

//     /* Copy the record with record id (nextRecId) to the record buffer (record)
//        For this Instantiate a RecBuffer class object by passing the recId and
//        call the appropriate method to fetch the record
//     */
//     RecBuffer recBuffer(nextRecId.block);
//     recBuffer.getRecord(record, nextRecId.slot);

//     return SUCCESS;
// }