# Solución Segura para Bloqueos I2C - Compatible con CubeMX

## Resumen de Seguridad

Esta solución está diseñada para ser **100% compatible con STM32CubeIDE** y respeta estrictamente los bloques USER CODE. Todos los cambios están clasificados como **SEGURO** o **NO SEGURO** para regeneración de código.

---

## 📋 Cambios por Archivo

### ✅ **SEGURO** - `Core/Src/bno08x/sh2_hal.c`
**Estado**: Archivo de librería externa, NO generado por CubeMX  
**Riesgo**: Ninguno - CubeMX no toca este archivo

#### Cambios Implementados:

1. **Timeout seguro en I2C** (líneas 10-11)
```c
// Timeout seguro para operaciones I2C (ms)
#define I2C_TIMEOUT_MS 50
```
✅ **SEGURO** - Define de usuario

2. **Contadores de error** (líneas 13-19)
```c
static uint32_t i2c_error_count = 0;
#define I2C_MAX_CONSECUTIVE_ERRORS 3
static uint32_t write_retry_count = 0;
#define I2C_MAX_WRITE_RETRIES 10
```
✅ **SEGURO** - Variables estáticas privadas

3. **Detección de errores en hal_read()** (líneas 69-91)
```c
static int hal_read(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len, uint32_t *t_us)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Receive(&hi2c1, BNO08X_ADDR, pBuffer, len, I2C_TIMEOUT_MS);
    
    if (status != HAL_OK)
    {
        i2c_error_count++;
        if (i2c_error_count >= I2C_MAX_CONSECUTIVE_ERRORS)
        {
            i2c_recover();
        }
        return 0;  // No hay datos disponibles
    }
    
    i2c_error_count = 0;
    *t_us = hal_getTimeUs(self);
    return len;
}
```
✅ **SEGURO** - Función estática, no afecta código generado

4. **Detección de errores en hal_write()** (líneas 93-124)
```c
static int hal_write(sh2_Hal_t *self, uint8_t *pBuffer, unsigned len)
{
    HAL_StatusTypeDef status = HAL_I2C_Master_Transmit(&hi2c1, BNO08X_ADDR, pBuffer, len, I2C_TIMEOUT_MS);
    
    if (status != HAL_OK)
    {
        i2c_error_count++;
        write_retry_count++;
        
        if (i2c_error_count >= I2C_MAX_CONSECUTIVE_ERRORS)
        {
            i2c_recover();
        }
        
        // Evitar bloqueo en shtp.c después de muchos reintentos
        if (write_retry_count >= I2C_MAX_WRITE_RETRIES)
        {
            write_retry_count = 0;
            return -1;  // Error para que shtp.c lo detecte
        }
        
        return 0;
    }
    
    i2c_error_count = 0;
    write_retry_count = 0;
    return len;
}
```
✅ **SEGURO** - Función estática, evita bloqueos

5. **Función de recuperación I2C** (líneas 151-201)
```c
static void i2c_bus_reset(void)
{
    HAL_I2C_DeInit(&hi2c1);
    
    // Configurar pines como salidas para reset del bus
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    
    // Generar 9 pulsos de clock
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_Delay(1);
    for (int i = 0; i < 9; i++)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
    
    // Restaurar configuración I2C
    GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
    HAL_I2C_Init(&hi2c1);
}
```
✅ **SEGURO** - Usa HAL, no modifica código generado

6. **Reinicialización completa BNO08x** (líneas 216-239)
```c
int sh2_hal_reinit_bno08x(void)
{
    sh2_close();
    i2c_bus_reset();
    
    // Reset físico del BNO08x
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(RST_imu_GPIO_Port, RST_imu_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
    
    i2c_error_count = 0;
    write_retry_count = 0;
    
    return sh2_open(&hal, sh2_eventCallback, NULL);
}
```
✅ **SEGURO** - Función pública, usa HAL estándar

---

### ✅ **SEGURO** - `Core/Inc/bno08x/sh2_hal.h`
**Estado**: Archivo de librería externa  
**Riesgo**: Ninguno

#### Cambios:
```c
void sh2_hal_init(void);
uint32_t sh2_hal_get_error_count(void);
void sh2_hal_reset_error_count(void);
int sh2_hal_reinit_bno08x(void);
```
✅ **SEGURO** - Solo declaraciones de funciones

---

### ⚠️ **REQUIERE ATENCIÓN** - `Core/Inc/stm32f1xx_hal_conf.h`
**Estado**: Archivo de configuración HAL  
**Riesgo**: Bajo - CubeMX puede regenerar, pero la habilitación de módulos generalmente se preserva

#### Cambio:
```c
#define HAL_IWDG_MODULE_ENABLED
```
⚠️ **REVISAR DESPUÉS DE REGENERAR** - Si CubeMX regenera este archivo, verificar que esta línea esté presente. Si se elimina, el watchdog no funcionará pero no causará errores de compilación (solo warnings).

**Recomendación**: Después de regenerar código con CubeMX, verificar manualmente que esta línea esté presente.

---

### ✅ **SEGURO** - `Core/Src/main.c`
**Estado**: Archivo generado por CubeMX  
**Riesgo**: Todos los cambios están dentro de bloques USER CODE

#### Cambios en `/* USER CODE BEGIN PV */` (líneas 60-81):

```c
/* USER CODE BEGIN PV */
volatile float heading, roll, pitch;
volatile int print_flag = 0;

/* aceleración lineal (m/s²) */
volatile float lin_ax, lin_ay, lin_az;

/* velocidad integrada (mm/s * 1000) */
volatile int32_t velX_mm = 0;
volatile int32_t velY_mm = 0;
volatile int32_t velZ_mm = 0;
volatile uint16_t still_cnt = 0;

/* Watchdog y recuperación */
IWDG_HandleTypeDef hiwdg;
#define WATCHDOG_TIMEOUT_MS 2000
#define I2C_ERROR_RECOVERY_THRESHOLD 5
static uint32_t last_sh2_service_time = 0;
static uint32_t consecutive_errors = 0;
static bool imu_configured = false;
/* USER CODE END PV */
```
✅ **SEGURO** - Todo dentro de USER CODE BEGIN/END PV

#### Cambios en `/* USER CODE BEGIN PFP */` (líneas 85-91):

```c
/* USER CODE BEGIN PFP */
void sensorCallback(void *cookie, sh2_SensorEvent_t *event);
static inline void split_float(float v, int *i, int *f);
static void MX_IWDG_Init(void);
static int reconfigure_imu_sensors(void);
static int recover_imu_communication(void);
/* USER CODE END PFP */
```
✅ **SEGURO** - Prototipos dentro de USER CODE

#### Cambios en `/* USER CODE BEGIN 2 */` (líneas 129-151):

```c
/* USER CODE BEGIN 2 */
// Inicializar watchdog ANTES de cualquier otra cosa crítica
MX_IWDG_Init();

sh2_hal_init();
sh2_setSensorCallback(sensorCallback, NULL);

// Configurar sensores
if (reconfigure_imu_sensors() != SH2_OK)
{
    if (recover_imu_communication() != SH2_OK)
    {
        Error_Handler();
    }
}

imu_configured = true;
last_sh2_service_time = HAL_GetTick();
/* USER CODE END 2 */
```
✅ **SEGURO** - Todo dentro de USER CODE BEGIN 2  
⚠️ **NOTA**: No se modifica ninguna llamada `MX_*_Init()` existente, solo se agrega código después

#### Cambios en `/* USER CODE BEGIN 3 */` (loop principal, líneas 159-249):

```c
/* USER CODE BEGIN 3 */
// Refresh del watchdog en cada iteración
HAL_IWDG_Refresh(&hiwdg);

// Verificación de timeout de comunicación
uint32_t current_time = HAL_GetTick();

if ((current_time - last_sh2_service_time) > 1000)
{
    consecutive_errors++;
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

// Llamar a sh2_service() - NO bloquea
sh2_service();

// Verificar errores I2C
uint32_t i2c_errors = sh2_hal_get_error_count();
if (i2c_errors >= I2C_ERROR_RECOVERY_THRESHOLD)
{
    if (recover_imu_communication() == SH2_OK)
    {
        sh2_hal_reset_error_count();
    }
}

// Procesar datos del sensor
if (print_flag)
{
    print_flag = 0;
    last_sh2_service_time = HAL_GetTick();

    // ... código de impresión ...
    
    // Timeout seguro en UART (100ms en lugar de HAL_MAX_DELAY)
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, 100);
}

HAL_Delay(1);
/* USER CODE END 3 */
```
✅ **SEGURO** - Todo dentro de USER CODE BEGIN 3  
✅ **IMPORTANTE**: Reemplazo de `HAL_MAX_DELAY` por timeout de 100ms

#### Cambios en `/* USER CODE BEGIN 4 */` (funciones, líneas 291-481):

```c
/* USER CODE BEGIN 4 */

static void MX_IWDG_Init(void)
{
    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = IWDG_PRESCALER_32;
    hiwdg.Init.Reload = 1250;
    
    if (HAL_IWDG_Init(&hiwdg) != HAL_OK)
    {
        Error_Handler();
    }
}

static int reconfigure_imu_sensors(void)
{
    // ... configuración de sensores ...
}

static int recover_imu_communication(void)
{
    // ... recuperación completa ...
}

void sensorCallback(void *cookie, sh2_SensorEvent_t *event)
{
    // ... callback existente ...
}

// ... resto de funciones ...
/* USER CODE END 4 */
```
✅ **SEGURO** - Todas las funciones dentro de USER CODE BEGIN 4

---

## 🚫 Cambios NO Realizados (Por Seguridad)

### ❌ **NO MODIFICADO** - `Core/Src/i2c.c`
**Razón**: Archivo generado por CubeMX  
**Acción**: No se toca - toda la lógica de recuperación está en `sh2_hal.c`

### ❌ **NO MODIFICADO** - `Core/Src/gpio.c`
**Razón**: Archivo generado por CubeMX  
**Acción**: No se toca - se usa HAL_GPIO_* directamente en `sh2_hal.c`

### ❌ **NO MODIFICADO** - `Core/Src/bno08x/shtp.c`
**Razón**: Archivo de librería externa, pero modificarlo podría romper compatibilidad  
**Acción**: Se evita el bloqueo modificando `hal_write()` para retornar -1 después de muchos reintentos, lo cual `shtp.c` ya maneja correctamente

### ❌ **NO MODIFICADO** - `Core/Src/bno08x/sh2.c`
**Razón**: Archivo de librería externa  
**Acción**: No se modifica - la detección de errores se hace en la capa HAL

---

## 📝 Checklist de Seguridad para Regeneración CubeMX

Después de regenerar código con CubeMX, verificar:

- [ ] `stm32f1xx_hal_conf.h`: Verificar que `#define HAL_IWDG_MODULE_ENABLED` esté presente
- [ ] `main.c`: Verificar que todos los bloques USER CODE estén intactos
- [ ] `sh2_hal.c`: No debería verse afectado (archivo externo)
- [ ] `sh2_hal.h`: No debería verse afectado (archivo externo)

---

## 🔧 Parámetros Configurables

Todos estos parámetros están en archivos seguros y pueden ajustarse:

### En `sh2_hal.c`:
```c
#define I2C_TIMEOUT_MS 50                    // Timeout I2C (ms)
#define I2C_MAX_CONSECUTIVE_ERRORS 3        // Errores antes de recuperación automática
#define I2C_MAX_WRITE_RETRIES 10            // Reintentos antes de error
```

### En `main.c` (USER CODE):
```c
#define WATCHDOG_TIMEOUT_MS 2000            // Timeout watchdog (ms) - informativo
#define I2C_ERROR_RECOVERY_THRESHOLD 5      // Errores antes de recuperación completa
```

---

## ✅ Resumen de Seguridad

| Archivo | Tipo | Seguro para CubeMX | Notas |
|---------|------|-------------------|-------|
| `sh2_hal.c` | Librería externa | ✅ 100% Seguro | CubeMX no lo toca |
| `sh2_hal.h` | Librería externa | ✅ 100% Seguro | CubeMX no lo toca |
| `main.c` | Generado | ✅ Seguro | Todo en USER CODE |
| `stm32f1xx_hal_conf.h` | Configuración | ⚠️ Revisar | Verificar después de regenerar |
| `i2c.c` | Generado | ✅ No modificado | No se toca |
| `gpio.c` | Generado | ✅ No modificado | No se toca |

---

## 🎯 Objetivos Cumplidos

✅ **Eliminación de bloqueos**: 
- `HAL_MAX_DELAY` reemplazado por timeout de 100ms
- `hal_write()` retorna -1 después de 10 reintentos (evita bloqueo en `shtp.c`)
- Timeouts de 50ms en todas las operaciones I2C

✅ **Timeouts seguros en I2C**:
- `I2C_TIMEOUT_MS = 50` en todas las operaciones
- Detección explícita de errores HAL_I2C

✅ **Recuperación automática**:
- Recuperación del bus I2C después de 3 errores
- Reinicialización completa después de 5 errores
- Función `sh2_hal_reinit_bno08x()` disponible

✅ **Watchdog (IWDG)**:
- Inicializado en USER CODE BEGIN 2
- Refrescado en cada iteración del loop
- Timeout de ~1 segundo

---

## 📚 Referencias

- Documentación STM32 HAL: [STM32F1xx HAL Drivers](https://www.st.com/resource/en/user_manual/um1850-description-of-stm32f1xx-hal-and-lowlayer-drivers-stmicroelectronics.pdf)
- IWDG Configuration: Ver `MX_IWDG_Init()` en `main.c` USER CODE BEGIN 4
- I2C Recovery: Ver `i2c_bus_reset()` en `sh2_hal.c`
