#!/usr/bin/make -f
# SPDX-License-Identifier: MPL-2.0
#{
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/
#}

default: all
	@echo "# $@: $^"

all: rule/setup

top_reldir?=../..
topdir?=${CURDIR}/${top_reldir}
-include ${topdir}/Makefile

AVR_TOOLS_DIR?=${ARDUINO_DIR}/hardware/tools/avr
ARDUINO_DIR?=/usr/share/arduino
ARDMK_DIR?=${ARDUINO_DIR}
arduino_mk?=${ARDMK_DIR}/Arduino.mk

#{ Config part
VARIANT?=mega
BOARD_TAG=mega2560
BOARD_SUB=
MCU?=at${BOARD_TAG}
F_CPU?=16000000L
MONITOR_PORT?=$(shell ls /dev/ttyUSB* /dev/ttyACM* | head -n1)
#}

ARCHITECTURE=avr
AVRDUDE_ARD_BAUDRATE?=115200
MONITOR_BAUDRATE?=${AVRDUDE_ARD_BAUDRATE}
AVRDUDE_ARD_PROGRAMMER?=wiring

CPPFLAGS+=-I${top_reldir}

ARDUINO_LIBS += Ethernet SPI
ARDUINO_LIBS += ArduinoMDNS
ARDUINO_LIBS += ArduinoJson

-include ${arduino_mk}
