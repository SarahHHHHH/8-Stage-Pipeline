//the file tested individually

//#include <stdio.h>
//#include <string.h>
//#include <stdlib.h>
//#include <stdbool.h>

#ifndef BRANCH_TABLE_ITEM_H
#define BRANCH_TABLE_ITEM_H

struct DataItem {
    int takenOne;
    int takenTwo;
    int key;
};

#endif


#define SIZE 64

#define BITMASK SIZE - 1


struct DataItem hashArray[SIZE];

int hashCode(int key){
    // Hash is bits 4-9
    key = key >> 4;
    key = key & BITMASK;
    return key;
}

struct DataItem *search(int key){
    //get the hash
    int hashIndex = hashCode(key);
    if (hashArray[hashIndex].key == key) {
        return &hashArray[hashIndex];
    }
    return NULL;
}

void insert(int key,int takenOne,int takenTwo){
    struct DataItem item;
    item.takenOne = takenOne;
    item.takenTwo = takenTwo;
    item.key = key;
    
    //get the hash
    int hashIndex = hashCode(key);
    
    hashArray[hashIndex] = item;
}

//int main(){
//    // Address as an int and prediction bit
//    insert(268788476, 0,0);
//    insert(268788448, 1,1);
//    insert(268788456, 1,1);
//    insert(268788472, 0,0);
//    insert(268788476, 1,1);
//    
//    // Search by address as int
//    struct DataItem *item = search(268788476);
//    if(item != NULL){
//        printf("Element found: %d, %d\n", item->takenOne, item->takenTwo);
//    }else {
//        printf("Element not found\n");
//    }
//    
//    // This element was overwritten so it returns NULL
//    item = search(268788472);
//    if(item != NULL){
//        printf("Element found: %d, %d\n", item->takenOne, item->takenTwo);
//    }else {
//        printf("Element not found\n");
//    }
//}
