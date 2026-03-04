/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    imu.c
  * @brief   Implementación del módulo de gestión del IMU BNO08x
  * 
  * Este módulo encapsula toda la lógica relacionada con el IMU BNO08x.
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "imu.h"

/* USER CODE BEGIN Includes */
#include "sh2.h"
#include "sh2_SensorValue.h"
#include "sh2_err.h"
#include "sh2_hal.h"
#include "bno08x/system_state.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

// Umbrales para detección de reposo y dead-band
#define ACC_STILL_THRESHOLD   50     // mm/s² - umbral para considerar reposo
#define STILL_COUNT_MAX       5      // Número de muestras en reposo antes de resetear velocidad
#define VEL_ZERO_THRESHOLD    5      // mm/s - umbral para dead-band de velocidad

/* USER CODE END Private defines */

/* USER CODE BEGIN Private typedef */

/* USER CODE END Private typedef */

/* USER CODE BEGIN Private macro */

/* USER CODE END Private macro */

/* USER CODE BEGIN Private variables */

// Variables de estado del IMU
static volatile float heading = 0.0f;
static volatile float roll = 0.0f;
static volatile float pitch = 0.0f;

// Estado del IMU (mantenido para compatibilidad con formato de salida UART)
static volatile uint8_t imu_status = 0;

// Aceleración lineal (m/s²)
static volatile float lin_ax = 0.0f;
static volatile float lin_ay = 0.0f;
static volatile float lin_az = 0.0f;

// Velocidad integrada (mm/s * 1000)
static volatile int32_t velX_mm = 0;
static volatile int32_t velY_mm = 0;
static volatile int32_t velZ_mm = 0;
static volatile uint16_t still_cnt = 0;

// Posición integrada (mm - para convertir a metros dividir por 1000)
static volatile int32_t posX_mm = 0;
static volatile int32_t posY_mm = 0;
static volatile int32_t posZ_mm = 0;

// Timestamp de la última muestra de Linear Acceleration (para calcular delta de tiempo real)
static uint64_t last_linear_accel_timestamp_us = 0;

/* USER CODE END Private variables */

/* USER CODE BEGIN Private function prototypes */

/**
  * @brief  Callback llamado por SH2 cuando hay nuevos datos del sensor
  * @param  cookie: Cookie pasado a sh2_setSensorCallback (no usado)
  * @param  event: Evento del sensor con los datos
  */
static void sensorCallback(void *cookie, sh2_SensorEvent_t *event);

/**
  * @brief  Configura los sensores del IMU BNO08x
  * @retval SH2_OK si tiene éxito, código de error en caso contrario
  */
static int configure_imu_sensors(void);


/* USER CODE END Private function prototypes */

/* USER CODE BEGIN Private functions */

/**
  * @brief  Configura los sensores del IMU BNO08x
  * 
  * Configura los sensores Game Rotation Vector y Linear Acceleration con sus
  * respectivos intervalos de reporte:
  * - Game Rotation Vector: 100 Hz (cada 10ms)
  * - Linear Acceleration: 200 Hz (cada 5ms)
  * 
  * @retval SH2_OK si tiene éxito, código de error en caso contrario
  */
static int configure_imu_sensors(void)
{
    int status;
    sh2_SensorConfig_t cfg;
    
    // Configuración base para todos los sensores
    cfg.changeSensitivityEnabled = false;
    cfg.changeSensitivityRelative = false;
    cfg.wakeupEnabled = false;
    cfg.alwaysOnEnabled = false;
    cfg.changeSensitivity = 0;
    cfg.reportInterval_us = 5000; // 200 Hz

    // ===== CONFIGURAR GAME ROTATION VECTOR (ORIENTACIÓN) =====
    // Configurar orientación (Game Rotation Vector) a 100 Hz
    sh2_SensorConfig_t cfg_rv = cfg;
    cfg_rv.reportInterval_us = 10000;   // 100 Hz para orientación

    status = sh2_setSensorConfig(SH2_GAME_ROTATION_VECTOR, &cfg_rv);
    if (status != SH2_OK)
    {
        return status;
    }
    
    // ===== CONFIGURAR LINEAR ACCELERATION (ACELERACIÓN LINEAL) =====
    // Configurar aceleración lineal a 200 Hz
    sh2_SensorConfig_t cfg_la = cfg;
    cfg_la.reportInterval_us = 5000; // 200 Hz

    status = sh2_setSensorConfig(SH2_LINEAR_ACCELERATION, &cfg_la);
    if (status != SH2_OK)
    {
        return status;
    }
    
    return SH2_OK;
}

/**
  * @brief  Callback llamado por SH2 cuando hay nuevos datos del sensor
  * 
  * Procesa los datos recibidos del IMU:
  * - Linear Acceleration: Calcula velocidad integrada, detecta reposo, aplica dead-band
  * - Game Rotation Vector: Calcula roll, pitch, heading y activa flag para envío por UART
  * 
  * @param  cookie: Cookie pasado a sh2_setSensorCallback (no usado)
  * @param  event: Evento del sensor con los datos
  */
static void sensorCallback(void *cookie, sh2_SensorEvent_t *event)
{
    sh2_SensorValue_t value;

    // Decodificar el evento del sensor
    if (sh2_decodeSensorEvent(&value, event) != SH2_OK)
        return;

    switch (value.sensorId)
    {
        case SH2_LINEAR_ACCELERATION:
        {
            // ===== LECTURA DE ACELERACIÓN LINEAL =====
            // Leer aceleración lineal en m/s²
            lin_ax = value.un.linearAcceleration.x;
            lin_ay = value.un.linearAcceleration.y;
            lin_az = value.un.linearAcceleration.z;

            // Convertir a mm/s² para cálculos internos
            int32_t ax_mm_s2 = (int32_t)(lin_ax * 1000.0f);
            int32_t ay_mm_s2 = (int32_t)(lin_ay * 1000.0f);
            int32_t az_mm_s2 = (int32_t)(lin_az * 1000.0f);

            // ===== CÁLCULO DEL DELTA DE TIEMPO REAL =====
            // Usar el timestamp del evento para calcular el delta de tiempo real
            // Esto es más preciso que asumir un intervalo fijo, especialmente si hay
            // errores de comunicación o variaciones en el intervalo de reporte
            uint64_t current_timestamp_us = value.timestamp;
            uint64_t delta_time_us = 0;
            
            if (last_linear_accel_timestamp_us != 0)
            {
                // Calcular delta de tiempo desde la última muestra
                delta_time_us = current_timestamp_us - last_linear_accel_timestamp_us;
                
                // Limitar el delta a un máximo razonable (ej: 100ms) para evitar
                // errores grandes si hay pérdida de datos o reinicio del timestamp
                if (delta_time_us > 100000)  // 100ms máximo
                {
                    delta_time_us = 100000;
                }
            }
            else
            {
                // Primera muestra: usar el intervalo configurado como aproximación
                delta_time_us = 5000;  // 5ms (200 Hz)
            }
            
            // Guardar timestamp actual para la próxima iteración
            last_linear_accel_timestamp_us = current_timestamp_us;
            
            // Convertir delta de tiempo de microsegundos a segundos (como float)
            float delta_time_s = (float)delta_time_us / 1000000.0f;

            // ===== DETECCIÓN DE REPOSO =====
            // Verificar si el dispositivo está en reposo (aceleración < umbral)
            bool is_still =
                (abs(ax_mm_s2) < ACC_STILL_THRESHOLD) &&
                (abs(ay_mm_s2) < ACC_STILL_THRESHOLD) &&
                (abs(az_mm_s2) < ACC_STILL_THRESHOLD);

            if (is_still)
            {
                // Incrementar contador de muestras en reposo
                still_cnt++;
                // Si se detecta reposo durante STILL_COUNT_MAX muestras consecutivas,
                // resetear la velocidad, posición y timestamp para evitar acumulación
                // de errores en el cálculo del delta de tiempo
                if (still_cnt >= STILL_COUNT_MAX)
                {
                    velX_mm = 0;
                    velY_mm = 0;
                    velZ_mm = 0;
                    posX_mm = 0;
                    posY_mm = 0;
                    posZ_mm = 0;
                    last_linear_accel_timestamp_us = 0;  // Resetear timestamp
                }
            }
            else
            {
                // No está en reposo: resetear contador y actualizar velocidad y posición
                still_cnt = 0;

                // ===== INTEGRACIÓN DE VELOCIDAD CON DELTA DE TIEMPO REAL =====
                // Integrar aceleración para obtener velocidad usando el delta de tiempo real
                // Velocidad = velocidad_anterior + aceleración × Δt
                // 
                // Nota: ax_mm_s2 está en mm/s², delta_time_s está en segundos
                // Resultado: mm/s² × s = mm/s, luego multiplicamos por 1000 para mantener
                // la precisión en mm/s × 1000
                velX_mm += (int32_t)(ax_mm_s2 * delta_time_s * 1000.0f);
                velY_mm += (int32_t)(ay_mm_s2 * delta_time_s * 1000.0f);
                velZ_mm += (int32_t)(az_mm_s2 * delta_time_s * 1000.0f);

                // ===== INTEGRACIÓN DE POSICIÓN CON DELTA DE TIEMPO REAL =====
                // Integrar velocidad para obtener posición usando el delta de tiempo real
                // Posición = posición_anterior + velocidad × Δt
                // 
                // Nota: velX_mm está en mm/s × 1000, delta_time_s está en segundos
                // Resultado: (mm/s × 1000) × s = mm × 1000, luego dividimos por 1000
                // para obtener posición en mm
                posX_mm += (int32_t)((velX_mm * delta_time_s) / 1000.0f);
                posY_mm += (int32_t)((velY_mm * delta_time_s) / 1000.0f);
                posZ_mm += (int32_t)((velZ_mm * delta_time_s) / 1000.0f);
            }

            // ===== DEAD-BAND DE VELOCIDAD =====
            // Aplicar dead-band: velocidades menores al umbral se consideran cero
            // Esto elimina drift y ruido en reposo
            if (abs(velX_mm) < VEL_ZERO_THRESHOLD) velX_mm = 0;
            if (abs(velY_mm) < VEL_ZERO_THRESHOLD) velY_mm = 0;
            if (abs(velZ_mm) < VEL_ZERO_THRESHOLD) velZ_mm = 0;

            // Copiar datos calculados a g_systemState
            g_systemState.posX_mm = posX_mm;
            g_systemState.posY_mm = posY_mm;
            g_systemState.posZ_mm = posZ_mm;

            break;
        }

        case SH2_GAME_ROTATION_VECTOR:
        {
            // ===== CÁLCULO DE ORIENTACIÓN (ROLL, PITCH, HEADING) =====
            // Leer cuaternión del Game Rotation Vector
            float qw = value.un.gameRotationVector.real;
            float qx = value.un.gameRotationVector.i;
            float qy = value.un.gameRotationVector.j;
            float qz = value.un.gameRotationVector.k;

            // Convertir cuaternión a ángulos de Euler (en grados)
            // Roll: rotación alrededor del eje X
            roll  = atan2f(2.0f*(qw*qx + qy*qz),
                            1.0f - 2.0f*(qx*qx + qy*qy)) * 57.2958f;

            // Pitch: rotación alrededor del eje Y
            pitch = asinf (2.0f*(qw*qy - qz*qx)) * 57.2958f;

            // Heading (Yaw): rotación alrededor del eje Z
            // Fórmula estándar: yaw = atan2(2*(qw*qz + qx*qy), 1 - 2*(qy² + qz²))
            float new_heading = atan2f(2.0f*(qw*qz + qx*qy),
                                      1.0f - 2.0f*(qy*qy + qz*qz)) * 57.295779513f; // 180/π con mayor precisión
            
            // Normalización robusta al rango 0-360° (sin clamping ni remapeo)
            // atan2f devuelve -180° a +180°, sumamos 360° si es negativo
            if (new_heading < 0.0f)
            {
                new_heading += 360.0f;
            }
            
            // ===== VERIFICAR QUE NEW_HEADING SEA VÁLIDO (FINITO) =====
            // Protección contra valores NaN/Inf corruptos del cuaternión
            // Si new_heading es finito, actualizar el heading
            bool heading_finite = (new_heading == new_heading) && (new_heading >= 0.0f) && (new_heading <= 360.0f);
            
            if (heading_finite)
            {
                heading = new_heading;
            }

            // Copiar datos calculados a g_systemState
            g_systemState.roll = roll;
            g_systemState.pitch = pitch;
            g_systemState.heading = heading;
            g_systemState.posX_mm = posX_mm;
            g_systemState.posY_mm = posY_mm;
            g_systemState.posZ_mm = posZ_mm;
            g_systemState.imu_status = imu_status;
            break;
        }

        default:
            break;
    }
}

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

/**
  * @brief  Inicializa el módulo IMU BNO08x
  * 
  * Esta función realiza la inicialización completa del IMU:
  * 1. Inicializa el HAL SH2 para comunicación I2C con el BNO08x
  * 2. Configura el callback para recibir eventos del sensor
  * 3. Configura los sensores Game Rotation Vector y Linear Acceleration
  * 
  * @retval 0 si tiene éxito, código de error en caso contrario
  * @note   Si falla la inicialización, el watchdog reseteará el MCU
  *         si el sistema queda bloqueado
  */
int imu_init(void)
{
    // ===== INICIALIZAR HAL SH2 =====
    // Inicializar la capa HAL para comunicación I2C con el BNO08x
    sh2_hal_init();

    // ===== CONFIGURAR CALLBACK =====
    // Configurar callback para recibir eventos del sensor
    // Este callback será llamado automáticamente cuando haya nuevos datos
    sh2_setSensorCallback(sensorCallback, NULL);
    
    // ===== CONFIGURAR SENSORES =====
    // Configurar los sensores del IMU (Game Rotation Vector y Linear Acceleration)
    if (configure_imu_sensors() != SH2_OK)
    {
        // Si falla la configuración, retornar error
        // El watchdog reseteará el MCU si el sistema queda bloqueado
        return -1;
    }
    
    // ===== INICIALIZAR VARIABLES DE ESTADO =====
    // Resetear timestamp para que la primera muestra use el intervalo configurado
    last_linear_accel_timestamp_us = 0;
    
    return 0;
}

/**
  * @brief  Servicio principal del módulo IMU (llamar en el loop principal)
  * 
  * Esta función debe llamarse periódicamente en el loop principal:
  * 1. Lee datos del IMU vía sh2_service() (no bloquea)
  * 2. Procesa datos si están disponibles (en el callback)
  * 3. Los datos calculados se copian automáticamente a g_systemState en el callback
  * 
  * @note   Esta función es no bloqueante gracias a los timeouts en sh2_hal.c
  */
void imu_service(void)
{
    // ===== SERVICIO SH2 (NO BLOQUEANTE) =====
    // Llamar a sh2_service() para leer datos del IMU
    // Esta función NO bloquea si hay error I2C:
    // - Si hay error de I2C, hal_read/hal_write retornarán 0/-1
    // - sh2_service() retornará rápidamente sin bloquear
    sh2_service();
}

/* USER CODE END Exported functions */
