#ifndef _ADVANCED_LCD_H
#define _ADVANCED_LCD_H

// Types for function writeValue()
#define	 TYPE_CHAR	 0
#define	 TYPE_INST	 1

#define LCD_WIDTH 16
#define LCD_HEIGHT 2       

#define FORMAT_DEC	0
#define FORMAT_HEX  1
#define FORMAT_CHR	2

// Function prototypes - tell the compiler that these functions exist somewhere
void initializeLCD(void);
void shortWait(int ms);
void writeLCDValue(char value, int type);
void printLCDText(char *str);
void printLCDNumber(int num);
void clearLCD(void);
void printLCDChar(unsigned char);

void moveLCDBack(int space);
void moveLCDTo(int x, int y);

#endif