# 2017FIRSTScouting
There are two parts to this:

1. The .ino with the Arduino code. I'll also include a schematic of the completed Arduino setup and pictures!
	The code has two state machines in it, a master that controls the state of the device, whether it's in data entry mode, autonomous mode, tele-op mode, data sync mode, and wait mode. The second machine handles the scoring. It changes modes whether it's in autonomous mode or tele-op mode.

2. The Python script that interfaces with the serial port on the Arduino.
	This is forthcoming