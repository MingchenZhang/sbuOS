#include <sys/kprintf.h>

void print_func(char c);
void print_num(int input, void (*print_func)(char));
void print_string(const char* input, void (*print_func)(char));
void print_pointer(void* input, void (*print_func)(char));
void print_hex(unsigned long input, void (*print_func)(char));

int cursor = 0;

void print_func(char c) {
	*(char*)(0xb8000 + cursor++*2) = c;
	if(cursor == 2000) cursor = 0;
}

void kprintf(const char *fmt, ...) {
	unsigned int fi = 0;
    //char *arg = (char *) &format + sizeof(format);
    va_list ap;
    va_start(ap, format);
    for(;;fi++){
        if(format[fi] == 0){
            return fi;
        }else if(format[fi] == '%'){
            char modifier = format[++fi];
            long sl;
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
                    sl = va_arg(ap, long);
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
                    return -1;
            }
        }else{
            print_func(format[fi]);
        }
    }
}


void print_num(int input, void (*print_func)(char)) {
    // max 2,147,483,647
    int is_negative = input >> sizeof(input)*8-1;
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
    int right_shift=60;
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
    while(i<point_char_index){
          print_func(point_char[i]);
            i++;
        }
    }
   
 

    
    
    
    
    
    
void print_hex(unsigned long input, void (*print_func)(char)) {
    unsigned long backup;
    int right_shift=60;
    char hex_char[20]="";
    int hex_char_index=0;
    int shift_index;
    for(shift_index=0;shift_index<=15;shift_index++){
        backup=(unsigned long)input;
        int left_shift=4*shift_index;
        unsigned  long intermedia_value1 =(backup<<left_shift);    
        unsigned  long intermedia_value2=(intermedia_value1>>60);
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
    while(hex_char[print_index]=='0'){            
        if(hex_char[print_index]!='0'){
            break;
        }
         print_index++;
    }
    while(print_index<hex_char_index){
        print_func(hex_char[print_index]);
        print_index++;
    }
   
   
}