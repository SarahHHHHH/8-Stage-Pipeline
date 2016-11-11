
#include <stdlib.h>
#include <stdio.h>

struct cache_blk_t {
   unsigned long tag;
   char valid;
   char dirty;
   unsigned long long ts;	//a timestamp that may be used to implement LRU replacement
   // To guarantee that L2 is inclusive of L1, you may need an additional flag
   // in L2 to indicate that the block is cached in L1
};

struct cache_t {
   // The cache is represented by a 2-D array of blocks.
   // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
   // The second dimension is "assoc", which is the number of blocks in each set.
   int nsets;				// # sets
   int blocksize;			// block size
   int assoc;				// associativity
   int hit_latency;			// latency in case of a hit
   struct cache_blk_t **blocks;    // the array of cache blocks
};

struct cache_t *cache_create(int size, int blocksize, int assoc, int latency)
{
   int i;
   int nblocks = (size * 8000) / blocksize;			// number of blocks in the cache
   int nsets = nblocks / assoc;			// number of sets (entries) in the cache

   printf("blocks: %d\n", nblocks);
   printf("sets: %d\n", nsets);

   struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

   C->nsets = nsets;
   C->assoc = assoc;
   C->hit_latency = latency;

   C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t));

   for(i = 0; i < nsets; i++) {
      struct cache_blk_t *block = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
      block->dirty = '1';
      C->blocks[i] = block;
   }

   return C;
}

int check_read_hit(struct cache_t *cp, unsigned long address, unsigned long long now) {
   struct cache_blk_t **blocks = cp->blocks;
   int nsets = cp->nsets;
   int assoc = cp->assoc;
   int i, j;

   // Find the address in the cache
   for (i = 0; i < nsets ; i++) {
      for (j = 0; j < assoc; j++) {
         struct cache_blk_t *thisblock = &blocks[i][j];
         if (thisblock->valid == '1') {
            // TODO: convert address to tag
            if (thisblock->tag == address) {
               // HIT
               return 1;
            }
         }
      }
   }

   printf("inserting: %lu\n", address);

   unsigned long long min_time;
   // not found in cache so we should add it
   for (i = 0; i < nsets ; i++) {
      for (j = 0; j < assoc; j++) {
         struct cache_blk_t *thisblock = &blocks[i][j];
         if (thisblock->dirty == '1') {
            // the block is dirty so we can replace it
            // TODO: conver address to tag
            thisblock->tag = address;
            // store the cycle_number so we can do LRU
            thisblock->ts = now;
            thisblock->valid = '1';
            thisblock->dirty = '0';
            return 0;
         }
         if (thisblock->valid == '1') {
            if (thisblock->ts < min_time) {
               min_time = thisblock->ts;
            }
         }
      }
   }

   // Replace the least recently used item
   for (i = 0; i < nsets ; i++) {
      for (j = 0; j < assoc; j++) {
         struct cache_blk_t *thisblock = &blocks[i][j];
         if (min_time == thisblock->ts) {
            if (thisblock->ts < min_time) {
               // TODO: conver address to tag
               thisblock->tag = address;
               // store the cycle_number so we can do LRU
               thisblock->ts = now;
               thisblock->valid = '1';
               thisblock->dirty = '0';
               return 0;
            }
         }
      }
   }

   return 0;
}

int cache_access(struct cache_t *cp, unsigned long address, char access_type, unsigned long long now, struct cache_t *next_cp)
{
   if (cp == NULL) {
      // this should return mem_latency
      return 100;
   }
   if (access_type == 'r') {
      // Check if its a hit
      if (!check_read_hit(cp, address, now)) {
         if (cp->hit_latency == 0) {
            L1misses++;
         } else {
            L2misses++;
         }
         return cache_access(next_cp, address, 'r', now, NULL);
      }
   } else if (access_type == 'w') {

   }
   //
   // Based on address, determine the set to access in cp and examine the blocks
   // in the set to check hit/miss and update the golbal hit/miss statistics
   // If a miss, determine the victim in the set to replace (LRU). Replacement for
   // L2 blocks should observe the inclusion property.
   //
   // The function should return the hit_latency in case of a hit. In case
   // of a miss, you need to figure out a way to return the time it takes to service
   // the request, which includes writing back the replaced block, if dirty, and bringing
   // the requested block from the lower level (from L2 in case of L1, and from memory in case of L2).
   // This will require one or two calls to cache_access(L2, ...) for misses in L1.
   // It is recommended to start implementing and testing a system with only L1 (no L2). Then add the
   // complexity of an L2.
   // return(cp->hit_latency);
   return(cp->hit_latency);
}
