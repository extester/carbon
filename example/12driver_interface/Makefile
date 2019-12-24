#
#   Carbon Framework example makefile
#
#   Copyright (c) 2017 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 27.01.2017 18:25:37
#	Initial revision.
#
#   make [all|clean|rebuild|r]
#
#

.PHONY: driver

PROGRAM = driver_app
OBJ = driver_app.o driver.o
DEPS = driver_app.h driver.h shared.h

all: carbon_dep $(PROGRAM) driver

driver:
	cd driver && ./build

driver_clean:
	cd driver && ./build clean

clean: driver_clean

rebuild r: driver_clean driver

include ../../tool/pkgrules.mak
