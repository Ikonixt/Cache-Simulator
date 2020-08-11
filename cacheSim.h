#include<stdlib.h>
#include<stdio.h>
#include<stdint.h>
#define DRAM_SIZE 1048576

typedef struct cb_struct {
    uint32_t data[8]; // One cache block is 32 bytes.
    uint32_t tag;
	uint32_t timeStamp;   /// This is used to determine what to evict using cycles as the counter
}cacheBlock;


typedef struct access {
    int readWrite; // 0 for read, 1 for write
    uint32_t address;
	uint32_t data; // If this is a read access, value here is 0
}cacheAccess;

// This is our "DRAM"
unsigned char * DRAM;

cacheBlock L1_cache[16][2]; // Our 2-way, 1024 byte cache
cacheBlock L2_cache[64][4]; // Our 4-way, 8kB cache

// Trace points to a series of cache accesses.
FILE *trace;

long cycles;

void init_DRAM();


// This function print the content of the cache
// Set 0   : CB1 | CB2 | CB 3 | ... | CB N
// Set 1   : CB1 | CB2 | CB 3 | ... | CB N
// ...
// Set M-1 : CB1 | CB2 | CB 3 | ... | CB N
void printCache();


// These functions perform a cache lookup return 0 if the address is not in the cache (cache miss) and 1 if the address is in the cache (cache hit)
int L1lookup(uint32_t address);
int L2lookup(uint32_t address);

// These functions returns a setID
unsigned int getL1SetID(uint32_t address);
unsigned int getL2SetID(uint32_t address);

// These functions returns a tag 
unsigned int getL1Tag(uint32_t address);
unsigned int getL2Tag(uint32_t address);


// This function performs a read to a cache content. Return the 4-byte data content given by the address
uint32_t read_fifo(uint32_t address);

// This function performs an eviction of the address at the L1 cache.
uint32_t * L1evict(int address);

// This function performs an eviction of the address at the L2 cache.
uint32_t * L2evict(int address);


// Modifies existing data or insert if the data does not previously exist.
uint32_t * write(uint32_t address, uint32_t data);

