# (Almost) Fastest Joystick

The idea to write the firmware for an USB controller raised in me when doing the USB controller measurements in
[LagMeter](https://github.com/maziac/lagmeter).
It is quite invisible what a usual game controller does when sampling the buttons and the axis data and what delay it might add.
The only information one gets is the suggested polling rate that the game controller requests from the USB host.
But this is only part of the story: the firmware of the game controller need to sample the joystick data and prepare it in time, i.e. within this polling interval.
Furthermore I wanted to have control on the debouncing and minimum press time for the buttons.
So the only way to get absolute control on what is happening on the firmware side is to write it by myself.

This project is about the firmware of a USB controller. It's not about the HW. So, if you would want to do this on your own you can use an Arcade stick or buttons or use the HW of an existing USB game controller, throw it's electronics away and substitute it with a Teensy board and this SW.

**The goal of this project was to create the fastest USB game controller firmware available with a guaranteed reaction time.**

The goals were:
- Requested 1ms USB poll time
- Additional delay of max. 200us.
- I.e. in total this is a reaction time in the range of 0.2ms to 1.2ms.
- Indication of the real used USB poll time

For comparison:
A typical game controller often has a poll time of 8ms. This in itself leads to a reaction time of 8 to 16ms. Sometimes the controller itself has another delay that needs to be added. So this not only jitters a lot, it is already quite close to 1 frame delay.
Even faster game controller with 1ms poll time introduce an inherent delay of about 5ms.


**Goal not met:**
I couldn't achieve the theoretical response time of 1.2ms in reality. There is somewhere an additional delay of 1 poll interval in the Teensy USB implementation.
So we  end up at 2.2ms response time. This is still a good value but unfortunately not the best :-(
E.g. an XBox controller can achieve 1-2ms response times.

With the [LagMeter](https://github.com/maziac/lagmeter) (which offers an accuray of 1ms) I could verify the response time of 2-3ms:
![](Images/LagMeter_FastestJoystick.jpg)


**But the winner is:**

Here is a link to a similar project but for the Arduino Pro Micro (ATMega32U4).
The SW is also much simpler. It seems that the ATMega32U4 does not suffer from 1 poll delay.
So it is really possible to achieve 1 ms response times.

Here is the link to the github project: [DaemonBite-Arcade-Encoder](https://github.com/MickGyver/DaemonBite-Arcade-Encoder)


# HW

Used HW is a cheap Teensy LC board that you can get around 10€.

The pinout used is as following (but can be changed to fit your needs):
![](Images/TeensyLCSchematics.png)

Note: All pinouts I found for the axes JST connectors were different, so I have chosen the pinout above. If you need a different one you just have to change the wiring.


# SW / Firmware

In general the main loop of a game controller is quite simple:
1. Read buttons and axis
2. Prepare data for USB
3. Wait until USB poll to transfer data
4. Goto 1


## Minimum Press Time

Measurements in the LagMeter project showed that it is possible to achieve key presses of 14ms with a leaf switch button:

![](Images/button_leaf_smallest_14ms.BMP)

With a micro switch  the smallest time is around 45ms:

![](Images/button_micro_smallest_45ms.BMP)

Bouncing was in the range of 5ms.
Here the bouncing of a micro switch:

![](Images/button_micro_bounce.BMP)

This results in the following requirements:
- We need a debouncing of 5ms
- Since I'm optimizing for a game's poll time of 16.7ms (US, Europe would be 20ms) and the minimum achievable time is 14ms, the time need to be extended to at least 16.7ms + USB poll time + firmware delay, so approx. 18ms.
To allow also for 50Hz systems (Europe, 20ms) I extend it to 25ms to allow for some variation.

Note: If you have lower game poll times, e.g. if you have higher screen update rates, then you could adapt this value (MIN_PRESS_TIME).
But anyhow: this is only important for leaf switches. Micro switches anyway have a higher close-time.

Debouncing and minimum press time uses the same algorithm. It extends the simple main loop to:
1. For each button and axis:
    - If timer value is 0: Check if button or axis changed
        - If changed: Restart timer, use the new value.
    - If timer value is not 0: Reduce timer value
2. Send USB data / wait for poll
3. Goto 1


## Additional Delay

If we would simply wait for an USB poll and then right after that poll read the joystick values this would introduce a delay of 1x the polling rate. I.e. if you would press a button just after the sampling happened it would require once the poll time until it is sampled the next time + the usb poll time.
E.g. for a 1ms USB poll time this would result in 2ms.

The delay is illustrated here: the USB poll time is 1ms. The sampling takes place right after the last USB poll.
The input (yellow) is generated by a function generator which runs at 100Hz and is not synchronized with the USB polling rate. Because it is not synchronized it sometimes presses the button (yellow line goes to 0) just before the sampling or just after the sampling which results in different total delays.
(When the blue line goes high the sampling takes place. When it goes down the next USB poll arrives.)

Button press just before the sampling:

![](Images/button_press_before_sampling.BMP)

Button press just after the sampling:

![](Images/button_press_after_sampling.BMP)

The total delay is from button press (yellow line goes down) until USB poll (blue line goes down), i.e. 1ms to 2ms.

This can be seen very nice in this [video](Images/button_press_delay.mp4).

In order to reduce this we move the sampling by 0.8ms after the last USB poll.
This will end up in 1ms + 0.2ms = 1.2ms total max lag.

Button press just before the sampling:

![](Images/button_press_before_sampling_reduced.BMP)

Button press just after the sampling:

![](Images/button_press_after_sampling_reduced.BMP)

The total delay here is 0.2ms to 1.2ms.


This approach has one main problem:
If the algorithm to read the buttons and axes values would take longer than the remaining 0.2ms we would miss the USB poll interval and the resulting lag would be even longer. Therefore there is a check done in SW that the execution of reading buttons and axis values is short enough.
If the check hits: the main LED will start to blink fast and the joystick will stop working.

In fact the tested time is even less than 0.2ms. I use 0.1ms. I.e. the algorithm starts 0.8ms after the last poll and has to finish 0.1ms before the next poll.
So we can cope also with a little jitter in the USB polling.



## Host's Poll Time

The firmware will show the current used USB polling rate in 2 ways.
1. On every USB poll it toggles the output of the USB_POLL_OUT output.
If you attach an oscilloscope you can see the the polling interval directly.
I.e. a 1ms polling looks like this:
![](Images/usb_poll_1ms.BMP)
2. The main LED (on the Teensy board) will be toggled depending on the USB poll interval. Every 1000th USB poll the LED will be toggled. I.e. for a 1ms interval the LED will toggle every 1 second. For 8ms USB interval the LED will toggle every 8 seconds.

Note: The LED and the USB poll digital out will be used only if the joystick endpoint is polled.
E.g. on Linux you may find that the main LED is not blinking if you connect the device to USB. You need to actually read from the joystick device (/dev/input/js?) e.g. by opening a joystick test program (e.g. jstest-gtk).


## USB packet queue

In the Teensy it doesn't seem to be easily possible to detect when the host does an USB poll.
One way is to send a packet and check when the packet has been read (usb_tx_packet_count).
Unfortunately this involves that a packet has to be sent even if no new data is available.

Furthermore the library uses a queue of 3 packets (TX_PACKET_LIMIT).
If we simply write into the queue we end up in a delay of 3ms.
So the SW makes sure that always only 1 packet is used. I.e. the packet is immediately sent in the next USB poll.


# Errors

You hopefully never see an error. The errors here are created by program ASSERTs. I.e. they are helpful during debugging but should not occur under normal conditions.

When an error occurs the main LED starts blinking fast.

From that point on the joystick is not reporting any axes information or button presses anymore until you re-connect it to USB.

Thus, as long as the joystick is working you can be sure of it's guaranteed lag time.



# Misc

The branch 'joystick_and_output' contains SW that allows to use the Teensy as joystick and additionally configure ports for digital output.

I'm not supporting this anymore because I decided to split this in 2 separate git repositories, see the 'usbout' git repository.


# Building

## Prerequisites:

You need to have Arduino installed and also the Teensyduino add-on (https://www.pjrc.com/teensy/teensyduino.html).

Furthermore this USB device uses a type with an USB poll interval that is not yet existing in Teensyduino.
Therefore you need to  modify the USB description in file "...your arduino installation.../Java/hardware/teensy/avr/cores/teensy3/usb_desc.h".

You have to search for the exact path for your OS. Under macos it is usually:
/Applications/Arduino.app/Contents/Java/hardware/teensy/avr/cores/teensy3/usb_desc.h".

Add the following lines
~~~c++
#elif defined(USB_FASTEST_JOYSTICK)
  #define VENDOR_ID		0x16C0
  #define PRODUCT_ID		0x0487
  #define DEVICE_CLASS		0xEF
  #define DEVICE_SUBCLASS	0x02
  #define DEVICE_PROTOCOL	0x01
  #define MANUFACTURER_NAME	{'M','a','z','i','a','c'}
  #define MANUFACTURER_NAME_LEN	6
  #define PRODUCT_NAME		{'F','a','s','t','e','s','t','J','o','y','s','t','i','c','k'}
  #define PRODUCT_NAME_LEN	15
  #define EP0_SIZE		64
  #define NUM_ENDPOINTS		4
  #define NUM_USB_BUFFERS	30
  #define NUM_INTERFACE		3
  #define CDC_IAD_DESCRIPTOR	1
  #define CDC_STATUS_INTERFACE	0
  #define CDC_DATA_INTERFACE	  1	// Serial
  #define CDC_ACM_ENDPOINT	    1
  #define CDC_RX_ENDPOINT       2
  #define CDC_TX_ENDPOINT       3
  #define CDC_ACM_SIZE          16
  #define CDC_RX_SIZE           64
  #define CDC_TX_SIZE           64
  #define JOYSTICK_INTERFACE    2	// Joystick
  #define JOYSTICK_ENDPOINT     4
  #define JOYSTICK_SIZE         12	//  12 = normal, 64 = extreme joystick
  #define JOYSTICK_INTERVAL     1
  #define ENDPOINT1_CONFIG	ENDPOINT_TRANSMIT_ONLY
  #define ENDPOINT2_CONFIG	ENDPOINT_RECEIVE_ONLY
  #define ENDPOINT3_CONFIG	ENDPOINT_TRANSMIT_ONLY
  #define ENDPOINT4_CONFIG	ENDPOINT_TRANSMIT_ONLY
~~~
to the file e.g. you can add it just before
~~~c++
#elif defined(USB_TOUCHSCREEN)
~~~

Then you need to add this configuration to the Arduino/Teensy IDE UI.
Locate  "..../Java/hardware/teensy/avr/boards.txt" and add the following lines:
~~~
teensyLC.menu.usb.hidjoystick=Fastest Joystick
teensyLC.menu.usb.hidjoystick.build.usbtype=USB_FASTEST_JOYSTICK
~~~
e.g. just before the line
~~~
teensyLC.menu.usb.touch=Keyboard + Touch Screen
~~~

If the Arduino app was running you need to restart it to take effect.


## Build the SW:
1. Start Arduino (incl. Teensyduino)
2. Open file "FastestJoystick.ino"
3. Select "Tools->Boards" = "Teensy LC"
4. Select "Tools->USB Type" = "Fastest Joystick"
5. Connect the Teensy LC via USB
6. Verify (compile) and Upload the sketch


## Testing:

Connect the device to USB and use some SW for testing USB controllers.
On Linux you can use "jstest-gtk", on macos you can use "Controllers Lite".
On Windows you can use the Windows USB game controller application.

Press some attached buttons or just short-circuit and input to ground and you see an reaction in your USB controller SW.


## Configuration:

### Pins:
The button pins and the axes pins can be configured.
This is done in the configuration section in FastestJoystick.ino via the arrays
~~~c++
uint8_t buttonPins[] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 11, 12 };
uint8_t axesPins[] = { 20, 21,  18, 19 };
~~~

### Poll time output:

If necessary you can reconfigure the main LED with ```MAIN_LED_PIN```.

You can also directly output the USB polling to a pin (this is disabled by default):
~~~c++
//#define USB_POLL_OUT_PIN  14
~~~
If you do so the pin is toggled each time the host does an USB poll. I.e. when connecting an oscilloscope you can directly see the polling rate.


### Debouncing:
Debouncing or "minimum press time" can be configured via
~~~c++
uint16_t MIN_PRESS_TIME = 25;
~~~

### Misc:
ANALOG_AXES_ENABLED should always be 0. This is not implemented.

