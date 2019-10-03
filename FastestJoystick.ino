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
// Button 0-6 = Pin 12, 11, 9, unused, 6, 3, 0  (at max 0-12 input buttons are possible)
// Axis left/right: Pin 18/19 AU/AD
// Axis down/up: Pin 20/21 AL/AR
// DOUT 0-3 = Pin 16, 17, 22, 23

// The buttons permutation table. Logical button ins [0;12] are mapped to physical pins.
//uint8_t buttonPins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12 }; // Normal configuration
uint8_t buttonPins[] = { /*Button0*/ 12, /*Button1*/11, /*Button2*/9, /*Unused*/1, /*Start1*/6, /*Start2*/3, /*Orientation*/0, /*Others unused*/ };

// The axes permutation table. Logical axes ins [0;12] are mapped to physical pins.
// 2 entries form a pair, e.g. left/right or up/down
uint8_t axesPins[] = { 20, 21,  18, 19 };

// The digital out permutation table. Logical outs [0;3] are mapped to physical pins.
uint8_t doutPins[] = { 16, 17, 22, 23 };  // Use pins capable of PWM


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
#define SW_VERSION "0.11"


// Uncomment to allow logging.
#define ENABLE_LOGGING

// ASSERT macro
#define ASSERT(cond)  {if(!(cond)) error(__FILE__ ", LINE " TOSTRING(__LINE__) ": ASSERT(" #cond ")");}
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)

// The delay of the joystick. I.e. the time given to the algorithm to read the joystick axes and buttons
// and to prepare the USB packet.
// Or in other words: the algorithm is started 1ms-JOYSTICK_DELAY before thenext USB poll.
#define JOYSTICK_DELAY  200   // in us

// This is the allowed jitter of the USB host. 100us at a USB poll rate of 1ms means that the next USB poll is allowed to arrive 
// at 1000ms-100us = 900ms.
// Note: The SW checks that the joystick algorithm is ready before usb-poll-time - JOYSTICK_POLL_JITTER.
// If this is violated then the dout values will start to blink.
#define JOYSTICK_POLL_JITTER  100   // in us


// Variables to measure the maximum timings.
uint16_t maxTimeSerial = 0;
uint16_t maxTimeJoystick = 0;
uint16_t maxTimeTotal = 0;



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
  initJoystick();
  initDout();

  // Setup timer
  setupTimer();
}




// MAIN LOOP
void loop() {
    
  // Endless loop
  while(true) {
 
    // There should be exact no packet in the queue.
    ASSERT(usb_tx_packet_count(JOYSTICK_ENDPOINT) == 0);
  
    // Prepare USB packet (note: this should immediately return as the packet queue is empty at this point.
    usb_joystick_send();

    // Wait on poll at max. double the poll time, then check for serial input.
    clearTimer(2000 * JOYSTICK_INTERVAL);
    
    // Wait on USB poll
    while(usb_tx_packet_count(JOYSTICK_ENDPOINT) > 0) {
#if 01
      if(isTimerOverflow()) {
        // Wait for 1ms
        clearTimer(1000 * JOYSTICK_INTERVAL);
        // Handle serial, otherwise no input/ouput is possible if no joystick consumer is listening on Linux.
        handleSerialIn();
        // Handle digital out
        handleDout();
        // Wait until 1ms is over.
        while(!isTimerOverflow() && usb_tx_packet_count(JOYSTICK_ENDPOINT) > 0);
      }
#endif
    }
 
    // Restart timer 0  
    clearTimer(1000*JOYSTICK_INTERVAL-JOYSTICK_POLL_JITTER); // ie. 900us
  //clearTimer(700);
ASSERT(!isTimerOverflow());

    // Handle poll interval output.
    indicateUsbJoystickPollRate();
ASSERT(!isTimerOverflow());

    // Handle serial in
    handleSerialIn();
ASSERT(!isTimerOverflow()); // Here gibt es Timer overflow

    // Handle digital out
    handleDout();
ASSERT(!isTimerOverflow());

    // Measure time 
    if(GetTime() > maxTimeSerial)  maxTimeSerial = GetTime();   
     
     ASSERT(!isTimerOverflow());
     
    // Wait some time
    while(GetTime() < 1000*JOYSTICK_INTERVAL-JOYSTICK_DELAY); // ie. 800us

     
    // For time measurement
    uint16_t timeStart = GetTime();
       
     
     if(isTimerOverflow()) {
       if(GetTime() > maxTimeTotal)  maxTimeTotal = GetTime();   
       if(GetTime()-timeStart > maxTimeJoystick)  maxTimeJoystick = GetTime()-timeStart;   
       error("Overflow y");
     }
     
    // Handle joystick buttons and axis
    handleJoystick();  // about 30-40us

    // Measure time
    if(GetTime() > maxTimeTotal)  maxTimeTotal = GetTime();   
    if(GetTime()-timeStart > maxTimeJoystick)  maxTimeJoystick = GetTime()-timeStart;   

    // Check that routines did not take too long (no overflow)
//    ASSERT(!isTimerOverflow());
     if(isTimerOverflow()) {
       if(GetTime() > maxTimeTotal)  maxTimeTotal = GetTime();   
       if(GetTime()-timeStart > maxTimeJoystick)  maxTimeJoystick = GetTime()-timeStart;   
       error("Overflow e");
     }
  }
}
