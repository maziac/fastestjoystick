#include <usb_desc.h>


// Handles the serial input.
// Via serial-in some digital output pins can be driven, e.g. for LED light of the buttons.
void handleSerialIn() {
  static char input[20];  // o12=100,65535,65535 -> 19
  static char* inpPtr = input;
  static bool skipLine = false;
  
  // Check if data available
  while(Serial.available()) {
    // Get data 
    char c = Serial.read();
#ifdef ENABLE_LOGGING
    logChar(c);
#endif

    if(c == '\r') 
      break;   // Skip windows character
      
    if(c == '\n') {
      // End found
      // Check for skip
      if(!skipLine) {
        *inpPtr = 0;
        inpPtr = input;
        // Decode input
        decodeSerialIn(input);
      }
      // Reset
      inpPtr = input;
      skipLine = false;
      break;
    }
    else {
      // Remember character
      *inpPtr = c;   
      inpPtr++;
      // Input too long?
      if(inpPtr > (input+sizeof(input)-1)) {
        // Print warning
        if(serialPrintAllowed())
          serialPrint("Error: Line too long.\n");
        // Reset
        inpPtr = input;
        skipLine = true;
        break;    
      }
    }
  }
}

  
// Takes a string from serial in and decodes it.
// Correct strings look like:
// "o7=0" or "o3=80" or "o1=255" with 255=max and 0=off.
// e.g. for setting ouput 7 to 70% brightness with an attack time of 2000ms and a delay of 1000ms.
// On host die (linux or mac) you can use e.g.:
// echo o0=70,2000,1000 > /dev/cu.usbXXXX
// Supported commands are:
// oN=X : Set output N to X (0 or 1)
// r : Reset
// t : Test the blinking
// p=Y : Change minimum press time to Y (in ms), e.g. "p=25"
// i : Info. Print version number and used times.
// d=X: Turn debug mode on/off (1/0). If off serial printing is disabled.
void decodeSerialIn(char* inputStr) {
  // Check for command
  switch(inputStr[0]) {
    
    // Set output
    case 'o':
    {
      // Get output
      int index = inputStr[1] - '0';
      // Check
      if(index < 0 || index >= COUNT_DOUTS) {
        if(serialPrintAllowed())
          serialPrint("Error: index\n");
        break;
      }
    
      // Get value [0;100]
      const char* inp = &inputStr[3];
      int value = asciiToUint(&inp);
    
      // Get attack time
      uint16_t attackTime = 0;
      uint16_t delayTime = 0;
      if(*inp++ == ',') {
        attackTime = asciiToUint(&inp);
        // Get delay time
        if(*inp++ == ',') 
           delayTime = asciiToUint(&inp);
      }
      
      // Set pin
      setDout(index, value, attackTime, delayTime);
    }
    break;    
      
    // Reset
    case 'r':
      serialPrint("Resetting\n");
      delay(2000);
#define RESTART_ADDR 0xE000ED0C
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
      WRITE_RESTART(0x5FA0004);
    break;
      
    // Test fast blinking
    case 't':
      serialPrint("Test");
      serialPrintln();
      fastBlinking();
    break;
      
    // Change minimum press time
    case 'p':
    {
      const char* inp = &inputStr[2];
      MIN_PRESS_TIME = asciiToUint(&inp);
      if(serialPrintAllowed()) {
        serialPrint("Changing press time to ");
        serialPrint(MIN_PRESS_TIME);
        serialPrint("ms\n");
      }
    }
    break;
    
    // Change minimum press time
    case 'i':
    {
      bool prevDbg = DEBUG;
      DEBUG = true;
      if(serialPrintAllowed()) {
        serialPrint("Version: " SW_VERSION "\n");
        serialPrint("Min. press time:    ");serialPrint(MIN_PRESS_TIME);serialPrint("ms\n");
        serialPrint("Max. time serial:   ");serialPrint(maxTimeSerial);serialPrint("us\n");
        serialPrint("Max. time joystick: ");serialPrint(maxTimeJoystick);serialPrint("us\n");
        serialPrint("Max. time total:    ");serialPrint(maxTimeTotal);serialPrint("us\n");
        serialPrint("Last error: ");serialPrint(lastError);serialPrintln();
        serialPrint("Debug Mode: ");
        serialPrint(prevDbg ? "ON" : "OFF");
        serialPrintln();
#ifdef ENABLE_LOGGING
        printLog();
#endif
      }
      DEBUG = prevDbg;
      // Reset times
      maxTimeSerial = 0;
      maxTimeJoystick = 0;
      maxTimeTotal = 0;
    }
    break;

    // Turn debug mode ON/OFF. Debug mode enables serial printing.
    case 'd':
    {
      bool on = (inputStr[2] != '0');
      DEBUG = true;
      serialPrint("Debug Mode=");
      serialPrint(on ? "ON" : "OFF");
      serialPrintln();
      DEBUG = on;
    }
    break;

    // Just a newline
    case 0:
    break;
      
    // Unknown command
    default:
     if(serialPrintAllowed()) {
        serialPrint("Error: Unknown command: ");
        serialPrint(inputStr);
        serialPrintln();
      }
      //Serial.clear();
    break; 
  }   
}


// Returns true if printing is allowed.
// Used to put several prints in a block.
inline bool serialPrintAllowed() {
    return DEBUG;
//  return usb_tx_packet_count(CDC_TX_ENDPOINT) == 0;
//  return usb_tx_packet_count(SEREMU_TX_ENDPOINT) == 0;
}


// Capsulates the printing functions.
// Printing will only be done if 'DEBUG' is on.
void serialPrint(const char* s) {
  if(DEBUG)
    Serial.print(s);
}

void serialPrint(char c) {
  if(DEBUG)
    Serial.print(c);
}

void serialPrint(uint8_t v) {
  if(DEBUG)
    Serial.print(v);
}

void serialPrint(uint16_t v) {
  if(DEBUG)
    Serial.print(v);
}

void serialPrint(int v) {
  if(DEBUG)
    Serial.print(v);
}

void serialPrint(int8_t v) {
  if(DEBUG)
    Serial.print(v);
}

void serialPrint(int16_t v) {
  if(DEBUG)
    Serial.print(v);
}

void serialPrint(bool b) {
  if(DEBUG)
    Serial.print(b);
}

void serialPrintln() {
  if(DEBUG)
    Serial.println();
}
