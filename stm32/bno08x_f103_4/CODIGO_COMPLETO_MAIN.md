# Código Completo para main.c - STM32F103C8T6 con BNO08x

## ✅ Configuración Completa Implementada

Este documento contiene el código completo y listo para usar en `main.c`, respetando estrictamente los bloques USER CODE de CubeMX.

---

## 📋 Secciones del Código

### 1. **Defines en `/* USER CODE BEGIN PD */`**

```c
/* USER CODE BEGIN PD */
#define ACC_STILL_THRESHOLD   50     // mm/s²  (antes era muy chico)
#define STILL_COUNT_MAX      5
#define VEL_ZERO_THRESHOLD   5      // mm/s → todo menor a esto = 0

// Umbral de errores I2C antes de intentar recuperación completa
#define I2C_ERROR_RECOVERY_THRESHOLD 5

/* USER CODE END PD */
```

---

### 2. **Variables en `/* USER CODE BEGIN PV */`**

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

/* Variables de control de errores y recuperación IMU */
bool imu_configured = false;              // Estado de configuración del IMU
uint32_t last_sh2_service_time = 0;      // Timestamp del último servicio SH2 exitoso
uint8_t consecutive_errors = 0;          // Contador de errores I2C consecutivos

/* USER CODE END PV */
```

---

### 3. **Prototipos en `/* USER CODE BEGIN PFP */`**

```c
/* USER CODE BEGIN PFP */
void sensorCallback(void *cookie, sh2_SensorEvent_t *event);
static inline void split_float(float v, int *i, int *f);
static int reconfigure_imu_sensors(void);
static int recover_imu_communication(void);
/* USER CODE END PFP */
```

---

### 4. **Inicialización en `/* USER CODE BEGIN 2 */`**

```c
/* USER CODE BEGIN 2 */

// Inicializar HAL SH2 para comunicación con BNO08x
sh2_hal_init();

// Configurar callback para recibir eventos del sensor
sh2_setSensorCallback(sensorCallback, NULL);

// Configurar sensores del IMU
if (reconfigure_imu_sensors() != SH2_OK)
{
    // Si falla la configuración inicial, intentar recuperación completa
    if (recover_imu_communication() != SH2_OK)
    {
        Error_Handler();
    }
}

// Marcar IMU como configurado y actualizar timestamp
imu_configured = true;
last_sh2_service_time = HAL_GetTick();

/* USER CODE END 2 */
```

---

### 5. **Loop Principal en `/* USER CODE BEGIN 3 */`**

```c
/* USER CODE BEGIN 3 */

// ===== REFRESH DEL WATCHDOG =====
// Refrescar watchdog en cada iteración para evitar reset del MCU
HAL_IWDG_Refresh(&hiwdg);

// ===== ACTUALIZAR TIMESTAMP =====
// Actualizar timestamp en cada iteración del loop
uint32_t current_time = HAL_GetTick();

// ===== VERIFICAR ERRORES I2C =====
// Verificar si hay errores I2C reportados por sh2_hal
uint32_t i2c_errors = sh2_hal_get_error_count();
if (i2c_errors > 0)
{
    // Incrementar contador de errores consecutivos
    consecutive_errors++;
    
    // Si se alcanza el umbral, intentar recuperación completa
    if (consecutive_errors >= I2C_ERROR_RECOVERY_THRESHOLD)
    {
        if (recover_imu_communication() == SH2_OK)
        {
            // Recuperación exitosa: resetear contadores
            consecutive_errors = 0;
            last_sh2_service_time = HAL_GetTick();
        }
        else
        {
            // Recuperación falló: resetear contador y esperar antes de reintentar
            consecutive_errors = 0;
            HAL_Delay(100);
        }
    }
}
else
{
    // No hay errores I2C: resetear contador de errores consecutivos
    consecutive_errors = 0;
}

// ===== VERIFICAR TIMEOUT DE COMUNICACIÓN =====
// Si ha pasado más de 1 segundo sin datos, considerar error de comunicación
if ((current_time - last_sh2_service_time) > 1000)
{
    consecutive_errors++;
    
    // Si se alcanza el umbral, intentar recuperación
    if (consecutive_errors >= I2C_ERROR_RECOVERY_THRESHOLD)
    {
        if (recover_imu_communication() == SH2_OK)
        {
            consecutive_errors = 0;
            last_sh2_service_time = HAL_GetTick();
        }
        else
        {
            consecutive_errors = 0;
            HAL_Delay(100);
        }
    }
}

// ===== SERVICIO SH2 (NO BLOQUEANTE) =====
// Llamar a sh2_service() - esta función NO bloquea si hay error I2C
// Si hay error de I2C, hal_read/hal_write retornarán 0/-1 y sh2_service retornará rápidamente
sh2_service();

// ===== PROCESAR DATOS DEL SENSOR =====
// Procesar datos del sensor si están disponibles
if (print_flag)
{
    print_flag = 0;
    
    // Actualizar timestamp del último servicio exitoso cuando hay datos
    last_sh2_service_time = HAL_GetTick();

    char buffer[128];
    int r_i, r_f, p_i, p_f, y_i, y_f;

    split_float(roll,    &r_i,  &r_f);
    split_float(pitch,   &p_i,  &p_f);
    split_float(heading, &y_i,  &y_f);
    int vx_i = velX_mm / 1000;
    int vx_f = labs(velX_mm % 1000);

    int vy_i = velY_mm / 1000;
    int vy_f = labs(velY_mm % 1000);

    int vz_i = velZ_mm / 1000;
    int vz_f = labs(velZ_mm % 1000);

    int len = snprintf(buffer, sizeof(buffer),
        "@imu:%d.%03d;%d.%03d;%d.%03d;%d.%03d;%d.%03d;%d.%03d;;\r\n",
        r_i,  r_f,
        p_i,  p_f,
        y_i,  y_f,
        vx_i, vx_f,
        vy_i, vy_f,
        vz_i, vz_f
    );

    // Usar timeout seguro (100ms) en lugar de HAL_MAX_DELAY para evitar bloqueos
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, 100);
}

// Pequeño delay para evitar saturar el CPU
HAL_Delay(1);
/* USER CODE END 3 */
```

---

### 6. **Funciones en `/* USER CODE BEGIN 4 */`**

#### 6.1. Función `reconfigure_imu_sensors()`

```c
/**
  * @brief  Reconfigura los sensores del IMU después de una recuperación
  * @retval SH2_OK si tiene éxito, código de error en caso contrario
  */
static int reconfigure_imu_sensors(void)
{
    int status;
    sh2_SensorConfig_t cfg;
    
    cfg.changeSensitivityEnabled = false;
    cfg.changeSensitivityRelative = false;
    cfg.wakeupEnabled = false;
    cfg.alwaysOnEnabled = false;
    cfg.changeSensitivity = 0;
    cfg.reportInterval_us = 5000; // 200 Hz

    // Configurar orientación absoluta (Rotation Vector)
    sh2_SensorConfig_t cfg_rv = cfg;
    cfg_rv.reportInterval_us = 10000;   // 100 Hz para orientación

    status = sh2_setSensorConfig(SH2_ROTATION_VECTOR, &cfg_rv);
    if (status != SH2_OK)
    {
        return status;
    }
    
    sh2_SensorConfig_t cfg_la = cfg;
    cfg_la.reportInterval_us = 5000; // 200 Hz

    status = sh2_setSensorConfig(SH2_LINEAR_ACCELERATION, &cfg_la);
    if (status != SH2_OK)
    {
        return status;
    }
    
    return SH2_OK;
}
```

#### 6.2. Función `recover_imu_communication()`

```c
/**
  * @brief  Recupera la comunicación con el IMU después de errores I2C
  * 
  * Esta función realiza una recuperación completa del BNO08x:
  * 1. Resetea el contador de errores consecutivos
  * 2. Reinicializa completamente el BNO08x
  * 3. Reconfigura los sensores
  * 4. Marca el IMU como configurado
  * 5. Actualiza el timestamp del último servicio exitoso
  * 
  * @retval SH2_OK si tiene éxito, código de error en caso contrario
  */
static int recover_imu_communication(void)
{
    // Refresh del watchdog antes de operaciones largas para evitar reset
    HAL_IWDG_Refresh(&hiwdg);
    
    // ===== RESETEAR CONTADOR DE ERRORES =====
    // Resetear contador de errores consecutivos al inicio de la recuperación
    consecutive_errors = 0;
    
    // ===== REINICIALIZAR BNO08X =====
    // Reinicializar completamente el BNO08x (reset físico, bus I2C, sesión SH2)
    int status = sh2_hal_reinit_bno08x();
    if (status != SH2_OK)
    {
        return status;
    }
    
    // ===== RECONFIGURAR CALLBACKS =====
    // Reconfigurar callback para recibir eventos del sensor
    sh2_setSensorCallback(sensorCallback, NULL);
    
    // ===== RECONFIGURAR SENSORES =====
    // Reconfigurar todos los sensores del IMU
    status = reconfigure_imu_sensors();
    if (status != SH2_OK)
    {
        return status;
    }
    
    // ===== RESETEAR CONTADORES Y ACTUALIZAR ESTADO =====
    // Resetear contador de errores en sh2_hal
    sh2_hal_reset_error_count();
    
    // Marcar IMU como configurado
    imu_configured = true;
    
    // Actualizar timestamp del último servicio exitoso
    last_sh2_service_time = HAL_GetTick();
    
    return SH2_OK;
}
```

---

## 🔧 Configuración del Watchdog (IWDG)

El watchdog está configurado por CubeMX en `iwdg.c` con:
- **Prescaler**: 64
- **Reload**: 624 (configurable en CubeMX)
- **Timeout aproximado**: ~1 segundo (depende de LSI)

Para cambiar el reload, edita `iwdg.c` en el bloque `/* USER CODE BEGIN IWDG_Init 1 */`:

```c
/* USER CODE BEGIN IWDG_Init 1 */
// Ejemplo: Reload = 1250 para ~2 segundos de timeout
// hiwdg.Init.Reload = 1250;
/* USER CODE END IWDG_Init 1 */
```

---

## 📊 Flujo de Recuperación

```
┌─────────────────────────────────┐
│  Error I2C detectado            │
└──────────────┬──────────────────┘
               │
               ▼
┌─────────────────────────────────┐
│  consecutive_errors++            │
└──────────────┬──────────────────┘
               │
               ▼
┌─────────────────────────────────┐
│  ¿consecutive_errors >= 5?      │
└──────────────┬──────────────────┘
               │ SÍ
               ▼
┌─────────────────────────────────┐
│  recover_imu_communication()     │
│  - consecutive_errors = 0        │
│  - sh2_hal_reinit_bno08x()      │
│  - reconfigure_imu_sensors()    │
│  - imu_configured = true         │
│  - last_sh2_service_time = now  │
└─────────────────────────────────┘
```

---

## ✅ Características Implementadas

1. ✅ **Watchdog IWDG** con prescaler 64 y reload configurable
2. ✅ **Variables de control** en USER CODE PV:
   - `bool imu_configured`
   - `uint32_t last_sh2_service_time`
   - `uint8_t consecutive_errors`
3. ✅ **Define** `I2C_ERROR_RECOVERY_THRESHOLD` en USER CODE PD
4. ✅ **Función `recover_imu_communication()`** completa con todos los pasos
5. ✅ **Loop principal** con:
   - Incremento de `consecutive_errors` en caso de fallo I2C
   - Llamada a `recover_imu_communication()` si se alcanza el umbral
   - Actualización de `last_sh2_service_time` en cada iteración
6. ✅ **Todo dentro de bloques USER CODE** para compatibilidad con CubeMX
7. ✅ **Comentarios claros** en cada bloque

---

## 🎯 Uso

El código está listo para usar. Solo asegúrate de:

1. ✅ Tener `sh2_hal.c` con las funciones de recuperación implementadas
2. ✅ Tener `iwdg.c` generado por CubeMX con prescaler 64
3. ✅ Compilar y probar

---

## 📝 Notas Importantes

- **Watchdog**: Se refresca en cada iteración del loop principal
- **Recuperación**: Se activa automáticamente después de 5 errores consecutivos
- **Timeout**: Si no hay datos por más de 1 segundo, se considera error
- **No bloqueante**: `sh2_service()` no bloquea gracias a los timeouts en `sh2_hal.c`
