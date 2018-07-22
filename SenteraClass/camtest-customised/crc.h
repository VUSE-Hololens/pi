/******************************************************************************
/
/   Filename:  crc.h
/   Date:      $Date$
/   Author:    Brian Eickhoff
/
/   $Id$
/
/   Copyright (C) 2015, Sentera
/
******************************************************************************/

#include <inttypes.h>


#ifndef CRC_H
#define	CRC_H

#ifdef	__cplusplus
extern "C" {
#endif

// Function prototypes. =======================================================
    
uint8_t CRC8_7(uint8_t cur_crc, uint8_t *msg, uint32_t msg_size);


#ifdef	__cplusplus
}
#endif

#endif	/* CRC_H */

