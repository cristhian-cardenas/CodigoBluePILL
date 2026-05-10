/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    system_state.h
  * @brief   Módulo de estado del sistema
  * 
  * Este módulo contiene el estado global del sistema que es actualizado
  * por el módulo IMU y leído por el módulo de telemetría.
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include <stdint.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Exported types */

typedef struct {
    float roll;
    float pitch;
    float heading;
    float lin_ax;
    float lin_ay;
    float lin_az;
    uint8_t imu_status;
    float encoder_angle;
    uint32_t encoder_total_ticks;
} SystemState_t;

/* USER CODE END Exported types */

/* USER CODE BEGIN Exported constants */

/* USER CODE END Exported constants */

/* USER CODE BEGIN Exported macro */

/* USER CODE END Exported macro */

/* USER CODE BEGIN Exported functions prototypes */

/* USER CODE END Exported functions prototypes */

/* USER CODE BEGIN Exported variables */
extern SystemState_t g_systemState;
/* USER CODE END Exported variables */

#ifdef __cplusplus
}
#endif

#endif /* SYSTEM_STATE_H */
