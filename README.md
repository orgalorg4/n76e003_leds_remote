# n76e003_leds_remote
n76 controll leds using IR remote and buttons, manage power and battery voltage

== work in progres ==

n76 register definitions file from:
https://github.com/erincandescent/libn76

compiler: sdcc
	debian variants: can be intalled from debian repositories 

buitd code:
- install sdcc as package or download the archive from their site
- set path in makefile (only for archive) in variable CC
- open a terminal in the same directory and type make all

upload code to board:
- get nvtispflash: https://github.com/frank-zago/nvtispflash
- set path in makfile to nvtispflash, variable FLASHER
- to solve 
"error while loading shared libraries: libserialport.so.0: cannot open shared object file: No such file or directory"
install sigrok (it includes libserial)
- $ make program


change used pins:
- set pin functions in main
- use pin in the apropiate file
- some pins use interrupts (one is IR) and interrupt number needs to be changed according to datasheet of micro
