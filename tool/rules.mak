#
#   Carbon Framework
#   General makefile rules
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 31.05.2015 16:06:10
#	Initial revision.
#
#
#   Input variables:
#	MACHINE
#	CROSS_COMPILE
#	RELEASE
#	CARBON_MACHINE
#	CARBON_BACKEND
#
#	PROGRAM
#	LIBRARY
#	OBJ
#
#	DEFINES
#	INCLUDES
#	CFLAGS
#	CPPFLAGS
#	LDFLAGS
#	LIBS
#	DEPS
#

CC = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
LD = $(CROSS_COMPILE)gcc
AR = $(CROSS_COMPILE)ar
RANLIB = $(CROSS_COMPILE)ranlib
MAKE ?= make

ifdef RELEASE
CFLAGS += -DRELEASE=1 -Os -O3
else
DEFINES += -DDEBUG=1 
CFLAGS += -ggdb -Os -O3
LDFLAGS += -ggdb -rdynamic
endif

ifeq ($(CARBON_MACHINE),unix)
_MACHINE_DEF=__unix__
else
_MACHINE_DEF=__embed__
endif
DEFINES += -D$(_MACHINE_DEF)=1

CFLAGS += -Wall -Werror -Wno-error=array-bounds 
CFLAGS_unix += -pthread
CFLAGS_embed += 

CPPFLAGS += $(CFLAGS) -std=c++0x
CPPFLAGS_unix += $(CFLAGS_unix)
CPPFLAGS_embed += $(CFLAGS_embed) -fno-exceptions -fno-rtti

SRC_PATH = $(CARBON_PATH)/src
THPARTY_PATH = $(CARBON_PATH)/thparty

-include $(CARBON_PATH)/backend/$(CARBON_BACKEND)/config.mak

################################################################################

.PHONY: carbon_dep thparty_dep

OBJ_PATH ?= $(BUILD_PATH)
_OBJ = $(addprefix $(OBJ_PATH)/, $(OBJ))

#
# Misc targets
#

build_output_path:
	@mkdir -p $(BUILD_PATH)
	@mkdir -p $(OBJ_PATH)

_VAR_LIST = $(filter CARBON_%,$(.VARIABLES))
_VARS = $(foreach var, $(_VAR_LIST), $(var)=$($(var)))

CONFIG_H=$(SRC_PATH)/shell/config.h

$(CONFIG_H): $(CARBON_PATH)/config.mak $(TOOL_PATH)/config_$(CARBON_MACHINE).mak
	echo "/*" >$(CONFIG_H)
	echo " *" >>$(CONFIG_H)
	echo " * Autogenerated configuration, do not edit!" >>$(CONFIG_H)
	echo " *" >>$(CONFIG_H)
	echo " */" >>$(CONFIG_H)
	echo >>$(CONFIG_H)
	echo "#ifndef __SHELL_CONFIG_H_INCLUDED__" >>$(CONFIG_H)
	echo "#define __SHELL_CONFIG_H_INCLUDED__" >>$(CONFIG_H)
	echo >>$(CONFIG_H)
	for var in $(_VARS); do \
	    value=`echo $$var | sed -n 's/.*=\\(.*\\)/\1/p'`; \
	    name=`echo $$var | sed -n 's/\\(.*\\)=\\(.*\\)/\1/p'`; \
	    case $$value in \
		[01-9x]*) echo "#define $$name $$value" >>$(CONFIG_H);; \
		*) echo "#define $$name \"$$value\"" >>$(CONFIG_H);; \
	    esac \
	done
	echo >>$(CONFIG_H)
	echo "#endif /* __SHELL_CONFIG_H_INCLUDED__ */" >>$(CONFIG_H)
	echo >>$(CONFIG_H)

#
# Object file rules
#

$(OBJ_PATH)/%.o: %.cpp $(CONFIG_H) $(DEPS) Makefile | build_output_path
	@if test "$(dir $<)" != "./" ; then mkdir -p $(OBJ_PATH)/$(dir $<); fi
	$(CPP) -c $(DEFINES) $(INCLUDES) $(CPPFLAGS) $(CPPFLAGS_$(CARBON_MACHINE)) $< -o $@

$(OBJ_PATH)/%.o: %.c $(CONFIG_H) $(DEPS) Makefile | build_output_path
	@if test "$(dir $<)" != "./" ; then mkdir -p $(OBJ_PATH)/$(dir $<); fi
	$(CC) -c $(DEFINES) $(INCLUDES) $(CFLAGS) $(CFLAGS_$(CARBON_MACHINE)) $< -o $@

#
# Output rules
#

$(PROGRAM): $(_OBJ)
	$(LD) $(LDFLAGS) -o $@ $(_OBJ) $(LIBS)

$(LIBRARY): $(_OBJ)
	$(AR) -rcs $@ $(_OBJ)
	$(RANLIB) $@

################################################################################

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

carbon_dep: thparty_dep
	(cd $(SRC_PATH); $(MAKE) $(CMD_DEF))

thparty_dep:
	(cd $(THPARTY_PATH); $(MAKE) $(CMD_DEF))

clean:
	rm -f $(_OBJ) $(CONFIG_H) $(LIBRARY) $(PROGRAM) *~ 
	rm -Rf $(OBJ_PATH)

cleanall: clean

rebuild r: clean all