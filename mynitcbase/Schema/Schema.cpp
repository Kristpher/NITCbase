#include "Schema.h"
#include<iostream>
#include <cmath>
#include <cstring>
int Schema::openRel(char relName[ATTR_SIZE]) {
  int ret = OpenRelTable::openRel(relName);

  // the OpenRelTable::openRel() function returns the rel-id if successful
  // a valid rel-id will be within the range 0 <= relId < MAX_OPEN and any
  // error codes will be negative
  if(ret >= 0){
    return SUCCESS;
  }

  //otherwise it returns an error message
  return ret;
}

int Schema::closeRel(char relName[ATTR_SIZE]) {
     //if it is either relation or attributr catalogue return not permitted
 if (strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
		return E_NOTPERMITTED;

  // this function returns the rel-id of a relation if it is open or
  // E_RELNOTOPEN if it is not. we will implement this later.
  int relId = OpenRelTable::getRelId(relName);

  if (relId==E_RELNOTOPEN)
   {
    return E_RELNOTOPEN;
  }

  return OpenRelTable::closeRel(relId);
}

int renameRel(char oldRelName[ATTR_SIZE], char newRelName[ATTR_SIZE]) {
    if(strcmp(oldRelName,"RELATIONCAT")==0||strcmp(oldRelName,"ATTRIBUTECAT")==0)
    return E_NOTPERMITTED;
    if (strcmp(newRelName, RELCAT_RELNAME) == 0 || strcmp(newRelName, ATTRCAT_RELNAME) == 0)
		return E_NOTPERMITTED;
    // if the oldRelName or newRelName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
     int check=OpenRelTable::getRelId(oldRelName);
     if(check!=E_RELNOTOPEN){
      return E_RELOPEN;
     }
     int retVal=BlockAccess::renameRelation(oldRelName,newRelName);
     return retVal;
    // if the relation is open
    //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
    //    return E_RELOPEN

    // retVal = BlockAccess::renameRelation(oldRelName, newRelName);
    // return retVal
}

int Schema::renameAttr(char *relName, char *oldAttrName, char *newAttrName) {
       if(strcmp(relName,"RELATIONCAT")==0||strcmp(relName,"ATTRIBUTECAT")==0)
       return E_NOTPERMITTED;

    // if the relName is either Relation Catalog or Attribute Catalog,
        // return E_NOTPERMITTED
        // (check if the relation names are either "RELATIONCAT" and "ATTRIBUTECAT".
        // you may use the following constants: RELCAT_RELNAME and ATTRCAT_RELNAME)
     int check=OpenRelTable::getRelId(relName);
     if(check!=E_RELNOTOPEN){
     return E_RELOPEN;
     }
    // if the relation is open
        //    (check if OpenRelTable::getRelId() returns E_RELNOTOPEN)
        //    return E_RELOPEN
   
    // Call BlockAccess::renameAttribute with appropriate arguments.
     int retVal=BlockAccess::renameAttribute(relName,oldAttrName,newAttrName);
     return retVal;
    // return the value returned by the above renameAttribute() call
}