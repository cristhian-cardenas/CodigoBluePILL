#include "sh2_hal.h"
#include "sh2.h"
#include "i2c.h"
#include "gpio.h"
#include "main.h"


#define BNO08X_ADDR (0x4A << 1)

static int hal_open(sh2_Hal_t *self);
static void hal_close(sh2_Hal_t *self);
static int hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
static uint32_t hal_getTimeUs(sh2_Hal_t *self);

static sh2_Hal_t hal;

static void sh2_eventCallback(void *cookie, sh2_AsyncEvent_t *event);




void sh2_hal_init(void)
{
    hal.open      = hal_open;
    hal.close     = hal_close;
    hal.read      = hal_read;
    hal.write     = hal_write;
    hal.getTimeUs = hal_getTimeUs;

    int status = sh2_open(&hal, sh2_eventCallback, NULL);

    if (status != 0)
    {
        while (1);
    }
}


static int hal_open(sh2_Hal_t *self)
{
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
    return 0;
}

static void hal_close(sh2_Hal_t *self)
{
}

static int hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
    if (HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, pBuffer, len, 100) != HAL_OK)
        return 0;

    *t_us = hal_getTimeUs(self);
    return len;
}

static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, pBuffer, len, 100) != HAL_OK)
        return 0;

    return len;
}

static uint32_t hal_getTimeUs(sh2_Hal_t *self)
{
    return HAL_GetTick() * 1000;
}

static void sh2_eventCallback(void *cookie, sh2_AsyncEvent_t *event)
{
    (void)cookie;

    switch (event->eventId)
    {
        case SH2_RESET:
            // El BNO08X se reseteó
            break;

        case SH2_SHTP_EVENT:
            // Evento de comunicación SHTP
            break;

        default:
            break;
    }
}
