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
#define BUTTON_PIN_OFFS 1

// Number of total joystick buttons.
#define COUNT_BUTTONS   8

// The deounce and minimal press- and release time of a button (in ms).
#define MIN_PRESS_TIME 25


// Timer values for all joystick buttons
uint8_t buttonTimers[COUNT_BUTTONS] = {};

// The (previous) joystick button values.
bool prevButtonValue[COUNT_BUTTONS];


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
  for(int i=0; i<COUNT_BUTTONS; i++) {
    // Check if button timer is 0
    if(buttonTimers[i] > JOYSTICK_INTERVAL) {
      // Decrease timer
      buttonTimers[i] -= JOYSTICK_INTERVAL;
      continue;
    }

    // Timer is zero (i.e. below JOYSTICK_INTERVAL). Now check if the button state has changed.
    bool buttonValue = (digitalRead(BUTTON_PIN_OFFS+i) == LOW); // Active LOW
    if(buttonValue == prevButtonValue[i])
      continue; // Not changed
      
    // Button value has changed
    prevButtonValue[i] = buttonValue;
    // Restart timer
    buttonTimers[i] = MIN_PRESS_TIME;

    // Handle button press/release
    Joystick.button(1+i, buttonValue);  // Buttons start at 1 (not 0)
  }
  
}


// SETUP
void setup() {
  // Initialize pins
  pinMode(LED_MAIN, OUTPUT);
  pinMode(USB_POLL_OUT, OUTPUT);

  // Initialize buttons
  for(int i=0; i<COUNT_BUTTONS; i++) {
    pinMode(BUTTON_PIN_OFFS+i, INPUT_PULLUP);
    prevButtonValue[i] = false;
  }

  // Initialize USB
  Joystick.useManualSend(true);
}



// MAIN LOOP
void loop() {

  // Loop forever
  while(true) {
    
    // Handle poll interval output.
    indicateUsbPollRate();

    // Handle joystick buttons and axis
    handleJoystick();

    // Prepare USB packet andwait for poll.
    Joystick.send_now();
    
  }
}





#if 0


/* 
   Basic USB Joystick Example
   Teensy becomes a USB joystick

   You must select Joystick from the "Tools > USB Type" menu

   Pushbuttons should be connected to digital pins 0 and 1.
   Wire each button between the digital pin and ground.
   Potentiometers should be connected to analog inputs 0 to 1.

   This example code is in the public domain.
*/
#include <usb_dev.h>

// Teensy LC LED
#define LED_MAIN  13


// The pin used for poll-out.
#define USB_POLL_OUT  14


// Measures the hosts's poll time and blinks accordingly.
// I.e. 1x blink = 1ms, 8x blink = 8ms
// If it fails (time < 1ms like in Linux) then false is returned.
bool measurePollTime() {
  // Measure time
  for(int i=0; i<10; i++)
    Joystick.send_now(); 
  unsigned long startTime = micros();
  Joystick.send_now();
  unsigned long endTime = micros();
  long diff = endTime - startTime;
  diff = (diff+500l)/1000l; // round number
  if(diff == 0 || diff > 8) {
    // Do fast blink during 5x for error
    for(int i=0; i<5; i++) {
      digitalWrite(LED_MAIN, true);
      delay(40);
      digitalWrite(LED_MAIN, false);    
      delay(40);
    }
    return false; // Measurement failed.
  }
  
  // Blink LED (1x for each millisecond)
  for(int i=0; i<diff; i++) {
    delay(300);
    digitalWrite(LED_MAIN, true);
    delay(300);
    digitalWrite(LED_MAIN, false);
  }

  // Measurement succeeded
  return true;
}


void setup() {
  pinMode(LED_MAIN, OUTPUT);
  pinMode(USB_POLL_OUT, OUTPUT);
  pinMode(15, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, OUTPUT);

  Joystick.useManualSend(true);
  //noInterrupts();

#if 0
  // Wait a few polls  
  delay(2000);
  for(int i=0; i<1000; i++)
    Joystick.send_now();
  // Measure host's poll time
  measurePollTime();
 #endif
}


#if 0
void loop() {
  Joystick.button(1, !digitalRead(1));

  Joystick.send_now();
}
#endif

#if 01




void loop() {
  // Normal mode: Check for button presses
  static bool out = false;

Joystick.useManualSend(true);

  out = !out;
  digitalWrite(USB_POLL_OUT, out);

  
        
#if 01
  //Joystick.X(analogRead(0));
  Joystick.button(1, !digitalRead(1));
  //digitalWrite(15, digitalRead(1));
  //Joystick.button(2, !digitalRead(1));
  //Joystick.button(3, !digitalRead(1));
  //Joystick.button(4, !digitalRead(1));
#endif
  
  //digitalWrite(USB_POLL_OUT, true);
  Joystick.send_now();
  //digitalWrite(USB_POLL_OUT, false);
}
#endif



#if 0
bool output = false;
void loop() {
#if 0
  output = !output;
  digitalWrite(2, output);
  //delay(1000);
#endif

#if 01
  // read analog inputs and set X-Y position
  //Joystick.X(analogRead(0));
  //Joystick.Y(analogRead(1));

  // read the digital inputs and set the buttons
 // Joystick.button(1, digitalRead(0));
  Joystick.button(2, !digitalRead(1));

  // a brief delay, so this runs 20 times per second
  //delay(50);

#if 0
  delay(1000);
  Joystick.button(1, false);
  digitalWrite(13, false);
  
  delay(1000);
  Joystick.button(1, true);
  digitalWrite(13, true);
 #endif
 
 #endif
}
#endif

#endif
