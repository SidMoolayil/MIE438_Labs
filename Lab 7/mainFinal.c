/**********************************************************
* MIE 438 - Microprocessors and Embedded Microcontrollers *
*---------------------------------------------------------*
* File:    main.c                                         *
* Author:  Matthew Mackay                                 *
*---------------------------------------------------------*
* Created on:          Mar.  8, 2015                      *
* Last modified on:    Mar.  10, 2015                      *
**********************************************************/

#include <hidef.h>          /* common defines and macros */
#include "derivative.h"     /* derivative-specific definitions */
#include "advancedLCD.h"

// States for keys pressed on keypad
#define KEY_UP		0
#define KEY_PRESS	1

// General program state - typing message or typing 'to'
#define STATE_MSG	0
#define STATE_TO	1

// Change this to a UNIQUE HEX number ('0'-'F')
#define	MACHINE_ID	'1'

// Scan codes used to check for keypad key presses
const char scanCode[4] = {0xF8, 0xF4, 0xF2, 0xF1};

// If you want the keypad keys to be detected by the default lettering, use the following line in your own programs:
//char keypadTable[16] = {'A','B','C','D', '3','6','9','#', '2','5','8','0', '1','4','7','*'};

// For this program, we re-map the 'A', 'B','C','D', etc. keys to unique functions - hence the different codes below
const unsigned char keypadTable[16] = {0x01,0x02,0x03,0x04, 'G','P','C',' ', 'D','M','W','0', 'A','J','T','*'};

// Storage for the message we are typing on this unit
unsigned char messageBuffer[LCD_WIDTH-7];
unsigned char messageLength;
unsigned char messageRecipient;
unsigned char messageSender;


// Current program state
int state;

// Pin mapping for input and output
#define RING_OUT_SCL		PORTB_BIT0
#define RING_OUT_SDA	  PORTB_BIT1
#define RING_OUT_ACK		PORTB_BIT2

#define RING_IN_SCL	    PTT_PTT0
#define RING_IN_SDA	    PTT_PTT1
#define RING_IN_ACK	    PTT_PTT2

// Return results for sendChar() function
#define SENDCHAR_SUCCESS  0
#define SENDCHAR_FAILURE  -1

// Timeouts for communication protocol - can fine-time to make this *much* faster
#define TIMEOUT_SENDCHAR      60000
#define TIMEOUT_RECIEVE       60000
#define TIMEOUT_SENDMESSAGE   20

// Function prototypes - User Interface
unsigned char scanKeypad(void);
void displayRecieverError(int message);

// Function prototypes - High Level Communications
void sendMessage(unsigned char len, unsigned char rec,unsigned sender, unsigned char *buf);
void receiveMessage(void);

// Function prototypes - Protocol Layer Communications
int sendChar(unsigned char m);
unsigned char recieveChar(void);

// Function prototypes - I/O Layer Communications
void sendI2CStart(void);
void sendI2CStop(void); 

void main()
{
  	// Initially, we assume no keys are pressed
  	int keyState = KEY_UP, keyPressed = 0, oldKeyPressed = 0, multiInd = 0;
  	
  	// We record the last state for SDA and SCL to detect the transition that indicates an I2C Start Condition
  	int lastSDA, lastSCL;
  	unsigned char k;

    // Various state variables tracking the typed message's state, length, and recipient  	
  	messageRecipient = '0';
  	messageLength = 0;
  	
  	state = STATE_MSG;

    /****** Port Assignment:  ******/
    // PB0 - Us     SCL (output)
    // PT0 - Them   SCL (input)
    
    // PB1 - Us     SDA (output)
    // PT1 - Them   SDA (input)
    
    // PB2 - Us     ACK (input)
    // PT2 - Them   ACK (output)
    /****** Port Assignment:  ******/
    
    
    /****** PORT Initilization ******/
    PEAR = 0x10;
   
    // RING_OUT Pins
    DDRB = 0b00000011;    //PB0, PB1 output, PB2 input
    PORTB = 0x00;
    
    // RING_IN Pins
    DDRT = 0b00000100;    //PT0, PT1 input, PT2 output     
    PTT = 0x00;

    // Keypad
    DDRA = 0x0F;
    PORTA = 0xFF;

    // LCD Screen    
    DDRK = 0xFF;
    /****** PORT Initilization ******/
    
    
  	// Initialize all lines to expected values: I2C output has SDA = SCL = 1, I2C input has ACK = 1
  	RING_OUT_SCL = 1;
  	RING_OUT_SDA = 1;
  	lastSCL = 1;
  	lastSDA = 1;
  	RING_IN_ACK = 1;
  	
  	// Start up and clear the LCD
  	initializeLCD();
  	clearLCD();
  	
  	// Default screen format: top line is message/recipient, bottom line is received
  	printLCDText("M: $");
  	moveLCDTo(LCD_WIDTH-3,0);
  	printLCDText("R:$");
  	moveLCDTo(3,0);
  	
  	// Main program loop
  	do
  	{
  		// First, scan for keypad - k is zero if nothing pressed, returns an ASCII code otherwise
  		k = scanKeypad();
  		
  		// Check to see if an I2C start condition has been generated on RING_IN: SCL=1, SDA 0->1
  		if (RING_IN_SDA == 0 && lastSDA == 1 && RING_IN_SCL == 1 && lastSCL == 1)
  		{
  		  // If so, we have a message waiting - recieve it. Also, we record the SDA/SCL state to avoid an
  		  // accidental extra 'recieveMessage()' call when we return (since some time will have passed)
  		  lastSDA = RING_IN_SDA;
  		  lastSCL = RING_IN_SCL;
  			receiveMessage();
  			keyState = KEY_UP;
  		} 
  		else 
  		{
  		  lastSDA = RING_IN_SDA;
  		  lastSCL = RING_IN_SCL;
  		}
  		
  		// If we detect a key has *just been* pressed, record it - we are then waiting for key raise
  		if (keyState == KEY_UP && k != 0)
  		{
  			keyState = KEY_PRESS;
  			keyPressed = k;
  		}
  		// If we detect a key was pressed, but has just been raised, record ending...
  		else if (keyState == KEY_PRESS && k == 0)
  		{
  			keyState = KEY_UP;
  		}
  		
  		// ... and then process the complete (key down+up) keystroke
  		if (keyState == KEY_UP && keyPressed != 0)
  		{
  			// If we press the same key multiple times, cycle through multiple characters for that key (ex. A->B->C->A...)
  	 		if (keyPressed == oldKeyPressed && keyPressed > 0x04)
  			{
  				// If we are typing a message, replace old character in place
  				if (state == STATE_MSG)
  				{
  					moveLCDBack(1);
  					messageLength--;
  				}
  				
  				// Update the index; multiInd contains offset for choice from 3 characters - keys P and 0 are unique (PQRS for 'P' and 0123456789 for '0')
  				multiInd++;
  				if ((multiInd == 3 && keyPressed != '0' && keyPressed != 'P') || (multiInd == 4 && keyPressed == 'P') || (multiInd == 10 && keyPressed == '0'))
  					multiInd = 0;
  			}
  			// If a new key was pressed, reset multiInd
  			else
  				multiInd = 0;

  			// If we pressed the 'D' button, switch states (Message->Recipient->Send->Message)
  			if (keyPressed == 0x04)
  			{
  				// Move from 'Type Message' state to 'Type Recipient' state
  				if (state == STATE_MSG)
  				{
  					moveLCDTo(LCD_WIDTH-1,0);
  					state = STATE_TO;
  				}
  				// Move from 'Type Recipient' state to send, and back to 'Type Message' state
  				else
  				{
  					// Send the old message
  					sendMessage(messageLength, messageRecipient,messageSender, messageBuffer);
  					clearLCD();
  					// And clear everything to make room for new message
  					printLCDText("M: $");
  					moveLCDTo(LCD_WIDTH-3,0);
  					printLCDText("R:$");
  					moveLCDTo(3,0);
  					state = STATE_MSG;
  					messageLength = 0;
  				}
  			}
  			// If we pressed the 'A' button, this effectively just ends the current key cycling (A->B->C->A...) and starts a new character
  			else if (keyPressed == 0x01)
  			{
  			
  			}
  			// If we pressed 'B', this is backspace -> move backward in message once, if possible
  			else if (keyPressed == 0x02 && state == STATE_MSG)
  			{
  				if (messageLength > 0)
  				{
  					messageLength--;
  					moveLCDBack(1);
  					printLCDText(" $");
  					moveLCDBack(1);
  				}
  			}
  			// If we pressed 'C', do nothing, would also end the character
  			else if (keyPressed == 0x03)
  			{
  			
  			}
  			// If any other key, then just add it to the message field (if typing message state)....
  			else if (state == STATE_MSG && messageLength < LCD_WIDTH-7)
  			{
  				messageBuffer[messageLength] = keyPressed+multiInd;
  				printLCDChar(keyPressed+multiInd);
  				messageLength++;
  			}
  			// ... or in the recipient field, if typing recipient state.
  			else if (state == STATE_TO)
  			{
  				int kpt = keyPressed+multiInd;
  				// Only allow single character, HEX
  				if ((kpt >= '0' && kpt <= '9') || (kpt >= 'A' && kpt <= 'F'))
  				{
  					messageRecipient = keyPressed+multiInd;
  					messageSender = MACHINE_ID;
  					moveLCDTo(LCD_WIDTH-1,0);
  					printLCDChar(keyPressed+multiInd);
  				}
  			}
  			// Record the last key pressed
  			oldKeyPressed = keyPressed;			
  			keyPressed = 0;
  		}
  	}
  	while (1);	
}

// Function which scans the keypad and returns keycode of any pressed key, zero if no key pressed
unsigned char scanKeypad()
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

// Sends a message of length 'len' to recipient 'rec,' stored in buffer 'buf'
// This function sits at the HIGH LEVEL communications layer; it defines the message format,
// but contains no specifics about any of the low-level implementation. The 'start', 'stop',
// and 'sendChar' functions could be implemented using any communication standard, not just
// I2C as seen below.
void sendMessage(unsigned char len, unsigned char rec, unsigned char sender, unsigned char *buf)
{
	int i, result, timeout = 0;
	unsigned char retry = 0;
  
  do 
  {
    // All communications require a starting condition, handshake, or header - we call a specific function for I2C, creating the I2C START CONDITION
    sendI2CStart();

  	// Our message format is [Start] [Length - 1 byte] [Recipient - 1 byte] [Message - n bytes] [Stop]
  	result = sendChar(len);
  	if (result != SENDCHAR_SUCCESS)
  	  retry = 1;
  	
  	result = sendChar(rec);
  	if (result != SENDCHAR_SUCCESS)
  	  retry = 1;
  	
  	result = sendChar(sender);
  	if (result != SENDCHAR_SUCCESS)
  	  retry = 1;
  	
  	for (i=0; i<len; i++) 
  	{
  		result = sendChar(buf[i]);
  		if (result != SENDCHAR_SUCCESS)
  	    retry = 1;
  	}
  		
    sendI2CStop();
    
    // For each character, we check if it sent correctly; if not, result is '1' and we would retry the complete message.
    // We also maintain a limit (timeout) so we do not get stuck waiting forever.
    timeout++;
  }
  while (retry == 1 && timeout < TIMEOUT_SENDMESSAGE); 
}

// Receives a complete message, assuming the I2C Start Condition was detected
// This function also sits at the HIGH LEVEL communications layer. It recieves a message using generic functions only, with
// the assumption that the start condition for our communication medium has been detected prior to calling. 
void receiveMessage()
{
  int i;
  unsigned char len, recp, sender;
  unsigned char buf[LCD_WIDTH-6];

  // According to the prior format, we recieve the [Length] and [Recipient] fields first  
  len = recieveChar();
  recp = recieveChar();
  sender = recieveChar();
  
  if (sender == MACHINE_ID && recp!=MACHINE_ID)
	{
		RING_OUT_SCL = 1;
    RING_OUT_SDA = 1;
    RING_IN_ACK = 1;
    return 1;
	}
  
  // We discard any message that is too long. Under correct communication these should not be sent, but always sanitize data
  // coming in from non-controlled sources. Anything sent over a communication medium should be bound-checked.
  if (len > (LCD_WIDTH-6)) 
  {
    clearLCD();
    printLCDText("Invalid Message:\n$");
    printLCDText("Too long [$");
    printLCDNumber(len);
    printLCDText("]$");  
  } 
  else 
  {
    // Recieve the message into buffer
    for (i=0; i<len; i++)
  		buf[i] = recieveChar();
  	buf[i] = '$';
  }
  
  // Send the I2C STOP CONDITION, and also wait for some time - this is not strictly necessary, but we added it to ensure there
  // are minimal timing problems.	
	sendI2CStop();
	shortWait(10);
	
	
	
	 //If this message is for us, display it on bottom LCD line then discard it (return cursor to typing position too)
	if (recp == MACHINE_ID)
	{
		
		moveLCDTo(0,1);
		printLCDText("Recv:           $");
		moveLCDTo(6,1);
		printLCDText(buf);
		if (state == STATE_MSG)
			moveLCDTo(3+messageLength,0);
		else
			moveLCDTo(LCD_WIDTH-1,0);
	}
	else if (len > 0)
	{
		// Otherwise, send it along to the next device in chain. 
		
  	// NOTE: For better responsiveness in large chains, it would be a good idea to modify this code to guarantee the keypad
  	// to be scanned once between receiving and re-sending, otherwise in a large chain, your device will spend much of its time
  	// resending messages, blocking off UI time.
		moveLCDTo(0,1);
		printLCDText("Passing message$");

    // We have added extra code here to force the low-level comm. medium to be completely reset, as we were having problems with
    // some boards being unreliable due to noise. This type of low-level code would (under best practices) not be found directly
    // in a high-level layer function such as this one. Unfortunately, we have no choice but to include it to guarantee correct
    // operation for these particular boards.
		shortWait(10);
		RING_OUT_SCL = 1;
    RING_OUT_SDA = 1;
    RING_IN_ACK = 1;
		sendMessage(len, recp, sender, buf);
		shortWait(10);
		RING_OUT_SCL = 1;
    RING_OUT_SDA = 1;
    RING_IN_ACK = 1;
		
		printLCDText("                $");
		if (state == STATE_MSG)
			moveLCDTo(3+messageLength,0);
		else
			moveLCDTo(LCD_WIDTH-1,0);
	}
	
	RING_OUT_SCL = 1;
  RING_OUT_SDA = 1;
  	
  RING_IN_ACK = 1;
}


// Sends a single character, assuming a communication message has already been set up (I2C Start Condition sent).
// This is a function in the PROTOCOL LAYER; it implements functionality specific to I2C - the minimum functionality being
// to send one character.
int sendChar(unsigned char m)
{
	int j;
	unsigned char b;
	unsigned int timeout;
	
	// Send 8 bits from character 'm'
	for (j=0; j<8; j++)
	{
		// Take MSB first from 'm' using logical AND
		b = m & 0x80;
		
		// Shift ahead for next bit
		m = m << 1;
		
		// Under I2C specification, data should change when SCL line is low, and remain stable when high. We set SCL 1->0, and
		// change the SDA line to the current bit. Strictly speaking, a small delay, likely less than shortWait(1), could be
		// added between SCL 1->0 and SDA = [bit]. But, we know that data is sampled on the rising edge, so it is unlikely
		// to be an issue.
		RING_OUT_SCL = 0;
		if (b == 0)
		  RING_OUT_SDA = 0;
		else
		  RING_OUT_SDA = 1;
		shortWait(1);

    // After a standard communications delay, we set SCL 0->1 to complete the clock pulse.
		RING_OUT_SCL = 1;
		shortWait(1);
	}
	
	// We are now waiting for the handshake 'ACK' from the recieving device. A premature ACK = 0 indicates that we are out
	// of synchronization, and should start over.
	if (RING_OUT_ACK == 0)
	  return SENDCHAR_FAILURE;
	RING_OUT_SCL = 0;   
	shortWait(1);

  // We wait now for ACK 1->0, indicating acknowledgement from the recieving device. The wait is bounded by a timeout to
  // prevent a deadlock situation if communication fails.	
	timeout = 0;
	while (RING_OUT_ACK == 1 && timeout < TIMEOUT_SENDCHAR) { timeout++; }     
	if (timeout == TIMEOUT_SENDCHAR)
    return SENDCHAR_FAILURE;
	
	// As soon as ACK = 0 is detected, we set SCL 0->1, completing the final clock pulse and the complete byte of data.
	RING_OUT_SCL = 1;
	shortWait(1);
	
  return SENDCHAR_SUCCESS;  	
}

// Receives a single character, assuming a communication is in progress (I2C Start Condition recieved).
// This is a function in the PROTOCOL LAYER; it implements functionality specific to I2C - the minimum functionality being
// to recieve one character.
unsigned char recieveChar()
{
	int i;
	unsigned char r = 0;
  unsigned long timeout;	
  
  // Start Condition: SCL = 1 (stable), SDA = 1->0
  RING_IN_ACK = 1;
  
  for (i=0; i<8; i++) 
  {
    
    // EVENT1: When SCL 1->0, this indicates is data changing on sender's side. We wait for this event first.
    timeout = 0;
  	while (RING_IN_SCL == 1 && timeout < TIMEOUT_RECIEVE) { timeout++; }   
  	if (timeout == TIMEOUT_RECIEVE) 
  	{
      displayRecieverError(0); // We have added error messages here to better diagnose where a timing issue occurs 
      return 0; 
  	}
  	
  	// EVENT2: Data is changing while SCL = 0 (stable); we wait for SCL 0->1 before starting to read each bit
    timeout = 0;
  	while (RING_IN_SCL == 0 && timeout < TIMEOUT_RECIEVE) { timeout++; }   
  	if (timeout == TIMEOUT_RECIEVE) 
  	{
      displayRecieverError(1); 
      return 0; 
  	}
  	
  	// Capture the bit, shift (r) to next bit position (recall: MSB sent first)
  	if (RING_IN_SDA != 0)
  	  r = r | 0b00000001;
  	else
  	  r = r & 0b11111110;
  	
  	// We do not shift on last bit recieved
  	if (i != 7)
  	  r = r << 1;
  }
  	
  // EVENT3: After 8 bits, we wait for final transition of SCL 1->0..
  timeout = 0;
	while (RING_IN_SCL == 1 && timeout < TIMEOUT_RECIEVE) { timeout++; }   
	if (timeout == TIMEOUT_RECIEVE) 
	{
    displayRecieverError(2); 
    return 0; 
	}
	
  // ...this indicates it is time for *us* to send ACK as 9th bit
  RING_IN_ACK = 0;
  shortWait(1);
  
  // EVENT4: Now, we wait for last pulse of SCK: first SCK 0->1, then SCK 1->0
  timeout = 0;
	while (RING_IN_SCL == 0 && timeout < TIMEOUT_RECIEVE) { timeout++; }   
	if (timeout == TIMEOUT_RECIEVE) 
	{
    displayRecieverError(3); 
    return 0; 
	}
	
	timeout = 0;
	while (RING_IN_SCL == 1 && timeout < (TIMEOUT_RECIEVE+100000)) { timeout++; }   
	if (timeout == TIMEOUT_RECIEVE) 
	{
    displayRecieverError(4); 
    return 0; 
	}
	
	// Once the final pulse is seen, we remove set ACK = 1 and are ready for a new transmission
	RING_IN_ACK = 1;
  
  
	return r;
}

// The following are functions from the I/O or PHYSICAL LAYER. They implement the most basic functionality required to 
// generate different messages or signals on the physical communications medium. In this case, we have two functions
// to create the I2C Start and Stop conditions. For best practice, one could also create functions for bounded waits
// on SCL 0->1 and 1->0 here. If possible, timing-related code should be abstracted here as well.

void sendI2CStart() 
{
  RING_OUT_SCL = 1;
  shortWait(1);
  RING_OUT_SDA = 0;  
  shortWait(1);
}

void sendI2CStop() 
{
  RING_OUT_SCL = 1;
  shortWait(1);
  RING_OUT_SDA = 1;
  shortWait(1);
}

// This is a short function to display informative error messages in case of a communication protocol problem.
void displayRecieverError(int message) 
{
  sendI2CStop();
  
  switch (message) 
  {
    case 0:
      moveLCDTo(0,1);
      printLCDText("SCL1->0, EVENT1 $");
      break;
    case 1:
      moveLCDTo(0,1);
      printLCDText("SCL0->1, EVENT2 $");
      break;
    case 2:
      moveLCDTo(0,1);
      printLCDText("SCL1->0, EVENT3 $");
      break;
    case 3:
      moveLCDTo(0,1);      
      printLCDText("SCL0->1, EVENT4 $");
      break;
    case 4:
      moveLCDTo(0,1);
      printLCDText("SCL1->0, EVENT4 $");
      break;      
  }
  
  moveLCDTo(0,0);
}


