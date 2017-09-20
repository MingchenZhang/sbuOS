#include <sys/defs.h>
#include <sys/kprintf.h>
#include <sys/keyboard.h>
#include <sys/misc.h>

int shift = 0;
int ctrl = 0;

uint8_t ascii_map[256] = {
	0,0,'1','2','3','4','5','6','7','8','9','0','-','=',0,0,
	'q','w','e','r','t','y','u','i','o','p','[',']',0,0,'a','s',
	'd','f','g','h','j','k','l',';','\'','`',0,'\\','z','x','c','v',
	'b','n','m',',','.','/',0,0,0,' ',0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0	
};

// index is the scan code, ascii_map[index] is the ascci character
static uint8_t translate_keyboard_scan_code(uint8_t code, char is_shift){
	uint8_t returnValue = ascii_map[(uint8_t)code];
	if(is_shift && _isAlphabet(returnValue)) returnValue-=32;
	else if(is_shift && !_isAlphabet(returnValue)) returnValue=shiftConvert(returnValue);
	return returnValue;
}



void handle_keyboard_scan_code(uint8_t code){
	uint8_t ascii;
	if(code == 0x2A){ // shift
		shift = 1;
	}else if(code == 0xAA){ // release shift
		shift = 0;
	}else if(code == 0x1D){ // ctrl
		ctrl = 1;
	}else if(code == 0x9D){ // release ctrl
		ctrl = 0;
	}else{
		int is_release = 0;
		if((ascii = translate_keyboard_scan_code(code, shift))){
		}else if((ascii = translate_keyboard_scan_code((code^0b10000000), shift))){
			is_release = 1;
		}
		if(ascii){
			if(!is_release && !ctrl){
				// kprintf("key press: %c\n", ascii);
				last_key_pressed[0] = ascii;
				last_key_pressed[1] = 0;
				update_topright_display();
			}else if(!is_release && ctrl){
				// kprintf("key press: ^%c\n", ascii);
				last_key_pressed[0] = '^';
				last_key_pressed[1] = ascii;
				last_key_pressed[2] = 0;
				update_topright_display();
			}
		}
	}
}

char shiftConvert(uint8_t code){
	char asc_array[100]={
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,'\"',
		0,0,0,0,'<','_','>','?',')','!',
		'@','#','$','%','^','&','*','(',0,':',
		0,'+',0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		0,0,0,0,0,0,0,0,0,0,
		'0','{','|','}',0,0,'~',0,0,0		
	};
	return asc_array[code];
}



int _isAlphabet(uint8_t code){
	if (code>=97 && code <=122)
		return 1;
	return 0;	
}
