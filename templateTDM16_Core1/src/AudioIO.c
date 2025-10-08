#include "AudioIO.h"
#include "DMAConfig.h"   // samplesPerBlock, TX/RX words/slots (frame interleaved)
#include <string.h>

/* ===== Fixed base channels from codecs ================================== */
#define BASE_ANALOG_INPUT_CHANNELS   4u    /* AN1..AN4 from ADAU1979 (RX slots 0..3) */
#define BASE_DAC_OUTPUT_CHANNELS     12u   /* DAC1..12 to ADAU1962  (TX slots 0..11) */

/* Each COAX port contributes 2 channels (L/R) when used as IN or OUT */
#define COAX_STEREO_CHANNELS         2u

/* Maximum possible unified counts when both COAX are inputs and outputs.
   For determinism (no heap), allocate to worst-case. */
#define MAX_INPUT_CHANNELS  (BASE_ANALOG_INPUT_CHANNELS + 2u * COAX_STEREO_CHANNELS)  /* 4 + 4 = 8 */
#define MAX_OUTPUT_CHANNELS (BASE_DAC_OUTPUT_CHANNELS   + 2u * COAX_STEREO_CHANNELS)  /* 12 + 4 = 16 */

/* ===== Globals requested ================================================= */
volatile uint32_t numberOfInputChannels  = BASE_ANALOG_INPUT_CHANNELS;
volatile uint32_t numberOfOutputChannels = BASE_DAC_OUTPUT_CHANNELS;
volatile CoaxState coaxState = COAX_OFF; /* default */

/* ===== Unified working blocks, frame-major interleaving ================== */
uint32_t inputBufferBlock [MAX_INPUT_CHANNELS  * samplesPerBlock];
uint32_t outputBufferBlock[MAX_OUTPUT_CHANNELS * samplesPerBlock];

/* ===== Internal helpers ================================================== */
static inline uint32_t countCoaxInputs(CoaxState s)
{
    switch (s) {
        case COAX_IN_IN:  return 2u;
        case COAX_IN_OUT: return 1u;
        case COAX_OUT_IN: return 1u;
        case COAX_OUT_OUT:  return 0u; /* COAX_OUT_OUT */
        case COAX_OFF : return 0;
    }
    return 0;
}
static inline uint32_t countCoaxOutputs(CoaxState s)
{
    switch (s) {
        case COAX_IN_IN:  return 0u;
        case COAX_IN_OUT: return 1u;
        case COAX_OUT_IN: return 1u;
        case COAX_OUT_OUT:  return 2u; /* COAX_OUT_OUT */
        case COAX_OFF : return 0;
    }
    return 0;
}

/* ===== Public API ======================================================== */
void AudioIO_resetConfiguration(void)
{
    coaxState               = COAX_OUT_OUT;
    numberOfInputChannels   = BASE_ANALOG_INPUT_CHANNELS;
    numberOfOutputChannels  = BASE_DAC_OUTPUT_CHANNELS;
}

void AudioIO_setCoaxState(CoaxState state)
{
    coaxState = state;
}

void AudioIO_applyConfiguration(void)
{
    const uint32_t coaxIn  = countCoaxInputs (coaxState);
    const uint32_t coaxOut = countCoaxOutputs(coaxState);

    numberOfInputChannels  = BASE_ANALOG_INPUT_CHANNELS + (coaxIn  * COAX_STEREO_CHANNELS);
    numberOfOutputChannels = BASE_DAC_OUTPUT_CHANNELS   + (coaxOut * COAX_STEREO_CHANNELS);
}

/* ===== Glue for the SPORT callback ======================================
   Your SPORT is in TDM16, DMAPack=true. RX buffer is [frame][slot] with 4
   active slots; TX buffer is [frame][slot] with 12 active slots.           */
/* Fill unified input from AN1..4. COAX inputs are zero for now. */
void fillInputBufferFromAN(const int32_t* adcRxFrameInterleaved)
{
    for (uint32_t f = 0; f < samplesPerBlock; ++f) {
        const uint32_t rxIndex = 4u * f;                         /* 4 RX slots per frame */
        const uint32_t inIndex = numberOfInputChannels * f;      /* start of unified frame */

        /* Map AN1..AN4 -> unified inputs 0..3 */
        inputBufferBlock[inIndex + 0] = adcRxFrameInterleaved[rxIndex + 0];
        inputBufferBlock[inIndex + 1] = adcRxFrameInterleaved[rxIndex + 1];
        inputBufferBlock[inIndex + 2] = adcRxFrameInterleaved[rxIndex + 2];
        inputBufferBlock[inIndex + 3] = adcRxFrameInterleaved[rxIndex + 3];

        /* Reserve COAX inputs (if any) by writing zeros for now */
        const uint32_t coaxInCount = countCoaxInputs(coaxState);
        uint32_t pos = inIndex + BASE_ANALOG_INPUT_CHANNELS;
        for (uint32_t i = 0; i < coaxInCount; ++i) {
            /* each COAX IN adds L,R */
            inputBufferBlock[pos + 0] = 0;
            inputBufferBlock[pos + 1] = 0;
            pos += 2u;
        }
    }
}

/* Scatter unified output to DAC1..12 (SPORT4A TX). COAX OUT lanes will later
   be consumed by the SPDIF Tx driver (not by SPORT4A). */
void fillDACFromOutputBuffer(int32_t* dacTxFrameInterleaved)
{
    for (uint32_t f = 0; f < samplesPerBlock; ++f) {
        const uint32_t txIndex = 12u * f;                        /* 12 TX slots per frame */
        const uint32_t outIndex = numberOfOutputChannels * f;    /* start of unified frame */

        /* First 12 unified channels feed DAC1..12 */
        for (uint32_t ch = 0; ch < 12u; ++ch) {
            dacTxFrameInterleaved[txIndex + ch] = outputBufferBlock[outIndex + ch];
        }
    }
}
