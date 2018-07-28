#!/bin/echo docker build . -f
# -*- coding: utf-8 -*-
#{
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/ .
#}

FROM i386/debian:stable
MAINTAINER Philippe Coval (p.coval@samsung.com)

ENV DEBIAN_FRONTEND noninteractive
ENV LC_ALL en_US.UTF-8
ENV LANG ${LC_ALL}

RUN echo "#log: Configuring locales" \
  && set -x \
  && apt-get update -y \
  && apt-get install -y locales \
  && echo "${LC_ALL} UTF-8" | tee /etc/locale.gen \
  && locale-gen ${LC_ALL} \
  && dpkg-reconfigure locales \
  && sync

ENV project webthing-arduino

RUN echo "#log: ${project}: Setup system" \
  && set -x \
  && apt-get update -y \
  && apt-get install -y \
  curl \
  git \
  make \
  sudo \
  xz-utils \
  && apt-get clean \
  && sync

ADD . /usr/local/src/${project}/${project}
WORKDIR /usr/local/src/${project}/${project}
RUN echo "#log: ${project}: Preparing sources" \
  && set -x \
  && make rule/setup \
  && sync

ADD . /usr/local/src/${project}/${project}
WORKDIR /usr/local/src/${project}/${project}
RUN echo "#log: ${project}: Preparing sources" \
  && set -x \
  && make rule/all \
  && sync
