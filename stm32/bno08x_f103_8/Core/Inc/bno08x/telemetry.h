/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    telemetry.h
  * @brief   Módulo de telemetría
  * 
  * Este módulo se encarga de enviar el estado del sistema por UART.
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef TELEMETRY_H
#define TELEMETRY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* USER CODE BEGIN Exported types */

/* USER CODE END Exported types */

/* USER CODE BEGIN Exported constants */

/* USER CODE END Exported constants */

/* USER CODE BEGIN Exported macro */

/* USER CODE END Exported macro */

/* USER CODE BEGIN Exported functions prototypes */

/**
  * @brief  Envía el estado del sistema por UART
  *
  * Cadena \@imu con orientación, aceleración lineal (m/s²), encoder y vueltas.
  * Timeout de transmisión 100 ms.
  */
void telemetry_send(void);

/* USER CODE END Exported functions prototypes */

#ifdef __cplusplus
}
#endif

#endif /* TELEMETRY_H */
