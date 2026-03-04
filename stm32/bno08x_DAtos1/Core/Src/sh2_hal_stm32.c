#include "sh2_hal.h"
#include "main.h"

extern I2C_HandleTypeDef hi2c1;

#define BNO08X_ADDR (0x4A << 1)

static uint32_t last_tick = 0;

int sh2_hal_open(sh2_Hal_t *self)
{
    last_tick = HAL_GetTick();
    return 0;
}

void sh2_hal_close(sh2_Hal_t *self)
{
}

int sh2_hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
    if (HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, pBuffer, len, 100) != HAL_OK)
        return -1;

    uint32_t now = HAL_GetTick();
    *t_us = (now - last_tick) * 1000;
    last_tick = now;

    return 0;
}

int sh2_hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    if (HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, pBuffer, len, 100) != HAL_OK)
        return -1;
    return 0;
}

void sh2_hal_delay_us(sh2_Hal_t *self, uint32_t t_us)
{
    HAL_Delay((t_us + 999) / 1000);
}
