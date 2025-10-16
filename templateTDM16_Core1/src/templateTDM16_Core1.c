/*****************************************************************************
 * templateTDM16_Core1.c
 *****************************************************************************/
#include <time.h>
#include "adi_initialize.h"
#include <sys/platform.h>
#include <sys/adi_core.h>

#include "templateTDM16_Core1.h"
#include "PingPongBuffer.h"
#include "ADAU1962Config.h"
#include "ADAU1979Config.h"
#include "sharedConfig.h"
#include "SPORTConfig.h"
#include "SoftConfig.h"
#include "SRUConfig.h"
#include "SPUConfig.h"
#include "AudioIO.h"
#include "TWI.h"

// If you plan to disable Jacks dynamically, gate it here:
static inline bool isJackActive(void) { return true; /* or a runtime flag */ }

void processBlock(void)
{
    if (!globalStream.Rx.isFreshData){
    	printf("processedBlock returned because globalStream.Rx.isFreshData = false\n");
        return;
    }
    const uint32_t * __restrict in  = (const uint32_t*)globalStream.Rx.readPtr;
    uint32_t       * __restrict out = (uint32_t*)      globalStream.Tx.writePtr;

    for (uint32_t f = 0; f < SAMPLES_PER_BLOCK; ++f)
    {
        const uint32_t *frameIn  = &in [(SLOTS_RX + SLOTS_SPDIF) * f];
        uint32_t       *frameOut = &out[(SLOTS_TX + SLOTS_SPDIF) * f];

        // Use your symbolic indexes for clarity
        frameOut[OUT_DAC01] = frameIn[IN_AN1];  frameOut[OUT_DAC02] = frameIn[IN_AN2];
        frameOut[OUT_DAC03] = frameIn[IN_AN3];  frameOut[OUT_DAC04] = frameIn[IN_AN4];
        frameOut[OUT_DAC05] = frameIn[IN_AN1];  frameOut[OUT_DAC06] = frameIn[IN_AN2];
        frameOut[OUT_DAC07] = frameIn[IN_AN3];  frameOut[OUT_DAC08] = frameIn[IN_AN4];
        frameOut[OUT_DAC09] = frameIn[IN_AN1];  frameOut[OUT_DAC10] = frameIn[IN_AN2];
        frameOut[OUT_DAC11] = frameIn[IN_AN3];  frameOut[OUT_DAC12] = frameIn[IN_AN4];

        // SPDIF passthrough (optional)
        frameOut[OUT_SPDIF_L] = frameIn[IN_SPDIF_L];
        frameOut[OUT_SPDIF_R] = frameIn[IN_SPDIF_R];
    }
    // Hand-off
    globalStream.Rx.isFreshData = false;// we just process the data so it is not fresh anymore
    flipPingPong(&globalStream.Tx);// has we processed the data we can flip buffers
    globalStream.Tx.isFreshData = true;// we just filled new data to Tx
}

int main(int argc, char *argv[])
{
    adi_initComponents();

    adi_core_enable(ADI_CORE_SHARC1);

    initPingPongBuffers();

    TwiOpen();

    ConfigureSpu();

    TwiSetAddr(I2cAddrSOFTConfig);
	if (Soft_resetAudio() != APP_SUCCESS) {printf("Soft_resetAudio failed\n");}
	Soft_init();

    ConfigureSru();

    TwiSetAddr(I2cAddrAdau1962);
    ADAU1962_init();

    TwiSetAddr(I2cAddrAdau1979);
    ADAU1979_init();

    AudioIO_setSPDIFState(SPDIF_DIGITAL_ON_OPTICAL_ON);
    AudioIO_applyConfiguration();
    sport_init();

    TwiClose();

    printf("main loop running...\n");
    for (;;)
    {
    	//printPingPongStates();
        // 1) If JACK_RX produced a fresh block, import it into global RX
        if (jackStream.Rx.isFreshData) {
            fillGlobalInputFromAN();           // consumes jackStream.Rx, produces globalStream.Rx
        }

        // 2) If a unified RX block is fresh, run the DSP (global RX -> global TX)
        if (globalStream.Rx.isFreshData) {
            processBlock();                    // consumes globalStream.Rx, produces globalStream.Tx
        }

        // 3) If unified TX is fresh AND JACK_TX has no fresh data pending,
        //    push unified TX into JACK TX frame
        if (globalStream.Tx.isFreshData && !jackStream.Tx.isFreshData) {
			fillDACOutputFromGlobal();         // consumes globalStream.Tx, produces jackStream.Tx
		}
    }
}


