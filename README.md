horrorophone-eclipse-with-makefile
==================================
This is a sound gadget for STM32F4 Discovery board from STmicroelectronics.
It is called "horrorophone" because it could make sounds for old horror movies...

You can watch a demo here : http://youtu.be/pwFJzrbW1aI

Press the user button and you will hear a sequence of random notes
produced by a sawtooth oscillator.
A pot will control the speed of the sequence.
There is also a fixed delay effect.

The oscillator is an alias-free minBLEP based generator :
Thanks to Sean Bolton (blepvco), Fons Adriaensen, Eli Brandt, ... for their work !
Wire a potentiometer on PC2 to vary speed : [GND, PC2, 3V]

This project was compiled with GNU toolchain under Windows Vista :
- arm-none-eabi-gcc 4.6 2012q4 from launchpad.net : https://launchpad.net/gcc-arm-embedded
- make from yagarto tools : http://www.yagarto.de/download/yagarto/yagarto-tools-20121018-setup.exe
- eclipse CDT 4.2.1

Beware : it's not beautiful C code !