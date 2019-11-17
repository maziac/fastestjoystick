
// We should never get here. But if we do the assumption that the joystick + sampling delay is handled within 
// 1ms (i.e. 900us) is wrong.
// Or some other error occured.
// This is indicated by fast blinking of the LED and also all digital outs will blink.
// The routine will never exit.
void error(const char* text) {
  // blink
  fastBlinking();
}


// Blink fast. Main LED and DOUTs. For error indication.
void fastBlinking() {
  // Endless loop
  while(true) {
#ifdef MAIN_LED_PIN
    digitalWrite(MAIN_LED_PIN, false);
#endif
    delay(150);
#ifdef MAIN_LED_PIN
    digitalWrite(MAIN_LED_PIN, true);
#endif
    delay(50);
  }
}


// Handles the Output and LED to indicate the USB polling rate for the joystick endpoint.
void indicateUsbJoystickPollRate() {
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
