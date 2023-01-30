#ifndef _KEYPAD_H
#define _KAYPAD_H

// Function prototypes - tell the compiler that these functions exist somewhere
void initializeKeypad(void);
unsigned char keypad_getKeypress(void);
unsigned long keypad_getNumber(void);

#endif