################################################################################
 # Copyright (C) 2016 Maxim Integrated Products, Inc., All Rights Reserved.
 # Ismail H. Kose <ismail.kose@maximintegrated.com>
 # Permission is hereby granted, free of charge, to any person obtaining a
 # copy of this software and associated documentation files (the "Software"),
 # to deal in the Software without restriction, including without limitation
 # the rights to use, copy, modify, merge, publish, distribute, sublicense,
 # and/or sell copies of the Software, and to permit persons to whom the
 # Software is furnished to do so, subject to the following conditions:
 #
 # The above copyright notice and this permission notice shall be included
 # in all copies or substantial portions of the Software.
 #
 # THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 # OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 # MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 # IN NO EVENT SHALL MAXIM INTEGRATED BE LIABLE FOR ANY CLAIM, DAMAGES
 # OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 # ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 # OTHER DEALINGS IN THE SOFTWARE.
 #
 # Except as contained in this notice, the name of Maxim Integrated
 # Products, Inc. shall not be used except as stated in the Maxim Integrated
 # Products, Inc. Branding Policy.
 #
 # The mere transfer of this software does not imply any licenses
 # of trade secrets, proprietary technology, copyrights, patents,
 # trademarks, maskwork rights, or any other form of intellectual
 # property whatsoever. Maxim Integrated Products, Inc. retains all
 # ownership rights.
 #
 # $Date: 2016-03-23 13:28:53 -0700 (Wed, 23 Mar 2016) $
 # $Revision: 22067 $
 #
 ###############################################################################

# Maxim ARM Toolchain and Libraries
# https://www.maximintegrated.com/en/products/digital/microcontrollers/MAX32630.html

# This is the name of the build output file
PROJECT=inject_project

# Specify the target processor
TARGET=MAX3263x
PORT_NAME=max32630-fthr
PROJ_CFLAGS+=-DRO_FREQ=96000000

# Create Target name variables
TARGET_UC:=$(shell echo $(TARGET) | tr a-z A-Z)
TARGET_LC:=$(shell echo $(TARGET) | tr A-Z a-z)

# Select 'GCC' or 'IAR' compiler
COMPILER=GCC
PYTHON=python

cc256x_init_script=bluetooth_init_cc2564B_1.8_BT_Spec_4.1.c

# A project must provide BTSTACK_ROOT.
# Relative works only when in port/max32630-fthr/example/some-project/Makefile
BTSTACK_ROOT ?= ../../../..
BTSTACK_ROOT := $(realpath /$(subst \,/,$(subst :,,$(BTSTACK_ROOT))))
PORT_DIR = ${BTSTACK_ROOT}/port/${PORT_NAME}

ifeq "$(MAXIM_PATH)" ""
MAXIM_PATH = $(realpath /$(subst \,/,$(subst :,,$(PORT_DIR))/maxim))
else
MAXIM_PATH := $(realpath /$(subst \,/,$(subst :,,$(MAXIM_PATH))))
endif

LIBS_DIR=$(MAXIM_PATH)/Firmware/$(TARGET_UC)/Libraries
CMSIS_ROOT=$(LIBS_DIR)/CMSIS

# Where to find source and header files for this test
VPATH = . ${PORT_DIR}/src
IPATH = . ${PORT_DIR}/src

# Source files for this test
SRCS = $(wildcard $(PORT_DIR)/src/*.c)

# Board files for the max32630fthr board
BOARD_DIR = ${LIBS_DIR}/Boards
ADAPT_BOARD_DIR = ${PORT_DIR}/board/max32630-fthr
include $(ADAPT_BOARD_DIR)/board.mk

# Enable assertion checking for development
PROJ_CFLAGS+=-DMXC_ASSERT_ENABLE

# Project specific makefile (<project_dir>/xxx.mk)
# - defines btstack deps (PROJECT_FEATURES)
# - defines local sources (extend IPATH, VPATH, and SRCS)
include $(wildcard *.mk)
include ${PORT_DIR}/example/template/Dependencies.mk

ifeq ($($(PROJECT)_deps),)
  $(error $(PROJECT)_deps None)
endif

PROJECT_DEPS = $($(PROJECT)_deps)
PROJECT_OBJS = $(PROJECT_DEPS:.c=.o)

SRCS += $(PROJECT_OBJS)
SRCS += $(PROJECT_DEPS)

#define gatt_h dependency before the target defining .mk include
all: inject_gatt_h

# Use this variables to specify and alternate tool path
#TOOL_DIR=/opt/gcc-arm-none-eabi-4_8-2013q4/bin

# Use these variables to add project specific tool options

# Opting to link agains -lc_nano to silence warnings from libnosys.
# Another option would be to implement _getpid, _kill, etc. with no attached warnings.
# For details see:
# https://metebalci.com/blog/demystifying-arm-gnu-toolchain-specs-nano-and-nosys/
PROJ_CFLAGS+=--specs=nano.specs
PROJ_LDFLAGS+=--specs=nano.specs

# Point this variable to a startup file to override the default file
#STARTUPFILE=start.S

# Point this variable to a linker file to override the default file
# LINKERFILE=$(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/GCC/$(TARGET_LC).ld

# Include the peripheral driver
PERIPH_DRIVER_DIR=$(LIBS_DIR)/$(TARGET_UC)PeriphDriver
include $(PERIPH_DRIVER_DIR)/periphdriver.mk

################################################################################
# Include the rules for building for this target. All other makefiles should be
# included before this one.
include $(CMSIS_ROOT)/Device/Maxim/$(TARGET_UC)/Source/$(COMPILER)/$(TARGET_LC).mk

EXAMPLE_BIN_DIR ?= ../bin

# Remove if you dont need to create and copy bin
all: bin

%.h: %.gatt
	$(PYTHON) ${BTSTACK_ROOT}/tool/compile_gatt.py $< $@

# Build and copy all $(PROJECT).bin to EXAMPLE_BIN_DIR
bin: $(BUILD_DIR)/$(PROJECT).bin
	$(info Copying $(PROJECT).bin to EXAMPLE_BIN_DIR: $(EXAMPLE_BIN_DIR))
	@mkdir -p $(EXAMPLE_BIN_DIR)
	@cp $(BUILD_DIR)/$(PROJECT).bin $(EXAMPLE_BIN_DIR)

rm-bin:
	rm -f $(EXAMPLE_BIN_DIR)/$(PROJECT).bin

rm-cc256x_init_script:
	$(MAKE) -f ${BTSTACK_ROOT}/chipset/cc256x/Makefile.inc BTSTACK_ROOT=${BTSTACK_ROOT} clean-scripts

rm-compiled-gatt-file:

clean: rm-compiled-gatt-file rm-cc256x_init_script rm-bin

# The rule to clean out all the build products.
distclean: clean
	$(MAKE) -C ${PERIPH_DRIVER_DIR} clean
