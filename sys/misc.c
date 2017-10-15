#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/config.h>
#include <sys/misc.h>

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

void panic_halt() {
	while(1);
}

void debug_wait(){
	for(uint64_t i=0; i<1000000000L; i++);
}