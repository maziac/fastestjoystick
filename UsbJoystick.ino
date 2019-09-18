/* 
USB Joystick using a Teensy LC board.
The features are:
- Requested 1ms USB poll time
- Additional delay of max. (??)
- Indication of the real used USB poll time
*/

#include <Arduino.h>

#include <usb_desc.h>


// *** HW CONFIGURATION BEGIN ************************************************

// The pin used for poll-out. (Note: the built-in LED is pin 13)
#define USB_POLL_OUT  14

// Buttons start at this digital pin
#define BUTTONS_PIN_OFFS 0    // Digital pin 0 is button 0

// Number of total joystick buttons.
#define COUNT_BUTTONS   13    // Pins 0-12

// Axes start at this digital pin
#define AXES_PIN_OFFS 20  // Pins 20-23

// Number of different joystick axes. At maximum 2 axes are supported.
#define COUNT_AXES   2

// Digital outputs start at this pin
#define DOUTS_PIN_OFFS  15

// Number of digital outs
#define COUNT_DOUTS  3

// *** HW CONFIGURATION END **************************************************


// ASSERT macro
#define ASSERT(cond)  {if(!(cond)) error();}


// Turn main LED on/off
#define MAIN_LED(on)    digitalWrite(LED_BUILTIN, on)
//#define MAIN_LED(on)    

// The debounce and minimal press- and release time of a button (in ms).
#define MIN_PRESS_TIME 1000


// Timer values for all joystick buttons
//uint8_t
int buttonTimers[COUNT_BUTTONS] = {};

// The (previous) joystick button values.
//bool prevButtonValue[COUNT_BUTTONS];
// The (previous) joystick button values. Bit map. 1=pressed.
uint32_t prevButtonsValue = 0;  

// Timer values for the axes.
//uint8_t
int axesTimers[COUNT_AXES] = {};

// The (previous) joystick button values.
// Values: 0=left/down, 512=no direction, 1023=right/up
int prevAxesValue[COUNT_AXES];

// Either button for low (0-512) was the last activity: 0.
// Or button for high (512-1023) was the last activity: 1.
int lastAxesActivity[COUNT_AXES] = {};




// We should never get here. But if we do the assumption that the joystick + sampling delay is handled within 
// 1ms (i.e. 900us) is wrong.
// Or some other error occured.
// This is indicated by fast blinking of the LED.
// The routine will never exit.
void error() {
  while(true) {
    MAIN_LED(false);
    delay(150);
    MAIN_LED(true);
    delay(50);
  }
}

  
// Handles the Output and LED to indicate the USB polling rate.
void indicateUsbPollRate() {
    // USB_POLL_OUT:
    static bool usbPollOut = false;
    usbPollOut = !usbPollOut;
    //digitalWrite(USB_POLL_OUT, usbPollOut);

    // Main LED (poll time / 1000)
    static bool mainLedOut = false;
    static int mainLedCounter = 0;
    mainLedCounter--;
    if(mainLedCounter < 0) {
      // toggle main LED
      mainLedOut = !mainLedOut;
      MAIN_LED(mainLedOut);
      mainLedCounter = 1000;
    }
}


// Reads the joystick buttons.
// Handles debouncing and minimum press time.
void handleButtons() {
  // Go through all buttons
  uint32_t mask = 0b00000001;
 // uint8_t input = PORTA;
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
void handleAxes() {
  int shift = 4;
  uint32_t mask = ~0xFFFFC00F;
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


// Takes a string from serial in and decodes it.
// Correct strings look like:
// "o7=1" or "o3=0"
// for setting ouput 7 to HIGH and output 3 to LOW.
// On host die (linux or mac) you can use e.g.:
// echo o0=1 > /dev/cu.usbXXXX
void decodeSerialIn(char* input) {
  // Check for 'o'utput
  if(input[0] != 'o')
    return;

  // Get output
  int pin = input[1] - '0';
  // Check
  if(pin < 0 || pin >= COUNT_DOUTS)
    return;

  // Get value
  int value = input[3] - '0';
  // Check
  if(value < 0 || value > 1)
    return;

  // Set pin
  digitalWrite(DOUTS_PIN_OFFS+pin, value);
#if 01
  Serial.print("PIN ");
  Serial.print(pin);
  Serial.print(" = ");
  Serial.println(value);
#endif 
}


// Handles the serial input.
// Via serial in some digital output pins can be driven, e.g. for LED light of the buttons.
void handleSerialIn() {
  static char input[10];
  static char* inpPtr = input;
  
  // Check if data available
  while(Serial.available()) {
    // Get data 
    char c = Serial.read();
    if(c == '\n') {
      // End found
      *inpPtr = 0;
      inpPtr = input;
      // Decode input
      decodeSerialIn(input);
      break;
    }
    else {
      // Input too long?
      if(inpPtr > (input+sizeof(input)-1))
        break;
      // Remember character
      *inpPtr = c;   
      inpPtr++;
    }
  }
}

  
// SETUP
void setup() {
#if 01
  delay(1000);
  Serial.println(F_CPU); 
   
  // Disable interrupts
  //noInterrupts();
  
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(USB_POLL_OUT, OUTPUT);

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
#endif


#if 0
  // Initialize pins
  pinMode(14, OUTPUT);

  // Setup timer
  MCG_C1 &= ~0b010; // Disable IRCLK
  MCG_SC &= ~0b01110; // Divider = 1 => 4MHz
  MCG_C2 |= 0b001;  // IRCS. fast internal reference clock enabled
  MCG_C1 |= 0b010;  // IRCLKEN = enabled
  
  // Clock source
  SIM_SOPT2 |= 0b00000011000000000000000000000000;  // MCGIRCLK

  // Prescaler: 4 -> 4MHz/4 = 1MHz => 1us
  TPM0_SC = 0;
  while (0 != (TPM0_SC & 0b00001000))
    TPM0_SC = 0;  // spin wait 
  TPM0_SC |= 0b00000010;  // Prescaler /4 = 1Mhz
  TPM0_SC |= 0b00001000;  // Enable timer
 

  // When this is reached the overflow flag is set.
  TPM0_MOD = 1000; // 1000us = 1ms

  // Loop: toggle a pin each time the timer elapses (overflows)
  bool out = false;
  while(true) {
    TPM0_CNT = 0;  // Start value for timer
    TPM0_SC |= FTM_SC_TOF;  // Clear overflow
    
    out = !out;
    digitalWrite(14, out);

    // Wait on timer overflow
    while(!(TPM0_SC & FTM_SC_TOF));
  }
#endif
}



// MAIN LOOP
void loop() {
  while(true) {

    // Prepare USB packet and wait for poll.
    usb_joystick_send();

    // Restart timer 0  
    TPM0_CNT = 0; 
    // Clear overflow
    while(TPM0_SC & FTM_SC_TOF)
      TPM0_SC |= FTM_SC_TOF; // spin wait

    digitalWrite(14, true);
 
    // Handle poll interval output.
    indicateUsbPollRate();

    // Handle serial in
    handleSerialIn();

    // Wait some time
    while(TPM0_CNT < 800);

    digitalWrite(14, false);
   
    // Handle joystick buttons and axis
    handleJoystick();  // about 30-40us

    // Check that routines did not take too long (no overflow)
    ASSERT(!(TPM0_SC & FTM_SC_TOF));
  }
}
