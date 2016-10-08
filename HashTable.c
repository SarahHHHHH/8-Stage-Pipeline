#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#define SIZE 64

#define BITMASK 0x3F0

struct DataItem {
   int data;
   int key;
};

struct DataItem* hashArray[SIZE];
struct DataItem* item;

int hashCode(int key){
   // Hash is bits 4-9
   key = key & BITMASK;
   key = key >> 4;
   return key;
}

struct DataItem *search(int key){
   //get the hash
   int hashIndex = hashCode(key);
   if (hashArray[hashIndex]->key == key) {
      return hashArray[hashIndex];
   }
   return NULL;
}

void insert(int key,int data){

   struct DataItem *item = (struct DataItem*) malloc(sizeof(struct DataItem));
   item->data = data;
   item->key = key;

   //get the hash
   int hashIndex = hashCode(key);

   hashArray[hashIndex] = item;
}

int main(){
   // Address as an int and prediction bit
   insert(268788476, 0);
   insert(268788448, 1);
   insert(268788456, 1);
   insert(268788472, 0);
   insert(268788476, 1);

   // Search by address as int
   item = search(268788476);
   if(item != NULL){
      printf("Element found: %d\n", item->data);
   }else {
      printf("Element not found\n");
   }

   // This element was overwritten so it returns NULL
   item = search(268788472);
   if(item != NULL){
      printf("Element found: %d\n", item->data);
   }else {
      printf("Element not found\n");
   }
}
