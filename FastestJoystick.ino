/* 
USB Joystick using a Teensy LC board.
The features are:
- Requested 1ms USB poll time
- Additional delay of max. 200us
- Indication of the real used USB poll time
*/

#include <Arduino.h>

#include <usb_desc.h>
#include <usb_dev.h>


// *** CONFIGURATION BEGIN ************************************************
// Button 0-12 = Pin 0-12
// Axis left/right: Pin 20/21 AU/AD
// Axis down/up: Pin 22/23 AR/AL
// DOUT 0-3 = Pin 16-19

// Buttons start at this digital pin
#define BUTTONS_PIN_OFFS 0    // Digital pin 0 is button 0

// Number of total joystick buttons.
#define COUNT_BUTTONS   13    // Pins 0-12

// Axes start at this digital pin
#define AXES_PIN_OFFS 20    // Pins 20-23

// Number of different joystick axes. At maximum 2 axes are supported.
#define COUNT_AXES   2

// Digital outputs start at this pin
#define DOUTS_PIN_OFFS  16  // Pins 16-19

// Number of digital outs
#define COUNT_DOUTS  4

// Turn main LED on/off (Note: the built-in LED is pin 13).
// Comment if you don't need this.
#define MAIN_LED_PIN   LED_BUILTIN

// Turn digital poll out pin (14) on/off.
// Comment if you don't need this.
#define USB_POLL_OUT_PIN  14


// The debounce and minimal press- and release time of a button (in ms).
uint16_t MIN_PRESS_TIME = 28;

// *** CONFIGURATION END **************************************************


// The SW version.
#define SW_VERSION "0.6"


// ASSERT macro
#define ASSERT(cond)  {if(!(cond)) error("LINE " TOSTRING(__LINE__) ": ASSERT(" #cond ")");}
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)



// Timer values for all joystick buttons
uint16_t buttonTimers[COUNT_BUTTONS] = {};

// The (previous) joystick button values.
//bool prevButtonValue[COUNT_BUTTONS];
// The (previous) joystick button values. Bit map. 1=pressed.
uint32_t prevButtonsValue = 0;  

// Timer values for the axes.
uint16_t axesTimers[COUNT_AXES] = {};

// The (previous) joystick button values.
// Values: 0=left/down, 512=no direction, 1023=right/up
int prevAxesValue[COUNT_AXES];

// Either button for low (0-512) was the last activity: 0.
// Or button for high (512-1023) was the last activity: 1.
int lastAxesActivity[COUNT_AXES] = {};


// Variables to measure the maximum timings.
uint16_t maxTimeSerial = 0;
uint16_t maxTimeJoystick = 0;
uint16_t maxTimeTotal = 0;

// Holds the last error. Used when info is printed.
char lastError[100] = "None";


// We should never get here. But if we do the assumption that the joystick + sampling delay is handled within 
// 1ms (i.e. 900us) is wrong.
// Or some other error occured.
// This is indicated by fast blinking of the LED and also all digital outs will blink.
// The routine will never exit.
void error(const char* text) {
  // Save error
  strcpy(lastError, text);
  // Print text to serial
  Serial.println(text);
  // Endless loop
  while(true) {
#ifdef MAIN_LED_PIN
    digitalWrite(MAIN_LED_PIN, false);
#endif
    for(int i=0; i<COUNT_DOUTS; i++) {
      pinMode(DOUTS_PIN_OFFS+i, false);
    }
    delay(150);
#ifdef MAIN_LED_PIN
    digitalWrite(MAIN_LED_PIN, true);
#endif
    for(int i=0; i<COUNT_DOUTS; i++) {
      pinMode(DOUTS_PIN_OFFS+i, true);
    }
    delay(50);
    
    // Handle serial in
    handleSerialIn();
  }
}


// Handles the Output and LED to indicate the USB polling rate.
void indicateUsbPollRate() {
#ifdef USB_POLL_OUT_PIN
    // USB_POLL_OUT:
    static bool usbPollOut = false;
    usbPollOut = !usbPollOut;
    digitalWrite(USB_POLL_OUT_PIN, usbPollOut);
#endif

#ifdef LED_MAIN_PIN
    // Main LED (poll time / 1000)
    static bool mainLedOut = false;
    static int mainLedCounter = 0;
    mainLedCounter--;
    if(mainLedCounter < 0) {
      // toggle main LED
      mainLedOut = !mainLedOut;
      digitalWrite(LED_MAIN_PIN, mainLedOut);
      mainLedCounter = 1000;
    }
#endif
}


// Reads the joystick buttons.
// Handles debouncing and minimum press time.
void handleButtons() {
  // Go through all buttons
  uint32_t mask = 0b00000001;
  for(int i=0; i<COUNT_BUTTONS; i++) {
    
    // Check if button timer is 0
    if(buttonTimers[i] > JOYSTICK_INTERVAL) {
      // Decrease timer
      buttonTimers[i] -= JOYSTICK_INTERVAL;
    }
    else {
      // Timer is zero (i.e. below JOYSTICK_INTERVAL). Now check if the button state has changed.
      uint32_t buttonValue = (digitalRead(BUTTONS_PIN_OFFS+i) == LOW) ? 1 : 0; // Active LOW
      buttonValue <<= i;
      if(buttonValue != (prevButtonsValue & mask)) {
        // Button value has changed
        prevButtonsValue &= ~mask;  // Clear bit
        prevButtonsValue |= buttonValue;  // Set bit (if set)
        
        // Restart timer
        buttonTimers[i] = MIN_PRESS_TIME;
      }
    }
    
    // Next
    mask <<= 1;
  }
  // Set buttons in USB data:
  usb_joystick_data[0] = prevButtonsValue; 
}


// Reads the joystick axis.
// Handles debouncing and minimum press time.
// Note: This algorithm needs to be modified if COUNT_AXES is bigger than 2.
void handleAxes() {
  int shift = 4;
  uint32_t mask = ~0xFFFFC00F;
  //int shift = 4+((COUNT_AXES-1)*10);;
  //uint32_t mask = (~0xFFFFC00F) << ((COUNT_AXES-1)*10);
  // Go through all axes
  for(int i=0; i<COUNT_AXES; i++) {
    // Read buttons for the axis of the joystick
    int axisLow = (digitalRead(AXES_PIN_OFFS+(i<<1)) == LOW) ? 0 : 512; // Active LOW
    int axisHigh = (digitalRead(AXES_PIN_OFFS+(i<<1)+1) == LOW) ? 1023 : 512; // Active LOW
    
    // Check if axis timer is 0
    if(axesTimers[i] > JOYSTICK_INTERVAL) {
      // Decrease timer
      axesTimers[i] -= JOYSTICK_INTERVAL;
      // Check if other direction is pressed
      if(lastAxesActivity[i]) {
        // high (512-1023). Check if other button is pressed.
        if(axisLow != 512)
          goto L_JOY_AXIS_MOVED;
      }
      else  {
        // low (0-512). Check if other button is pressed.
        if(axisHigh != 512)
          goto L_JOY_AXIS_MOVED;
      }
    }
    else {
      // Timer is zero (i.e. below JOYSTICK_INTERVAL). Now check if the button state has changed.
L_JOY_AXIS_MOVED:
      int axisValue = 512;
      if(axisLow != 512) {
        lastAxesActivity[i] = 0;
        axisValue = axisLow;
      }
      if(axisHigh != 512) {
        lastAxesActivity[i] = 1;
        axisValue = axisHigh;
      }

      // Check for change
      if(axisValue != prevAxesValue[i]) {
        // Button value has changed
        prevAxesValue[i] = axisValue;
        // Restart timer
        axesTimers[i] = MIN_PRESS_TIME;
    
        // Handle axes movement
        usb_joystick_data[1] = (usb_joystick_data[1] & (~mask)) | (axisValue << shift);
      }
    }

    // Next
    shift += 10;
    mask <<= 10;
    //shift -= 10;
    //mask >>= 10;
  }
}


// Reads the joystick buttons and axis.
// Handles debouncing and minimum press time.
void handleJoystick() {
  // Buttons
  handleButtons();
  // Axes
  handleAxes();
}


// Converts an ascii string into a number.
uint16_t AsciiToUint(const char* s) {
  uint16_t value = 0;
  while(char c = *s++) {
    value = 10*value + c-'0';
  }
  return value;
}


// Takes a string from serial in and decodes it.
// Correct strings look like:
// "o7=1" or "o3=0"
// e.g. for setting ouput 7 to HIGH and output 3 to LOW.
// On host die (linux or mac) you can use e.g.:
// echo o0=1 > /dev/cu.usbXXXX
// Supported commands are:
// oN=X : Set output N to X (0 or 1)
// r : Reset
// t : Test the blinking
// p=Y : Change minimum press time to Y (in ms), e.g. "p=25"
// i : Info. Print version number and used times.
void decodeSerialIn(char* input) {
  // Check for command
  switch(input[0]) {
    
    // Set output
    case 'o':
    {
      // Get output
      int pin = input[1] - '0';
      // Check
      if(pin < 0 || pin >= COUNT_DOUTS) {
        if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0)
          Serial.println("Error: pin");
        return;
      }
    
      // Get value
      int value = input[3] - '0';
      // Check
      if(value < 0 || value > 1) {
        if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0)
          Serial.println("Error: value");
        return;
      }
    
      // Set pin
      digitalWrite(DOUTS_PIN_OFFS+pin, value);
      if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0) {
        Serial.print("DOUT");
        Serial.print(pin);
        Serial.print(" (Pin=");
        Serial.print(pin);
        Serial.print(") set to ");
        Serial.println(value);
      }
    }
    break;    
      
    // Reset
    case 'r':
     Serial.println("Resetting");
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
      MIN_PRESS_TIME = AsciiToUint(&input[2]);
      if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0) {
        Serial.print("Changing press time to ");
        Serial.print(MIN_PRESS_TIME);
        Serial.println("ms");
      }
    }
    break;
    
    // Change minimum press time
    case 'i':
     if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0) {
        Serial.println("Version: " SW_VERSION);
        Serial.print("Min. press time:    ");Serial.print(MIN_PRESS_TIME);Serial.println("ms");
        Serial.print("Max. time serial:   ");Serial.print(maxTimeSerial);Serial.println("us");
        Serial.print("Max. time joystick: ");Serial.print(maxTimeJoystick);Serial.println("us");
        Serial.print("Max. time total:    ");Serial.print(maxTimeTotal);Serial.println("us");
        Serial.print("Last error: ");Serial.println(lastError);
      }
      // Reset times
      maxTimeSerial = 0;
      maxTimeJoystick = 0;
      maxTimeTotal = 0;
    break;

    // Unknown command
    default:
     if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0) {
        Serial.print("Error: Unknown command: ");
        Serial.println(input);
      }
    break; 
  }   
}


// Handles the serial input.
// Via serial in some digital output pins can be driven, e.g. for LED light of the buttons.
void handleSerialIn() {
  static char input[10];
  static char* inpPtr = input;

  // Restrict input to now more than about 10 characters.
  // Prevent flooding the device.
  if(Serial.available() > (int)sizeof(input)) {
    Serial.clear();
  }
  
  // Check if data available
  while(Serial.available()) {
    // Get data 
    char c = Serial.read();
    
    if(c == '\r') 
      break;   // Skip windows character
      
    if(c == '\n') {
      // End found
      *inpPtr = 0;
      // Decode input
      decodeSerialIn(input);
      // Reset
      inpPtr = input;
      break;
    }
    else {
      // Remember character
      *inpPtr = c;   
      inpPtr++;
      // Input too long?
      if(inpPtr > (input+sizeof(input)-1)) {
        // Reset
        inpPtr = input;
        break;    
      }
    }
  }
}

  
// SETUP
void setup() {
  // Disable interrupts
  noInterrupts();
  
  // Initialize pins
#ifdef MAIN_LED_PIN
  pinMode(MAIN_LED_PIN, OUTPUT);
#endif
#ifdef USB_POLL_OUT_PIN
  pinMode(USB_POLL_OUT_PIN, OUTPUT);
#endif

  // Initialize buttons, axes and digital outs
  for(int i=0; i<COUNT_BUTTONS; i++) {
    pinMode(BUTTONS_PIN_OFFS+i, INPUT_PULLUP);
  }
  for(int i=0; i<2*COUNT_AXES; i++) {
    pinMode(AXES_PIN_OFFS+i, INPUT_PULLUP);
  }
  for(int i=0; i<COUNT_DOUTS; i++) {
    pinMode(DOUTS_PIN_OFFS+i, OUTPUT);
  }

    // Setup timer
  MCG_C1 &= ~0b010; // Disable IRCLK
  MCG_SC &= ~0b01110; // Divider = 1 => 4MHz
  MCG_C2 |= 0b001;  // IRCS. fast internal reference clock enabled
  MCG_C1 |= 0b010;  // IRCLKEN = enabled
  
  // Clock source
  SIM_SOPT2 |= 0b00000011000000000000000000000000;  // MCGIRCLK

  // Prescaler: 4 -> 4MHz/4 = 1MHz => 1us
  while (0 != (TPM0_SC & 0b00001000))
    TPM0_SC = 0;  // spin wait 
  TPM0_SC |= 0b00000010;  // Prescaler /4 = 1Mhz
  TPM0_SC |= 0b00001000;  // Enable timer

  // When this is reached the overflow flag is set. (In us).
  TPM0_MOD = 900; // 1000us = 1ms
}




// MAIN LOOP
void loop() {
  // Make sure that no USB packet is in the queue
  while(usb_tx_packet_count(JOYSTICK_ENDPOINT) > 0);


  // Endless loop
  while(true) {
 
    // There should be exact no packet in the queue.
    ASSERT(usb_tx_packet_count(JOYSTICK_ENDPOINT) == 0);
  
    // Prepare USB packet (note: this should immediately return as the packet queue is empty at this point.
    usb_joystick_send();
  
    // Wait on USB poll
    while(usb_tx_packet_count(JOYSTICK_ENDPOINT) > 0);
 
    // Restart timer 0  
    TPM0_CNT = 0; 
    // Clear overflow
    while(TPM0_SC & FTM_SC_TOF)
      TPM0_SC |= FTM_SC_TOF; // spin wait

    // Handle poll interval output.
    indicateUsbPollRate();

    // Handle serial in
    handleSerialIn();

    // Measure time
    if(TPM0_CNT > maxTimeSerial)  maxTimeSerial = TPM0_CNT;   

    // Wait some time
    while(TPM0_CNT < 800);

    // For time measurement
    uint16_t timeStart = TPM0_CNT;
       
    // Handle joystick buttons and axis
    handleJoystick();  // about 30-40us

    // Measure time
    if(TPM0_CNT > maxTimeTotal)  maxTimeTotal = TPM0_CNT;   
    if(TPM0_CNT-timeStart > maxTimeJoystick)  maxTimeJoystick = TPM0_CNT-timeStart;   

    // Check that routines did not take too long (no overflow)
    ASSERT(!(TPM0_SC & FTM_SC_TOF));
  }
}
