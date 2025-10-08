/*
 * AudioIO.h
 *
 *  Created on: 8 oct. 2025
 *      Author: t.cleton
 */

#ifndef AUDIOIO_H_
#define AUDIOIO_H_

#include <stdint.h>

/* ===== Combined COAX state (exactly one of these) ======================= */
typedef enum {
    COAX_IN_IN = 0,   /* COAX1: IN,  COAX2: IN  */
    COAX_IN_OUT,      /* COAX1: IN,  COAX2: OUT */
    COAX_OUT_IN,      /* COAX1: OUT, COAX2: IN  */
    COAX_OUT_OUT,     /* COAX1: OUT, COAX2: OUT */
	COAX_OFF
} CoaxState;

/* ===== Global topology (you requested globals for clarity) ============== */
/* Total channel counts for the unified block (depend only on coaxState) */
extern volatile uint32_t numberOfInputChannels;   /* 4 + 2*#(COAX inputs)  */
extern volatile uint32_t numberOfOutputChannels;  /* 12 + 2*#(COAX outputs)*/

extern uint32_t inputBufferBlock[];
extern uint32_t outputBufferBlock[];

/* The current COAX role state driving the counts */
extern volatile CoaxState coaxState;

/* ===== Topology configuration =========================================== */
/* Reset to defaults (COAX out-out => no extra INs; only DAC adds if set) */
void AudioIO_resetConfiguration(void);

/* Set the single state (in-in, in-out, out-in, out-out) */
void AudioIO_setCoaxState(CoaxState state);

/* Recompute numberOfInputChannels / numberOfOutputChannels from coaxState */
void AudioIO_applyConfiguration(void);

/* ===== Unified processing blocks ======================================== */
/* Access to the working unified blocks [frame-major: frame][channel] */
const int32_t* AudioIO_getInputBufferBlock(void);
int32_t*       AudioIO_getOutputBufferBlock(void);

/* ===== Glue called by the SPORT callback =================================
   These names reflect the real endpoints (AN for ADC; DAC for outputs).     */

/* Merge SPORT4B RX (AN1..AN4) into the unified input buffer. COAX inputs
   are reserved but currently zero-filled (we'll wire SPDIF later). */
void fillInputBufferFromAN(const int32_t* adcRxFrameInterleaved);

/* Scatter the unified output buffer to SPORT4A TX (DAC1..12) */
void fillDACFromOutputBuffer(int32_t* dacTxFrameInterleaved);

#endif /* AUDIOIO_H_ */
