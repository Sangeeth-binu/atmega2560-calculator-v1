CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude

MCU = atmega2560
F_CPU = 16000000UL

SRC_DIR = src
INC_DIR = include

SRC = $(SRC_DIR)/main.c $(SRC_DIR)/uart.c
ELF = main.elf
HEX = main.hex

CFLAGS = -mmcu=$(MCU) -DF_CPU=$(F_CPU) -Os -Wall -Wextra -I$(INC_DIR)

all: $(HEX)

$(ELF): $(SRC)
	$(CC) $(CFLAGS) $^ -o $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

flash: $(HEX)
	$(AVRDUDE) -c wiring -p m2560 -P /dev/ttyUSB0 -b 115200 -D -U flash:w:$(HEX):i

clean:
	rm -f $(ELF) $(HEX)

