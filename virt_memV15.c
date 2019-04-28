//
//  virt_mem.c
//  virt_mem
//
//  Created by William McCarthy on 3/23/19.
//  Copyright © 2019 William McCarthy. All rights reserved.
//	This is the completed version of the virt_mem PROJECT with FIFO and can write to BACKING_STORE.bin when a page has been modified
//   Aarib Abdullah

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define FRAME_SIZE  256

#define TLB_ENTRIES 16
#define TLB_ROWS 2    // 0 - page ; 1 - offset
#define PAGE_TABLE_ENTRIES  256

#define FRAME_SIZE 256
#define NUM_OF_FRAMES 8


//int TLB[TLB_SIZE][TLB_ROWS];
//int PAGE_TABLE[PAGE_TABLE_ENTRIES];
//char PHYSICAL_MEM[NUM_OF_FRAMES][FRAME_SIZE];
//char * Physical_addr = NULL;

struct  tlb {
  int page_number[TLB_ENTRIES];
  int frame_number[TLB_ENTRIES];
  int validBit[TLB_ENTRIES] ;// = { 0 };  // 0 - not valid, index not in use ; 1 - index occupied by page
  int queueOutPoint;
  //int queueInPoint = 0;
  int tlb_hit;
  int tlb_miss;
  int Modified_bit[TLB_ENTRIES];
} TLB;
// remember to initialize the validBits
// remember to initialize queueOutPoint
struct  page_table{
  int page_number[PAGE_TABLE_ENTRIES];
  int frame_number[PAGE_TABLE_ENTRIES];
  int validBit[PAGE_TABLE_ENTRIES];	// remember to intialize the validBits  0 -not valid; 1 - index occupied by page
  int queueOutPoint;
  int page_table_miss;
  int page_table_hit;
  int Modified_bit[PAGE_TABLE_ENTRIES];
} PGTBL;
 //struct tlb TLB;
 //struct page_table PGTBL;
void initialize_tables(){
	TLB.queueOutPoint = 0;
	for(int i = 0; i < TLB_ENTRIES; i++){
		TLB.validBit[i] = 0;
		TLB.Modified_bit[i] = 0;
	}
	TLB.queueOutPoint = 0;
	TLB.tlb_miss=0;
	TLB.tlb_hit = 0;
	
	for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
		PGTBL.validBit[i] = 0;
		PGTBL.Modified_bit[i] = 0;
	}
	PGTBL.queueOutPoint = 0;
	PGTBL.page_table_miss = 0;
	PGTBL.page_table_hit = 0 ;
}
//-------------------------------------------------------------------
unsigned int getpage(size_t x) { return (0xff00 & x) >> 8; }
unsigned int getoffset(unsigned int x) { return (0xff & x); }

void update_TLB(int pageNum, int frame_number){
  /*for(int i = 0 ; i < TLB_ENTRIES; i++){
      if( TLB.validBit[i] == 0){
        TLB.page_number[i] = pageNum;
        TLB.frame_number[i] = frame_number;
        TLB.validBit[i] = 1;
        return;
      }
  }*/
int index = TLB.queueOutPoint;
TLB.page_number[index] = pageNum;
TLB.frame_number[index] = frame_number;
TLB.validBit[index] = 1;
TLB.queueOutPoint++;
if(TLB.queueOutPoint >= TLB_ENTRIES) {TLB.queueOutPoint = 0;}

}

void update_PGTBL(int pageNum, int frame_number){
  /*for(int i = 0 ; i < PAGE_TABLE_ENTRIES; i++){
      if( PGTBL.validBit[i] == 0){
        PGTBL.page_number[i] = pageNum;
        PGTBL.frame_number[i] = frame_number;
        PGTBL.validBit[i] = 1;
        return;
      }
  }*/
int index = PGTBL.queueOutPoint;
PGTBL.page_number[index] = pageNum;
PGTBL.frame_number[index] = frame_number;
PGTBL.validBit[index] = 1;
PGTBL.queueOutPoint++;
if(PGTBL.queueOutPoint >= PAGE_TABLE_ENTRIES) {PGTBL.queueOutPoint = 0;}

//update_TLB(pageNum, frame_number);
}

int IsItInTLB(int pageNumber){  // will return frame number, if not found will return -1
  for(int i = 0 ; i < TLB_ENTRIES ; i++){
      //if(TLB.validBit[i] != 0){ return -1;}
      if( TLB.validBit[i] == 1 && TLB.page_number[i] == pageNumber) {
		  TLB.tlb_hit++; 
		  printf(" (--------), ");
		  return TLB.frame_number[i];
	  }
  }
  TLB.tlb_miss++;
  printf(" (tlbMss: %d), ", pageNumber);
  return -1;
}

int IsItInPageTable(int logical_address, int pageNum){
	for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
		if(PGTBL.validBit[i] == 1 && PGTBL.page_number[i] == pageNum) {
			PGTBL.page_table_hit++; 
			printf(" (--------), ");	
			update_TLB(PGTBL.page_number[i], PGTBL.frame_number[i] );
			return PGTBL.frame_number[i];
		}
	}		
	PGTBL.page_table_miss++;
	printf(" (pgFlt: %d), ", pageNum);
	return -1;				// may add a page fault function either 
}
	
void invalidate_Bit(int pageNum, int frameNum){
	
	for(int i = 0; i < TLB_ENTRIES; i++){
		if(TLB.page_number[i] != pageNum && TLB.frame_number[i] == frameNum){TLB.validBit[i] = 0;}
	}
	
	for(int i= 0 ; i < PAGE_TABLE_ENTRIES; i++){
		if(PGTBL.page_number[i] != pageNum && PGTBL.frame_number[i] == frameNum){PGTBL.validBit[i] = 0;}
	}
	
}

void PageModified_BITset(int pageNumber){
	for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
		if(PGTBL.page_number[i] == pageNumber && PGTBL.validBit[i] == 1 ){
			PGTBL.Modified_bit[i] = 1;
			return;
		}
	}
}

int IsPageModified(int frame_OF_page){
	for(int i = 0; i < PAGE_TABLE_ENTRIES; i++){
		if( PGTBL.frame_number[i] == frame_OF_page && PGTBL.validBit[i] == 1 && PGTBL.Modified_bit[i] == 1){
			PGTBL.Modified_bit[i] = 0;
			return PGTBL.page_number[i];
		}
	}
	return -1;
}

/*											//this code is not in use
void getpage_offset(unsigned int x) {
  unsigned int page = getpage(x);
  unsigned int offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}
*/

int main(int argc, const char * argv[]) {
	char * Physical_addr = NULL;
	Physical_addr = (char *) malloc(NUM_OF_FRAMES * FRAME_SIZE );
		
  FILE* fadd = fopen("copy_addresses.txt", "r");
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("copy_correct.txt", "r");
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fbinary = fopen("copy_BACKING_STORE.bin", "rb+");
  if( fbinary == NULL)	{ fprintf(stderr, "Could not open file: 'BACKING_STORE.BIN'\n");  exit(FILE_ERROR); }


  char buf[BUFLEN];
  unsigned int page, offset, physical_add, frame = 0;
  unsigned int logic_add;                  // read from file address.txt
  unsigned int virt_add, phys_add, value;  // read from file correct.txt

      // not quite correct -- should search page table before creating a new entry
      //   e.g., address # 25 from addresses.txt will fail the assertion
      // TODO:  add page table code
      // TODO:  add TLB code
   //initialize_tables();
  int numberOfAddressesRead = 1;    	
  while (frame < NUM_OF_FRAMES && !feof(fadd) && !feof(fcorr)) {
	  
	  if(frame % 50 == 0 ){ printf("\n\nframe value is : %d \n\n", frame);}
	  
    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add,
           buf, buf, &phys_add, buf, &value);  // read from file correct.txt

    fscanf(fadd, "%d", &logic_add);  // read from file address.txt
    if(feof(fadd) && feof(fcorr)){break;}
    printf("#%d of addressline read\n", numberOfAddressesRead++);
    page = getpage(logic_add);
    offset = getoffset(logic_add);
	printf("PAGE: %d\tOFFSET: %d\n", page, offset);
	
	printf("LOGICAL ADDR: %d\t%d\n", logic_add, virt_add);
	
    if(frame < NUM_OF_FRAMES){  printf("nextAvail Frame: %d   ", frame + 1); }
    
    // todo: read BINARY_STORE and confirm value matches read value from correct.txt
    //int found_page;
    int found_frame;
     
  
    
    if((found_frame = IsItInTLB(page)) == -1){
		if((found_frame = IsItInPageTable(logic_add, page)) == -1){	// when this is true, this handles the page fault
			
			int start_index = frame * FRAME_SIZE;
			////////////////////////////////////////////////////////////////////
			//int fame_OF_page;
			int modified_page;
			if( (modified_page = IsPageModified(frame)) != -1){
				
				fseek(fbinary, modified_page * 256, SEEK_SET);
				fwrite((Physical_addr + start_index), sizeof(char), 256, fbinary);
			
			}
			////////////////////////////////////////////////////////////////////
			
			
			
			
			//fseek(fbinary , page * 256, SEEK_SET);
			
			
			//found_frame = frame;
			//found_page = page ;		
			
			
			fseek(fbinary , page * 256, SEEK_SET);
			fread(  (Physical_addr + start_index), sizeof(char), 256, fbinary);

			
			physical_add = frame++ * FRAME_SIZE + offset;
			frame = frame % NUM_OF_FRAMES;
			
			printf("PHYSICAL ADDR: %d\t%d\n", physical_add, phys_add);
			
			update_PGTBL(page, frame);
			update_TLB(page, frame);
			invalidate_Bit(page, frame);
			
			printf(" \nLogic: %d (pg: %d/off: %d) --> phys: %d (fr: %d/off: %d), phys_mem/corr_val: %d \n\n", 
						logic_add, page, offset, physical_add, frame, offset, (int) Physical_addr[physical_add]);
			//assert((int) Physical_addr[physical_add] == value);	
			if(page == 244){
					for(int i = 0; i < 127; i++){
						Physical_addr[physical_add + i ] = (char)i;
					}
					PageModified_BITset(page);
			}
			
		}else{
				physical_add = found_frame * FRAME_SIZE + offset;
				
				/*if(found_frame == 244){
					for(int i = 0; i < 127; i++){
						Physical_addr[physical_add + i ] = (char)i;
					}
				}*/
				
				printf("PHYSICAL ADDR: %d\t%d\n", physical_add, phys_add);
				
				printf(" Logic: %d (pg: %d/off: %d) --> phys: %d (fr: %d/off: %d), phys_mem/corr_val: %d \n\n", 
						logic_add, page, offset, physical_add, found_frame, offset, (int) Physical_addr[physical_add]); 
				//assert((int) Physical_addr[physical_add] == value);	
		}
	}else{
		physical_add = found_frame * FRAME_SIZE + offset;
		printf("PHYSICAL ADDR: %d\t%d\n", physical_add, phys_add);			
		printf(" Logic: %d (pg: %d/off: %d) --> phys: %d (fr: %d/off: %d), phys_mem/corr_val: %d \n\n", 
				logic_add, page, offset, physical_add, found_frame, offset, (int) Physical_addr[physical_add]);
						
		//assert((int) Physical_addr[physical_add] == value);	
	}
    //printf("logical: %5u (page:%3u, offset:%3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }
  }
  fclose(fcorr);
  fclose(fadd);
  fclose(fbinary);
	free(Physical_addr);

  printf("ALL logical ---> physical assertions PASSED!\n");
  printf("!!! This doesn't work passed entry 24 in correct.txt, because of a duplicate page table entry\n");
  printf("--- you have to implement the PTE and TLB part of this code\n");
  printf("\n\t\t...done.\n");
  
  printf("\nSTATISTICS:\nPage-fault rate: %f \nTLB hit rate: %f \n\n",(((double)PGTBL.page_table_miss/numberOfAddressesRead)*100), (((double)TLB.tlb_hit/numberOfAddressesRead)*100));
  return 0;
}
