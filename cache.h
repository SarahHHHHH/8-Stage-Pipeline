
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
    int mem_latency; //time to access memory
    int bitmask_tag; //gets upper bits of address for tag
    int bitmask_index; //gets middle bits of address for index
    int boffset; //# bits to right-shift the index
    struct cache_blk_t **blocks;    // the array of cache blocks
};

struct cache_t *
cache_create(int size, int blocksize, int assoc, int latency, int mem_latency)
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

    
    C->mem_latency = mem_latency;
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

int cache_access(struct cache_t *cp, unsigned long address,
                 char access_type, unsigned long long now, struct cache_t *next_cp)
{
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
    
    int i;
    int timeTaken = cp->hit_latency;
    
    //Addr is something like Ox10004884 -> 268454020
    //-> 00010000000000000100100010000100
    //32 bit address must fit in size n cache
    //byte offset = lower two bits
    //block index = a number of lowest bits equal to log 2 of block size
    //block size 1 -> 0 bits
    //block size 2 -> 1 bits
    //block size 4 -> 2 bits
    //block size 16 -> 4 bits
    //block size 32 -> 5 bits
    //block size 64 -> 6 bits
    //block size 256 -> 8 bits
    //index = address mod nsets = lower y bits after block index
    //tag = remaining upper bits
    
    //cp->blocks[i][j] i is address, j is assoc
    
    //get tag from requested address
    unsigned long req_tag = address & cp->bitmask_tag;
    
    //get index from the requested address
    unsigned long req_index = address & cp->bitmask_index;
    req_index = req_index >> cp->boffset;
    
    //accessing data at the index
    struct cache_blk_t *row = cp->blocks[req_index % cp->nsets];
    
    int isHit = 0;
    
    int lru = 0;
    unsigned long long leastval = row[0].ts;
    
    //scanning for valid data
    for (i = 0; i < cp->assoc; i++) {
        //if valid data found
        if (row[i].valid == '1') {
            //if tag matches
            if (row[i].tag == req_tag) {
                isHit = 1;
                
                //don't worry about L2 right now. Now, you know you're in L1.
                L1hits = L1hits + 1;
                
                if (access_type == 'w') {
                    row[i].dirty = '1';
                }
                
                break;
            }
            
            //look for LRU
            if (row[i].ts < leastval) {
                lru = 1;
                leastval = row[i].ts;
            }
        }
    }
    
    if (isHit == 0) {
        L1misses = L1misses + 1;
        
        //write back if block is valid but dirty
        if ((row[lru].valid == '1') && (row[lru].dirty == '1')) {
            //add to timeTaken for writing to mem
            //if can't read and write to mem in one request
            //again, not worrying about L2 right now
            timeTaken += cp->mem_latency;
        }
        
        //request block from mem
        row[lru].valid = '1';
        row[lru].dirty = '0';
        row[lru].ts = now;
        row[lru].tag = address & req_tag;
        timeTaken += cp->mem_latency; //add if can't read and write in one request
    }
    
    

    return(timeTaken);
}
