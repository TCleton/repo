/*****************************************************************************
 * templateTDM16_Core1.h
 *****************************************************************************/

#ifndef __TEMPLATETDM16_CORE1_H__
#define __TEMPLATETDM16_CORE1_H__

#include <stdint.h>
#include <drivers/twi/adi_twi.h>
#define APP_SUCCESS             (0)
#define APP_FAILED              (1)

//TWI
int TwiOpen(void);
int TwiSetAddr(uint16_t addr);
int TwiClose(void);
ADI_TWI_RESULT TwiWrite8(uint8_t reg, uint8_t val);
uint8_t TwiRead8(uint8_t reg);
int Soft_FindExpanderAddr(uint8_t *found);
extern ADI_TWI_HANDLE sTwiHandle;
uint8_t getNumFromBits(int numArrays, uint8_t* arrays[], int sizes[]);

void map4to12(const int32_t* __restrict rx, int32_t* __restrict tx);
#endif /* __TEMPLATETDM16_CORE1_H__ */
