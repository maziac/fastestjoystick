# Fastest Joystick

**Note: This is still work in progress. I.e. not finished.**

The idea to write the firmware for an USB controller raised in me when doing the USB controller measurements in 
[LagMeter](../LagMeter/Readme.md).
It is quite invisible what a usual game controller does when sampling the buttons and the axis data and what delay it might add.
The only information one gets is the suggested polling rate that the game controller requests from the USB host.
But this is only part of the story: the firmware of the game controller need to sample the joystick data and prepare it in time, i.e. within this polling interval.
Furthermore I wanted to have control on the debouncing and minimum press time for the buttons.
So the only way to get absolute control on what is happening on the firmware side is to write it by myself.

This project is about the firmware of a USB controller. It's not about the HW. So, if you would want to do this on your own you can use an Arcade stick or buttons or use the HW of an existing USB game controller, throw it's electronics away and substitute it with a Teensy board and this SW.

**The goal of this project is to create the fastest USB game controller firmware available with a guaranteed reaction time.** 

The features are:
- Requested 1ms USB poll time
- Additional delay of max. 200us.
- I.e. in total this is a reaction time in the range of 0.2ms to 1.2ms.
- Indication of the real used USB poll time

For comparison:
A typical game controller often has a poll time of 8ms. This in itself leads to a reaction time of 8 to 16ms. Sometimes the controller itself has another delay that needs to be added. So this not only jitters a lot, it is already quite close to 1 frame delay.
Even faster game controller with 1ms poll time introduce an inherent delay of about 5ms.
(I only heard of 1 USB controller with a 1ms delay but unfortunately I lost the link.)


# HW

Used HW is a cheap Teensy LC board that you can get around 10€.

The pinout is used as following (but can be changed to fit your needs):
![](Images/TeensyLCSchematics.png)

Note: All pinouts I found for the axes JST connector were different, so I have chosen the pinout above. If you need a different one you just have to change the wiring.


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
- Since I'm optimizing for a game's poll time of 20ms (europe, US would be 17ms) and the minimum achievable time is 14ms, the time need to be extended to at lease 20ms + USB poll time + firmware delay, so approx. 22ms.
Note: If you have lower game poll time's, e.g. if you have higher screen update rates, then you should adapt this value (MIN_PRESS_TIME).
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
If this happens: the main LED and the button LEDs will start to blink fast.

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
There is also a serial device implemented. If this is used without the joystick being used at the same time the LED may not blink.
E.g. on Linux you may find that the main LED is not blinking if you just send commands to the serial device.


## USB packet queue

In the Teensy it doesn't seem to be easily possible to detect when the host does an USB poll.
One way is to send a packet and watch when the packet has been read (usb_tx_packet_count).
Unfortunately this involves that a packet has to be sent even if no new data is available.

Furthermore the library uses a queue of 3 packets (TX_PACKET_LIMIT).
If we simply write into the queue we end up in a delay of 3ms.
So the SW makes sure that always only 1 packet is used. I.e. the packet is immediately sent in the next USB poll.


# Functionality

## Digital Out

Of course the joystick can transmit button presses and axis changes via USB to the PC.
But it is also capable to turn on/off certain digital outputs controlled by the PC (USB host).
This can be useful to turn on/off the LED light of some of the buttons. e.g. one could enlighten only those buttons that really have a function in the particular game. (See also the LEDBlinky project).

This is done through serial communication. The joystick additionally opens up a serial communication with the PC.
with a simple command it is possible to turn an output on or off, e.g. "o2=100" would turn the digital out 2 (DOUT2) to 100%.

On mac can find the serial device e.g. with:
~~~
$ ls /dev/cu*
$ /dev/cu.Bluetooth-Incoming-Port	/dev/cu.usbmodem56683601
~~~
It's a little bit unclear how the naming is done but it is clear that we are not the Bluetooth device so the right device is:
/dev/cu.usbmodem56683601

To turn on DOUT0 use:
~~~
$ echo o0=100 > /dev/cu.usbmodem56683601
~~~

The full syntax for this command is:

oN=X[,attack[,delay] : Set output N to X (0-100) with an 'attack' time and a delay.
'attack' means that the digital out will change smoothly from its current value to the target (X) within the 'attack' time.
'delay' adds an additional delay before 'attack' starts.
All times are in ms.
If you omit the values 0 is assumed for 'attack' and 'delay' and the value of the digital out is changed immediately.

PWM is used to allow to "dim" the digital outputs. It works with a frequency of 100kHz to avoid flickering.



## Debug Commands

There are a few more commands that you should not need to use unless you are going to make own changes to the SW.
Often these command will result in some output.
The output can be read from the same device (see above), e.g. in another terminal use:
~~~
$ cat /dev/cu.usbmodem56683601
~~~

Commands:
- o : Change digital out. See above.
- r : Reset. e.g. echo r > /dev/cu.usbmodem56683601
- t : Test the blinking. This will let the joystick (all digital outs + main LED) blink at the same rate as if an error had occurred.
  If you are unsure how it looks like when an error occurs you can use the "t" command.
  Please note: To get into normal mode afterwards you have to the "r".
- p=Y : Change minimum press time to Y (in ms), e.g. "p=25". After startup the minimum press time is set to a default value (MIN_PRESS_TIME, please look the exact value in the sources, it is about 25ms). If you would like to test other values you can do with this command. To test the functionality you can even use extraordinary high numbers like 2000 for 2 secs. This value cannot be used in a game, but you can test the behavior.
Note: To go back to the default value use "r".
- i : Info. Print version number, min. press time and used times. The 'used times' measures 3 times:
    - Max. time serial:   The max. time used to read the serial bus (i.e. the commands).
    - Max. time joystick: The max. time used to read the joystick values (buttons and axes).
    - Max. time total:    The max. total time used. this needs to be below 900us otherwise an error is shown.
    Note: this info is printed even if debug mode is off.
- d=X: Turn debug mode on/off (1/0). The default is off.
This exists as the serial out of the Teensy sometimes uses a substantial amount of time. This would violate the time check for the button/axis algorithm and lead to an error.
So I decided to skip the serial output completely in normal mode.
But if you need serial out for debugging you can turn it on here. I.e. you will get a confirmation for the changes to the digital outputs. 
But don't use this in normal operation as the response time of 0.2 to 1.2 ms are not guaranteed anymore if ON.


## Errors

You hopefully never see an error. The errors here are created by program ASSERTs. I.e. they are helpful during debugging but should not occur under normal conditions.

When an error occurs the main LED and all digital outs start blinking fast. To get an idea what this looks like you can send a "t" command through the serial device.
At the same time the joystick will send the ASSERT text through the serial device so that you can read it on the PC e.g. with
~~~
$ cat /dev/cu.usbmodem56683601
~~~
Since the line is buffered you can also use this command after the error happened.

Errors (ASSERTs) may occur when either the algorithm to read the joystick would take too long (>900us) or the USB queue is filled.
In both cases the guaranteed delay time would be compromised.

So, in reverse, as long as you don't see any error you know that the joystick is answering fast.

Note 1: Once an error has occurred it stops reading the joystick and transmitting joystick data. You need to reset the device, either by powering down or by sending "r" on the serial, before you can use it again.

Note 2: You don't have to fear that an error happens during usage. The routines are fast enough to always stay below the limit. This is just a tool for debugging and also a constant check that the delay is as low as possible.

Note 3: In debug mode ("-d=1") the check is turned off.


