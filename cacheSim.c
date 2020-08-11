#include "cacheSim.h"
#include "stdio.h"
#include "stdlib.h"

/*

In test trace
---------------------------------------
Total hits: 4 + 1 <--- !!!!BECAUSE OF THE REPEATING ENDING REQUEST BUG!!!!
First 12 trace tests fifo reads/eviction/sets/tags
Next 3 trace tests writes
Next 5 test hits


0 8A2C 26   L1set:1 L2set:17 L1 and L2-evicted tag1:45 tag2:11
0 CA21 71   L1set:1 L2set:17 L1-evicted tag1:65 tag2:19
0 FA23 14   L1set:1 L2set:17 L1-evicted tag1:7d  tag2:1f
0 BA28 10   L1set:1 L2set:17 L1-evicted tag1:5d tag2:17
0 AA2E 51   L1set:1 L2set:17 L1-evicted tag1:55 tag2:15
0 0E38 11   L1set:1 L2set:49 tag:7
0 1E24 1    L1set:1 L2set:49 tag:f
0 0ABC 2    L1set:5 L2set:21 tag:5
0 14E4 3    L1set:7 L2set:39 tag:a
0 0848 4    L1set:2 L2set:2 tag:4
0 1D04 5    L1set:8 L2set:40 tag:e
0 1E08 6    L1set:0 L2set:48 tag:f
1 0AA0 99   L1set:0 L2set:48 Index:5 hit
1 084C 71   L1set:2 L2set:2 Index:3 hit
1 1964 63   L1set:11 L2set:11 Index:1
0 54F0 11   miss
0 0848 22   hit
0 0E24 33   hit
0 188C 44   miss
*/

void init_DRAM()
{
	unsigned int i=0;
	DRAM = malloc(sizeof(char) * DRAM_SIZE);
	for(i=0;i<DRAM_SIZE/4;i++)
	{
		*((unsigned int*)DRAM+i) = i;
	}
	//for(int n=0;n<1000;n++){
       // printf("byte %d: %d\n", n/4,DRAM[n]);
	//}
}

void printCache()
{
	int i,j,k;
	printf("===== L1 Cache Content =====\n");
	for(i=0;i<16;i++)
	{
		printf("Set %d :", i);
		for(j=0;j<2;j++)
		{
			printf(" {(TAG: 0x%x)", (unsigned int)(L1_cache[i][j].tag));
			for(k=0;k<8;k++)
				printf(" 0x%x,", (unsigned int)(L1_cache[i][j].data[k]));
			printf(" |");
		}
		printf("\n");
	}
	printf("===== L2 Cache Content =====\n");
	for(i=0;i<64;i++)
	{
		printf("Set %d :", i);
		for(j=0;j<4;j++)
		{
			printf(" {(TAG: 0x%x)", (unsigned int)(L2_cache[i][j].tag));
			for(k=0;k<8;k++)
				printf(" 0x%x,", (unsigned int)(L2_cache[i][j].data[k]));
			printf(" |");
		}
		printf("\n");
	}
}
uint32_t getDat(uint32_t address){
    //For getting offset from address
    uint32_t data = address;
    int bitmask = (1<<31)>>26;
    return ~bitmask&(data);
}

uint32_t getIdx(uint32_t address){
    //For getting index from offset
    uint32_t data = address;
    data = data>>2;
    int bitmask = (1<<31)>>28;
    return ~bitmask&(data);
}

uint32_t read_fifo(uint32_t address)
{


    int set1 = getL1SetID(address);
    int set2 = getL2SetID(address);
    int tag1 = getL1Tag(address);
    int tag2 = getL2Tag(address);
    int datbits = getDat(address);
    int datidx = getIdx(datbits);

    //printf("datidx %d tag1 %x tag2 %x set1 %d set2 %d\n",datidx, tag1,tag2,set1,set2);
    //Read and update L1


    //Return data from address offset index if hit
    if(L1lookup(address)==1){
        if(L1_cache[set1][0].tag==tag1){
            return L1_cache[set1][0].data[datidx];
        }
        else{
            return L1_cache[set1][1].data[datidx];
        }
    }



    //printf("addr: %d\n",address);
    if(L1lookup(address)==0 && L1_cache[set1][0].tag==0){
            //Case index 1 is free
            L1_cache[set1][0].tag=tag1;
            for(int i=0;i<8;i++){
                L1_cache[set1][0].data[i] = DRAM[address/32*32+((i)*4)];//load from DRAM
            }
            L1_cache[set1][0].timeStamp=cycles; //update time
    }

    else if(L1lookup(address)==0 && L1_cache[set1][1].tag==0){
            //Case index 2 is free
            L1_cache[set1][1].tag=tag1;
            for(int i=0;i<8;i++){
            L1_cache[set1][1].data[i] = DRAM[address/32*32+((i)*4)];//load from DRAM
        }
        L1_cache[set1][1].timeStamp=cycles; //update time
    }
    else if(L1lookup(address)==0){
        //Case full, then evict
            L1evict(address);
    }


    //If L2 hit return data from offset index
    if(L2lookup(address)==1){
        if(L2_cache[set2][0].tag==tag2){
            return L2_cache[set2][0].data[datidx];
        }
        else if(L2_cache[set2][1].tag==tag2){
            return L2_cache[set2][1].data[datidx];
        }
        else if(L2_cache[set2][2].tag==tag2){
            return L2_cache[set2][2].data[datidx];
        }
        else{
            return L2_cache[set2][3].data[datidx];
        }
    }

    //Read and update L2
    if(L2lookup(address)==0 && L2_cache[set2][0].tag==0){
        //Case index 1 is free
        L2_cache[set2][0].tag=tag2;
        for(int i=0;i<8;i++){
            L2_cache[set2][0].data[i] = DRAM[address/32*32+((i)*4)];// get from DRAM
        }
        L2_cache[set2][0].timeStamp=cycles; //update time
    }
    else if(L2lookup(address)==0 && L2_cache[set2][1].tag==0){
            //Case index 2 is free
            L2_cache[set2][1].tag=tag2;
            for(int i=0;i<8;i++){
            L2_cache[set2][1].data[i] = DRAM[address/32*32+((i)*4)];// get from DRAM
        }
        L2_cache[set2][1].timeStamp=cycles; //update time
    }
    else if(L2lookup(address)==0 && L2_cache[set2][2].tag==0){
            //Case index 3 is free
            L2_cache[set2][2].tag=tag2;
            for(int i=0;i<8;i++){
            L2_cache[set2][2].data[i] = DRAM[address/32*32+((i)*4)];// get from DRAM
        }
        L2_cache[set2][2].timeStamp=cycles; //update time
    }
    else if(L2lookup(address)==0 && L2_cache[set2][3].tag==0){
            L2_cache[set2][3].tag=tag2;
            for(int i=0;i<8;i++){
            L2_cache[set2][3].data[i] = DRAM[address/32*32+((i)*4)];// get from DRAM
        }
        L2_cache[set2][3].timeStamp=cycles; //update time
    }
    else if(L2lookup(address)==0){
            L2evict(address);
    }

return 0;
}


unsigned int getL1SetID(uint32_t address)
{

    // There are 16 sets
    uint32_t data = address;
    // shift data away
    data=data>>5;
    int bitmask = (1<<31)>>27;
    return ~bitmask&(data);

}

unsigned int getL2SetID(uint32_t address)
{


    // There are 64 sets
    uint32_t data = address;
    // shift data away
    data = data>>5;
    int bitmask = (1<<31)>>25;
    unsigned int sett= ~bitmask&(data);
    return sett;
}


unsigned int getL1Tag(uint32_t address)
{

    uint32_t addr = address;
    int bitmask = (1<<31)>>8;
    //shift 1 to get the required mask

    //apply mask
    int x = ~bitmask&(addr>>9);
    return x;
}

unsigned int getL2Tag(uint32_t address)
{

    uint32_t addr = address;
    int bitmask = (1<<31)>>10; //6?
    //shift 1 back to get the required mask

    //apply mask
    return ~bitmask&(addr>>11);
}

int L1lookup(uint32_t address)
{

    uint32_t adr = address;
    int set = getL1SetID(adr);
    // check cache blocks at set. If any blocks contains tag then hit
    for(int j=0;j<2;j++){
        if(L1_cache[set][j].tag==getL1Tag(address)){
                    //printf("set1 %d\n",set);
            return 1;
        }
    }
    // else miss

    return 0;
}

int L2lookup(uint32_t address)
{
    uint32_t adr = address;
    int set = getL2SetID(adr);
    // check cache blocks at set. If any blocks contains tag then hit
    for(int j=0;j<4;j++){
        if(L2_cache[set][j].tag==getL2Tag(address)){
            return 1;
        }
    }
    // else miss
    return 0;
}


uint32_t * L1evict(int address)
{
    int tempaddr=address;
    int setindex = getL1SetID(address);
    int tag = getL1Tag(address);
    uint32_t *temp = malloc(sizeof(uint32_t)*8);
    // Data = DRAM address calculate which byte in DRAM to copy to data. Pick 8 elements from DRAM into that structure
    if(L1_cache[setindex][0].timeStamp<L1_cache[setindex][1].timeStamp){
        //  return data
        *temp=L1_cache[setindex][0].data;
        //  set Tag
        L1_cache[setindex][0].tag=tag;
        for(int i=0;i<8;i++){
        // tHis +4 -> +8 -> up until 8 entries ENtrIES
        L1_cache[setindex][0].data[i] = DRAM[address/32*32+((i)*4)];
        }
        L1_cache[setindex][0].timeStamp=cycles;
        //  update timestamp
    }
    else{
        // similar to above
        *temp=L1_cache[setindex][1].data;
        L1_cache[setindex][1].tag=tag;
        for(int i=0;i<8;i++){
        L1_cache[setindex][1].data[i] = DRAM[address/32*32+((i)*4)];
        }
        L1_cache[setindex][1].timeStamp=cycles;
    }

    return temp;
}



uint32_t * L2evict(int address)
{

    int tempaddr=address;
    int setindex = getL2SetID(address);
    int tag = getL2Tag(address);
    uint32_t *temp = malloc(sizeof(uint32_t)*8);
    int lowtimeindex=0;
    int time=9999999999;
    //Find the earliest cacheblock
    for(int i=0;i<4;i++){
        if(L2_cache[setindex][i].timeStamp<time){
            time=L2_cache[setindex][i].timeStamp;
            lowtimeindex=i;
        }
    }
        //return data
        *temp=L2_cache[setindex][lowtimeindex].data;
        L2_cache[setindex][lowtimeindex].tag=tag;
        for(int i=0;i<8;i++){
            L2_cache[setindex][lowtimeindex].data[i] = DRAM[address/32*32+((i)*4)];// THis +4 -> +8 -> up until 8 entries ENtrIES
        }
        L2_cache[setindex][lowtimeindex].timeStamp=cycles;
        // update timestamp

    return temp;
}


uint32_t * write(uint32_t address, uint32_t data)
{
    //Update L1, update L2, update DRAM


	uint32_t data1 = data;
	uint32_t data2 = data;
	uint32_t dataDram = data;
    int tempaddr=address;
    int set1 = getL1SetID(tempaddr);
    int set2 = getL2SetID(tempaddr);
    int tag1 = getL1Tag(tempaddr);
    int tag2 = getL2Tag(tempaddr);
    int datbits = getDat(address);
    int datidx = getIdx(datbits);

    //printf("datidx %d tag1 %x tag2 %x set1 %d set2 %d\n", datidx, tag1,tag2,set1,set2);

    //CASE NONE FOUND
    if(L1lookup(tempaddr)==0 && L2lookup(tempaddr)==0){
        //cache miss put in L1, L2, update DRAM

         //PUT L1
            if(L1_cache[set1][0].tag==0){
                    L1_cache[set1][0].tag=tag1;
                    L1_cache[set1][0].data[datidx] = data;
                    L1_cache[set1][0].timeStamp=cycles;
            }
            else if(L1_cache[set1][1].tag==0){
                    L1_cache[set1][1].tag=tag1;
                    L1_cache[set1][1].data[datidx] = data;
                    L1_cache[set1][1].timeStamp=cycles;
            }
            else{
                    // Evict and write
                if(L1_cache[set1][0].timeStamp<L1_cache[set1][1].timeStamp){
                    L1_cache[set1][0].tag=tag1;
                    L1_cache[set1][0].data[datidx] = data;
                    L1_cache[set1][0].timeStamp=cycles;
                }
                else{
                    L1_cache[set1][1].tag=tag1;
                    L1_cache[set1][1].data[datidx] = data;
                    L1_cache[set1][1].timeStamp=cycles;

                }
            }

        //PUT L2
            if(L2_cache[set2][0].tag==0){
                    L2_cache[set2][0].tag=tag2;
                    L2_cache[set2][0].data[datidx] = data;
                    L2_cache[set2][0].timeStamp=cycles; //update time
            }

            else if(L2_cache[set2][1].tag==0){
                    L2_cache[set2][1].tag=tag2;
                    L2_cache[set2][1].data[datidx] = data;
                    L2_cache[set2][1].timeStamp=cycles; //update time
            }
            else if(L2_cache[set2][2].tag==0){
                    L2_cache[set2][2].tag=tag2;
                    L2_cache[set2][2].data[datidx] = data;
                    L2_cache[set2][2].timeStamp=cycles; //update time
            }
            else if(L2_cache[set2][3].tag==0){
                    L2_cache[set2][3].tag=tag2;
                    L2_cache[set2][3].data[datidx] = data;
                    L2_cache[set2][3].timeStamp=cycles;
            }
            else{
                //Evict L2 and write
                int lowtimeindex=0;
                int time=9999999999;
                for(int i=0;i<4;i++){
                    if(L2_cache[set2][i].timeStamp<time){
                        time=L2_cache[set2][i].timeStamp;
                        lowtimeindex=i;
                    }
                }
                    L2_cache[set2][lowtimeindex].tag=tag2;
                    L2_cache[set2][lowtimeindex].data[datidx] = data;
                    L2_cache[set2][lowtimeindex].timeStamp=cycles;
            }

            //PUT DRAM
            DRAM[address/32*32+((datidx)*4)] = data;
    }
    //CASE L2 FOUND
    else if(L1lookup(tempaddr)==0 && L2lookup(tempaddr)==1){
            //put into L1, update L2 and DRAM

            //PUT L1
            if(L1_cache[set1][0].tag==0){
                    L1_cache[set1][0].tag=tag1;
                    L1_cache[set1][0].data[datidx] = data;
                    L1_cache[set1][0].timeStamp=cycles;
            }
            else if(L1_cache[set1][1].tag==0){
                    L1_cache[set1][1].tag=tag1;
                    L1_cache[set1][1].data[datidx] = data;
                    L1_cache[set1][1].timeStamp=cycles;
            }
            else{
                    // Evict and write
                if(L1_cache[set1][0].timeStamp<L1_cache[set1][1].timeStamp){
                    L1_cache[set1][0].tag=tag1;
                    L1_cache[set1][0].data[datidx] = data;
                    L1_cache[set1][0].timeStamp=cycles;
                }
                else{
                    L1_cache[set1][1].tag=tag1;
                    L1_cache[set1][1].data[datidx] = data;
                    L1_cache[set1][1].timeStamp=cycles;
                }
            }

            //UPDATE L2
            int targetidx=0;
            for(int i=0;i<4;i++){
                if(L2_cache[set2][i].tag==tag2){
                    targetidx=i;
                }
            }
            L2_cache[set2][targetidx].data[datidx] = data;
            L2_cache[set2][targetidx].timeStamp=cycles;

            //UPDATE DRAM
            DRAM[address/32*32+((datidx)*4)] = data;
    }
    else{
            //UPDATE ALL

            //UPDATE L1
            if(L1_cache[set1][0].tag==tag1){
                L1_cache[set1][0].data[datidx] = data ;
                L1_cache[set1][0].timeStamp=cycles;
            }
            else{
                L1_cache[set1][1].data[datidx] = data ;
                L1_cache[set1][1].timeStamp=cycles;

            }

            //UPDATE L2
            int targetidx=0;
            for(int i=0;i<4;i++){
                if(L2_cache[set2][i].tag==tag2){
                    targetidx=i;
                }
            }
            L2_cache[set2][targetidx].data[datidx] = data ;
            L2_cache[set2][targetidx].timeStamp=cycles;

            //UPDATE DRAM
            DRAM[address/32*32+((datidx)*4)] = data;

        }

}


int main()
{
	init_DRAM();
	cacheAccess buffer;
	int timeTaken=0;
	FILE *trace = fopen("input.trace","r");
	int L1hit = 0;
	int L2hit = 0;
	cycles = 0;
	while(!feof(trace))
	{
		fscanf(trace,"%d %x %x", &buffer.readWrite, &buffer.address, &buffer.data);
		printf("Processing the request for [R/W] = %d, Address = %x, data = %x\n", buffer.readWrite, buffer.address, buffer.data);

		// Checking whether the current access is a hit or miss so that we can advance time correctly
		if(L1lookup(buffer.address))// Cache hit
		{
			timeTaken = 1;
			L1hit++;
		}
		else if(L2lookup(buffer.address))// L2 Cache Hit
		{
			L2hit++;
			timeTaken = 5;
		}
		else timeTaken = 50;
		if (buffer.readWrite) write(buffer.address, buffer.data);
		else read_fifo(buffer.address);
		cycles+=timeTaken;
	}
	printCache();
	printf("Total cycles used = %ld\nL1 hits = %d, L2 hits = %d", cycles, L1hit, L2hit);
	fclose(trace);
	free(DRAM);
	return 0;



}












