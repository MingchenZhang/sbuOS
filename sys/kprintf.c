#include <sys/defs.h>
#include <sys/kprintf.h>
#include <stdarg.h>

#define _SCREEN_BASE 0xb8000
#define _SCREEN_LINES 25
#define _SCREEN_COLOUMS 80

void print_func(char c);
void print_num(int input, void (*print_func)(char));
void print_string(const char* input, void (*print_func)(char));
void print_pointer(void* input, void (*print_func)(char));
void print_hex(unsigned long input, void (*print_func)(char));

int cursor = 0;
uint64_t topright_length=0;
uint8_t color = 0b00001111;

static inline void scroll_lines_up() {
	for(char* c = (char*)(_SCREEN_BASE + _SCREEN_COLOUMS*2);c < (char*)(_SCREEN_BASE + _SCREEN_COLOUMS*2*2 - (topright_length*2)); c++){
		*(c-_SCREEN_COLOUMS*2) = *c;
	}
	for(char* c = (char*)(_SCREEN_BASE + _SCREEN_COLOUMS*2*2);c < (char*)(_SCREEN_BASE+ _SCREEN_COLOUMS*_SCREEN_LINES*2); c++){
		*(c-_SCREEN_COLOUMS*2) = *c;
	}
	for(char* c = (char*)_SCREEN_BASE+ _SCREEN_COLOUMS*(_SCREEN_LINES-1)*2;c < (char*)(_SCREEN_BASE+ _SCREEN_COLOUMS*_SCREEN_LINES*2); c++){
		*c = 0;
	}
}

void print_topright(const char* str, uint8_t color){
	uint64_t old_topright_length=topright_length;
	topright_length = 0;
	while(*(str+topright_length)){
		topright_length++;
	}
	if(old_topright_length > topright_length){ // overwrite with zero if the length is shortened
		for(int c = _SCREEN_COLOUMS-old_topright_length; c<_SCREEN_COLOUMS-topright_length; c++){
			*((char*)_SCREEN_BASE+c*2)=0;
			*((char*)_SCREEN_BASE+c*2+1)=0;
		}
	}
	int old_cursor = cursor;
	cursor=_SCREEN_COLOUMS-topright_length; 
	int index=0;
	while(*(str+index)){
		*((char*)_SCREEN_BASE+cursor*2)=*(str+index);
		*((char*)_SCREEN_BASE+cursor*2+1)=color;
		index++;
		cursor++;	
	}
	cursor = old_cursor;
}

void print_func(char c) {
	*((char*)_SCREEN_BASE + cursor*2 + 1) &= 0b10111111; // de-blink
	if(c == '\n'){
		cursor = (cursor/80 + 1)*80;
	}else{
		*(char*)((char*)_SCREEN_BASE + cursor*2) = c;
		*(char*)((char*)_SCREEN_BASE + cursor*2 + 1) = color;
		cursor++;
	}
	if(cursor >= (_SCREEN_LINES*_SCREEN_COLOUMS)) {
		scroll_lines_up();
		cursor -= _SCREEN_COLOUMS;
	}
	*((char*)_SCREEN_BASE + cursor*2 + 1) |= 0b01000000; // blink
}


void kprintf(const char *format, ...) {
	unsigned int fi = 0;
    //char *arg = (char *) &format + sizeof(format);
    va_list ap;
    va_start(ap, format);
    for(;;fi++){
        if(format[fi] == 0){
            return;
        }else if(format[fi] == '%'){
            char modifier = format[++fi];
            unsigned long sl;
            int si;
            char sc;
            void* sv;
            switch(modifier){
                case 'c': 
                    sc = va_arg(ap, int);
                    print_func(sc);
                    break;
                case 'd':
                    si = va_arg(ap, int);
                    print_num(si, print_func);
                    break;
                case 'x':
                    sl = va_arg(ap, unsigned long);
                    print_hex(sl, print_func);
                    break;
                case 's':
                    sv = va_arg(ap, char*);
                    print_string((char*)sv, print_func);
                    break;
                case 'p':
                    sv = va_arg(ap, void*);
                    print_pointer(sv, print_func);
                    break;
                default:
                    return;
            }
        }else{
            print_func(format[fi]);
        }
    }
}

void clear_screen(){
	for(char* c = (char*)(_SCREEN_BASE);c < (char*)(_SCREEN_BASE + _SCREEN_LINES*_SCREEN_COLOUMS*2); c++){
		*(c) = 0;
	}
	cursor = 0;
	*((char*)_SCREEN_BASE + cursor*2 + 1) |= 0b01000000; // blink
}

char* kprintfa_dest;
int kprintfa_index;
int kprintfa_limit;
static inline void kprintfa_print(char c){
	if(kprintfa_index>=(kprintfa_limit-1) && c) return;
	*(kprintfa_dest + kprintfa_index++) = c;
	// kprintf("%c", c);
}
void kprintfa(char* dest, int size, const char *format, ...) {
	unsigned int fi = 0;
    kprintfa_dest = dest;
	kprintfa_index = 0;
	kprintfa_limit = size;
    va_list ap;
    va_start(ap, format);
    for(;;fi++){
        if(format[fi] == 0){
			kprintfa_print(0); // add null terminator
            return;
        }else if(format[fi] == '%'){
            char modifier = format[++fi];
            unsigned long sl;
            int si;
            char sc;
            void* sv;
            switch(modifier){
                case 'c': 
                    sc = va_arg(ap, int);
                    kprintfa_print(sc);
                    break;
                case 'd':
                    si = va_arg(ap, int);
                    print_num(si, kprintfa_print);
                    break;
                case 'x':
                    sl = va_arg(ap, unsigned long);
                    print_hex(sl, kprintfa_print);
                    break;
                case 's':
                    sv = va_arg(ap, char*);
                    print_string((char*)sv, kprintfa_print);
                    break;
                case 'p':
                    sv = va_arg(ap, void*);
                    print_pointer(sv, kprintfa_print);
                    break;
                default:
                    return;
            }
        }else{
            kprintfa_print(format[fi]);
        }
    }
}


void print_num(int input, void (*print_func)(char)) {
    // max 2,147,483,647
    int is_negative = input >> (sizeof(input)*8-1);
    unsigned int in = input;
    if(is_negative) {
        print_func('-');
        in = ~in;
        in++;
    }
    char buffer[16];
    int index = 0;
    for(;;){
        buffer[index++] = 48 + (in % 10);
        in /= 10;
        if(in <= 0) break;
    }
    for(index--; index >=0; index--){
        print_func(buffer[index]);
    }
}

void print_string(const char* input, void (*print_func)(char)) {
    for(int i=0; input[i] != 0; i++){
        print_func(input[i]);
    }
}

void print_pointer(void* input, void (*print_func)(char)) {
    unsigned long backup;
    char point_char[20]="";
    int point_char_index=0;
    int shift_index;
    for(shift_index=0;shift_index<=15;shift_index++){
        backup=(unsigned long)input;
        int left_shift=4*shift_index;
        unsigned  long intermedia_value1 =(backup<<left_shift);    
        unsigned long intermedia_value2=(intermedia_value1>>60);
        int i;
        for(i=0;i<=15;i++){
            if(intermedia_value2==i){
                if(i<10){
                    point_char[point_char_index]=(char)(intermedia_value2+48);
                }
                else if(i>=10){
                    point_char[point_char_index]=(char)(intermedia_value2-10+65);
                }
            }
        }
        
        point_char_index++;        
    }
    int i;
    print_func('0');
    print_func('x');
    i=0;
    while(point_char[i]=='0'){        
        if(point_char[i]!='0'){
            break;
        }
		i++;		
    }
	if(point_char[i]=='\0'){
		print_func('0');
		print_func('0');
	}
	else {
    while(i<point_char_index){
		print_func(point_char[i]);
		i++;
		}
	}
}
    
void print_hex(unsigned long input, void (*print_func)(char)) {
    unsigned long backup;
    char hex_char[20]="";
    int hex_char_index=0;
    int shift_index;
    for(shift_index=0;shift_index<=15;shift_index++){
        backup=(unsigned long)input;
        int left_shift=4*shift_index;
        unsigned long intermedia_value1 =(backup<<left_shift);    
        unsigned long intermedia_value2=(intermedia_value1>>60);
        int i;
        for(i=0;i<=15;i++){
            if(intermedia_value2==i){
                if(i<10){
                    hex_char[hex_char_index]=(char)(intermedia_value2+48);
                }
                else if(i>=10){
                    hex_char[hex_char_index]=(char)(intermedia_value2-10+65);
                }
            }
        }
        
        hex_char_index++;        
    }
   // hex_char[index]='\0';
    // now print the hex value
    print_func('0');
    print_func('x');
    int print_index=0;
    while(hex_char[print_index]=='0' && print_index<hex_char_index){            
        if(hex_char[print_index]!='0'){
            break;
        }		
         print_index++;		 
    }
	if(hex_char[print_index]=='\0'){
		print_func('0');
		print_func('0');
	}
	else {
    while(print_index<hex_char_index){
        print_func(hex_char[print_index]);
        print_index++;
    }
	}
}