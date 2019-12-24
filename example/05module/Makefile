#
#   Carbon Framework example makefile
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 25.01.2017 12:28:03
#	Initial revision.
#
#   make [all|clean|rebuild|r]
#
#

PROGRAM = module
OBJ = module_app.o receiver.o keyboard.o
DEPS = module_app.h receiver.h keyboard.h keyboard_driver.h

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	OBJ += linux/keyboard_tty.o
	DEPS += linux/keyboard_tty.h
endif

all: carbon_dep $(PROGRAM)

include ../../tool/pkgrules.mak
