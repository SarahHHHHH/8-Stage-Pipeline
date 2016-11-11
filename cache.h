
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

struct cache_blk_t {
   unsigned long tag;
   char valid;
   char dirty;
   unsigned long long ts;	//a timestamp that may be used to implement LRU replacement
   // To guarantee that L2 is inclusive of L1, you may need an additional flag
   // in L2 to indicate that the block is cached in L1
   //char alsoL1 = 0;
};

struct cache_t {
   // The cache is represented by a 2-D array of blocks.
   // The first dimension of the 2D array is "nsets" which is the number of sets (entries)
   // The second dimension is "assoc", which is the number of blocks in each set.
   int nsets;				// # sets
   int blocksize;			// block size
   int assoc;				// associativity
   int hit_latency;			// latency in case of a hit
   int bitmask_tag; //gets upper bits of address for tag
   int bitmask_index; //gets middle bits of address for index
   int boffset; //# bits to right-shift the index
   struct cache_blk_t **blocks;    // the array of cache blocks
};

struct cache_t *
cache_create(int size, int blocksize, int assoc, int latency)
{
   int i;
   int nblocks = 1;			// number of blocks in the cache
   int nsets = 1;			// number of sets (entries) in the cache

   // YOUR JOB: calculate the number of sets and blocks in the cache
   //
   // nblocks = X;
   // nsets = Y;
   nblocks = (size * 1024) / blocksize;
   nsets = nblocks / assoc;


   struct cache_t *C = (struct cache_t *)calloc(1, sizeof(struct cache_t));

   //calc bits in block offset (block size in bytes, not words)
   int boffset = (int)(log(blocksize)/log(2));
   //calc bits in index
   C->boffset = boffset;
   int bindex = (int)(log(nsets)/log(2));
   //make bitmask for index
   unsigned long bitmask_index = ((1 << bindex) - 1) << (boffset);
   C->bitmask_index = bitmask_index;
   //calc bits in tag
   int btag = 32 - boffset - bindex;
   //make bitmask for tags
   unsigned long bitmask_tag = ((1 << btag) - 1) << (boffset + bindex);
   C->bitmask_tag = bitmask_tag;

   C->blocksize = blocksize;

   C->nsets = nsets;
   C->assoc = assoc;
   C->hit_latency = latency;

   C->blocks= (struct cache_blk_t **)calloc(nsets, sizeof(struct cache_blk_t));

   for(i = 0; i < nsets; i++) {
      C->blocks[i] = (struct cache_blk_t *)calloc(assoc, sizeof(struct cache_blk_t));
   }

   return C;
}

int check_hit(struct cache_t *cp, unsigned long address, char access_type, unsigned long long now) {
   int i;

   //get tag from requested address
   unsigned long req_tag = address & cp->bitmask_tag;

   //get index from the requested address
   unsigned long req_index = address & cp->bitmask_index;
   req_index = req_index >> cp->boffset;

   //accessing data at the index
   struct cache_blk_t *row = cp->blocks[req_index % cp->nsets];

   int lru = 0;
   unsigned long long leastval = row[0].ts;

   //scanning for valid data
   for (i = 0; i < cp->assoc; i++) {
      //if valid data found
      if (row[i].valid == '1') {
         //if tag matches
         if (row[i].tag == req_tag) {
            // HIT!
            row[i].ts = now;

            if (access_type == 'w') {
               row[i].dirty = '1';
            }

            return 1;
         }

         //look for LRU
         if (row[i].ts < leastval) {
            lru = i;
            leastval = row[i].ts;
         }
      }
   }

   // If we got here that means we missed :)

   //write back if block is valid but dirty
   if ((row[lru].valid == '1') && (row[lru].dirty == '1')) {
      //add to timeTaken for writing to mem
      //if can't read and write to mem in one request

      // TODO: timeTaken += mem_latency;
   }

   //request block from mem
   row[lru].valid = '1';
   row[lru].dirty = '0';
   row[lru].ts = now;
   row[lru].tag = address & req_tag;
   return 0;
}

int cache_access(struct cache_t *cp, unsigned long address, char access_type, unsigned long long now, struct cache_t *next_cp, int mem_latency)
{
   int timeTaken = 0;

   if (cp == NULL) {
      return mem_latency;
   }

   // If its not a hit
   if (!check_hit(cp, address, access_type, now)) {
      if (cp->hit_latency == 0) {
         L1misses++;
      } else {
         L2misses++;
      }

      // Since we missed we need to go to the next cache if available
      timeTaken += cache_access(next_cp, address, access_type, now, NULL, mem_latency);

   } else {
      if (cp->hit_latency == 0) {
         L1hits++;
      } else {
         L2hits++;
      }
   }

   return(timeTaken);
}
