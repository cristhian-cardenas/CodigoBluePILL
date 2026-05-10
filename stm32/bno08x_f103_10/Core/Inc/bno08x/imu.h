/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    imu.h
  * @brief   Módulo de gestión del IMU BNO08x
  * 
 * Este módulo encapsula toda la lógica relacionada con el IMU BNO08x:
 * - Inicialización y configuración de sensores
 * - Lectura y procesamiento de datos
 * - Cálculo de orientación (roll, pitch, heading)
 * - Integración de velocidad y posición
 * - Detección de reposo
 * - Actualización de g_systemState con los datos calculados
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef IMU_H
#define IMU_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdbool.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Exported types */

/* USER CODE END Exported types */

/* USER CODE BEGIN Exported constants */

/* USER CODE END Exported constants */

/* USER CODE BEGIN Exported macro */

/* USER CODE END Exported macro */

/* USER CODE BEGIN Exported functions prototypes */

/**
  * @brief  Inicializa el módulo IMU BNO08x
  * 
  * Esta función:
  * - Inicializa el HAL SH2 para comunicación I2C con el BNO08x
  * - Configura los callbacks para recibir eventos del sensor
  * - Configura los sensores Rotation Vector y Linear Acceleration
  * 
  * @retval 0 si tiene éxito, código de error en caso contrario
  * @note   Si falla la inicialización, el watchdog reseteará el MCU
  *         si el sistema queda bloqueado
  */
int imu_init(void);

/**
  * @brief  Servicio principal del módulo IMU (llamar en el loop principal)
  * 
  * Esta función debe llamarse periódicamente en el loop principal:
  * - Lee datos del IMU vía sh2_service() (no bloquea)
  * - Procesa datos si están disponibles
  * - Los datos calculados se copian automáticamente a g_systemState en el callback
  * 
  * @note   Esta función es no bloqueante gracias a los timeouts en sh2_hal.c
  */
void imu_service(void);

/* USER CODE END Exported functions prototypes */

#ifdef __cplusplus
}
#endif

#endif /* IMU_H */
