#ifndef SOFTCONFIG_H_
#define SOFTCONFIG_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define I2cAddrSOFTConfig		0x22

int Soft_resetAudio(void);
int Soft_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SOFTCONFIG_H_ */
