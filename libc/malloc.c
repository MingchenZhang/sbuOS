

#include <unistd.h>
#include <stdio.h>
#include <sys/defs.h>
#include <debuglib.h>

/*
#define bool int
#define size_t uint64_t
#define false 0
#define true 1
#define sf_sbrk sbrk
#define error printf
#define debug(...)

#define ALLOC_SIZE_BITS 4
#define BLOCK_SIZE_BITS 28
#define REQST_SIZE_BITS 32
#define PAGE_SIZE 4096

static void __memset(volatile void* addr, uint8_t value, uint64_t size){
	uint8_t* cursor = (void*)addr;
	for(uint8_t* end = (uint8_t*)addr + size; cursor < end; *(cursor++) = value);
}

static void* __memcpy(void *dest, const void *src, size_t n){
	char *pszDest =(char*)dest;
	const char *pszSource =(char*)src;
	while (n--){
		*pszDest =*pszSource;
		pszDest++; pszSource++;
	}
	return dest;
}


struct __attribute__((__packed__)) sf_header{
    uint64_t alloc : ALLOC_SIZE_BITS;
    uint64_t block_size : BLOCK_SIZE_BITS;
    uint64_t requested_size : REQST_SIZE_BITS;
};
typedef struct sf_header sf_header;

struct __attribute__((__packed__)) sf_free_header {
    sf_header header;
    struct sf_free_header *next;
    struct sf_free_header *prev;
};
typedef struct sf_free_header sf_free_header;

struct __attribute__((__packed__)) sf_footer {
    uint64_t alloc : ALLOC_SIZE_BITS;
    uint64_t block_size : BLOCK_SIZE_BITS;
};
typedef struct sf_footer sf_footer;

void* sf_malloc(size_t size);
void sf_free(void *ptr);
void* sf_realloc(void *ptr, size_t size);
void* sf_calloc(size_t nmemb, size_t size);

sf_header* heapStartPtr = NULL;//in case someone sabotage
sf_free_header* freelist_head = NULL;
bool firstPageInitialized = false;
#define PAGE_SIZE 4096

struct sbrk_return {
    void* startAddress;
    size_t sizeGet;
};
typedef struct sbrk_return sbrk_return;

static inline sf_header* getFooter(sf_header* header){
	return header+(header->block_size<<1)-1;
}
static inline sf_header* getHeader(sf_header* footer){
	return footer-(footer->block_size<<1)+1;
}
static inline sf_header* headerToNextHeader(sf_header* header){
	return header+(header->block_size<<1);
}
static inline sf_header* footerToPrevFooter(sf_header* footer){
	return footer-(footer->block_size<<1);
}

void sf_free_of(void *ptr, bool onlyCoalesceForward);
sf_free_header* sf_initializeFreeBlock(void* startPt, sf_free_header* lastBlock, size_t newPageSize);
sf_free_header* sf_coalescing(sf_free_header* header, bool onlyForward);
void* sf_allocate(sf_free_header* freeHeader, size_t requiredSize);
sbrk_return sf_getMoreSpace(size_t size);
size_t getSize(void* ptr);
bool checkPtrValidity(void* ptr);
bool checkSizeValidity(size_t size);


void* sf_malloc(size_t size) {
	if(!checkSizeValidity(size)) return NULL;
	sf_free_header* cursor = freelist_head;
	
	sf_free_header* lastCursor = NULL;
	int sizeNeeded = (((size + 16 - 1) / 16)*2 + 2)*8;
	sizeNeeded = sizeNeeded;// suppress unused variable warning
	
	while (cursor != NULL) {
		if (cursor->header.alloc == 1) error("debug: error, meet allocated block while searching free list\n"); //debug
		if ((cursor->header.block_size << 4) >= sizeNeeded) break;
		lastCursor = cursor;
		cursor = cursor->next;
	}
	
	if (cursor == NULL) {// no block has enough space, ask sbrk() for space and put in cursor
		sbrk_return unallocSpace = sf_getMoreSpace((int)sizeNeeded);
		if(unallocSpace.startAddress == NULL) {
			// errno = ENOMEM; 
			return NULL;
		}
		cursor = sf_initializeFreeBlock(unallocSpace.startAddress, lastCursor, unallocSpace.sizeGet);
		firstPageInitialized = true;
		cursor = sf_coalescing(cursor, false);
	}
	debug("snapshot before sf_allocate, allocate to %p\n", cursor);
	
	debug("start sf_allocate\n");
	return sf_allocate(cursor, size);
}

void sf_free(void *ptr) {
	sf_free_of(ptr, false);
}

void sf_free_of(void *ptr, bool onlyCoalesceForward) {
	if(!checkPtrValidity(ptr)) return;
	
	sf_free_header* freedHeader = (sf_free_header*)(((sf_header*)ptr)-1);
	debug("header is being freed:\n");//debug
	freedHeader->header.alloc = 0;
	freedHeader->header.requested_size = 0; //not required
	sf_header* footer = getFooter((sf_header*)freedHeader);
	footer->alloc = 0;
	footer->requested_size = 0;

	sf_free_header* prevFree = NULL;
	sf_free_header* nextFree = NULL;
	
	sf_header* prevHeader = (sf_header*)freedHeader;
	while((prevHeader-1)->alloc==1 && (prevHeader-1)->block_size!=0){
		debug("sf_free_of proceeding to prev block current:%p\n", prevHeader);
		prevHeader -= (prevHeader-1)->block_size<<1;
		if(prevHeader->alloc==0) error("debug: error, No.545\n"); //debug
	}
	if((prevHeader-1)->block_size!=0)
		prevFree = (sf_free_header*)(prevHeader- ((prevHeader-1)->block_size<<1));
	
	sf_header* nextFooter = getFooter((sf_header*)freedHeader);
	while((nextFooter+1)->alloc==1 && (nextFooter+1)->block_size!=0){
		debug("sf_free_of proceeding to next block current:%p\n", nextFooter);
		nextFooter += (nextFooter+1)->block_size<<1;
		if(nextFooter->alloc==0) error("debug: error, No.546\n"); //debug
	}
	if((nextFooter+1)->block_size!=0)
		nextFree = (sf_free_header*)getHeader(nextFooter+ ((nextFooter+1)->block_size<<1));
	
	freedHeader->next = nextFree;
	freedHeader->prev = prevFree;
	if(nextFree!=NULL)nextFree->prev = freedHeader;
	if(prevFree!=NULL)prevFree->next = freedHeader;
	
	if(freedHeader->prev==NULL) freelist_head = freedHeader;// fix root
	
	debug("sf_free_of - snapshot - before sf_coalescing\n");
	
	sf_coalescing(freedHeader, onlyCoalesceForward);
}

void* sf_realloc(void *ptr, size_t size) {
	if(!checkSizeValidity(size)) return NULL;
	if(!checkPtrValidity(ptr)) return NULL;
	
	if (((unsigned long) ptr & 15) != 0) error("debug: error, sf_realloc received misaligned address\n"); //debug
	sf_header* originalHeader = (((sf_header*)ptr)-1);
	//size_t originalRSize = originalHeader->requested_size;
	size_t originalRealSize = originalHeader->block_size<<4;
	size_t newRealSize = (((size + 16 - 1) / 16)*2 + 2)*8;
	if(newRealSize > originalRealSize){// expand
		size_t totalAvailSize = originalRealSize;
		if(headerToNextHeader(originalHeader)->alloc == 0)
			totalAvailSize += (headerToNextHeader(originalHeader)->block_size<<4) -16;
		if(totalAvailSize>=newRealSize){// merge a part of free block
			debug("sf_realloc branch 1.1 taken\n");
			sf_free_header* dataInNext = ((sf_free_header*)originalHeader)->next;
			sf_free_header* dataInPrev = ((sf_free_header*)originalHeader)->prev;
			sf_free_of(originalHeader+1, true);
			sf_free_header* newAllocHeader = (sf_free_header*)(((sf_header*)sf_allocate((sf_free_header*)originalHeader, size))-1);
			newAllocHeader->prev = dataInPrev;
			newAllocHeader->next = dataInNext;
			return ((sf_header*)newAllocHeader)+1;
		}else{// malloc at new place and copy
			debug("sf_realloc branch 1.2 taken\n");
			void* newSpace = sf_malloc(size);
			if(newSpace == NULL) return NULL;
			__memcpy(newSpace, originalHeader+1, originalHeader->requested_size);//not everything in block gets copied
			sf_free(originalHeader+1);
			return newSpace;
		}
	}else if(newRealSize > ((originalRealSize>32)?originalRealSize:32)-32){// only update info
		debug("sf_realloc branch 2 taken\n");
		originalHeader->requested_size = size;
		getFooter(originalHeader)->requested_size = size;// not required
	}else{// break down block
		debug("sf_realloc branch 3 taken\n");
		originalHeader->block_size = newRealSize>>4;
		originalHeader->requested_size = size;
		sf_header* newAllocfooter = getFooter(originalHeader);
		newAllocfooter->alloc = 1;
		newAllocfooter->block_size = originalHeader->block_size;
		newAllocfooter->requested_size = size; //not required
		sf_header* freedHeader = newAllocfooter+1;
		freedHeader->alloc = 1;
		freedHeader->block_size = (originalRealSize-newRealSize)>>4;
		freedHeader->requested_size = 0; //not required
		__memcpy(getFooter(freedHeader), freedHeader, sizeof(sf_header));//setup footer
		
		sf_free(freedHeader+1);
	}
	return originalHeader+1;
}

void* sf_calloc(size_t nmemb, size_t size) {
	if(!checkSizeValidity(nmemb*size)) return NULL;
	void* newSpace = sf_malloc(nmemb*size);
	if(newSpace != NULL)
		__memset(newSpace, 0, nmemb*size);
	return newSpace;
}

void* sf_allocate(sf_free_header* freeHeader, size_t requiredSize) {
	if (((unsigned long) freeHeader & 15) != 8) error("sf_allocate received misaligned address\n"); //debug
	size_t available8Bytes = headerToNextHeader((sf_header*)freeHeader) - ((sf_header*)freeHeader);
	size_t real8Bytes = ((requiredSize + 16 - 1) / 16)*2 + 2;
	if(available8Bytes < real8Bytes) error(" (SRIS) \n"); //debug: sf_allocate received insufficient space
	if(available8Bytes-4 < real8Bytes) { // will normally produce unusable space
		real8Bytes = available8Bytes;
	}
	sf_free_header* prevFree = freeHeader->prev;
	sf_free_header* nextFree = freeHeader->next;
	
	sf_header* allocHeader = (sf_header*)freeHeader;
	allocHeader->alloc = 1;
	allocHeader->block_size = real8Bytes>>1;
	allocHeader->requested_size = requiredSize;
	sf_header* allocFooter = getFooter(allocHeader);
	allocFooter->alloc = 1;
	allocFooter->block_size = allocHeader->block_size;
	allocFooter->requested_size = 0; // not required
	
	size_t leftFree8Bytes = available8Bytes - real8Bytes;
	if(leftFree8Bytes!=0){ // left space for free block
		sf_free_header* leftFreeHeader = (sf_free_header*)(((sf_header*)freeHeader)+real8Bytes);
		leftFreeHeader->header.alloc = 0;
		leftFreeHeader->header.block_size = leftFree8Bytes>>1;
		leftFreeHeader->header.requested_size = 0; // not required
		sf_header* leftFreeFooter = getFooter((sf_header*)leftFreeHeader);
		leftFreeFooter->alloc = 0;
		leftFreeFooter->block_size = leftFreeHeader->header.block_size;
		leftFreeFooter->requested_size = 0; //not required
		
		leftFreeHeader->next = nextFree;
		leftFreeHeader->prev = prevFree;
		if(nextFree!=NULL){
			nextFree->prev = leftFreeHeader;
		}
		if(prevFree == NULL){
			if(freeHeader!=freelist_head) error("No.4532\n");//debug
			freelist_head = leftFreeHeader;
		}else prevFree->next = leftFreeHeader;
	}else{ // no free block left
		if(nextFree!=NULL){
			nextFree->prev = prevFree;
		}
		if(prevFree == NULL) {
			if(freeHeader!=freelist_head) error("No.4533\n");//debug
			freelist_head = nextFree;
		}else{
			prevFree->next = nextFree;
		}
	}
	
	return allocHeader+1;
}

sf_free_header* sf_initializeFreeBlock(void* startPt, sf_free_header* lastBlock, size_t newPageSize) {
	sf_free_header* header;
	if(firstPageInitialized){
		header = (sf_free_header*)(((sf_header*)startPt)-1);
		header->header.alloc = 0;
		header->header.block_size = newPageSize>>4;
		header->header.requested_size = 0;
		header->next = NULL;
		header->prev = lastBlock;
		sf_header* footer = getFooter((sf_header*)header);
		footer->alloc = 0;
		footer->block_size = newPageSize>>4;
		footer->requested_size = 0;
		__memset(footer+1, 0, sizeof(sf_header));
	}else{
		header = (sf_free_header*)(((sf_header*)startPt)+1);
		heapStartPtr = (sf_header*)header; // in case someone sabotage
		header->header.alloc = 0;
		header->header.block_size = (newPageSize>>4)-1;
		header->header.requested_size = 0;
		header->next = NULL;
		header->prev = lastBlock;
		sf_header* footer = getFooter((sf_header*)header);
		footer->alloc = 0;
		footer->block_size = (newPageSize>>4)-1;
		footer->requested_size = 0;
		__memset(((sf_header*)header)-1, 0, sizeof(sf_header));
		__memset(((sf_header*)footer)+1, 0, sizeof(sf_header));
	}
	if(lastBlock!=NULL) lastBlock->next = header;
	else {
		freelist_head = header;
	}
	
	return sf_coalescing(header, false);
}

sf_free_header* sf_coalescing(sf_free_header* header, bool onlyForward) {
	
	sf_header* prevHeader = (sf_header*)header;
	while((prevHeader-1)->alloc==0 && (prevHeader-1)->block_size!=0 && !onlyForward){
		debug("sf_coalescing proceeding to prev block current:%p\n", prevHeader);
		prevHeader -= (prevHeader-1)->block_size<<1;
		if(prevHeader->alloc!=0) error("debug: error, No.543\n"); //debug
	}
	sf_free_header* prevPt = ((sf_free_header*)prevHeader)->prev;
	
	sf_header* nextFooter = getFooter((sf_header*)header);
	while((nextFooter+1)->alloc==0 && (nextFooter+1)->block_size!=0){
		debug("sf_coalescing proceeding to next block current:%p\n", nextFooter);
		nextFooter += (nextFooter+1)->block_size<<1;
		if(nextFooter->alloc!=0) error("debug: error, No.544\n"); //debug
	}
	sf_free_header* nextPt = ((sf_free_header*)getHeader(nextFooter))->next;
	
	sf_free_header* newHeader = ((sf_free_header*)prevHeader);
	newHeader->header.alloc = 0;
	newHeader->header.block_size = (nextFooter+1-prevHeader)>>1;
	newHeader->header.requested_size = 0;//not required
	newHeader->next = nextPt;
	newHeader->prev = prevPt;
	if(nextPt!=NULL) newHeader->next->prev = newHeader;
	if(prevPt!=NULL) newHeader->prev->next = newHeader;
	
	__memcpy(nextFooter, newHeader, sizeof(sf_header));
	
	if(newHeader->prev==NULL) freelist_head = newHeader;
	
	return newHeader;
}

sbrk_return sf_getMoreSpace(size_t size){
	sbrk_return result;
	int pageNeeded;
	if(firstPageInitialized){
		sf_header* lastFreeBlock = sf_sbrk(0);
		lastFreeBlock -= 2;
		if(lastFreeBlock->alloc==0){
			size -= lastFreeBlock->block_size << 4;
		}
		pageNeeded = (size + PAGE_SIZE - 1) / PAGE_SIZE; //integer division ceiling
	}else
		pageNeeded = (size+16 + PAGE_SIZE - 1) / PAGE_SIZE; //integer division ceiling, first page has smaller space
	
	void* newPageAddress = sf_sbrk(pageNeeded * PAGE_SIZE);//todo: check piazza for sf_sbrk size and errno here
	
	if(newPageAddress != (void*)-1) result.startAddress = newPageAddress;
	else result.startAddress = NULL;
	result.sizeGet = pageNeeded * PAGE_SIZE;
	return result;
}

bool checkPtrValidity(void* ptr){
	// check validity
	if(ptr>sf_sbrk(0) || ptr<(void*)heapStartPtr) {
		// errno = EFAULT;
		return false;
	}
	if( ((uint64_t)ptr&15) != 0 ){
		// errno = EFAULT;
		return false;
	}
	sf_header* footer = getFooter(((sf_header*)ptr)-1);
	if(((void*)footer>sf_sbrk(0) || (void*)footer<(void*)heapStartPtr || (void*)footer < ptr ) ||
			( (((sf_header*)ptr)-1)->block_size != footer->block_size && footer->block_size > 1) ){
		// errno = EFAULT;
		return false;
	}
	return true;
}

bool checkSizeValidity(size_t size){
	if(size == 0){
		// errno = EINVAL;
		return false;
	}
	if(size > ((long)4<<30) ){
		// errno = EINVAL;
		return false;
	}
	return true;
}




void* malloc(size_t size){
	void* ret = sf_malloc(size);
	return ret;
}
void free(void *ptr){
	
}

*/
uint64_t current_brk = 0;
uint64_t allocated = 0;

void* malloc(size_t size){
	if(current_brk == 0){
		current_brk = (uint64_t)brk(0);
		allocated = current_brk;
	}
	uint64_t want_to = (((allocated+size)-1)/4096+1)*4096;
	if(want_to > current_brk){
		brk(want_to);
		current_brk = want_to;
	}
	uint64_t ret = allocated;
	allocated += size;
	return (void*)ret;
}
void free(void *ptr){
	
}