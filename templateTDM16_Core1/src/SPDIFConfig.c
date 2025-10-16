/*
 * SPDIFConfig.c
 *
 *  Created on: 10 oct. 2025
 *      Author: t.cleton
 */


#include "SPDIFConfig.h"
#include <drivers/spdif/adi_spdif_rx.h>
#include <drivers/spdif/adi_spdif_tx.h>

#define CHECK_RESULT(eResult) \
        if(eResult != 0)\
		{\
			return (1);\
        }



