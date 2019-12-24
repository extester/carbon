/*
 *  Carbon/Vep module
 *  Verinet Exchange Protocol (VEP)
 *
 *  Copyright (c) 2016 Softland. All rights reserved.
 *  Licensed under the Apache License, Version 2.0
 */
/*
 *  Revision history:
 *
 *  Revision 1.0, 30.05.2016 16:26:11
 *      Initial revision.
 */

#ifndef __CARBON_VEP_H_INCLUDED__
#define __CARBON_VEP_H_INCLUDED__

#include "shell/types.h"

static const uint8_t	VEP_IDENT[] = "veri";
static const uint32_t	VEP_VERSION = 1;
static const size_t		VEP_CONTAINER_MAX_SIZE = 16*1024*1024;
static const size_t		VEP_PACKET_MAX_SIZE = 8*1024*1024;

typedef uint16_t        vep_container_type_t;

#include "vep/vep_packet.h"
#include "vep/vep_container.h"


#endif /* __CARBON_VEP_H_INCLUDED__ */

