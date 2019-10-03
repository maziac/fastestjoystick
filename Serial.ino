#include <usb_desc.h>


// Handles the serial input.
// Via serial-in some digital output pins can be driven, e.g. for LED light of the buttons.
void handleSerialIn() {
  static char input[20];  // o12=100,65535,65535 -> 19
  static char* inpPtr = input;
  static bool skipLine = false;
  
  // Restrict input to no more than 50 characters.
  // Prevent flooding the device.
#if 0
  if(Serial.available() > 50) {
    Serial.clear();
  }
#endif

#if 0
int k = 0;
while(Serial.available()) {
    // Get data 
    char c = Serial.read();
    if(k++>10)
      return;
}
#endif
 
  // Check if data available
  while(Serial.available()) {
    // Get data 
    char c = Serial.read();
    logChar(c);
    
    if(c == '\r') 
      break;   // Skip windows character
      
    if(c == '\n') {
      // End found
      // Check for skip
      if(!skipLine) {
        *inpPtr = 0;
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
        if(serialTxPacketCount() == 0)
          serialPrintln("Error: Line too long.\n");
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
void decodeSerialIn(char* input) {
  // Check for command
  switch(input[0]) {
    
    // Set output
    case 'o':
    {
      // Get output
      int index = input[1] - '0';
      // Check
      if(index < 0 || index >= COUNT_DOUTS) {
        if(serialTxPacketCount() == 0)
          serialPrintln("Error: index\n");
        break;
      }
    
      // Get value [0;100]
      const char* inp = &input[3];
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
      serialPrintln("Resetting\n");
      delay(2000);
#define RESTART_ADDR 0xE000ED0C
#define WRITE_RESTART(val) ((*(volatile uint32_t *)RESTART_ADDR) = (val))
      WRITE_RESTART(0x5FA0004);
    break;
      
    // Test fast blinking
    case 't':
      error("Test");
    break;
      
    // Change minimum press time
    case 'p':
    {
      const char* inp = &input[2];
      MIN_PRESS_TIME = asciiToUint(&inp);
      if(serialTxPacketCount() == 0) {
        serialPrint("Changing press time to ");
        serialPrint(MIN_PRESS_TIME);
        serialPrintln("ms\n");
      }
    }
    break;
    
    // Change minimum press time
    case 'i':
      if(serialTxPacketCount() == 0) {
        serialPrint("Version: " SW_VERSION "\n");
        serialPrint("Min. press time:    ");serialPrint(MIN_PRESS_TIME);serialPrint("ms\n");
        serialPrint("Max. time serial:   ");serialPrint(maxTimeSerial);serialPrint("us\n");
        serialPrint("Max. time joystick: ");serialPrint(maxTimeJoystick);serialPrint("us\n");
        serialPrint("Max. time total:    ");serialPrint(maxTimeTotal);serialPrint("us\n");
        serialPrint("Last error: ");serialPrint(lastError);serialPrintln();
#ifdef ENABLE_LOGGING
        printLog();
#endif
      }
      // Reset times
      maxTimeSerial = 0;
      maxTimeJoystick = 0;
      maxTimeTotal = 0;
    break;

    // Turn debug mode ON/OFF. Debug mode enables serial printing.
    case 'd':
    {
      bool on = (input[2] != 0);
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
     if(serialTxPacketCount() == 0) {
        serialPrint("Error: Unknown command: ");
        serialPrint(input);serialPrintln();
      }
      //Serial.clear();
    break; 
  }   
}


// Returns the number of tx packets in the buffer.
int serialTxPacketCount() {
    return 0;   // Testen, ob ich auf die Funktion verzichten kann.
  return usb_tx_packet_count(CDC_TX_ENDPOINT);
//  return usb_tx_packet_count(SEREMU_TX_ENDPOINT);
}


// Capsulates the printing functions.
// Printing will only be done if 'DEBUG' is on.
void serialPrint(const char* s) {
  if(DEBUG)
    Serial.print(s);
}

void serialPrintln() {
  if(DEBUG)
    Serial.println();
}
