/* 
   Basic USB Joystick Example
   Teensy becomes a USB joystick

   You must select Joystick from the "Tools > USB Type" menu

   Pushbuttons should be connected to digital pins 0 and 1.
   Wire each button between the digital pin and ground.
   Potentiometers should be connected to analog inputs 0 to 1.

   This example code is in the public domain.
*/


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
  pinMode(0, INPUT_PULLUP);
  pinMode(1, INPUT_PULLUP);
  pinMode(2, OUTPUT);

  Joystick.useManualSend(true);
  //noInterrupts();

  // Wait a few polls  
  delay(2000);
  for(int i=0; i<1000; i++)
    Joystick.send_now();
  // Measure host's poll time
  measurePollTime();
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
  out = !out;
  digitalWrite(USB_POLL_OUT, out);
  
#if 01
  Joystick.X(analogRead(0));
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
