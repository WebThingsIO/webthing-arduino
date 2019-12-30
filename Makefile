#!/usr/bin/make -f
# SPDX-License-Identifier: MPL-2.0
#{
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/
#}

default: rule/help

topdir?=${CURDIR}
extra_dir?=${topdir}/tmp

arduino_version?=1.8.5
arduino_host?=linux32
arduino_url?=https://downloads.arduino.cc/arduino-${arduino_version}-${arduino_host}.tar.xz
ARDUINO_DIR?=${extra_dir}/arduino-${arduino_version}

arduino_mk_url?=https://github.com/sudar/Arduino-Makefile
ARDMK_DIR?=${extra_dir}/arduino-mk


rule/help:
	@echo "# Usage:"
	@echo "# make rule/setup # Will install tools and deps"
	@echo "# make rule/all # Will build all examples"

${ARDUINO_DIR}:
	mkdir -p ${@D}
	cd ${@D} && curl ${arduino_url} | tar -xJ

rule/arduino_dir: ${ARDUINO_DIR}
	ls $<

${ARDMK_DIR}:
	mkdir -p ${@D}
	git clone --recursive --depth 1 ${arduino_mk_url} $@

rule/arduino_mk_dir: ${ARDMK_DIR}
	ls $<

#{ Libraries
USER_LIB_PATH?=${extra_dir}/Arduino/libraries

ArduinoJson_url?=https://github.com/bblanchon/ArduinoJson
ArduinoJson_dir?=${extra_dir}/Arduino/libraries/ArduinoJson
ArduinoJson_version?=v6.13.0
arduino_lib_dirs+=${ArduinoJson_dir}

${ArduinoJson_dir}:
	@mkdir -p ${@D}
	git clone --depth 1 --recursive --branch ${ArduinoJson_version} ${ArduinoJson_url} $@

ArduinoMDNS_url?=https://github.com/arduino-libraries/ArduinoMDNS
ArduinoMDNS_dir?=${extra_dir}/Arduino/libraries/ArduinoMDNS
ArduinoMDNS_version?=master # TODO pin version
arduino_lib_dirs+=${ArduinoMDNS_dir}

${ArduinoMDNS_dir}:
	@mkdir -p ${@D}
	git clone --depth 1 --recursive ${ArduinoMDNS_url} $@

ArduinoWiFi101_url?=https://github.com/arduino-libraries/WiFi101
ArduinoWiFi101_dir?=${extra_dir}/Arduino/libraries/WiFi101
ArduinoWiFi101_version?=0.15.3
arduino_lib_dirs+=${ArduinoWiFi101_dir}

${ArduinoWiFi101_dir}:
	@mkdir -p ${@D}
	git clone --depth 1 --recursive --branch ${ArduinoWiFi101_version} ${ArduinoWiFi101_url} $@

Adafruit_GFX_url?=https://github.com/adafruit/Adafruit-GFX-Library
Adafruit_GFX_version?=1.5.6
Adafruit_GFX_dir=${extra_dir}/Arduino/libraries/Adafruit_GFX
arduino_lib_dirs+=${Adafruit_GFX_dir}

${Adafruit_GFX_dir}:
	@mkdir -p ${@D}
	git clone --depth 1 --recursive --branch ${Adafruit_GFX_version} ${Adafruit_GFX_url} $@


Adafruit_SSD1306_url?=https://github.com/adafruit/Adafruit_SSD1306
Adafruit_SSD1306_version?=1.3.0
Adafruit_SSD1306_dir=${extra_dir}/Arduino/libraries/Adafruit_SSD1306
arduino_lib_dirs+=${Adafruit_SSD1306_dir}

${Adafruit_SSD1306_dir}:
	@mkdir -p ${@D}
	git clone --depth 1 --recursive --branch ${Adafruit_SSD1306_version} ${Adafruit_SSD1306_url} $@

rule/arduino_lib_dirs: ${arduino_lib_dirs}
	ls $^
#}

rule/setup: rule/arduino_dir rule/arduino_mk_dir rule/arduino_lib_dirs
	sync

rule/all: $(wildcard examples/*/Makefile | sort)
	for file in $^; do \
  dirname=$$(dirname -- "$${file}") ; ${MAKE} -C $${dirname} || exit $$? ; \
 done
