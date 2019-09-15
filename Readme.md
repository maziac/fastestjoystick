# USB Teensy Joystick

**Note: This is still work in progress. I.e. not finished.**

The idea to write the firmware for an USB controller raise din me when doing the USB controller measurements in 
[LagMeter](../LagMeter/Readme.md).
It is quite invisible what a usual game controllers does when sampling the buttons and the axis data and what delay it might add.
The only information one gets is the suggested polling rate that the game controller requests from the USB host.
But this is only part of the story: the firmware of the game controller need to sample the joystick data and prepare it in time, i.e. within this polling interval.
Furthermore I wanted to have control on the debouncing and minimum press time.
So the only way to get absolute control on what is happening on the firmware side is to write it by myself.

This project is about the firmware of a USB controller. It's not about the HW. So, if you would want to do this on your own you can use an Arcade stick or buttons or use the HW of a USB controller, throw it's electronics away and substitute it with a Teensy board and this SW.

The features are:
- Requested 1ms USB poll time
- Additional delay of max. (??)
- Indication of the real used USB poll time

Used HW is a cheap Teensy LC board that you can get around 10€.


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
Note: If you have lower game poll time's, e.g. if you have higher screen update rates, then you should adapt his value.
But anyhow: this is only important for leaf switches. Micro switches anyway have a higher close-time.

Debouncing and minimum press time uses the same algorithm. It extends the simple main loop to:
1. For each button and axis:
    - If timer value is 0: Check if button or axis changed
        - If changed: Restart timer, use the new value.
    - If timer value is not 0: Reduce timer value
2. Send USB data / wait for poll
3. Goto 1



## Host's Poll Time

The firmware will show the current used USB polling rate in 2 ways.
1. On every USB poll it toggles the output of the USB_POLL_OUT output.
If you attach an oscilloscope you can see the the polling interval directly.
I.e. a 1ms polling looks like this:
![](Images/usb_poll_1ms.BMP)
2. The main LED (on the Teensy board) will be toggled depending on the USB poll interval. Every 1000th USB poll the LED will be toggled. I.e. for a 1ms interval the LED will toggle every 1 second. For 8ms USB interval the LED will toggle every 8 seconds.



/* REMOVE:
At startup the device will measure the polling time of the USB host.
The measured value is then output to the main LED:
- 1x blink = 1ms
- 2x blink = 2ms
- 4x blink = 4ms
- 8x blink = 8ms
- 5x fast blink 
The measurement is done only once.
The behavior very mich depends on the usb host.
On Mac you will see the blinking LED when you attach the device to the USB port.
On Linux it does not work: the right USB polling rate is used only if the device is really used.
As this is not the case directly after plugging it in the USB port the measurement fails.
On Windows (10) the USB poll measurement shows the right value on plug-in but the device itself is not recognized.

Another way to measure the poll rate is to use an oscilloscope at pin USB_POLL_OUT. It is toggle on each poll.
I.e. a 1ms polling looks like this:
![](Images/usb_poll_1ms.BMP)
*/
