#
#   Carbon Framework
#   Modules makefile
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 19.01.2017 18:26:59
#   	Initial revision.
#

include ../../tool/config.mak

_MODS1 = $(MODULE_DEP)
-include $(patsubst %, %/depend.mak, $(_MODS1))
_MODS2 = $(MODULE_DEP)
-include $(patsubst %, %/depend.mak, $(_MODS2))

_MODS_ALL := db vep net_media contact

ifeq ($(MAKECMDGOALS), cleanall)
MODS := $(_MODS_ALL)
else ifeq ($(MAKECMDGOALS), makeall)
MODS := $(_MODS_ALL)
else
MODS := $(MODULE_DEP)
endif

MODS := $(sort $(MODS))
include $(patsubst %, %/Makefile, $(MODS))

# Variable MODS contains list of unique dependent modules to compile
# Variable OBJ contains list of source files to compile
# Variable DEPS contains list of source header files

#
#xxx:
#	@echo XXX=$(MAKECMDGOALS)
#	@echo MODS=$(MODS)
#	@echo DEP_MODS=$(MODULE_DEP)
#	@echo OBJ=$(OBJ)
#	@echo TH_DEP=$(THPARTY_DEP)
#

LIBRARY = $(BUILD_PATH)/libmodule.a
OBJ_PATH = $(BUILD_PATH)/module

INCLUDES += -I. -I../ -I../../thparty/__root__/include

all makeall: $(LIBRARY)

cleanall: clean

include ../../tool/rules.mak
