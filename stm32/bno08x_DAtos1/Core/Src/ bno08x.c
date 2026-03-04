#include <string.h>
#include "bno08x.h"    // tu librería principal del sensor
#include "sh2.h"       // <<< esto es lo que falta para SH2_OK
#include "sh2_SensorValue.h"
#include "bno08x_hal.h" // si necesitas acceso a funciones de I2C/HAL
#include <stdio.h>

#ifndef SH2_OK
#define SH2_OK 0
#endif


static sh2_Hal_t hal;
static bool reset_occurred = false;
static sh2_SensorValue_t _sensor_value;

static void hal_callback(void *cookie, sh2_AsyncEvent_t *event)
{
    if(event->eventId == SH2_RESET)
        reset_occurred = true;
}

bool BNO08X_Init(void)
{
    hal.open       = BNO08X_HAL_Open;
    hal.close      = BNO08X_HAL_Close;
    hal.read       = BNO08X_HAL_Read;
    hal.write      = BNO08X_HAL_Write;
    hal.getTimeUs  = BNO08X_HAL_GetTimeUs;

    if(sh2_open(&hal, hal_callback, NULL) != SH2_OK)
        return false;

    sh2_setSensorCallback(BNO08X_HAL_SensorHandler, NULL);

    return true;
}

void BNO08X_HardwareReset(void)
{
    BNO08X_HAL_HardwareReset();
}

bool BNO08X_WasReset(void)
{
    bool tmp = reset_occurred;
    reset_occurred = false;
    return tmp;
}

bool BNO08X_EnableSensor(sh2_SensorId_t sensorId, uint32_t interval_us)
{
    sh2_SensorConfig_t cfg = {0};
    cfg.reportInterval_us = interval_us;
    return sh2_setSensorConfig(sensorId, &cfg) == SH2_OK;
}

bool BNO08X_GetSensorEvent(sh2_SensorValue_t *value)
{
    _sensor_value.timestamp = 0;
    value = &_sensor_value;
    sh2_service();
    return (_sensor_value.timestamp != 0);
}
