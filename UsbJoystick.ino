/* 
   Basic USB Joystick Example
   Teensy becomes a USB joystick

   You must select Joystick from the "Tools > USB Type" menu

   Pushbuttons should be connected to digital pins 0 and 1.
   Wire each button between the digital pin and ground.
   Potentiometers should be connected to analog inputs 0 to 1.

   This example code is in the public domain.
*/


// Measures the hosts's poll time and blinks accordingly.
// I.e. 1x blink = 1ms, 8x blink = 8ms
// If it fails (time < 1ms like in Linux) then false is returned.
bool measurePollTime() {
  // Measure time
  Joystick.send_now();
  unsigned long startTime = micros();
  Joystick.send_now();
  unsigned long endTime = micros();
  long diff = endTime - startTime;
  diff = (diff+500l)/1000l; // round number
  if(diff == 0)
    return false; // Measurement failed.
    
  // Blink LED (1x for each millisecond)
  for(int i=0; i<diff; i++) {
    digitalWrite(13, true);
    delay(300);
    digitalWrite(13, false);
    delay(300);
  }

  // Measurement succeeded
  return true;
}


void setup() {
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, OUTPUT);

  Joystick.useManualSend(true);
  //noInterrupts();

  // Wait a few polls  
  delay(2000);
  for(int i=0; i<1000; i++)
    Joystick.send_now();
}


#if 0
void loop() {
  Joystick.button(1, !digitalRead(1));

  Joystick.send_now();
}
#endif

#if 01
bool out = false;
bool pollTimeMeasured = false;
void loop() {
  // Check if we would like to measure/indicate the polling time
  if(!pollTimeMeasured) {
    // Measure
    pollTimeMeasured = measurePollTime();
    return;
  }

  // Normal mode: Check for button presses
  out = !out;
  digitalWrite(14, out);
#if 01
  Joystick.button(1, !digitalRead(1));
  //Joystick.button(2, !digitalRead(1));
  //Joystick.button(3, !digitalRead(1));
  //Joystick.button(4, !digitalRead(1));
#endif
  Joystick.send_now();
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
