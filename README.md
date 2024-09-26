# n76e003_leds_remote
n76 controll leds using IR remote and buttons, manage power and battery voltage

- work in progres -

n76 register definitions file from:
https://github.com/erincandescent/libn76

compiler: sdcc
	debian variants: can be intalled from debian repositories 

change used pins:
	- set pin functions in main
	- use pin in the apropiate file
	- some pins use interrupts (one is IR) and interrupt number needs to be changed according to datasheet of micro
