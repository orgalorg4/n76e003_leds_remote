.PHONY: all program clean

FILE = lamp_remote

OUTDIR = build/

REGS = ./include

SRC1 = main
SRC2 = infrared
SRC3 = ws2812
SRC4 = power_manage
SRC5 = battery
SRC6 = eeprom
SRC7 = buttons
SRC8 = outmux

CC = sdcc

FLASHER = /mnt/BarraCuda/3_workspaces/8051/nvtispflash/nvtispflash

all: $(OUTDIR)$(FILE).bin

$(OUTDIR)$(FILE).ihx: $(SRC1).c $(SRC2).c $(SRC3).c $(SRC4).c $(SRC5).c $(SRC6).c $(SRC7).c $(SRC8).c
	mkdir -p $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC8).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC7).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC6).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC5).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC4).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC3).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC2).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 -c $(SRC1).c -I $(REGS) -o $(OUTDIR)
	$(CC) -mmcs51 --out-fmt-ihx -o $(OUTDIR)$(FILE).ihx $(OUTDIR)$(SRC1).rel $(OUTDIR)$(SRC2).rel $(OUTDIR)$(SRC3).rel $(OUTDIR)$(SRC4).rel $(OUTDIR)$(SRC5).rel $(OUTDIR)$(SRC6).rel $(OUTDIR)$(SRC7).rel $(OUTDIR)$(SRC8).rel 

$(OUTDIR)$(FILE).bin: $(OUTDIR)$(FILE).ihx
	makebin -p $(OUTDIR)$(FILE).ihx $(OUTDIR)$(FILE).bin

program: $(OUTDIR)$(FILE).bin
	$(FLASHER) -d /dev/ttyUSB0 -a $(OUTDIR)$(FILE).bin

clean:
	rm $(OUTDIR)*.asm $(OUTDIR)*.bin $(OUTDIR)*.ihx $(OUTDIR)*.lk $(OUTDIR)*.lst $(OUTDIR)*.map $(OUTDIR)*.mem $(OUTDIR)*.rel $(OUTDIR)*.rst $(OUTDIR)*.sym
