#include "DMAConfig.h"
#include "SPORTConfig.h"


ADI_PDMA_DESC_LIST dmaDescriptorTxPing;
ADI_PDMA_DESC_LIST dmaDescriptorTxPong;
ADI_PDMA_DESC_LIST dmaDescriptorRxPing;
ADI_PDMA_DESC_LIST dmaDescriptorRxPong;

ADI_CACHE_ALIGN uint32_t bufferTxPing[TX_WORDS];
ADI_CACHE_ALIGN uint32_t bufferTxPong[TX_WORDS];
ADI_CACHE_ALIGN uint32_t bufferRxPing[RX_WORDS];
ADI_CACHE_ALIGN uint32_t bufferRxPong[RX_WORDS];

void PrepareDescriptors(void)
{
    // --- TX (SPORT4A, 12 slots) ---
    dmaDescriptorTxPing.pStartAddr = (int*)bufferTxPing;
    dmaDescriptorTxPing.pNxtDscp   = &dmaDescriptorTxPong;
    dmaDescriptorTxPing.Config     = ENUM_DMA_CFG_XCNT_INT;    // match example: 1D, Xcount interrupt
    dmaDescriptorTxPing.XCount     = TX_WORDS;//number of elements in the buffer (numberOfSlots*numberOfSamplePerBlock)
    dmaDescriptorTxPing.XModify    = sizeof(int);//size of one element
    dmaDescriptorTxPing.YCount     = 0;
    dmaDescriptorTxPing.YModify    = 0;

    dmaDescriptorTxPong.pStartAddr = (int*)bufferTxPong;
    dmaDescriptorTxPong.pNxtDscp   = &dmaDescriptorTxPing;
    dmaDescriptorTxPong.Config     = ENUM_DMA_CFG_XCNT_INT;
    dmaDescriptorTxPong.XCount     = TX_WORDS;
    dmaDescriptorTxPong.XModify    = sizeof(int);
    dmaDescriptorTxPong.YCount     = 0;
    dmaDescriptorTxPong.YModify    = 0;

    // --- RX (SPORT4B, 4 slots) ---
    dmaDescriptorRxPing.pStartAddr = (int*)bufferRxPing;
    dmaDescriptorRxPing.pNxtDscp   = &dmaDescriptorRxPong;
    dmaDescriptorRxPing.Config     = ENUM_DMA_CFG_XCNT_INT;
    dmaDescriptorRxPing.XCount     = RX_WORDS;
    dmaDescriptorRxPing.XModify    = sizeof(int);
    dmaDescriptorRxPing.YCount     = 0;
    dmaDescriptorRxPing.YModify    = 0;

    dmaDescriptorRxPong.pStartAddr = (int*)bufferRxPong;
    dmaDescriptorRxPong.pNxtDscp   = &dmaDescriptorRxPing;
    dmaDescriptorRxPong.Config     = ENUM_DMA_CFG_XCNT_INT;
    dmaDescriptorRxPong.XCount     = RX_WORDS;
    dmaDescriptorRxPong.XModify    = sizeof(int);
    dmaDescriptorRxPong.YCount     = 0;
    dmaDescriptorRxPong.YModify    = 0;

    //printf("descriptors linked with buffers\n");
}
