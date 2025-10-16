/*
 * TWI.h
 *
 *  Created on: 13 oct. 2025
 *      Author: t.cleton
 */

#ifndef TWI_H_
#define TWI_H_

#include <stdint.h>
#include <drivers/twi/adi_twi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define APP_SUCCESS             (0)
#define APP_FAILED              (1)

#define TwiDevNum               (2u)
#define TwiBitrateKHz           (100u)
#define TwiDutyCyclePct         (50u)
#define TwiPrescale             (12u)

extern ADI_TWI_HANDLE   sTwiHandle;
uint8_t          sTwiDevMem[ADI_TWI_MEMORY_SIZE];
uint8_t          sTwiBuf[32]; /* small scratch buffer */
int TwiOpen(void);
int TwiSetAddr(uint16_t addr);
int TwiClose(void);
ADI_TWI_RESULT TwiWrite8(uint8_t reg, uint8_t val);
uint8_t TwiRead8(uint8_t reg);

#endif /* TWI_H_ */
