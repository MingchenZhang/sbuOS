#ifndef _KEYBOARD_H
#define _KEYBOARD_H

//static uint8_t translate_keyboard_scan_code(uint8_t code, char is_shift);
void handle_keyboard_scan_code(uint8_t code);
char shiftConvert(uint8_t code);
int _isAlphabet(uint8_t code);

#endif