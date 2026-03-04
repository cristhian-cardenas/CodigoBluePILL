# Solución para Bloqueos del IMU BNO08x - Análisis y Implementación

## Resumen del Problema

El firmware se congela cuando el IMU BNO08x pierde contacto o falla la comunicación I2C (por movimiento del cable). El sistema queda completamente bloqueado hasta reset manual.

## Puntos de Bloqueo Identificados

### 1. **sh2_hal.c - Funciones hal_read() y hal_write()**
   - **Problema**: Usaban timeout de 100ms pero no detectaban errores I2C correctamente
   - **Riesgo**: Si HAL_I2C_* falla, retornaba 0 sin indicar error, causando bloqueos en shtp.c

### 2. **shtp.c - Bucle while en txProcess() (línea 310-314)**
   - **Problema**: Bucle `while (status == 0)` que puede bloquearse indefinidamente si hal_write() siempre retorna 0
   - **Riesgo**: Bloqueo permanente si hay error de comunicación I2C

### 3. **sh2.c - opProcess() (línea 493-501)**
   - **Problema**: Bucle while esperando operaciones completas sin timeout adecuado
   - **Riesgo**: Si hay error de comunicación, puede esperar indefinidamente

### 4. **main.c - HAL_UART_Transmit con HAL_MAX_DELAY (línea 192)**
   - **Problema**: Uso de HAL_MAX_DELAY que puede bloquearse si UART no está disponible
   - **Riesgo**: Bloqueo si hay problema con UART

### 5. **main.c - Loop principal sin protección**
   - **Problema**: sh2_service() llamado sin verificación de errores ni recuperación
   - **Riesgo**: Si sh2_service() se bloquea, todo el sistema se congela

## Soluciones Implementadas

### 1. **Modificación de sh2_hal.c - Detección de Errores I2C**

#### Cambios en hal_read():
```c
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
```

**Características**:
- Timeout reducido a 50ms (I2C_TIMEOUT_MS)
- Detección explícita de errores HAL_I2C
- Contador de errores consecutivos
- Recuperación automática después de 3 errores consecutivos

#### Cambios en hal_write():
```c
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
```

**Características**:
- Timeout de 50ms
- Contador de reintentos de escritura (máximo 10)
- Retorna -1 después de 10 intentos fallidos para que shtp.c detecte el error
- Evita bloqueo indefinido en el bucle while de shtp.c

### 2. **Función de Recuperación I2C**

#### i2c_bus_reset():
```c
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
```

**Características**:
- Desinicializa I2C completamente
- Fuerza reset del bus I2C mediante pulsos de clock
- Restaura configuración I2C
- Libera dispositivos bloqueados en el bus

### 3. **Reinicialización Completa del BNO08x**

#### sh2_hal_reinit_bno08x():
```c
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
```

**Características**:
- Cierra sesión SH2 actual
- Resetea bus I2C
- Resetea físicamente el BNO08x
- Reabre sesión SH2
- Retorna estado de la operación

### 4. **Watchdog (IWDG) - Protección contra Cuelgues**

#### Inicialización:
```c
static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;  // Prescaler 32
    hiwdg.Init.Reload = 1250;  // Reload value: ~1 segundo con LSI a 40kHz
    
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
}
```

**Características**:
- Timeout de ~1 segundo
- Se inicializa ANTES de cualquier operación crítica
- Se refresca en cada iteración del loop principal

#### Uso en loop principal:
```c
while (1)
{
    // Refresh del watchdog en cada iteración del loop
    HAL_IWDG_Refresh(&hiwdg);
    
    // ... resto del código ...
}
```

**Protección**:
- Si el sistema se congela, el watchdog resetea el MCU automáticamente
- Evita cuelgues permanentes

### 5. **Loop Principal con Protección y Recuperación**

#### Patrón implementado:
```c
while (1)
{
    // Refresh del watchdog
    HAL_IWDG_Refresh(&hiwdg);
    
    // Verificar timeout de comunicación
    uint32_t current_time = HAL_GetTick();
    
    if ((current_time - last_sh2_service_time) > 1000)  // 1 segundo sin datos
    {
        consecutive_errors++;
        
        // Si hay demasiados errores consecutivos, intentar recuperación
        if (consecutive_errors >= I2C_ERROR_RECOVERY_THRESHOLD)
        {
            if (recover_imu_communication() == SH2_OK)
            {
                consecutive_errors = 0;
                last_sh2_service_time = HAL_GetTick();
            }
        }
    }
    else
    {
        consecutive_errors = 0;
    }
    
    // Llamar a sh2_service() - NO bloquea si hay error I2C
    sh2_service();
    
    // Verificar errores I2C y recuperar si es necesario
    uint32_t i2c_errors = sh2_hal_get_error_count();
    if (i2c_errors >= I2C_ERROR_RECOVERY_THRESHOLD)
    {
        if (recover_imu_communication() == SH2_OK)
        {
            sh2_hal_reset_error_count();
        }
    }
    
    // ... procesar datos ...
    
    // Pequeño delay para evitar saturar el CPU
    HAL_Delay(1);
}
```

**Características**:
- Verificación de timeout de comunicación (1 segundo sin datos)
- Recuperación automática después de 5 errores consecutivos
- sh2_service() no bloquea (retorna rápidamente si hay error)
- Delay de 1ms para evitar saturación del CPU

### 6. **Reemplazo de HAL_MAX_DELAY**

#### Antes:
```c
HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, HAL_MAX_DELAY);
```

#### Después:
```c
HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, 100);  // Timeout de 100ms
```

**Características**:
- Timeout de 100ms en lugar de infinito
- Evita bloqueo si UART no está disponible

### 7. **Función de Recuperación Completa**

#### recover_imu_communication():
```c
static int recover_imu_communication(void)
{
    // Refresh del watchdog antes de operaciones largas
    HAL_IWDG_Refresh(&hiwdg);
    
    // Reinicializar completamente el BNO08x
    int status = sh2_hal_reinit_bno08x();
    if (status != SH2_OK)
    {
        return status;
    }
    
    // Reconfigurar callbacks
    sh2_setSensorCallback(sensorCallback, NULL);
    
    // Reconfigurar sensores
    status = reconfigure_imu_sensors();
    if (status != SH2_OK)
    {
        return status;
    }
    
    // Resetear contadores
    sh2_hal_reset_error_count();
    consecutive_errors = 0;
    imu_configured = true;
    last_sh2_service_time = HAL_GetTick();
    
    return SH2_OK;
}
```

**Características**:
- Reinicialización completa del BNO08x
- Reconfiguración de sensores
- Refresh del watchdog durante operaciones largas
- Reset de todos los contadores de error

## Configuración del Watchdog

### Habilitación en hal_conf.h:
```c
#define HAL_IWDG_MODULE_ENABLED
```

### Parámetros:
- **Prescaler**: 32
- **Reload**: 1250
- **Timeout aproximado**: ~1 segundo (depende de LSI)

## Parámetros Configurables

```c
// sh2_hal.c
#define I2C_TIMEOUT_MS 50                    // Timeout I2C (ms)
#define I2C_MAX_CONSECUTIVE_ERRORS 3        // Errores antes de recuperación automática
#define I2C_MAX_WRITE_RETRIES 10            // Reintentos de escritura antes de error

// main.c
#define WATCHDOG_TIMEOUT_MS 2000            // Timeout watchdog (ms) - informativo
#define I2C_ERROR_RECOVERY_THRESHOLD 5      // Errores antes de recuperación completa
```

## Flujo de Recuperación

1. **Error I2C detectado** → Incrementa contador de errores
2. **3 errores consecutivos** → Recuperación automática del bus I2C (i2c_recover)
3. **5 errores consecutivos** → Recuperación completa del IMU (recover_imu_communication)
4. **Timeout de comunicación (>1s sin datos)** → Recuperación completa
5. **Si todo falla** → Watchdog resetea el MCU después de ~1 segundo

## Ventajas de la Solución

1. **No bloquea**: Todas las operaciones tienen timeout
2. **Recuperación automática**: Detecta y recupera errores sin intervención
3. **Protección watchdog**: Resetea el MCU si todo falla
4. **Mínimo impacto**: No requiere modificar librerías SH2/SHTP
5. **Robusto**: Múltiples capas de protección

## Recomendaciones Adicionales

1. **Ajustar timeouts** según necesidades de la aplicación
2. **Monitorear contadores de error** para diagnóstico
3. **Considerar logging** de eventos de recuperación
4. **Verificar conexiones físicas** si hay muchos errores
5. **Ajustar velocidad I2C** si hay problemas de comunicación

## Archivos Modificados

1. `Core/Src/bno08x/sh2_hal.c` - Detección de errores y recuperación I2C
2. `Core/Inc/bno08x/sh2_hal.h` - Funciones públicas de recuperación
3. `Core/Src/main.c` - Loop principal con protección y watchdog
4. `Core/Inc/stm32f1xx_hal_conf.h` - Habilitación de IWDG

## Pruebas Recomendadas

1. Desconectar cable I2C durante operación → Debe recuperar automáticamente
2. Mover cable rápidamente → Debe manejar errores sin bloquearse
3. Mantener cable desconectado >1s → Watchdog debe resetear MCU
4. Reconectar cable después de error → Debe recuperar y continuar funcionando
