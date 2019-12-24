#
#   Carbon Framework
#   Make definitions for carbon library usage
#
#   Copyright (c) 2016-2018 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 30.03.2016 17:43:21
#	Initial revision.
#
#   Revision 1.1, 09.01.2018 13:16:29
#	Added $(LIBRARY) target
#
#	CROSS_COMPILE="gcc_prefix"
#	CARBON_MACHINE="machine"
#	CARBON_BACKEND="backend"
#
#   Optional input variables:
#	RELEASE=1
#

__ROOT_PATH := $(abspath $(shell dirname $(realpath $(lastword $(MAKEFILE_LIST))))/..)

include $(__ROOT_PATH)/tool/config.mak


CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
SZ = $(CROSS_COMPILE)size
MAKE ?= make

ifdef RELEASE
CFLAGS += -DRELEASE=1 -Os -O3
else
DEFINES += -DDEBUG=1 
#CFLAGS += -ggdb -Os -O3
LDFLAGS += -ggdb
ifeq ($(CARBON_MACHINE),unix)
LDFLAGS += -rdynamic
endif
endif

ifeq ($(CARBON_MACHINE),unix)
_MACHINE_DEF=__unix__
else
_MACHINE_DEF=__embed__
endif

DEFINES += -D$(_MACHINE_DEF)=1

CFLAGS += -Wall -Werror -Wno-error=array-bounds 
CFLAGS_unix += -pthread
CFLAFS_embed +=

CPPFLAGS += $(CFLAGS) -std=c++0x
CPPFLAGS_unix += $(CFLAGS_unix)
CPPFLAGS_embed += $(CFLAGS_embed) -fno-exceptions -fno-rtti

THPARTY_PATH = $(CARBON_PATH)/thparty/__root__


-include $(CARBON_PATH)/backend/$(CARBON_BACKEND)/config.mak

################################################################################

INCLUDES += -I$(CARBON_PATH)/src -I$(THPARTY_PATH)/include -I$(CARBON_PATH)/src/module
LDFLAGS += -L$(THPARTY_PATH)/lib -L$(BUILD_PATH)
LIBS_DEP += $(BUILD_PATH)/libcarbon.a $(BUILD_PATH)/libshell.a

ifeq ($(CARBON_MACHINE),unix)
_LIBS := -lrt
endif


#_LIBS += -lfreertos -lstdc++ -lm -lmodule -lcarbon -lshell $(LIBS)
_LIBS +=  -lcarbon -lshell -lstdc++ -lm $(LIBS)
#-lcarbon -lshell 
#-lfreertos

ifdef MODULE_DEP
_LIBS += -lmodule
LIBS_DEP += $(BUILD_PATH)/libmodule.a
endif

ifeq ($(CARBON_MACHINE),unix)
LDFLAGS += -pthread
endif

ifeq ($(CARBON_JEMALLOC),1)
_LIBS += -ljemalloc
endif

ifeq ($(CARBON_UDNS),1)
_LIBS += -ludns
endif

ifeq ($(CARBON_JANSSON),1)
_LIBS += -ljansson
endif

ifeq ($(CARBON_XML),1)
_LIBS += -lpugixml
endif

ifeq ($(CARBON_ZLIB),1)
_LIBS += -lz
endif

%.o: %.cpp $(DEPS) Makefile
	$(CPP) -c $(DEFINES) $(INCLUDES) $(CPPFLAGS) $(CPPFLAGS_$(CARBON_MACHINE)) $< -o $@

%.o: %.c $(DEPS) Makefile 
	$(CC) -c $(DEFINES) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(CARBON_MACHINE)) $< -o $@

#
# Output rules
#

$(PROGRAM): $(LIBS_DEP) $(OBJ)
	$(LD) $(LDFLAGS) -o $@ $(OBJ) $(_LIBS)
ifeq ($(CARBON_MACHINE),embed)
	$(SZ) -d $@
endif

$(LIBRARY): $(OBJ)
	$(AR) -rcs $@ $(OBJ)
	$(RANLIB) $@

clean:
	rm -f $(OBJ) $(PROGRAM) $(LIBRARY) *~

cleanall: clean

rebuild r: clean all

#
# Carbon library
#

.PHONY: carbon_dep carbon_clean

CMD_DEF=
ifdef CROSS_COMPILE
CMD_DEF+=CROSS_COMPILE=$(CROSS_COMPILE)
endif

ifdef MACHINE
CMD_DEF+=MACHINE=$(MACHINE)
endif
 
ifdef RELEASE
CMD_DEF+=RELEASE=1
endif

carbon_dep:
	(cd $(CARBON_PATH); $(MAKE) $(CMD_DEF)) 

carbon_clean:
	(cd $(CARBON_PATH); $(MAKE) $(CMD_DEP) clean)
