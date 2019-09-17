/* 
USB Joystick using a Teensy LC board.
The features are:
- Requested 1ms USB poll time
- Additional delay of max. (??)
- Indication of the real used USB poll time
*/

#include <Arduino.h>

#include <usb_desc.h>

// Turn main LED on/off
//#define MAIN_LED(on)    digitalWrite(LED_BUILTIN, on)
#define MAIN_LED(on)    

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



  
// SETUP
void setup() {
  delay(1000);
  Serial.println(F_CPU); 
   
  // Disable interrupts
  noInterrupts();
  
  // Initialize pins
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(USB_POLL_OUT, OUTPUT);

  // Initialize buttons
  for(int i=0; i<COUNT_BUTTONS; i++) {
    pinMode(BUTTONS_PIN_OFFS+i, INPUT_PULLUP);
  }
  for(int i=0; i<2*COUNT_AXES; i++) {
    pinMode(AXES_PIN_OFFS+i, INPUT_PULLUP);
  }


 // Setup timer 2 (8 bit timer) to check that the whole joystick handling is handled
 // completely within 900us (<1ms).

  uint32_t mcg_sc = MCG_SC;
  int fcrdiv = (MCG_SC >> 1) & 0b0111;
  Serial.println(fcrdiv);
  MCG_C1 &= ~ 0b010; // Disable IRCLK
  MCG_SC = (mcg_sc & (~0b01110)); // 4MHz. | 0b0010;  // Divider = 2 => 2MHz = 0.5us (*2 => 1us)
  fcrdiv = (MCG_SC >> 1) & 0b0111;
  Serial.println(fcrdiv);
  int irclken = (MCG_C1 >> 1) & 0b01; 
  Serial.println(irclken);
  MCG_C2 |= 0b001;  // IRCS. fastinternal reference clock enabled
  
  // Clock source
  uint32_t sim_sopt2 = SIM_SOPT2;
  // 11 = MCGIRCLK
  SIM_SOPT2 = (sim_sopt2 & 0b11111100111111111111111111111111) | 0b00000011000000000000000000000000;

  
  // Prescaler: 64 -> resolution 1.333us=4/3us (at F_CPU=48MHz).
  TPM0_SC = 0;
//  TPM0_SC |= 0b00000101;//10;  // Prescaler :32
  TPM0_SC |= 0b00000100;  // Prescaler :4 = 1Mhz
  //FUNKTIOIERT noch nicht!!!!!
  TPM0_SC |= 0b00001000;  // Enable timer
  TPM0_SC |= FTM_SC_TOF;  // Clear overflow

  MCG_C1 |= 0b010;  // IRCLKEN = enabled

  // Time (900us). When this is reached the overflow flag is set.
  TPM0_MOD = 1000; // 1000us

  Serial.println(TPM0_MOD);
  
//TPM0_MOD = 4000; //2*1333;

interrupts();
  bool out = false;
  while(true) {
    TPM0_CNT = 0;  // Start value for timer
    TPM0_SC |= FTM_SC_TOF;  // Clear pending bits
    out = !out;
    digitalWrite(USB_POLL_OUT, out);
    TPM0_CNT = 0;  // Start value for timer
    TPM0_SC |= FTM_SC_TOF;  // Clear pending bits
    while(!(TPM0_SC & FTM_SC_TOF));
    //delay(1);
  }
   
}



// MAIN LOOP
void loop() {

  // Loop forever
  while(true) {
    // Prepare USB packet and wait for poll.
    usb_joystick_send();
    
    // Reset timer 2 (8 bit timer) to check that the whole joystick handling is handled
    // completely within 900us (<1ms).
    TPM0_CNT = 0;  // Start value for timer
 TPM0_MOD = 1000; 
  TPM0_SC = FTM_SC_TOF | 0b00001110;  // Clear pending bits
    if(TPM0_SC & FTM_SC_TOF) 
      break;
   
    // Handle poll interval output.
    indicateUsbPollRate();

    // Delay sampling
    delayMicroseconds(800);

    // Handle joystick buttons and axis
    handleJoystick();

    // Assure that joystick handling didn't take too long
   // if(TPM0_SC & FTM_SC_TOF) 
   //   break;
  }

  // We should never get here. But if we do the assumption that the joystick + sampling delay is handled within 
  // 1ms (i.e. 900us) is wrong.
  // This is indicated by fast blinking of the LED.
  while(true) {
    MAIN_LED(false);
    delay(150);
    MAIN_LED(true);
    delay(50);
  }
  
}
