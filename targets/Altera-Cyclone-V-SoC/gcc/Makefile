# Makefile for the Altera Cyclone V SoC target.
# Copyright (c) Andras Zsoter 2016.
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.

# Usage:
# make clean ; make EXAMPLE=timeshare
# make clean ; make EXAMPLE=two-hellos # Default.

# The assumption is that make is run from the command shell provided as part of the Altera software development / Quartus environment.
# Various environment variables are assumed to be set up as default in the shell.

ALTERA_ROOT := $(SOCEDS_DEST_ROOT)
ALTERA_HWLIB := $(ALTERA_ROOT)/ip/altera/hps/altera_hps/hwlib

DEVICE_DIR := ./targets/Altera-Cyclone-V-SoC/gcc
RTOS_DIR := ./rtos
EXTRA_DIR := ./extras
EXAMPLES_DIR := ./examples

FPGA_DESIGN=soc_cv_av

MKIMAGE := mkimage
ifndef EXAMPLE
EXAMPLE = two-hellos
endif

BUILD_DIR = build
APP_DIR = $(EXAMPLES_DIR)/$(EXAMPLE)

MULTILIBFLAGS := -mfloat-abi=softfp -mtune=cortex-a9 -mcpu=cortex-a9 -march=armv7-a
INCLUDE_FLAGS := -I $(RTOS_DIR) -I$(APP_DIR) -I $(DEVICE_DIR) -I$(EXAMPLES_DIR) -I$(EXTRA_DIR) -I$(ALTERA_HWLIB)/include \
	-I$(ALTERA_HWLIB)/include/$(FPGA_DESIGN) -D $(FPGA_DESIGN) -D ALT_INT_PROVISION_VECTOR_SUPPORT=0

# CFLAGS := $(MULTILIBFLAGS) $(INCLUDE_FLAGS) -g -O0 -Werror -Wall -Wstrict-prototypes
CFLAGS := $(MULTILIBFLAGS) $(INCLUDE_FLAGS) -g -O1  -Wall -Wstrict-prototypes

LDFLAGS := -Xlinker --defsym=__cs3_isr_irq=rtos_Isr_Handler -Xlinker --defsym=__cs3_isr_swi=rtos_Invoke_Scheduler

CROSS_COMPILE := arm-altera-eabi-
CC := $(CROSS_COMPILE)gcc
NM := $(CROSS_COMPILE)nm
OD := $(CROSS_COMPILE)objdump
OBJCOPY := $(CROSS_COMPILE)objcopy


ifeq ($(or $(COMSPEC),$(ComSpec)),)
RM := rm -rf
else
RM := cs-rm -rf
endif

LINKER_SCRIPT := $(ALTERA_ROOT)/host_tools/mentor/gnu/arm/baremetal/arm-altera-eabi/lib/cycloneV-dk-ram.ld

OBJ = main.o board.o alt_16550_uart.o alt_clock_manager.o

# ARM/Altera tools call the executable .axf not .elf.
# Even though we are using gcc here, keep that terminology.
ELF = $(BUILD_DIR)/rtos.axf
IMG = $(BUILD_DIR)/rtos.img

default:	all

-include $(APP_DIR)/app.src
SRC += $(EXAMPLES_DIR)/utility.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/alt_16550_uart.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/alt_timers.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/alt_watchdog.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/alt_globaltmr.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/alt_interrupt.c
SRC += $(ALTERA_HWLIB)/src/hwmgr/$(FPGA_DESIGN)/alt_clock_manager.c

FILES = $(notdir $(SRC) )
PATHS = $(sort $(dir $(SRC) ) )

OBJ = $(addprefix $(BUILD_DIR)/, $(FILES:.c=.o))
DEP = $(OBJ:.o=.d)

vpath %.c $(PATHS)

default:	$(IMG)


$(ELF): $(BUILD_DIR) $(OBJ)
	$(CC) -T$(LINKER_SCRIPT) $(LDFLAGS) $(MULTILIBFLAGS) $(OBJ) -o $@
	$(OD) -D $@ > $@.objdump
	$(NM) $@ > $@.map


$(IMG): $(BUILD_DIR) $(ELF)
	$(OBJCOPY) -O binary $(ELF) $(BUILD_DIR)/rtos.bin
	$(MKIMAGE) -A arm -O u-boot -T standalone -C none -a 0x100000 -e 0x100000 -n 'JaeOS' -d $(BUILD_DIR)/rtos.bin $(IMG)

all: $(IMG)

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/%.o:%.c
	$(CC) $(CFLAGS) $(APP_FLAGS) -c $< -o $@
	$(CC) $(CFLAGS) -MM  -MT $@ $< > $(patsubst %.o,%.d,$@)

clean:
	$(RM) $(BUILD_DIR)/*

