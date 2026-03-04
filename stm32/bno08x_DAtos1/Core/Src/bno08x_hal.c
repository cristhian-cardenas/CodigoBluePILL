#include "bno08x_hal.h"
#include "main.h"      // para hi2c1, huart2, pines, HAL_Delay
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;  // declara el handler de I2C que CubeMX generó
extern UART_HandleTypeDef huart2;
#define BNO08X_ADDR  (0x4A << 1)

/* Funciones HAL para SH2 */

int BNO08X_HAL_Open(sh2_Hal_t *self)
{
    // Aquí podrías mandar un soft reset si quieres
    return 0;
}

int BNO08X_HAL_Close(sh2_Hal_t *self)
{
    // Nada por ahora
    return 0;
}

int BNO08X_HAL_Read(sh2_Hal_t *self, uint8_t *buf, unsigned len, uint32_t *t_us)
{
    // Copiar tu función bno_read_raw
    uint8_t header[4];
    if(HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, header, 4, 100) != HAL_OK)
        return 0;

    uint16_t packet_len = header[0] | (header[1] << 8);
    packet_len &= ~0x8000;

    if(packet_len > len) return 0;

    HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, buf, packet_len, 100);
    if(t_us) *t_us = HAL_GetTick() * 1000;
    return packet_len;
}

int BNO08X_HAL_Write(sh2_Hal_t *self, uint8_t *buf, unsigned len)
{
    if(HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, buf, len, 100) != HAL_OK)
        return 0;
    return len;
}

uint32_t BNO08X_HAL_GetTimeUs(sh2_Hal_t *self)
{
    return HAL_GetTick() * 1000;
}

void BNO08X_HAL_HardwareReset(void)
{
    // Usando pin de reset si existe
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET); // ejemplo
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
    HAL_Delay(10);
}

void BNO08X_HAL_SensorHandler(void *cookie, sh2_SensorEvent_t *event)
{
    // por ahora solo vacío, lo maneja bno08x.c
}
