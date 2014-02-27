#  Part of Grbl
#  Copyright (c) 2009-2011 Simen Svale Skogsrud
#  Copyright (c) 2012 Sungeun K. Jeon
#  Copyright (c) 2014 Matthew D. Sorensen
#
#  Grbl is free software: you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  Grbl is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

# User configurable firmware setting:
CLOCK = 48000000

TEENSY_PATH = ~/498/teensy-toolchain
COMPILER = $(TEENSY_PATH)/hardware/tools/arm-none-eabi/bin
VENDOR = ./minimal-k20-env/vendor


CPPFLAGS = -Wall -g -Os -mcpu=cortex-m4 -mthumb -nostdlib -MMD -DF_CPU=$(CLOCK) -DUSB_SERIAL -I$(VENDOR) -D__MK20DX256__
CXXFLAGS = -std=gnu++0x -felide-constructors -fno-exceptions -fno-rtti
CFLAGS =
LDFLAGS = -Os -Wl,--gc-sections -mcpu=cortex-m4 -mthumb -T$(VENDOR)/mk20dx256.ld
LIBS = -lm
CC = $(COMPILER)/arm-none-eabi-gcc
CXX = $(COMPILER)/arm-none-eabi-g++
OBJCOPY = $(COMPILER)/arm-none-eabi-objcopy
SIZE = $(COMPILER)/arm-none-eabi-size

OBJECTS = parser.o parameters.o queue.o protocol/message_structs.o main.o hardware.o

VENDOR_C = $(wildcard $(VENDOR)/*.c)
VENDOR_OBJECTS = $(patsubst %.c,%.o,$(VENDOR_C))

%.hex: %.elf
	$(SIZE) $<
	$(OBJCOPY) -O ihex -R .eeprom $< $@


main.elf: $(OBJECTS) $(VENDOR_OBJECTS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) 

parser_test.elf: parser.o parameters.o parser_test.o protocol/message_structs.o $(VENDOR_OBJECTS) 
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS) 


-include $(OBJS:.o=.d)

all: main.hex

clean:
	rm -f *.o *.d *.elf *.hex

