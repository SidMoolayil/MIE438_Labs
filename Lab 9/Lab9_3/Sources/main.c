#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "advancedLCD.h"

// Scan codes used to check for keypad key presses
const char scanCode[4] = {0xF8, 0xF4, 0xF2, 0xF1};

// For this program, we mostly keep the original keypad mapping. All numbers return their literal number (0-9, not ASCII '0','1', etc.).
const unsigned char keypadTable[16] = {0x00,0x00,0x00,0x00, 0x03,0x06,0x09,0x0C, 0x02,0x05,0x08,0x0B, 0x01,0x04,0x07,0x0A};


void setOutput(int output);
char scanKeypad();
int limitMagnitude(long a, unsigned int mag);

int position;

unsigned char lastEncoderState = 0, currentEncoderState = 0;

// Try different KP values with P-control only:
//  * 50 - Fast response, oscillates near final value, jump due to slow LCD update is noticeable
//  * 10 - Mod. to Fast response, minimal oscillation
//  * 1  - Slow response, no oscillation, some absolute position error

#define KP 10
#define KI 1
#define KD 1

void main(void) 
{
  long reference = 0, newReference = 0, error, lastError, control;
  unsigned char keyOld = 0, key = 0;
  
  int update = 0;
  
  // **************** Keypad Initilization ****************
  DDRA = 0x0F;
  PORTA = 0xFF;
  // **************** Keypad Initilization ****************

  
  // **************** PWM Initilization ****************
  DDRB = 0xFF;
  DDRP = 0xFF;
  PORTB = 0b00000001;
  
  PWMCLK = 0x01;
  PWMPRCLK = 0x07;
  PWMPOL = 0x01;
  PWMCAE = 0;
  PWMCTL = 0;
  PWME = 0x01;
  
  PWMSCLA = 0x04;
  PWMPER0 = 0x74;
  PWMDTY0 = 58;
  
  setOutput(0);
  // **************** PWM Initilization ****************
  
  
  // **************** LCD Initilization ****************
  DDRK = 0xFF;  
  shortWait(160);
  initializeLCD();
  shortWait(160);
  clearLCD();
  printLCDText("Servo Lab v1.0$");
  moveLCDTo(0,1);
  printLCDText("Refer: $");
  // **************** LCD Initilization ****************
  
  // **************** Port P Interrupt / Encoder Initilization ****************
  DDRP = 0b11110011;
  PERP = 0b00000000;
  PPSP_PPSP2 = 1;
  PPSP_PPSP3 = 1;
  PIFP = 0b00001100;
  PIEP = 0b00001100;
  __asm CLI;     
  position = 0;         
  // **************** Port P Interrupt / Encoder Initilization ****************
    
  for(;;) 
  {
    update++;
    if (update % 60000 == 0) 
    {
      moveLCDTo(0,0);
      printLCDText("Actual:   $");
      printLCDNumber(position);    
    }
    
    
    if (update % 100 == 0) 
    {
      keyOld = key;
      key = scanKeypad();
      
      if (keyOld != 0 && key == 0) 
      {
        if (keyOld == 0x0A) 
        {
          reference = newReference;
          newReference = 0;
          
          moveLCDTo(10,1);
          printLCDNumber(limitMagnitude(reference,32766));
        } 
        else if (keyOld == 0x0C) 
        {
          newReference *= -1;  
          
          moveLCDTo(10,1);
          printLCDNumber(limitMagnitude(newReference,32766));
        }
        else 
        {
          newReference *= 10;
          if (keyOld != 0x0B && keyOld != 0x0C)
            newReference += keyOld;
          
          moveLCDTo(10,1);
          printLCDNumber(limitMagnitude(newReference,32766));
        }
      }
    }
    
    
    lastError = error;
    error = position - reference;
    
    control = KP*lastError;
    control = limitMagnitude(control, 116);
    
    setOutput(control);
  }
}

// Function which scans the keypad and returns keycode of any pressed key, zero if no key pressed
char scanKeypad()
{
  
  int i, j = 0;
  
	// Try each of four scan codes; this checks one row at a time
	for (i=0; i<4; i++)
	{
		PORTA = scanCode[i];
		
		// Check each column in this row, return ASCII code if pressed. Could be made shorter, but format shows function.
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

// Function that sets the PWM output for Motor 1 on Port B motor driver; assume correct initialization
//   * Direction is selected by sign of 'output'
//   * Speed is selected by magnitude of 'output'
//   * Under original PWM settings, 116 is maximum speed (100% duty cycle)
void setOutput(int output) 
{
    if (output < 0) 
    {
        PORTB = 0b00000010;
        PWMDTY0 = (unsigned char)(-1*output);
    } 
    else 
    {
        PORTB = 0b00000001;
        PWMDTY0 = (unsigned char)(output);
    }
  
}

void interrupt VectorNumber_Vportp Port_P_ISR(void) 
{
    lastEncoderState = currentEncoderState;
    currentEncoderState = (PTP & 0b00001100) >> 2;
    
    if (currentEncoderState == 0x01 && lastEncoderState == 0x03)
      position++;
    else if  (currentEncoderState == 0x03 && lastEncoderState == 0x02)
      position--; 
    
    if (PIFP_PIFP2 == 1) 
       PIFP = PIFP_PIFP2_MASK;
    else if (PIFP_PIFP3 == 1) 
       PIFP = PIFP_PIFP3_MASK;
}

int limitMagnitude(long a, unsigned int mag) 
{
  if (a > 0) 
  {
    if(a>mag)
      return mag;
    else
      return a;
  } else 
  {
    if((-1*a)>mag) 
    {
      return -1*mag;
    } 
    else 
    {
      return a;
    }
      
  }
}


