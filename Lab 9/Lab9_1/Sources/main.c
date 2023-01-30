#include <hidef.h>      /* common defines and macros */
#include "derivative.h"      /* derivative-specific definitions */
#include "advancedLCD.h"

// Scan codes used to check for keypad key presses
const char scanCode[4] = {0xF8, 0xF4, 0xF2, 0xF1};

// For this program, we mostly keep the original keypad mapping. All numbers return their literal number (0-9, not ASCII '0','1', etc.).
const unsigned char keypadTable[16] = {0x00,0x00,0x00,0x00, 0x03,0x06,0x09,0x00, 0x02,0x05,0x08,0x0B, 0x01,0x04,0x07,0x0A};


void setOutput(int output);
char scanKeypad();


void main(void)
{
  int speed = 0;
  unsigned char keyOld = 0, key = 0;

  // **************** Keypad Initilization ****************
  DDRA = 0x0F;
  PORTA = 0xFF;
  // **************** Keypad Initilization ****************


  // **************** PWM Initilization ****************
  DDRB = 0xFF;      // Port B are outputs; only need PB0 and PB1
  DDRP = 0xFF;      // Set Port P as outputs; only need PP0
  PORTB = 0b00000001; // Set initial direction; PB0 and PB1 are direction control for Motor A

  PWMCLK = 0x01;    // Select SA Clock for additional prescaling
  PWMPRCLK = 0x07;  // Maximum general prescaling, b111 = 127 setting
  PWMPOL = 0x01;    // Inverted polarity for motor driver
  PWMCAE = 0;
  PWMCTL = 0;       // Defaults are fine for CAE/CTL
  PWME = 0x01;      // Enable PWM

  PWMSCLA = 0x04;   // SA Clock prescaling of 4
  PWMPER0 = 0x74;   // Maximum PWM at DTY = 116 (decimal)
  PWMDTY0 = 58;     // Overwritten immediately by setOutput, but would be 50% duty

  setOutput(speed);
  // **************** PWM Initilization ****************


  // **************** LCD Initilization ****************
  DDRK = 0xFF;
  shortWait(160);
  initializeLCD();
  shortWait(160);
  clearLCD();
  printLCDText("Servo Lab v1.0$");
  moveLCDTo(0,1);
  printLCDText("Speed: $");
  // **************** LCD Initilization ****************

  for(;;)
  {
    keyOld = key;
    key = scanKeypad();

    if (keyOld != 0 && key == 0)
    {
      if (keyOld == 0x0A)
      {
        // Sets the motor speed
        setOutput(speed);
        moveLCDTo(8,1);
        printLCDText("     $");
        speed = 0;
      }
      else
      {
        // Digit 0-9; build new speed setting until '*' pressed
        speed *= 10;
        if (keyOld != 0x0B)
          speed += keyOld;
        moveLCDTo(8,1);
        printLCDNumber(speed);

      }
    }
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
