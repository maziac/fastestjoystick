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
      analogWrite(doutPins[i], 255);
    }
    delay(150);
#ifdef MAIN_LED_PIN
    digitalWrite(MAIN_LED_PIN, true);
#endif
    for(int i=0; i<COUNT_DOUTS; i++) {
      analogWrite(doutPins[i], 0);
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

#ifdef MAIN_LED_PIN
    // Main LED (poll time / 1000)
    static bool mainLedOut = false;
    static int mainLedCounter = 0;
    mainLedCounter--;
    if(mainLedCounter < 0) {
      // toggle main LED
      mainLedOut = !mainLedOut;
      digitalWrite(MAIN_LED_PIN, mainLedOut);
      mainLedCounter = 1000;
    }
#endif
}



// Converts an ascii string into a number.
uint16_t asciiToUint(const char* s) {
  uint16_t value = 0;
  uint8_t k=0;
  while(char digit = *s++) {
    // Check digit
    if(digit < '0' || digit > '9') {
      if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0)
          Serial.println("Error: value");
      return 0;
    }
    // check count of digits
    if(++k > 5) {
      if(usb_tx_packet_count(CDC_TX_ENDPOINT) == 0)
        Serial.println("Error: too many digits");
      return 65535;
    }
    value = 10*value + digit-'0';
  }
  return value;
}

