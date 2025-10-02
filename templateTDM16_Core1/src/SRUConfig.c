// SRUConfig.c
#include <SRU.h>
#include <stdint.h>

// for clarity redefine the different use of the SRU2 function
#define DSP_LISTEN_TO(PBEN)  SRU2(LOW,  PBEN)
#define DSP_DRIVE(PBEN)      SRU2(HIGH, PBEN)
#define ROUTE(SRC, DST)      SRU2(SRC,  DST)

#define ADAU1962_MCLKO
/*
 pin connection can be found in this datasheet for the ezkit :
 https://www.analog.com/media/en/technical-documentation/eval-board-schematic/ev-somcrr-ezkit-schematic.pdf#page=4.48
 */

void ConfigureSru(void)
{
    // 1) Enable pad input receivers
    *pREG_PADS0_DAI0_IE = 0xFFFFF;
    *pREG_PADS0_DAI1_IE = 0xFFFFF;

    // ====== BIT CLOCK FROM DAC TO SPORTS ======
    DSP_LISTEN_TO(DAI1_PBEN05_I); // this is where the BCLK of ADAU1962 is connected
    ROUTE(DAI1_PB05_O, SPT4_ACLK_I);  // route the clock from ADAU1962 to sport A
    ROUTE(DAI1_PB05_O, SPT4_BCLK_I);  // route the clock from ADAU1962 to sport B

    // ====== BIT CLOCK FROM DAC TO ADC ======
	DSP_DRIVE(DAI1_PBEN12_I); // DSP takes control of the PIN12 to send the clock to ADAU1979 (ADC)
	ROUTE(DAI1_PB05_O, DAI1_PB12_I); // clock from ADAU1962 is on DAI1_PB05_O and the DSP routes it to the ADAU1979 clock on pin DAI1_PIN_12

    // ====== FSCLK FROM DAC TO SPORTS ======
    DSP_LISTEN_TO(DAI1_PBEN04_I); // this is where the FSCLK (or LRCLK) of ADAU1962 is connected
	ROUTE(DAI1_PB04_O, SPT4_AFS_I);  // route the Frame Sync clock from ADAU1962 to sport A
	ROUTE(DAI1_PB04_O, SPT4_BFS_I);  // route the Frame Sync clock from ADAU1962 to sport B

	// ====== FSCLK FROM DAC TO ADC ======
	DSP_DRIVE(DAI1_PBEN20_I); // DSP takes control of the PIN20 to send the LR (or FS) clock to ADAU1979 (ADC)
	ROUTE(DAI1_PB04_O, DAI1_PB20_I); // LR (or FS) clock from ADAU1962 is on DAI1_PB04_O and the DSP routes it to the ADAU1979 clock on pin DAI1_PIN_20

    // ====== DATA FROM ADC TO SPORTB ======
	DSP_LISTEN_TO(DAI1_PBEN06_I); // DSP listen to DAI1_PIN06 that is connected to the SDATA1 of ADAU1979 (ADC)
	ROUTE(DAI1_PB06_O, SPT4_BD0_I); // the data from SDATA1 of ADAU1979 (ADC) is routed to the D0 pin of SPORT4B

	// ====== DATA FROM SPORTA TO DAC ======
	DSP_DRIVE(DAI1_PBEN01_I); // the DSP drives this pin to send DATA to the DAC (ADAU1962)
	ROUTE(SPT4_AD0_O, DAI1_PB01_I); // D0 is data from SP4A and DAI1_PB01_I is the pin connected to ADAU1962 SDATA1
}

