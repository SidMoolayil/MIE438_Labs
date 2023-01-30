///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Lab8_1.mcp: Basic SPI Output
//
// Final Lab Code: Matthew Mackay, 2019
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
#include <hidef.h>        // Common definitions for microcontroller family
#include "derivative.h"   // Definitions specific to 9S12DG256
#include "pll.h"          // Function to modify PLL on HCS12 to allow 24MHz operation


// PHYSICAL LAYER 
void InitializeSPI(void);
void SPI_Send(unsigned char);

void main(void) 
{
  // Sets the CPU clock to maximum possible (24 MHz)
  PLL_Init();                                   

  // Set up the SPI subsystem  
  InitializeSPI();
  
  // Enter a short loop that continuously sends a recognizable pattern
  while(1) 
  {
      SPI_Send(0b10010110);
  }
  
}

////////// Start SPI Physical Layer ////////// 

// Initializes the basic SPI functionality; all instructions are specific
// to the HCS12DG256 and the Dragon12-Plus2 connections.
void InitializeSPI() 
{
  // YOUR tasks: 
  //    1. Set SPI0CR1 according to pre-lab discussion.
  //    2. Set SPI0CR2 according to pre-lab discussion.
  //    3. Set SPI0BR according to pre-lab discussion.
  //    4. Set DDRS according to pre-lab discussion.
  //    5. Set DDRM according to pre-lab discussion.
  
  // CS line idles high
  PTM_PTM6 = 1;
}

// Sends a double byte to the device connected to the SPI bus. The CS line
// should be held high for the entire duration of the send. Could be improved
// by replacing microcontroller-specific names (PTM_PTM6, SPI0DR, etc.) with
// more generic inline functions or aliases in the physical layer.
void SPI_Send(unsigned char data) 
{
  // YOUR tasks:
  // 1. Assert SS low
  // 2. Send the 8-bit word 'data'  
  // 3. Wait for SPIF = 1
  // 4. Read and discard the incoming word; note that assigning SPI0DR to any storage location forces a 'read'
  // 5. De-assert SS high  
}
 








