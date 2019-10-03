// The PWM frequency for the digital outs. Is set higher than the default to avoid flickering.
#define DOUT_PWM_FREQUENCY    100000 // In Hz
//#define DOUT_PWM_FREQUENCY    100 // In Hz

// Number of digital outs.
#define COUNT_DOUTS  ((uint8_t)sizeof(doutPins))


// States for the digital out pins.
enum DOUT_STATE {
  SUSTAIN = 0,  // or idle. Stays in this stae forever (until next command)
  WAIT,     // Waiting (do nothing) for a certain time until next state
  ATTACK,   // Change brightness from current value to new value in a certain time
};


// The state variable for each digital out.
struct DoutState {
  uint8_t state;  // DOUT_STATE
  uint16_t brightness; // The last set value.
  uint16_t delay;  // Delay time in ms.
  uint16_t attack;  // Attack time in ms.
  int16_t deltaBrightness;  // During attack this is added to brightness in each step.
  uint16_t endBrightness; // The resulting brightness.
};


// The state for each digital out.
DoutState doutState[COUNT_DOUTS] = {};


// Initialize.
void initDout() {
  for(uint8_t i=0; i<COUNT_DOUTS; i++) {
    pinMode(doutPins[i], OUTPUT);
    analogWriteFrequency(doutPins[i], DOUT_PWM_FREQUENCY*1000/81); // Note: we have a factor here because the clock source will be changed
  }
}


// Sets a digital out. Configure the envelope of the signal.
// You can configure
//  - the time to wait before something happens, e.g. to delay the ouput.
//  - the time it takes to change form current brightness to given brightness.
//  - the end brightness.
// dout: the index of the digital out.
// delayMs: The time to wait before next state (in ms).
// attackMs: The time to wait before next state (in ms).
// brightness: The brightness [0;100] after 'attack'.
void setDout(uint8_t dout, int brightness, int attackMs, uint16_t delayMs) {
  DoutState* statePtr = &doutState[dout];
  statePtr->state = WAIT;
  statePtr->delay = delayMs;
  statePtr->attack = attackMs;
  statePtr->endBrightness = 0; // REMOVE
  statePtr->endBrightness = (brightness*65535l)/100;
  statePtr->deltaBrightness = (statePtr->endBrightness - statePtr->brightness)/statePtr->attack;

  // Give feedback
  if(serialPrintAllowed()) {
    serialPrint("DOUT");
    serialPrint(dout);
    serialPrint(" (Pin=");
    serialPrint(doutPins[dout]);
    serialPrint(") set to ");
    serialPrint(brightness);
    if(attackMs) {
      serialPrint(", attack=");
      serialPrint(attackMs);
      serialPrint("ms");
    }
    if(delayMs) {
      serialPrint(", delay=");
      serialPrint(delayMs);
      serialPrint("ms");
    }
    serialPrintln();
  }
}


// Handles the output to the digital outs.
// I.e. controls the wait and attack time and also the sustain level.
void handleDout() {  
  DoutState* statePtr = doutState;
 
  for(uint8_t i=0; i<COUNT_DOUTS; i++) { 
    // Check state
    switch(statePtr->state) {
      case WAIT: 
        // Decrement time (1ms each turn).
        if(statePtr->delay > 0)
          statePtr->delay--;
        else {
          // Wait time is over, start 'attack'
          statePtr->state = ATTACK;
          goto L_ATTACK;
        }
        break;

      case ATTACK: 
L_ATTACK:
        // Change brightness to 'end' brightness.
        // Check timer:
        if(statePtr->attack > 0) {
          // Decement timer
          statePtr->attack --;
          // Control brightness
          uint16_t b = statePtr->brightness;
          if(statePtr->deltaBrightness >= 0) {
            b += statePtr->deltaBrightness;
            if(b < statePtr->deltaBrightness) {
              // Overflow happened
              b = 0xFFFFu;
            }
          }
          else {
            if(b < -statePtr->deltaBrightness) {
              // underflow would happen
              b = 0;
            }
            else
              b += statePtr->deltaBrightness;
          }
          statePtr->brightness = b;
       }
        else {
          // Attack time is over -> SUSTAIN
          statePtr->state = SUSTAIN;
          statePtr->brightness = statePtr->endBrightness;
        }
        // Output
        analogWrite(doutPins[i], statePtr->brightness >> 8);
        break;

      case SUSTAIN: 
        // Do nothing.
        break;  
    }
    // Next
    statePtr++;
  }
}
