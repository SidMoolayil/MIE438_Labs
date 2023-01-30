///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Lab8_3.mcp: Demonstration of function generator application using DDS
//
// Final Lab Code: Matthew Mackay, 2019
//
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
 
#include <hidef.h>        // Common definitions for microcontroller family
#include "derivative.h"   // Definitions specific to 9S12DG256
#include "pll.h"          // Function to modify PLL on HCS12 to allow 24MHz operation
#include "advancedLCD.h"  // LCD Functions
#include "keypad.h"       // Keypad Functions
#include <math.h>         // Sine function


// TO USE FUNCTION GENERATOR: CONNECT SCOPE PROBE TO DACA CHANNEL ON PIN HEADERS (don't forget a ground connection too)

// This is the core of the program. We need samples to be outputted to the DAC very quickly. We do not have time
// to wait for the microcontroller to calculate a single floating point operation (as it does not have built-in
// support - everything is done with 10s or 100s of int-calculations). To compensate, we pre-calculate a 'lookup
// table' array based on the current frequency and amplitude. Arbitrary functions are possible in this way as
// well. The main loop is thus very short - it just outputs a value to the DAC and increments a counter.
unsigned int lookupTable[512] = { 0 };
// In order to generate a range of frequencies, we shorten or lengthen the space used (to a max. of 512).
unsigned int lookupTableDepth = 8;
// At very low frequencies, the buffer gets too large. We cap its size and instead insert a delay between each
// change in the DAC output.
unsigned long addedDelay = 0;

// This function is called whenever the frequency or amplitude changes
void calculateLookupTable(unsigned long, unsigned long);

// PHYSICAL LAYER - Communication over SPI specific to 68HCS12DG256, including PORT setup. No helper/inline functions
// needed in this application.
void InitializeSPI(void);

// PROTOCOL LAYER - Communication over SPI in general; does not care how InitializeSPI is implemented. Would call
// helper functions if they were used.
void SPI_Send(unsigned char, unsigned char);

// APPLICATION LAYER - SPI communications specific to this DAC chip.
void InitializeDAC(void);
void DAC_SetOutputA(unsigned int);



// Helper function; used to add delays with low frequency
#pragma INLINE
void delay_us(long us) 
{
  long i;
  for (i=0; i<us; i++)
    asm NOP;
}

void main(void) 
{
  // Stores current amplitude and frequency
  unsigned long amplitude = 5000, frequency = 1000;
  unsigned int i=0;
  // Sets the CPU clock to maximum possible (24 MHz)
  PLL_Init();                                   
  
	////////////////////////// LCD Initialization //////////////////////////////////
  	initializeLCD();
  	clearLCD();
	printLCDText("f = 1000 Hz\nA = 5000 mV(P-P)$");
	////////////////////////// LCD Initialization //////////////////////////////////
	
	////////////////////////// UI Initialization ///////////////////////////////////
	initializeKeypad();
	////////////////////////// UI Initialization ///////////////////////////////////
	
	////////////////////////// Hardware Initialization /////////////////////////////
	InitializeDAC();
	////////////////////////// Hardware Initialization /////////////////////////////

  // Setup the initial loopup table, f=1000Hz, A=5000mV
  calculateLookupTable(frequency, amplitude);
  
  while (1) 
  {
    // If PH1 is pressed, change frequency
    if (PTH_PTH1 != 1) 
    {
       clearLCD();
       printLCDText("Enter new freq.:\n$"); 
       // Use the new KEYPAD library to get a full number, with editing/correction possible
       frequency = keypad_getNumber();
       // Limits of generation: 1Hz-20KHz
       if (frequency > 20000)
          frequency = 20000;
       if (frequency < 1)
          frequency = 1;
       // Display new generator status
       clearLCD();
       printLCDText("f = $"); printLCDNumber(frequency); printLCDText(" Hz\n$");
       printLCDText("A = $"); printLCDNumber(amplitude); printLCDText(" mV(P-P)\n$");
       // Update lookup table once; does not need to run multiple times
       calculateLookupTable(frequency, amplitude);
    } 
    else if (PTH_PTH0 != 1) 
    {
       clearLCD();
       printLCDText("Enter new ampl.:\n$"); 
       // Use the new KEYPAD library to get a full number, with editing/correction possible
       amplitude = keypad_getNumber();
       // Limits of generation: 0-5000mV, always with DC offset = 2.5V
       if (amplitude > 5000)
         amplitude = 5000;
       // Display new generator status
       clearLCD();
       printLCDText("f = $"); printLCDNumber(frequency); printLCDText(" Hz\n$");
       printLCDText("A = $"); printLCDNumber(amplitude); printLCDText(" mV(P-P)\n$");
       // Update lookup table once; does not need to run multiple times
       calculateLookupTable(frequency, amplitude);
    }
    
    // Calls the DAC rapidly to set output equal to each step of the sine wave 
    i++; 
    if (i >= lookupTableDepth) 
      i = 0;
    DAC_SetOutputA(lookupTable[i]);
    // If there is added delay (on low frequencies), call delay function as needed
    if (addedDelay > 0)
      delay_us(addedDelay);
  }
}

// Function to calculate the lookup table based on frequency and amplitude
// NOTE: It is poor practice to build in calibration using 'magic numbers' as is done below.
// Normally, I would recommend that this be placed in some easily-modifiable location. In code
// this could be done through defines, but in this case I would even recommend storing the
// settings in EEPROM or on the SDCard (if present)
void calculateLookupTable(unsigned long f, unsigned long a) 
{
  unsigned long i;
  // For higher frequencies, we simply adjust the lookup table size
  if (f >= 313) 
  {
    // Found through calculation, knowing the number of assembly instructions per loop iteration and CPU clock (24 MHz)
    lookupTableDepth = (unsigned int)(150000/f);
    addedDelay = 0;
    for (i=0; i<lookupTableDepth; i++) 
      lookupTable[i] = (unsigned int)(sinf((float)i*2*3.141592f/(float)lookupTableDepth) * (a/2500.0f * 255) + 512);
  } 
  // For lower frequencies, we fix the size and add a variable delay based on frequency
  else 
  {
    lookupTableDepth = 160;
    for (i=0; i<lookupTableDepth; i++) 
      lookupTable[i] = (unsigned int)(sinf((float)i*2*3.141592f/(float)lookupTableDepth) * (a/2500.0f * 255) + 512);
    // Found through testing different values of delay vs. frequency and fitting a curve in Excel
    addedDelay = (unsigned long)(5570.4 * powf(f , -1.12));
  }
}

////////// Start SPI Physical Layer ////////// 

void InitializeSPI() 
{
   // Your code from Part 1 goes here.
}

////////// End SPI Physical Layer ////////// 



////////// Start SPI Protocol Layer ////////// 


// Sends a double byte to the device connected to the SPI bus. The CS line
// should be held high for the entire duration of the send. Could be improved
// by replacing microcontroller-specific names (PTM_PTM6, SPI0DR, etc.) with
// more generic inline functions or aliases in the physical layer.
void SPI_Send(unsigned char data1, unsigned char data2) 
{
  // Your code from Part 2 goes here.
}

////////// End SPI Protocol Layer //////////

 

////////// Start Application Layer ////////// 

// Initializes this specific DAC chip. Due to the minimal initialization needed,
// this could arguably be omitted or simplified.
void InitializeDAC() 
{
  InitializeSPI();
  DAC_SetOutputA(0);
}

// Sends the two data bytes formatted as expected by this specific chip.
void DAC_SetOutputA(unsigned int output) 
{
  // Your code from Part 2 goes here.
}

////////// End Application Layer ////////// 







