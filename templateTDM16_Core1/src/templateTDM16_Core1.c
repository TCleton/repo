/*****************************************************************************
 * templateTDM16_Core1.c
 *****************************************************************************/

#include "adi_initialize.h"
#include "templateTDM16_Core1.h"
#include "ADAU1962Config.h"
#include "ADAU1979Config.h"
#include "sharedConfig.h"
#include "DMAConfig.h"
#include "SPORTConfig.h"
#include "SRUConfig.h"
#include "SPUConfig.h"
#include "SoftConfig.h"
#include <sys/platform.h>
#include <sys/adi_core.h>


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/platform.h>
#include <sys/adi_core.h>
#include "adi_initialize.h"
#include <services/int/adi_int.h>
#include <drivers/sport/adi_sport.h>
#include <services/spu/adi_spu.h>
#include <drivers/twi/adi_twi.h>
#include "math.h"
#include <SRU.h>

// ========================= functions =======================

uint8_t getNumFromBits(int numArrays, uint8_t* arrays[], int sizes[]) {
	// this function takes any number of arrays (numArrays), each array contains a certain number of bits given in the sizes array
	// it bascally creates a uint8_t word from all those bits so sum(sizes) should be 8.
	uint8_t result = 0;
    for (int i = 0; i < numArrays; ++i) {
        for (int j = 0; j < sizes[i]; ++j) {
            result = (result << 1) | (arrays[i][j] & 1);
        }
    }
    return result;
}


static void delay_ms(uint32_t ms)
{
    // Simple coarse delay; at 1 GHz core, ~100k iterations ~0.1 ms (tune as needed)
    volatile uint32_t loops = ms * 100000u;
    while (loops--) { __asm__ __volatile__("nop;"); }
}

// =========================   ENABLE COMMUNICATION TO READ/WRITE TO THE DEVICES VIA TWI/I2C =======================
ADI_TWI_HANDLE   sTwiHandle = NULL;
static uint8_t          sTwiDevMem[ADI_TWI_MEMORY_SIZE];
static uint8_t          sTwiBuf[32]; /* small scratch buffer */


int TwiOpen(void)
{
    ADI_TWI_RESULT r;
    r = adi_twi_Open(TwiDevNum, ADI_TWI_MASTER, sTwiDevMem, sizeof sTwiDevMem, &sTwiHandle);
    if (r != ADI_TWI_SUCCESS) { printf("TWI open failed 0x%X\n", r); return APP_FAILED; }
    if ((r = adi_twi_SetPrescale(sTwiHandle, TwiPrescale)) != ADI_TWI_SUCCESS) { printf("TWI prescale fail 0x%X\n", r); return APP_FAILED; }
    if ((r = adi_twi_SetBitRate(sTwiHandle, TwiBitrateKHz)) != ADI_TWI_SUCCESS){ printf("TWI bitrate fail 0x%X\n", r);  return APP_FAILED; }
    if ((r = adi_twi_SetDutyCycle(sTwiHandle, TwiDutyCyclePct)) != ADI_TWI_SUCCESS){ printf("TWI duty fail 0x%X\n", r);  return APP_FAILED; }
    printf("TWI opened\n");
    return APP_SUCCESS;
}


/* Change the active 7-bit address without closing */
static uint16_t sActiveI2CAddr = 0xFFFF;
int TwiSetAddr(uint16_t addr)
{
    ADI_TWI_RESULT r = adi_twi_SetHardwareAddress(sTwiHandle, addr);
    if (r != ADI_TWI_SUCCESS) {
        printf("TWI: SetHardwareAddress(0x%02X) FAILED (0x%X)\n", addr, r);
        return APP_FAILED;
    }
    sActiveI2CAddr = addr;
    // Optional: small settle is not required, but the print helps debugging
    printf("TWI: active 7-bit addr = 0x%02X\n", addr);
    return APP_SUCCESS;
}

int TwiClose(void)
{
    ADI_TWI_RESULT r = adi_twi_Close(sTwiHandle);
    sTwiHandle = NULL;
    return (r == ADI_TWI_SUCCESS) ? APP_SUCCESS : APP_FAILED;
}

/* Write an 8-bit register of the currently-selected device */
ADI_TWI_RESULT TwiWrite8(uint8_t reg, uint8_t val)
{
    sTwiBuf[0] = reg;
    sTwiBuf[1] = val;
    ADI_TWI_RESULT r = adi_twi_Write(sTwiHandle, sTwiBuf, 2u, false);
    int delay1=0xffff;
	while(delay1--)
	{
		asm("nop;");
	}
    return r;
}

/* Read an 8-bit register of the currently-selected device */
uint8_t TwiRead8(uint8_t reg)
{
    ADI_TWI_RESULT r;
    uint8_t v = 0;
    sTwiBuf[0] = reg;
    r = adi_twi_Write(sTwiHandle, sTwiBuf, 1u, true);  // repeated-start
    if (r == ADI_TWI_SUCCESS) {
        r = adi_twi_Read(sTwiHandle, &v, 1u, false);
    }
    if (r != ADI_TWI_SUCCESS) {
        printf("TWI READ FAIL addr=0x%02X reg=0x%02X err=0x%X\n", sActiveI2CAddr, reg, r);
    }
    return v;
}

// processing :
static void printBits(uint32_t value) {
    for(int bit = 31; bit >= 0; bit--) {
        printf("%d", (value >> bit) & 1);
        if(bit % 8 == 0 && bit != 0) printf(" "); // Space every 8 bits for readability
    }
}

void map4to12(const int32_t* __restrict rx, int32_t* __restrict tx)
{
    for (uint32_t f = 0; f < samplesPerBlock; ++f)
    {
        const uint32_t r = 4u  * f;   // RX base (4 words per frame)
        const uint32_t t = 12u * f;   // TX base (12 words per frame)

        const int32_t in0 = rx[r + 0];
        const int32_t in1 = rx[r + 1];
        const int32_t in2 = rx[r + 2]*0;
        const int32_t in3 = rx[r + 3]*0;

        // Write 12 TX slots for this frame (3x fan-out of the 4 RX channels)
        tx[t + 0]  = in0;  tx[t + 1]  = in1;  tx[t + 2]  = in2;  tx[t + 3]  = in3;
        tx[t + 4]  = in0;  tx[t + 5]  = in1;  tx[t + 6]  = in2;  tx[t + 7]  = in3;
        tx[t + 8]  = in0;  tx[t + 9]  = in1;  tx[t +10]  = in2;  tx[t +11]  = in3;
    }
}


int main(int argc, char *argv[])
{
    adi_initComponents();

    adi_core_enable(ADI_CORE_SHARC1);

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

    sport_init();

    TwiClose();

    printf("main loop running...\n");
    for(;;)
    {
    }
}


