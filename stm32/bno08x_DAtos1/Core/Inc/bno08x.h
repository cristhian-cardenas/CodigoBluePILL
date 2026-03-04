#ifndef BNO08X_H
#define BNO08X_H

#include "sh2.h"
#include "sh2_SensorValue.h"
#include <stdint.h>
#include <stdbool.h>

bool BNO08X_Init(void);
void BNO08X_HardwareReset(void);
bool BNO08X_WasReset(void);
bool BNO08X_EnableSensor(sh2_SensorId_t sensorId, uint32_t interval_us);
bool BNO08X_GetSensorEvent(sh2_SensorValue_t *value);

#endif
