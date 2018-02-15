#Change this variable to point to your Arduino device
#Mac - it may be different
DEVICE = /dev/tty.usbmodem1411

#Linux (/dev/ttyACM0 or possibly /dev/ttyUSB0)
#DEVICE = /dev/ttyACM0

#Windows
#DEVICE = COM3

#default target to compile the code
program2: program2.c os.c serial.c
	avr-gcc -std=c99 -mmcu=atmega2560 -DF_CPU=16000000 -O2 -o main.elf os.c serial.c program2.c
	avr-objcopy -O ihex main.elf main.hex
	avr-size main.elf

assemble: os.c
	avr-gcc -std=c99 -mmcu=atmega2560 -S -DF_CPU=16000000 -O2 -o main.s os.c


#flash the Arduino with the program
program: main.hex
	#Mac
	avrdude -D -pm2560 -P $(DEVICE) -c wiring -F -u -U flash:w:main.hex

#remove build files
clean:
	rm -fr *.elf *.hex *.o
