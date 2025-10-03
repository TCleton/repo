#include "SPORTConfig.h"
#include "DMAConfig.h"
#include "templateTDM16_Core1.h"
#include <drivers/sport/adi_sport.h>
#include "templateTDM16_Core1.h"
#include <stdio.h>
#include <sys/adi_core.h>
#include "SPUConfig.h"
#include"sharedConfig.h"
#include <sys/cache.h>
#include <stdio.h>
#include <string.h>

static void printEvent(uint32_t event)
{
	printf("\n");
    if (event == ADI_SPORT_HW_ERR_NONE) {
        printf("[Callback event] No event\n");
        return;
    }

    // Print all documented bits that are set
    if (event & ADI_SPORT_HW_ERR_PRIMARY_CHNL_UNDERFLOW)
        printf("[Callback event] TX primary underflow\n");

    if (event & ADI_SPORT_HW_ERR_PRIMARY_CHNL_OVERFLOW)
        printf("[Callback event] RX primary overflow\n");

    if (event & ADI_SPORT_HW_ERR_SECONDARY_CHNL_UNDERFLOW)
        printf("[Callback event] TX secondary underflow\n");

    if (event & ADI_SPORT_HW_ERR_SECONDARY_CHNL_OVERFLOW)
        printf("[Callback event] RX secondary overflow\n");

    if (event & ADI_SPORT_HW_ERR_FS)
        printf("[Callback event] Frame‑sync error\n");

    if (event & ADI_SPORT_HW_ERR_DMA)
        printf("[Callback event] DMA error\n");

    if (event & ADI_SPORT_EVENT_RX_BUFFER_PROCESSED)
        printf("[Callback event] RX buffer processed\n");

    if (event & ADI_SPORT_EVENT_TX_BUFFER_PROCESSED)
        printf("[Callback event] TX buffer processed\n");

    if (event & ADI_SPORT_EVENT_BUFFER_PROCESSED)
        printf("[Callback event] Global group buffer processed\n");

    // Print any remaining (undocumented) bits, if present
    {
        uint32_t known =
            ADI_SPORT_HW_ERR_PRIMARY_CHNL_UNDERFLOW   |
            ADI_SPORT_HW_ERR_PRIMARY_CHNL_OVERFLOW    |
            ADI_SPORT_HW_ERR_SECONDARY_CHNL_UNDERFLOW |
            ADI_SPORT_HW_ERR_SECONDARY_CHNL_OVERFLOW  |
            ADI_SPORT_HW_ERR_FS                       |
            ADI_SPORT_HW_ERR_DMA                      |
            ADI_SPORT_EVENT_RX_BUFFER_PROCESSED       |
            ADI_SPORT_EVENT_TX_BUFFER_PROCESSED       |
            ADI_SPORT_EVENT_BUFFER_PROCESSED;

        uint32_t unknown = event & ~known;
        if (unknown) {
            printf("[Callback event] other bits: 0x%08lX\n", (unsigned long)unknown);
        }
    }
}




typedef enum { BUF_PING = 0, BUF_PONG = 1 } BufSel;

// Assumptions from your DMATransfer() order:
// - RX descriptor list starts at Ping -> first RX processed will be PING
// - TX descriptor list starts at Ping -> TX starts sending PING first
static volatile BufSel rxNextProcessed = BUF_PING;  // which RX half will report next
static volatile BufSel txNow           = BUF_PING;  // TX half currently being sent
static volatile BufSel txFree          = BUF_PONG;  // therefore PONG is free at t0
static volatile int    txFreeValid     = 1;         // free TX half available at t0

// NEW: latch the most recent RX completion when TX isn't free yet
static volatile int    rxPendingValid  = 0;
static volatile BufSel rxPendingSel    = BUF_PING;

static inline void copyRxToTx(BufSel rxSel, BufSel txSel)
{
    const int32_t* __restrict src =
        (rxSel == BUF_PING) ? (const int32_t*)bufferRxPing : (const int32_t*)bufferRxPong;
    int32_t* __restrict dst =
        (txSel == BUF_PING) ? (int32_t*)bufferTxPing : (int32_t*)bufferTxPong;

    // Your fan-out (L,R,0,0) repeated across 12 slots:
    map4to12(src, dst);
}

// ---- REPLACE your SportCallback with this no-drop version ----
void SportCallback(void *pAppHandle, uint32_t event, void *pArg)
{
    switch (event)
    {
        case ADI_SPORT_EVENT_RX_BUFFER_PROCESSED:
        {
            // Identify which RX half just completed
            BufSel rxJust = rxNextProcessed;
            rxNextProcessed = (rxNextProcessed == BUF_PING) ? BUF_PONG : BUF_PING;

            if (txFreeValid) {
                // TX free now -> copy immediately
                copyRxToTx(rxJust, txFree);
                txFreeValid    = 0;       // free slot consumed
                rxPendingValid = 0;       // nothing pending anymore
            } else {
                // TX not free yet -> remember this RX until TX finishes
                rxPendingSel   = rxJust;
                rxPendingValid = 1;
            }
            break;
        }

        case ADI_SPORT_EVENT_TX_BUFFER_PROCESSED:
        {
            // The TX buffer that just finished becomes the new free buffer
            BufSel justSent = txNow;
            txNow = (txNow == BUF_PING) ? BUF_PONG : BUF_PING;
            txFree = justSent;

            if (rxPendingValid) {
                // We have a latched RX block waiting -> use the newly free TX half now
                copyRxToTx(rxPendingSel, txFree);
                rxPendingValid = 0;
                txFreeValid    = 0; // we've consumed the free slot by writing into it
            } else {
                // No RX ready -> leave the free slot available
                txFreeValid = 1;
            }
            break;
        }

        default:
            break;
    }
}


static volatile uint8_t rxCallbackCount = 0;
void SportCallback2(void *pAppHandle, uint32_t event, void *pArg)
{
	//printEvent(event);
	switch (event)
	{
		/* CASE (buffer processed) */
		case ADI_SPORT_EVENT_RX_BUFFER_PROCESSED:
			rxCallbackCount += 1;
			if (rxCallbackCount == 1) {
				map4to12((const int32_t*)bufferRxPing, (int32_t*)bufferTxPing);
			}
			if (rxCallbackCount == 2) {
				map4to12((const int32_t*)bufferRxPong, (int32_t*)bufferTxPong);
				rxCallbackCount = 0;
			}
			break;
		default:
			break;
	}
}

static inline int _chk(const char* what, ADI_SPORT_RESULT r)
{
    if (r != ADI_SPORT_SUCCESS && r != ADI_SPORT_TRANSFER_IN_PROGRESS) {
        printf("%s rc=%d\n", what, (int)r);
        return APP_FAILED;
    }
    return APP_SUCCESS;
}

int sport_init(void)
{
    ADI_SPORT_RESULT r;

    /* 0) Open SPORT4 halves:
       - 4A = TX  -> ADAU1962A (DAC)
       - 4B = RX <-  ADAU1979 (ADC)
       Both in Multichannel (TDM) mode.
    */
    r = adi_sport_Open(
            SportDeviceNum4,
            ADI_HALF_SPORT_A,
            ADI_SPORT_DIR_TX,
            ADI_SPORT_MC_MODE,
            memorySport4ATx,
            sizeof memorySport4ATx,
            &handleSport4ATx);
    if (_chk("Open 4A TX", r)) return APP_FAILED;

    r = adi_sport_Open(
            SportDeviceNum4,
            ADI_HALF_SPORT_B,
            ADI_SPORT_DIR_RX,
            ADI_SPORT_MC_MODE,
            memorySport4BRx,
            sizeof memorySport4BRx,
            &handleSport4BRx);
    if (_chk("Open 4B RX", r)) return APP_FAILED;

    //adi_sport_MuxHalfSport(handleSport4BRx, /*bUseOtherFS*/true, /*bUseOtherClk*/true);
    /* 1) Data format on both halves:
       - 32-bit slots (SLEN = 31)
       - MSB-first
       - RightJustified=false (we're not in RJ; codecs are TDM/LJ)
       - Pack=false (SPORT word packing disabled; MC "DMAPack" is set separately)
    */
    r = adi_sport_ConfigData(
            handleSport4ATx,
            ADI_SPORT_DTYPE_SIGN_FILL, /* right-justify sign fill */
            31,                         /* SLEN */
            /*LSBFirst*/false,
            /*Pack*/false,
            /*RightJustified*/false);
    if (_chk("ConfigData 4A TX", r)) return APP_FAILED;

    r = adi_sport_ConfigData(
            handleSport4BRx,
            ADI_SPORT_DTYPE_SIGN_FILL,
            31,
            /*LSBFirst*/false,
            /*Pack*/false,
            /*RightJustified*/false);
    if (_chk("ConfigData 4B RX", r)) return APP_FAILED;

    /* 2) Clocks (external from ADAU1962A master):
       - TX should LAUNCH on FALLING (DAC samples on RISING).
       - RX must SAMPLE on FALLING because ADAU1979 changes data on RISING (BCLKEDGE=1).  [ADAU1979 DS]
    */
    r = adi_sport_ConfigClock(
            handleSport4ATx,
            /*ratio*/1,
            /*use internal*/false,      /* external BCLK */
            /*bFallingEdge*/true,       /* TX (feeding DAC): DAC latches on falling → SPORT TX should launch on rising → bFallingEdge = false.*/
            /*gated*/false);
    if (_chk("ConfigClock 4A TX (launch on FALLING)", r)) return APP_FAILED;

    r = adi_sport_ConfigClock(
            handleSport4BRx,
            /*ratio*/1,
            /*use internal*/false,      /* external BCLK */
            /*bFallingEdge*/false,       /* RX (listening to ADC): ADC changes on rising → SPORT RX should sample on falling → bFallingEdge = true. */
            /*gated*/false);
    if (_chk("ConfigClock 4B RX (sample on FALLING)", r)) return APP_FAILED;

    /* 3) Frame-sync (LRCLK) in TDM16:
       - Pulse LRCLK (you set LRCLK_MODE=1 in sharedConfig -> codecs).
       - Edge-sensitive FS (not level).
       - Detect the RISING edge of the pulse (active-high FS) in MC mode.  [SPORT API]
       - No late FS; frameDelay=0 for LJ+pulse.
    */
    const bool bEdgeSensitive = true;
    const bool bLateFS        = false;
    const bool bActiveHighFS  = true;  /* rising-edge FS in MC mode */
    const int  frameDelay     = 0;
    const bool dmaPacked      = true;  /* frame-interleaved words in memory */

    r = adi_sport_ConfigFrameSync(
            handleSport4ATx,
            /*FsDiv*/31,                /* not used in external FS, keep nominal */
            /*FSRequired*/false,
            /*internalFS*/false,
            /*dataFS*/false,
            bActiveHighFS,
            bLateFS,
            bEdgeSensitive);
    if (_chk("ConfigFS 4A TX", r)) return APP_FAILED;

    r = adi_sport_ConfigFrameSync(
            handleSport4BRx,
            /*FsDiv*/31,
            /*FSRequired*/false,
            /*internalFS*/false,
            /*dataFS*/false,
            bActiveHighFS,
            bLateFS,
            bEdgeSensitive);
    if (_chk("ConfigFS 4B RX", r)) return APP_FAILED;

    /* 4) Multichannel (TDM16):
       - frameDelay=0 (slot 0 starts at FS edge)
       - numSlots=15 (slots 0..15 = 16 slots)
       - windowSize=0 (no windowing)
       - DMAPack=true (DMA buffers are frame-interleaved)
    */
    r = adi_sport_ConfigMC(
            handleSport4ATx,
            /*frameDelay*/frameDelay,
            /*nNumSlots*/15u,
            /*windowSize*/0u,
            /*bEnableDMAPack*/dmaPacked);
    if (_chk("ConfigMC 4A TX", r)) return APP_FAILED;

    r = adi_sport_ConfigMC(
            handleSport4BRx,
            /*frameDelay*/frameDelay,
            /*nNumSlots*/15u,
            /*windowSize*/0u,
            /*bEnableDMAPack*/dmaPacked);
    if (_chk("ConfigMC 4B RX", r)) return APP_FAILED;

    /* 5) Slot selection:
       - RX: read slots 0..3 (ADAU1979 C1..C4 -> 0..3)
       - TX: drive slots 0..11 (ADAU1962A uses first 12 audio slots)
    */
    r = adi_sport_SelectChannel(handleSport4BRx, 0u, 3u);
    if (_chk("SelectCh 4B RX 0..3", r)) return APP_FAILED;

    r = adi_sport_SelectChannel(handleSport4ATx, 0u, 11u);
    if (_chk("SelectCh 4A TX 0..11", r)) return APP_FAILED;

    /* 6) Prime TX buffers with silence to avoid startup crackles. */
    for (uint32_t k = 0; k < TX_WORDS; ++k) {
        bufferTxPing[k] = 0;
        bufferTxPong[k] = 0;
    }

    /* 7) Register callbacks (recommend printing events inside the callback). */

    adi_sport_RegisterCallback(handleSport4BRx, SportCallback, handleSport4BRx);
    adi_sport_RegisterCallback(handleSport4ATx, SportCallback, handleSport4ATx);

    /* 8) Prepare PDMA descriptor chains (Ping<->Pong circular). */
    PrepareDescriptors();

    /* 9) Submit DMA transfers for RX and TX. */
    r = adi_sport_DMATransfer(
            handleSport4BRx,
            &dmaDescriptorRxPing,
            2,                         /* Ping+Pong list */
            ADI_PDMA_DESCRIPTOR_LIST,
            ADI_SPORT_CHANNEL_PRIM);
    if (_chk("DMATransfer 4B RX", r)) return APP_FAILED;

    r = adi_sport_DMATransfer(
            handleSport4ATx,
            &dmaDescriptorTxPing,
            2,
            ADI_PDMA_DESCRIPTOR_LIST,
            ADI_SPORT_CHANNEL_PRIM);
    if (_chk("DMATransfer 4A TX", r)) return APP_FAILED;

    /* 10) Enable RX first, then TX. */
    r = adi_sport_Enable(handleSport4BRx, true);
    if (_chk("Enable 4B RX", r)) return APP_FAILED;

    r = adi_sport_Enable(handleSport4ATx, true);
    if (_chk("Enable 4A TX", r)) return APP_FAILED;

    printf("SPORT initialized (core=%u)\n", (unsigned)adi_core_id());
    return APP_SUCCESS;
}
