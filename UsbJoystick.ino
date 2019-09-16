/* 
USB Joystick using a Teensy LC board.
The features are:
- Requested 1ms USB poll time
- Additional delay of max. (??)
- Indication of the real used USB poll time
*/

#include <usb_desc.h>


// Teensy LC main LED
#define LED_MAIN  13

// The pin used for poll-out.
#define USB_POLL_OUT  0

// Buttons start at this digital pin
#define BUTTONS_PIN_OFFS 0    // Digital pin 0 is button 0

// Number of total joystick buttons.
#define COUNT_BUTTONS   13    // Pins 0-12

// Axes start at this digital pin
#define AXES_PIN_OFFS 20  // Pins 20-23

// Number of different joystick axes. At maximum 2 axes are supported.
#define COUNT_AXES   2

// The deounce and minimal press- and release time of a button (in ms).
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



// Handles the Output and LED to indicate the USB polling rate.
void indicateUsbPollRate() {
    // USB_POLL_OUT:
    static bool usbPollOut = false;
    usbPollOut = !usbPollOut;
    digitalWrite(USB_POLL_OUT, usbPollOut);

    // Main LED (poll time / 1000)
    static bool mainLedOut = false;
    static int mainLedCounter = 0;
    mainLedCounter--;
    if(mainLedCounter < 0) {
      // toggle main LED
      mainLedOut = !mainLedOut;
      digitalWrite(LED_MAIN, mainLedOut);
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
    int axisLow = (digitalRead(AXES_PIN_OFFS+2*i) == LOW) ? 0 : 512; // Active LOW
    int axisHigh = (digitalRead(AXES_PIN_OFFS+2*i+1) == LOW) ? 1023 : 512; // Active LOW
    
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



  
// SETUP
void setup() {
  // Initialize pins
  pinMode(LED_MAIN, OUTPUT);
  pinMode(USB_POLL_OUT, OUTPUT);

  // Initialize buttons
  for(int i=0; i<COUNT_BUTTONS; i++) {
    pinMode(BUTTONS_PIN_OFFS+i, INPUT_PULLUP);
  }
  for(int i=0; i<2*COUNT_AXES; i++) {
    pinMode(AXES_PIN_OFFS+i, INPUT_PULLUP);
  }


        pinMode(14,OUTPUT);

}



// MAIN LOOP
void loop() {

  // Loop forever
  while(true) {
    
    // Handle poll interval output.
    indicateUsbPollRate();

    delayMicroseconds(800);

    // Handle joystick buttons and axis
    handleJoystick();

    // Prepare USB packet and wait for poll.
    //Joystick.send_now();
    usb_joystick_send();
    
      digitalWrite(14, false);
  }
}
