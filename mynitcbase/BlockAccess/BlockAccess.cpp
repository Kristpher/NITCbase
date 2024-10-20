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
        return E_ATTRNOTEXIST;
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
    return SUCCESS;
}
