#include "sh2_hal.h"
#include "sh2.h"
#include "i2c.h"
#include "gpio.h"
#include "main.h"


#define BNO08X_ADDR (0x4A << 1)

// Timeout seguro para operaciones I2C (ms)
#define I2C_TIMEOUT_MS 50

// Contador de errores consecutivos
static uint32_t i2c_error_count = 0;
#define I2C_MAX_CONSECUTIVE_ERRORS 3

// Contador de intentos de escritura fallidos (para evitar bloqueo en shtp.c)
static uint32_t write_retry_count = 0;
#define I2C_MAX_WRITE_RETRIES 10

static int hal_open(sh2_Hal_t *self);
static void hal_close(sh2_Hal_t *self);
static int hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us);
static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len);
static uint32_t hal_getTimeUs(sh2_Hal_t *self);

static sh2_Hal_t hal;

static void sh2_eventCallback(void *cookie, sh2_AsyncEvent_t *event);

// Función para resetear el bus I2C
static void i2c_bus_reset(void);
// Función para recuperar el bus I2C después de errores
static int i2c_recover(void);




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
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, pBuffer, len, I2C_TIMEOUT_MS);
    
    if (status != HAL_OK)
    {
        i2c_error_count++;
        
        // Si hay demasiados errores consecutivos, intentar recuperar el bus
        if (i2c_error_count >= I2C_MAX_CONSECUTIVE_ERRORS)
        {
            i2c_recover();
        }
        
        // Retornar 0 indica que no hay datos disponibles (SH2 espera 0 cuando no hay datos)
        return 0;
    }
    
    // Éxito: resetear contador de errores
    i2c_error_count = 0;
    *t_us = hal_getTimeUs(self);
    return len;
}

static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, pBuffer, len, I2C_TIMEOUT_MS);
    
    if (status != HAL_OK)
    {
        i2c_error_count++;
        write_retry_count++;
        
        // Si hay demasiados errores consecutivos, intentar recuperar el bus
        if (i2c_error_count >= I2C_MAX_CONSECUTIVE_ERRORS)
        {
            i2c_recover();
        }
        
        // Si hay demasiados reintentos de escritura, retornar error negativo
        // para que shtp.c detecte el error y no se bloquee indefinidamente
        if (write_retry_count >= I2C_MAX_WRITE_RETRIES)
        {
            write_retry_count = 0;
            return -1;  // Error: shtp.c maneja valores negativos
        }
        
        // Retornar 0 indica que no se pudo escribir (SH2 lo interpreta como "no listo")
        return 0;
    }
    
    // Éxito: resetear contadores de errores
    i2c_error_count = 0;
    write_retry_count = 0;
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
            // El BNO08X se reseteó - resetear contador de errores
            i2c_error_count = 0;
            break;

        case SH2_SHTP_EVENT:
            // Evento de comunicación SHTP
            break;

        default:
            break;
    }
}

// Función para resetear el bus I2C forzando los pines a bajo
static void i2c_bus_reset(void)
{
    // Deshabilitar I2C
    HAL_I2C_DeInit(&hi2c1);
    
    // Configurar SDA y SCL como salidas en bajo para forzar el reset del bus
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;  // SCL y SDA
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Forzar SCL bajo
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(1);
    
    // Generar 9 pulsos de clock para liberar cualquier dispositivo bloqueado
    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
    
    // Restaurar configuración I2C
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Reinicializar I2C
    HAL_I2C_Init(&hi2c1);
}

// Función para recuperar el bus I2C después de errores
static int i2c_recover(void)
{
    // Resetear el bus I2C
    i2c_bus_reset();
    
    // Pequeño delay para estabilización
    HAL_Delay(10);
    
    // Resetear contador de errores después de la recuperación
    i2c_error_count = 0;
    
    return 0;
}

// Función pública para obtener el estado de errores I2C
uint32_t sh2_hal_get_error_count(void)
{
    return i2c_error_count;
}

// Función pública para resetear el contador de errores
void sh2_hal_reset_error_count(void)
{
    i2c_error_count = 0;
    write_retry_count = 0;
}

// Función para reinicializar completamente el BNO08x
int sh2_hal_reinit_bno08x(void)
{
    // Cerrar sesión SH2 actual
    sh2_close();
    
    // Resetear el bus I2C
    i2c_bus_reset();
    
    // Resetear el BNO08x físicamente
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
    
    // Resetear contadores de errores
    i2c_error_count = 0;
    write_retry_count = 0;
    
    // Reabrir sesión SH2
    int status = sh2_open(&hal, sh2_eventCallback, NULL);
    
    return status;
}
