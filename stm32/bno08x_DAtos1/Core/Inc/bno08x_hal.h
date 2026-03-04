#ifndef BNO08X_HAL_H
#define BNO08X_HAL_H

#include "sh2.h"
#include "sh2_SensorValue.h"
#include <stdint.h>
#include <stdbool.h>

/* Funciones HAL requeridas por SH2 */
int BNO08X_HAL_Open(sh2_Hal_t *self);
int BNO08X_HAL_Close(sh2_Hal_t *self);
int BNO08X_HAL_Read(sh2_Hal_t *self, uint8_t *buf, unsigned len, uint32_t *t_us);
int BNO08X_HAL_Write(sh2_Hal_t *self, uint8_t *buf, unsigned len);
uint32_t BNO08X_HAL_GetTimeUs(sh2_Hal_t *self);

/* Funciones de hardware */
void BNO08X_HAL_HardwareReset(void);
void BNO08X_HAL_SensorHandler(void *cookie, sh2_SensorEvent_t *event);

#endif
