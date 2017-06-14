#
# FILE: Makefile - sel4osapi makefile
#
# Copyright (c) 2015, Real-Time Innovations, Inc. All rights reserved.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#

# Targets
TARGETS := libsel4osapi.a

# Source files required to build the target
CFILES := $(patsubst $(SOURCE_DIR)/%,%,$(wildcard $(SOURCE_DIR)/src/*.c))

CFILES_SYSCLOCK := clock.c
ifeq ($(CONFIG_LIB_OSAPI_SYSCLOCK),)
  CFILES := $(filter-out $(CFILES_SYSCLOCK:%=src/%), $(CFILES))
endif

CFILES_NET := network.c \
			  udp.c
ifeq ($(CONFIG_LIB_OSAPI_NET),)
  CFILES := $(filter-out $(CFILES_NET:%=src/%), $(CFILES))
endif

CFILES_SERIAL := io.c
ifeq ($(CONFIG_LIB_OSAPI_SERIAL),)
  CFILES := $(filter-out $(CFILES_SERIAL:%=src/%), $(CFILES))
endif

# Header files/directories this library provides
HDRFILES := $(wildcard $(SOURCE_DIR)/include/*)

CFLAGS += -std=gnu99 -Werror

include $(SEL4_COMMON)/common.mk
