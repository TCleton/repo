#ifndef DMACONFIG_H_
#define DMACONFIG_H_
#include <drivers/sport/adi_sport.h>
#include <stdio.h>
#include <stdint.h>


#include <sys/adi_core.h>
#include "adi_initialize.h"
#include <services/int/adi_int.h>

#define PDMA_CFG_COMMON   (ENUM_DMA_CFG_XCNT_INT | ENUM_DMA_CFG_COPY)
#define PDMA_CFG_RX (ENUM_DMA_CFG_XCNT_INT)

#define samplesPerBlock 128u      // frames per DMA block

#define SLOTS_TX     12u//12
#define SLOTS_RX     4u//4
#define TX_WORDS     (SLOTS_TX * samplesPerBlock)  // 12 * 200 = 2400
#define RX_WORDS     (SLOTS_RX * samplesPerBlock)  //  4 * 200 =  800

extern ADI_PDMA_DESC_LIST dmaDescriptorTxPing;
extern ADI_PDMA_DESC_LIST dmaDescriptorTxPong;
extern ADI_PDMA_DESC_LIST dmaDescriptorRxPing;
extern ADI_PDMA_DESC_LIST dmaDescriptorRxPong;

extern uint32_t bufferTxPing[TX_WORDS];
extern uint32_t bufferTxPong[TX_WORDS];
extern uint32_t bufferRxPing[RX_WORDS];
extern uint32_t bufferRxPong[RX_WORDS];

void PrepareDescriptors(void);


#endif /* DMACONFIG_H_ */
