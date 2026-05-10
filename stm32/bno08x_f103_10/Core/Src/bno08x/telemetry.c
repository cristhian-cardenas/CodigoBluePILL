/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    telemetry.c
  * @brief   Implementación del módulo de telemetría
  * 
  * Este módulo se encarga de enviar el estado del sistema por UART.
  * 
  * @note    Este archivo es seguro para CubeMX - no se modifica al regenerar código
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "telemetry.h"
#include "usart.h"

/* USER CODE BEGIN Includes */
#include "system_state.h"
#include <stdio.h>
#include <math.h>
/* USER CODE END Includes */

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

/* USER CODE BEGIN Private typedef */

/* USER CODE END Private typedef */

/* USER CODE BEGIN Private macro */

/* USER CODE END Private macro */

/* USER CODE BEGIN Private variables */

/* USER CODE END Private variables */

/* USER CODE BEGIN Private function prototypes */

/**
  * @brief  Divide un float en parte entera y fraccionaria (para formateo)
  * @param  v: Valor float a dividir
  * @param  i: Puntero para almacenar parte entera
  * @param  f: Puntero para almacenar parte fraccionaria (×1000)
  */
static inline void split_float(float v, int *i, int *f);

/* USER CODE END Private function prototypes */

/* USER CODE BEGIN Private functions */

/**
  * @brief  Divide un float en parte entera y fraccionaria (para formateo)
  * 
  * Útil para formatear números flotantes en formato "entero.fraccionaria"
  * donde la fraccionaria está multiplicada por 1000.
  * 
  * @param  v: Valor float a dividir
  * @param  i: Puntero para almacenar parte entera
  * @param  f: Puntero para almacenar parte fraccionaria (×1000)
  */
static inline void split_float(float v, int *i, int *f)
{
    *i = (int)v;
    *f = (int)fabsf((v - *i) * 10.0f);
}

/* USER CODE END Private functions */

/* USER CODE BEGIN Exported functions */

/**
  * @brief  Envía el estado del sistema por UART
  * 
  * Lee g_systemState y formatea el string exacto para envío por UART.
  * Usa timeout de 100ms para evitar bloqueos.
  */
void telemetry_send(void)
{
    // ===== FORMATEAR DATOS =====
    // Preparar buffer para envío por UART
    char buffer[128];
    int r_i, r_f, p_i, p_f;

    // Dividir roll y pitch en parte entera y fraccionaria
    split_float(g_systemState.roll,    &r_i,  &r_f);
    split_float(g_systemState.pitch,   &p_i,  &p_f);

    // Vueltas totales para odometría (3 decimales)
    double total_turns = (double)g_systemState.encoder_total_ticks / 4096.0;

    // Formato: @imu:roll;pitch;heading;lin_ax;lin_ay;lin_az;encoder_angle;total_turns;;\r\n
    int len = snprintf(buffer, sizeof(buffer),
    	"@imu:%d.%01d;%d.%01d;%.1f;%.3f;%.3f;%.3f;%.1f;%.3f;;\r\n",
        r_i,  r_f,
        p_i,  p_f,
        g_systemState.heading,
        g_systemState.lin_ax,
        g_systemState.lin_ay,
        g_systemState.lin_az,
        g_systemState.encoder_angle,
        total_turns
    );

    // ===== ENVIAR POR UART =====
    // Usar timeout seguro (100ms) en lugar de HAL_MAX_DELAY para evitar bloqueos
    // Si UART no está disponible, retornará después de 100ms sin bloquear
    HAL_UART_Transmit(&huart3, (uint8_t*)buffer, len, 100);
}

/* USER CODE END Exported functions */
