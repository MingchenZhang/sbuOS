#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/config.h>
#include <sys/misc.h>
#include <stdarg.h>

volatile uint64_t pic_tick_count = 0;
uint8_t last_key_pressed[5] = {0,0,0,0,0};

void asm_outb(uint16_t port, uint8_t value) {
    __asm__( "outb %0, %1" : : "a"(value), "Nd"(port) );
}
uint8_t asm_inb(uint16_t port) {
    uint8_t ret;
    __asm__( "inb %1, %0": "=a"(ret): "Nd"(port) );
    return ret;
}

void asm_outl(uint16_t port, uint32_t value) {
	__asm__( "outl %0, %1" : : "a"(value), "Nd"(port) );
}
uint32_t asm_inl(uint16_t port) {
    uint32_t ret;
    __asm__( "inl %1, %0": "=a"(ret): "Nd"(port) );
    return ret;
}

void update_topright_display(){
	char line_buffer[50];
	kprintfa(line_buffer, sizeof(line_buffer), "dT: %d, Key: %s", pic_tick_count/PIT_FREQUENCY, last_key_pressed);
	print_topright(line_buffer, (get_random(8) << 4) | 0b00001111);
}

uint32_t _seed = 23213;
uint32_t get_random(uint32_t limit){
	_seed = _seed * 25173 + 13849;
	return _seed%limit;
}

/* size is the range of space in byte*/
void memset(volatile void* addr, uint8_t value, uint64_t size){
	uint8_t* cursor = (void*)addr;
	for(uint8_t* end = (uint8_t*)addr + size; cursor < end; *(cursor++) = value);
}

void* memcpy(void *dest, const void *src, size_t n){
	char *pszDest =(char*)dest;
	const char *pszSource =(char*)src;
	while (n--){
		*pszDest =*pszSource;
		pszDest++; pszSource++;
	}
	return dest;
}

int streq(char* a, char* b){
	for(;*a == *b; a++, b++){
		if(*a == 0) return 1;
	}
	return 0;
}

uint32_t strlen(char* a){
	uint32_t ret = 0;
	for(;*a; a++, ret++);
	return ret;
}

int memeq(void* a1, void* b1, int64_t size){
	unsigned char* a = a1;
	unsigned char* b = b1;
	for(;*a == *b; a++, b++, size--){
		if(size == 1) return 1;
	}
	return 0;
}

uint64_t math_min(uint64_t first, ...){
	va_list ap;
    va_start(ap, first);
	uint64_t min = first;
	for(;;){
		uint64_t n = va_arg(ap, uint64_t);
		if(n == (uint64_t)-1) break;
		if(n<min) min = n;
	}
	return min;
}

void panic_halt() {
	while(1);
}

void assert(int boolean, char* error){
	if(!boolean){
		kprintf(error);
		while(1);
	}
}

void debug_wait(){
	for(uint64_t i=0; i<1000000000L; i++);
}