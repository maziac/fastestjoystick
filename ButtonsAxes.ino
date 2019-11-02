
// Number of total joystick buttons.
#define COUNT_BUTTONS   ((uint8_t)sizeof(buttonPins))

// Number of different joystick axes. (At maximum 2 axes are supported.)
#if ANALOG_AXES_ENABLED
#define COUNT_AXES  (sizeof(axesPins))
#else
#define COUNT_AXES   ((uint8_t)sizeof(axesPins)/2)
#endif

// Timer values for all joystick buttons.
uint16_t buttonTimers[COUNT_BUTTONS] = {};

// The (previous) joystick button values.
//bool prevButtonValue[COUNT_BUTTONS];
// The (previous) joystick button values. Bit map. 1=pressed.
uint32_t prevButtonsValue = 0;  


// Initialize.
void initJoystick() {
  for(uint8_t i=0; i<COUNT_BUTTONS; i++) {
    //pinMode(buttonPins[i], INPUT_PULLUP);
    pinMode(buttonPins[i], INPUT_PULLUP);
  }
  for(uint8_t i=0; i<sizeof(axesPins); i++) {
    pinMode(axesPins[i], (ANALOG_AXES_ENABLED)? INPUT : INPUT_PULLUP);
  }
}


// Reads the joystick buttons and axis.
// Handles debouncing and minimum press time.
void handleJoystick() {
  // Buttons
  handleButtons();
  // Axes
#if ANALOG_AXES_ENABLED
  handleAnalogAxes();
#else
  handleDigitalAxes();
#endif
}


// Reads the joystick buttons.
// Handles debouncing and minimum press time.
void handleButtons() {
  // Go through all buttons
  uint32_t mask = 0b00000001;
  for(uint8_t i=0; i<COUNT_BUTTONS; i++) {
    
    // Check if button timer is 0
    if(buttonTimers[i] > JOYSTICK_INTERVAL) {
      // Decrease timer
      buttonTimers[i] -= JOYSTICK_INTERVAL;
    }
    else {
      // Timer is zero (i.e. below JOYSTICK_INTERVAL). Now check if the button state has changed.
      uint32_t buttonValue = (digitalRead(buttonPins[i]) == LOW) ? 1 : 0; // Active LOW
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


#if ! ANALOG_AXES_ENABLED
// Reads the joystick digital axes.
// Note: This algorithm needs to be modified if COUNT_AXES is bigger than 2.
void handleDigitalAxes() {
  int shift = 4;
  uint32_t mask = ~0xFFFFC00F;
  //int shift = 4+((COUNT_AXES-1)*10);;
  //uint32_t mask = (~0xFFFFC00F) << ((COUNT_AXES-1)*10);
  // Go through all axes
  for(uint8_t i=0; i<COUNT_AXES; i++) {
    // Read buttons for the axis of the joystick
    int axisLow = (digitalRead(axesPins[i<<1]) == LOW) ? 0 : 512; // Active LOW
    int axisHigh = (digitalRead(axesPins[(i<<1)+1]) == LOW) ? 1023 : 512; // Active LOW
    
    int axisValue = 512;
    if(axisLow != 512) {
      axisValue = axisLow;
    }
    if(axisHigh != 512) {
      axisValue = axisHigh;
    }

    // Handle axes movement
    usb_joystick_data[1] = (usb_joystick_data[1] & (~mask)) | (axisValue << shift);

    // Next
    shift += 10;
    mask <<= 10;
    //shift -= 10;
    //mask >>= 10;
  }
}
#endif


#if ANALOG_AXES_ENABLED
// Reads the joystick analog axes.
// Can handle max. 2 analog axes.
void handleAnalogAxes() {
#error NOT YET IMPLEMENTED !!!!
}
#endif
