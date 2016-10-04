# -*- makefile -*-
#
# Sensory Confidential
#
# Copyright (C) 2015 Sensory, Inc. http://sensory.com/
#------------------------------------------------------------------------------
# Builds TrulyHandsfree SDK code examples.
# usage: make -j
#------------------------------------------------------------------------------


# List of sample application targets
SAMPLES =\
  $(filter-out Data/% shared/% %/statics,$(patsubst %.c,%,$(wildcard */*.c)))

# The directory this Makefile was read from.
last-include-dir = $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

SAMPLE_DATA_DIR = $(abspath $(last-include-dir)/Data)
SHARED_DIR      = $(abspath $(last-include-dir)/shared)
SDK_INC_DIR     = $(abspath $(last-include-dir)/include)
SDK_LIB_DIR     = $(abspath $(last-include-dir)/lib)
SYS_LIB_DIR	= $(SYSROOT)/usr/lib

CFLAGS  += -I$(SHARED_DIR) -I$(SDK_INC_DIR) -DDATADIR_SAMP=$(SAMPLE_DATA_DIR) -I$(SYSROOT)/usr/include
LDFLAGS += -L$(SDK_LIB_DIR) -L$(SYS_LIB_DIR) --sysroot=$(SYSROOT)
LIBS    += $(if $(filter Linux,$(shell uname -s)),-lasound)

SAMPLE_LIB = $(SHARED_DIR)/libsdksample.a
SAMPLE_LIB_OBJ = $(addprefix $(SHARED_DIR)/,audio.o console.o checkflagstext.o)
ALL_OBJ  = $(SAMPLE_LIB_OBJ) $(addsuffix .o,$(SAMPLES))
ALL_OBJ += StaticBuildList/statics.o StaticRecogList/statics.o

all: $(SAMPLES)

clean:
	$(foreach e,.a .o,rm -f $(last-include-dir)/*/*$e;)
	rm -f $(SAMPLES)

$(ALL_OBJ): $(addprefix $(SHARED_DIR)/,audio.h console.h datalocations.h)
$(ALL_OBJ): %.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

$(SAMPLE_LIB): $(SAMPLE_LIB_OBJ)
	$(AR) crv $@ $^

# Dependencies not covered by the $(SAMPLES): pattern below
StaticBuildList/staticBuildList: StaticBuildList/statics.o
StaticRecogList/staticRecogList: StaticRecogList/statics.o

# staticRecogList includes names.c from Samples/Data
StaticRecogList/staticRecogList: CFLAGS+=-I$(SAMPLE_DATA_DIR)

$(SAMPLES): $(SAMPLE_LIB)
$(SAMPLES): %: %.o
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ -lthf -lm $(LIBS)
