# ESP32 Mini BT Speaker

This is an ESP32 based mini bluetooth speaker. It is based on the ESP bluetooth audio library by Phil Schatzman https://github.com/pschatzmann/ESP32-A2DP.git . The speaker uses an ESP32 Thing Plus to receive bluetooth audio which it sends via i2s to an amplifier module. It features a push button to turn the speaker on/off, a touch sensor to play/pause whatever audio is playing, volume up/down, and a relatively large battery. The cabinet is 3D printed and can be printed on small build plates as it's only 150mm wide! I have an ender 2 pro which I used.

A speaker like this could be built using a simple bluetooth amplifier board, but you don't get access to play/pause commands and LED indicators in the same way. Additionally, fast forward / prev track, is also possible using the bluetooth library if you want to implement that in your own speaker.
