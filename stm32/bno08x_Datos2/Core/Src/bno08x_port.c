#include "main.h"
#include "sh2.h"
#include "sh2_hal.h"
#include <string.h>

extern I2C_HandleTypeDef hi2c1;
#define BNO08X_ADDR (0x4A << 1) // Prueba con 0x4A, si falla cambia a 0x4B

// --- 1. Prototipos de funciones ---
// Esto es necesario para que la estructura bno_hal sepa que las funciones existen abajo
uint32_t HAL_GetTimeUs(sh2_Hal_t *self);
int bno08x_open(sh2_Hal_t *self);
int bno08x_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
int bno08x_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);

// --- 2. Definición de la estructura HAL ---
sh2_Hal_t bno_hal = {
    .open = bno08x_open,
    .close = NULL,
    .read = bno08x_read,
    .write = bno08x_write,
    .getTimeUs = HAL_GetTimeUs
};

// --- 3. Implementación de las funciones ---

uint32_t HAL_GetTimeUs(sh2_Hal_t *self) {
    return HAL_GetTick() * 1000;
}

int bno08x_open(sh2_Hal_t *self) {
    // Soft Reset: {longitud_low, longitud_high, canal, secuencia, comando}
    uint8_t softreset_pkt[] = {5, 0, 1, 0, 1};

    // El BNO08x necesita un tiempo tras el reset
    if (HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, softreset_pkt, 5, 100) == HAL_OK) {
        HAL_Delay(300);
        return 0;
    }
    return -1;
}

int bno08x_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us) {
    uint8_t header[4];
    // Timeout corto de 10ms para polling rápido
    if (HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, header, 4, 10) != HAL_OK) {
        return 0;
    }
    // ... resto de tu lógica de packet_size ...
}

int bno08x_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len) {
    if (HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, pBuffer, len, 100) != HAL_OK) {
        return 0;
    }
    return len;
}
