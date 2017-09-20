# Multimedia Center PreAmpBoard firmware

I2C interface for signal routing etc. provided by a Texas Instruments MSP430G2553 MCU.

For my [Multimedia Center project](https://github.com/terjeio/MultimediaCenter/).

I2CAddress: 0x4A

#### Set signal routing

* First byte: 0x01
* Second byte: routing bitfield

bit2|bit1|bit0| Input select
----|----|----|-------------
0|0|0|AUX (phono)
0|0|1|CD
0|1|0|TUNER
0|1|1|CD
1|x|x|no change

bit5|bit4|bit3|Tape monitor
----|----|----|------------
0|0|0|no change
0|0|1|Tape 1 monitor
0|1|0|Tape 2 monitor
0|1|1|no change
1|x|x|no change

bit7|bit6|Tape copy
----|----|---------
0|0|no change
0|1|Tape copy 1 > 2
1|0|Tape copy 2 > 1
1|1|no change

#### Mute

* First byte: 0x02 - mute
* Second byte: 0 - unmute, > 0 - mute

#### Power amp on/off relay drive

* First byte: 0x03 - standby
* Second byte: 0 - relay drive on, > 0 - relay drive off

#### Reset DAB radio module

* 0x04 - drives reset pin low for 100 ms

---

This project can be directly imported into TI's cloud based IDE available at [dev.ti.com](https://dev.ti.com), there is no need to download and set up a local development environment in order to compile and upload code.

A [MSP-EXP430G2 LaunchPad](http://www.ti.com/tool/MSP-EXP430G2) can be used as a programmer, remove the MCU from the launchpad and connect P18 \(SBW\) to the corresponding pins on the launchpad.
