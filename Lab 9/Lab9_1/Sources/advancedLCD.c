/*************************************************************************
* MIE 438 - Microprocessors and Embedded Microcontrollers                *
*------------------------------------------------------------------------*
* File:    advancedLCD.c                                                 *
* Author:  Andrio Renan Gonzatti Frizon                                  *
*          andrio.frizon@gmail.com                                       *
*------------------------------------------------------------------------*
* Created on:          July 22, 2014                                     *
* Last modified on:    July 28, 2014                                     *
*------------------------------------------------------------------------*
* Function writeLCDValue based on LCD example from                       *
* MicroDigitalEd website.                                                *
* http://www.microdigitaled.com/HCS12/Hardware/Dragon12-Plus-Support.htm *
*************************************************************************/

#include "derivative.h"
#include "advancedLCD.h"

#define LCD_DATA PORTK
#define LCD_CTRL PORTK
#define RS 0x01
#define EN 0x02

int linePosition = 0;
int lineNumber = 0;


/************************************************
*   shortWait	                                *
*	                                            *
*	Desc.: Waits roughly (ms) milliseconds	    *
*	Inputs:	 ms - Number of milliseconds	    *
*	Outputs: None	                            *
************************************************/

void shortWait(int ms){
    int i, j;
    for (i=0; i<ms; i++){
        for (j=0; j<4000; j++);
    }
}

/************************************************
*	writeLCDValue	                            *
*	                                            *
*	Desc.: Writes a byte to the LCD	            *
*	Inputs:	value - Byte to write	            *
*	        type - TYPE_CHAR or TYPE_INST	    *
*	Outputs: None	                            *
************************************************/ 

void writeLCDValue(char value, int type){
    unsigned char x;

    x = (value & 0xF0) >> 2;            //shift high nibble to center of byte for Pk5-Pk2
    LCD_DATA =LCD_DATA & ~0x3C;         //clear bits Pk5-Pk2
    LCD_DATA = LCD_DATA | x;            //sends high nibble to PORTK
    LCD_CTRL = type ? (LCD_CTRL & ~RS) : (LCD_CTRL | RS);   //set RS to command (RS=0) if type = TYPE_INST
                                                            //                  (RS=1) if type = TYPE_CHAR
    LCD_CTRL = LCD_CTRL | EN;           //raise enable
    shortWait(1);
    LCD_CTRL = LCD_CTRL & ~EN;          //Drop enable to capture command
    shortWait(1);                       //wait
    x = (value & 0x0F)<< 2;             // shift low nibble to center of byte for Pk5-Pk2
    LCD_DATA =LCD_DATA & ~0x3C;         //clear bits Pk5-Pk2
    LCD_DATA =LCD_DATA | x;             //send low nibble to PORTK
    LCD_CTRL = LCD_CTRL | EN;           //raise enable
    shortWait(1);
    LCD_CTRL = LCD_CTRL & ~EN;          //drop enable to capture command
    shortWait(1);
}

/************************************************
*	initializeLCD	                            *
*	                                            *
*	Desc.: Initializes the LCD	                *
*	Inputs:	 None	                            *
*	Outputs: None	                            *
************************************************/ 

void initializeLCD(){
    writeLCDValue(0x33,TYPE_INST);      //reset sequence provided by data sheet
    shortWait(160);
    writeLCDValue(0x32,TYPE_INST);      //reset sequence provided by data sheet
    shortWait(160);
    writeLCDValue(0x28,TYPE_INST);      //Function set to four bit data length
                                        //2 line, 5 x 7 dot format
    writeLCDValue(0x08,TYPE_INST);      //Turn the display OFF 
    writeLCDValue(0x0E,TYPE_INST);      //Turn the display ON 
    writeLCDValue(0x01,TYPE_INST);      //Clear the display
    writeLCDValue(0x06,TYPE_INST);      //Turn to entry mode
    writeLCDValue(0x02,TYPE_INST);      //Send cursor to home 
}

/************************************************
*	printLCDText	                            *
*	                                            *
*	Desc.: Prints a string of text to the 	    *
*          current line of LCD. If there is a   *
*          '\n' character, the function         *
*          automatically moves down to the next *
*          LCD line. All strings must and with  *
*          a '$' character.                     *
*	Inputs:	 String str                         *
*	Outputs: None	                            *
************************************************/ 

void printLCDText(char *str){
	int i=0;
	while (str[i] != '$')
	{
		if (str[i] == '\n')
		{
			if (lineNumber == 0)
			{
				writeLCDValue(0xC0, TYPE_INST);
				lineNumber = 1;
			}
			else
			{
				writeLCDValue(0x80, TYPE_INST);
				lineNumber = 0;
			}
			linePosition = 0;
		}
		else if (linePosition < LCD_WIDTH)
		{
			writeLCDValue(str[i], TYPE_CHAR);
			linePosition++;
		}
		
		i++;
	}
}

/************************************************
*	printLCDNumber	                            *
*	                                            *
*	Desc.: Converts a int to a string, adds a   *
*          '$' at the end, and call             *
*          printLCDText function to print it.   *
*	Inputs:	 unsigned int num                   *
*	Outputs: None	                            *
************************************************/ 

void printLCDNumber(unsigned int num){
	char dig0, dig1, dig2, dig3, dig4;
	
	dig0 = (num % 10) + 48;
	dig1 = (num % 100)/10 + 48;
	dig2 = (num % 1000)/100 + 48;
	dig3 = (num % 10000)/1000 + 48;
	dig4 = (num/10000) + 48;
	
	writeLCDValue(dig4, TYPE_CHAR);
	writeLCDValue(dig3, TYPE_CHAR);
	writeLCDValue(dig2, TYPE_CHAR);
	writeLCDValue(dig1, TYPE_CHAR);
	writeLCDValue(dig0, TYPE_CHAR);
	linePosition += 5;
}

/************************************************
*	clearLCD    	                            *
*	                                            *
*	Desc.: Clear the LCD display and send the   *
*          cursor home                          *
*          printLCDText function to print it.   *
*	Inputs:	 None                               *
*	Outputs: None	                            *
************************************************/ 

void clearLCD(void){
    writeLCDValue(0x01,TYPE_INST);      //Clear the display
    writeLCDValue(0x02,TYPE_INST);      //Send cursor home
    shortWait(1);    
    linePosition = 0;
	  lineNumber = 0;
}



void moveLCDBack(int space)
{
	//if (space > 0 && space <= linePosition)
	{
		linePosition -= space;
		if (lineNumber == 0)
		{
			writeLCDValue(0x80+linePosition, TYPE_INST);
		}
		else
		{
			writeLCDValue(0xC0+linePosition, TYPE_INST);
		}
	}
}

void moveLCDTo(int x, int y)
{
	if (y >= 0 && y < LCD_HEIGHT && x >= 0 && x < LCD_WIDTH)
	{
		if (y == 0)
			writeLCDValue(0x80+x, TYPE_INST);
		else
			writeLCDValue(0xC0+x, TYPE_INST);
		linePosition = x;
		lineNumber = y;
	}
}

void printLCDChar(unsigned char c) 
{
  if (linePosition < LCD_WIDTH) 
  {
    writeLCDValue(c, TYPE_CHAR);
    linePosition++;  
  }
}