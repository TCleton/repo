#include "AudioIO.h"
#include "PingPongBuffer.h"   // samplesPerBlock, TX/RX words/slots (frame interleaved)
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
/* ===== Fixed base channels from codecs ================================== */
#define BASE_ANALOG_INPUT_CHANNELS   4u    /* AN1..AN4 from ADAU1979 (RX slots 0..3) */
#define BASE_DAC_OUTPUT_CHANNELS     12u   /* DAC1..12 to ADAU1962  (TX slots 0..11) */

/* Maximum possible unified counts when both COAX are inputs and outputs.
   For determinism (no heap), allocate to worst-case. */
#define MAX_INPUT_CHANNELS  (BASE_ANALOG_INPUT_CHANNELS + 2u)  /* 4 + 4 = 8 */
#define MAX_OUTPUT_CHANNELS (BASE_DAC_OUTPUT_CHANNELS   + 2u)  /* 12 + 4 = 16 */

/* ===== Globals requested ================================================= */
volatile uint32_t numberOfInputChannels  = BASE_ANALOG_INPUT_CHANNELS;
volatile uint32_t numberOfOutputChannels = BASE_DAC_OUTPUT_CHANNELS;
volatile SPDIF_STATE SPDIFState = SPDIF_DIGITAL_ON_OPTICAL_ON; /* default */
/* ===== Public API ======================================================== */
void AudioIO_resetConfiguration(void)
{
    SPDIFState              = SPDIF_DIGITAL_ON_OPTICAL_ON;
    numberOfInputChannels   = BASE_ANALOG_INPUT_CHANNELS;
    numberOfOutputChannels  = BASE_DAC_OUTPUT_CHANNELS;
}

void AudioIO_setSPDIFState(SPDIF_STATE state)
{
    SPDIFState = state;
}

bool isSPDIFactive(void)
{
	return SPDIFState != SPDIF_DIGITAL_OFF_OPTICAL_OFF;
}

void AudioIO_applyConfiguration(void)
{
    const uint32_t numberOfSPDIFChannels  = isSPDIFactive() ? 2 : 0;

    numberOfInputChannels  = BASE_ANALOG_INPUT_CHANNELS + numberOfSPDIFChannels;
    numberOfOutputChannels = BASE_DAC_OUTPUT_CHANNELS   + numberOfSPDIFChannels;
}

/* ===== Glue for the SPORT callback ======================================
   Your SPORT is in TDM16, DMAPack=true. RX buffer is [frame][slot] with 4
   active slots; TX buffer is [frame][slot] with 12 active slots.           */
/* Fill unified input from AN1..4. COAX inputs are zero for now. */

void fillGlobalInputFromAN(void)
{
    if (!jackStream.Rx.isFreshData) return;

    const uint32_t * __restrict src = (const uint32_t*)jackStream.Rx.readPtr;
    uint32_t       * __restrict dst = (uint32_t*)      globalStream.Rx.writePtr;

    for (uint32_t f = 0; f < SAMPLES_PER_BLOCK; ++f) {
        const uint32_t *srcFrame = &src[4u * f];
        uint32_t       *dstFrame = &dst[(SLOTS_RX + SLOTS_SPDIF) * f];
        dstFrame[0] = srcFrame[0];
        dstFrame[1] = srcFrame[1];
        dstFrame[2] = srcFrame[2];
        dstFrame[3] = srcFrame[3];
        dstFrame[4] = 0;  // SPDIF L placeholder
        dstFrame[5] = 0;  // SPDIF R placeholder
    }

    flipPingPong(&globalStream.Rx); // read half becomes writePtr
    globalStream.Rx.isFreshData = true;   // globalRx (dst) has just been filled with new data
    jackStream.Rx.isFreshData    = false; // jackRx has just been read so the values are obsolete
}


/* Scatter unified output to DAC1..12 (SPORT4A TX). COAX OUT lanes will later
   be consumed by the SPDIF Tx driver (not by SPORT4A). */

void fillDACOutputFromGlobal(void)
{
    // need: a fresh unified TX block AND a free JACK-TX frame
    if (!globalStream.Tx.isFreshData || jackStream.Tx.isFreshData) return;

    const uint32_t * __restrict src = (const uint32_t*)globalStream.Tx.readPtr;
    uint32_t       * __restrict dst = (uint32_t*)      jackStream.Tx.writePtr;

    for (uint32_t f = 0; f < SAMPLES_PER_BLOCK; ++f) {
        const uint32_t *srcFrame = &src[(SLOTS_TX + SLOTS_SPDIF) * f]; // 14 unified outs
        uint32_t       *dstFrame = &dst[SLOTS_TX * f];                  // 12 DAC slots
        dstFrame[ 0] = srcFrame[ 0]; dstFrame[ 1] = srcFrame[ 1];
        dstFrame[ 2] = srcFrame[ 2]; dstFrame[ 3] = srcFrame[ 3];
        dstFrame[ 4] = srcFrame[ 4]; dstFrame[ 5] = srcFrame[ 5];
        dstFrame[ 6] = srcFrame[ 6]; dstFrame[ 7] = srcFrame[ 7];
        dstFrame[ 8] = srcFrame[ 8]; dstFrame[ 9] = srcFrame[ 9];
        dstFrame[10] = srcFrame[10]; dstFrame[11] = srcFrame[11];
    }

    // consumer done with unified TX; producer for JACK-TX just produced a frame
    flipPingPong(&globalStream.Tx); // read half becomes writePtr ?????
    globalStream.Tx.isFreshData = false; // globalTx has just been read so the values are obsolete
    jackStream.Tx.isFreshData   = true; // jackTx (dst) has just been filled with new data
}


void fillSpdifFromOutputBuffer(void){
	return;
}
