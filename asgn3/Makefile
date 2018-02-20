#Change this variable to point to your Arduino device
#Mac - it may be different
DEVICE = /dev/tty.usbmodem1411
CC = avr-gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -Wstrict-overflow -fno-strict-aliasing \
-Wno-missing-field-initializers -Wno-unused-parameter
AVRFLAGS = -mmcu=atmega2560 -DF_CPU=16000000
OFLAGS = -O2

#Linux (/dev/ttyACM0 or possibly /dev/ttyUSB0)
#DEVICE = /dev/ttyACM0

#Windows
#DEVICE = COM3

#default target to compile the code
program_3: program3.c os.c serial.c synchro.c
	$(CC) $(CFLAGS) $(AVRFLAGS) $(OFLAGS) -o main.elf $^
	avr-objcopy -O ihex main.elf main.hex
	avr-size main.elf

assemble: os.c
	$(CC) $(CFLAGS) $(AVRFLAGS) $(OFLAGS) -S -o main.s $<


#flash the Arduino with the program
program: main.hex
	#Mac
	avrdude -D -pm2560 -P $(DEVICE) -c wiring -F -u -U flash:w:main.hex

#remove build files
clean:
	rm -fr *.elf *.hex *.o
