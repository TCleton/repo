#ifndef SPORTCONFIG_H_
#define SPORTCONFIG_H_

#include <drivers/sport/adi_sport.h>

/* SPORT driver handles and per-device memory (required by the driver). */
ADI_SPORT_HANDLE handleSport4ATx;   /* SPORT4A, Tx transmit data to the dac*/
ADI_SPORT_HANDLE handleSport4BRx;   /* SPORT4B, Rx receive data from the adc */

uint8_t memorySport4ATx[ADI_SPORT_MEMORY_SIZE];
uint8_t memorySport4BRx[ADI_SPORT_MEMORY_SIZE];

#define SportDeviceNum4  4u // Sport4
void SportCallback(void *pAppHandle, uint32_t event, void *pArg);
int sport_init(void);

#endif /* SPORTCONFIG_H_ */
