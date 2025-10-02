#ifndef SPUCONFIG_H_
#define SPUCONFIG_H_

#include <stdint.h>

/* Configure the SPU service and set security attributes for SPORT4A/4B.
 * Returns 0 on success, non-zero on failure.
 */
int ConfigureSpu(void);

/* Optional: uninitialize the SPU service (not required for normal run). */
int DeinitSpu(void);

#endif /* SPUCONFIG_H_ */
