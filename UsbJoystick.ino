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
#define BUTTONS_PIN_OFFS 1

// Number of total joystick buttons.
#define COUNT_BUTTONS   8

// Axes start at this digital pin
#define AXES_PIN_OFFS 1

// Number of different joystick axes.
#define COUNT_AXES   2

// The deounce and minimal press- and release time of a button (in ms).
#define MIN_PRESS_TIME 5



// Timer values for all joystick buttons
uint8_t buttonTimers[COUNT_BUTTONS] = {};

// The (previous) joystick button values.
//bool prevButtonValue[COUNT_BUTTONS];
// The (previous) joystick button values. Bit map. 1=pressed.
uint32_t prevButtonsValue = 0;  

// Timer values for the axes.
uint8_t axesTimers[COUNT_AXES] = {};

// The (previous) joystick button values.
// Values: 0=left/down, 512=no direction, 1023=right/up
int prevAxesValue[COUNT_AXES];




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


// Reads the joystick buttons and axis.
// Handles debouncing and minimum press time.
void handleJoystick() {
    
  // Go through all buttons
  uint32_t mask = 0b00000001;
  for(int i=0; i<3; i++) {
   Serial.print("i=");Serial.println(i);

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

  // Go through all axes
  for(int i=0; i<COUNT_AXES; i++) {
    // Check if axis timer is 0
    if(axesTimers[i] > JOYSTICK_INTERVAL) {
      // Decrease timer
      axesTimers[i] -= JOYSTICK_INTERVAL;
      continue;
    }

    // Timer is zero (i.e. below JOYSTICK_INTERVAL). Now check if the button state has changed.
    int axisValue = (digitalRead(AXES_PIN_OFFS+i) == LOW); // Active LOW
    if(axisValue == prevAxesValue[i])
      continue; // Not changed
      
    // Button value has changed
    prevAxesValue[i] = axisValue;
    // Restart timer
    axesTimers[i] = MIN_PRESS_TIME;

    // Handle button press/release
  //  Joystick.button(1+i, buttonValue);  // Buttons start at 1 (not 0)
  }
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

    // Prepare USB packet andwait for poll.
    //Joystick.send_now();
    usb_joystick_send();
    
      digitalWrite(14, false);
  }
}
