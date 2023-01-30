///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Lab8_2.mcp: Implementation of protocol and application layer for LTC1661
//
// Final Lab Code: Matthew Mackay, 2019
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
#include <hidef.h>        // Common definitions for microcontroller family
#include "derivative.h"   // Definitions specific to 9S12DG256
#include "pll.h"          // Function to modify PLL on HCS12 to allow 24MHz operation


// PHYSICAL LAYER 
void InitializeSPI(void);

// PROTOCOL LAYER 
void SPI_Send(unsigned char, unsigned char);

// APPLICATION LAYER
void DAC_SetOutputA(unsigned int output);

void main(void) 
{
  // Sets the CPU clock to maximum possible (24 MHz)
  PLL_Init();                                   
  
	////////////////////////// Hardware Initialization /////////////////////////////
	InitializeSPI();
	////////////////////////// Hardware Initialization /////////////////////////////

  while (1) 
  {
    DAC_SetOutputA(0);
    DAC_SetOutputA(150);
    DAC_SetOutputA(512);
    DAC_SetOutputA(874);
    DAC_SetOutputA(1023);
    DAC_SetOutputA(874);
    DAC_SetOutputA(512);
    DAC_SetOutputA(150);
  }
}

////////// Start SPI Physical Layer ////////// 

void InitializeSPI() 
{
  // TODO: Copy your code for InitializeSPI() from Part 1 here. No changes needed.
  
  
  // CS line idles high
  PTM_PTM6 = 1;
}

////////// End SPI Physical Layer ////////// 



////////// Start SPI Protocol Layer ////////// 

void SPI_Send(unsigned char data1, unsigned char data2) 
{
  // TODO: Copy your code for SPI_Send() from Part 1 and modify as follows:
  
  /*
        1.	Assert SS for the LTC1661 low.
        
        2.	Set SPI0DR equal to the first byte we want to send ('data1').
        3.	Wait for SPIF = 1.
        4.	'Read' and discard the incoming word from SPI0DR.
        
        5.	Set SPI0DR equal to the second byte we want to send ('data2').
        6.	Wait for SPIF = 1.
        7.	'Read' and discard the incoming word from SPI0DR.
        
        8.	De-assert SS back to idle (high).
  */

  // Note that we do not use a loop here, we just repeat the send byte code.
}

////////// End SPI Protocol Layer //////////

 

////////// Start Application Layer ////////// 

// Sends the two data bytes formatted as expected by this specific chip.
void DAC_SetOutputA(unsigned int output) 
{
  // Implement the procedure for sending one full DAC update from the lab manual. Recall the format:
  
  // [  A3 A2 A3 A0   D9 D8 D7 D6 D5 D4 D3 D2 D1 D0 XX XX ]
  // [ Control Code | Input code / DACA value      | Any  ]
  // [-----------data1-----------|----------data2---------]    SPI_Send(data1, data2)
  
}

////////// End Application Layer ////////// 







