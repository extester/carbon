#
#   Carbon framework example makefile
#
#   Copyright (c) 2015 Softland. All rights reserved.
#   Licensed under the Apache License, Version 2.0
#
#   Revision history:
#
#   Revision 1.0, 10.06.2015 11:46:22
#	Initial revision.
#
#   make [CROSS_COMPILE=<gcc-prefix>] [RELEASE=1]
#

OBJ_remote_event_recv = remote_event_recv_app.o
OBJ_remote_event_send = remote_event_send_app.o
INCLUDE = remote_event_recv_app.h remote_event_send_app.h shared.h

OBJ = $(OBJ_remote_event_recv) $(OBJ_remote_event_send)

all: carbon_dep remote_event_recv_app remote_event_send_app

clean: clean_program

remote_event_recv_app: Makefile $(OBJ_remote_event_recv)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_remote_event_recv) $(_LIBS)

remote_event_send_app: Makefile $(OBJ_remote_event_send)
	$(LD) $(LDFLAGS) -o $@ $(OBJ_remote_event_send) $(_LIBS)

clean_program:
	rm -f remote_event_recv_app remote_event_send_app

include ../../tool/pkgrules.mak
