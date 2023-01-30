#include "derivative.h"
#include "keypad.h"
#include "advancedLCD.h"

#define KEY_UP		0
#define KEY_DOWN	1

const char scanCode[4] = {0xF8, 0xF4, 0xF2, 0xF1};

char keypadTable[16] = {'A','B','C','D', '3','6','9','#', '2','5','8','0', '1','4','7','*'};

unsigned char scanKeypad(void);

void initializeKeypad() 
{
  DDRA = 0x0F;
  PORTA = 0xFF;  
}


// Gets a number, terminated when 'D' is pressed. Pressing 'A' erases the last input.
unsigned long keypad_getNumber(void) 
{
  unsigned char k = 0;
  unsigned int number = 0;
  do 
  {
    k = keypad_getKeypress();
    if (k >= '0' && k <= '9') 
    {
      number *= 10;
      number += (k - '0');
      printLCDChar(k);  
    }
    else if (k == 'A') 
    {
      number /= 10;
      moveLCDBack(1);
      printLCDChar(' ');  
      moveLCDBack(1);
    }
  } while (k != 'D');
  
  return number;
}

unsigned char keypad_getKeypress(void) 
{
  int keyState = KEY_UP, keyPressed = 0;
  unsigned char k;
  do 
  {
    k = scanKeypad();
    if (keyState == KEY_UP && k != 0)
		{
			keyState = KEY_DOWN;
			keyPressed = k;
		}
		else if (keyState == KEY_DOWN && k == 0)
		{
			return keyPressed;
		}
  } while (1);
}

unsigned char scanKeypad()
{
  
  int i, j = 0;
  
	// Try each of four scan codes; this checks one row at a time
	for (i=0; i<4; i++)
	{
		PORTA = scanCode[i];
		
		if (PORTA_BIT4 == 1)
			return keypadTable[j];
		else
			j++;
			
		if (PORTA_BIT5 == 1)
			return keypadTable[j];
		else
			j++;
			
		if (PORTA_BIT6 == 1)
			return keypadTable[j];
		else
			j++;
			
		if (PORTA_BIT7 == 1)
			return keypadTable[j];
		else
			j++;
	}
	return 0;
}