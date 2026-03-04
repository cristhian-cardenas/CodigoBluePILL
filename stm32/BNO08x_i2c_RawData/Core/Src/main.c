/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
/* ==================== CONFIGURACIÓN BNO08X ==================== */

#define BNO08X_I2C_ADDR 0x4A // Dirección I2C (SA0 = GND)

// Pines del sensor
#define BNO08X_RST_Pin   D7_Pin
#define BNO08X_RST_Port  D7_GPIO_Port
#define BNO08X_PS1_Pin   D6_Pin
#define BNO08X_PS1_Port  D6_GPIO_Port
#define BNO08X_PS0_Pin   D5_Pin
#define BNO08X_PS0_Port  D5_GPIO_Port
#define BNO08X_INT_Pin   D2_Pin
#define BNO08X_INT_Port  D2_GPIO_Port

// Buffers y banderas
uint8_t buf[256];
volatile uint8_t sensor_data_ready = 0;
volatile uint8_t i2c_error = 0;

// Report IDs (BNO08X - según datasheet CEVA SHTP)
#define SHTP_REPORT_COMMAND_RESPONSE    0xF1
#define SHTP_REPORT_BASE_ACCEL          0x04  // Accelerometer
#define SHTP_REPORT_BASE_GYRO           0x05  // Gyroscope
#define SHTP_REPORT_BASE_MAG            0x06  // Magnetometer

// Escalas de conversión (BNO085/086 según datasheet)
// Acelerómetro: ±8g, 12-bit → ~4mg/LSB @ ±8g
#define ACC_SCALE (4.0f / 1000.0f) * 9.81f  // m/s² (4mg/LSB * 9.81/1000)
// Giroscopio: ±2000°/s, 16-bit → ~61 LSB/°/s  
#define GYR_SCALE (1.0f / 16.4f)  // °/s (1/16.4 °/s per LSB)

/* Prototipos de funciones */
void BNO08X_Init(void);
void BNO08X_SetFeature(uint8_t report_id, uint16_t interval_ms);
void BNO08X_ReadSensorData(void);
HAL_StatusTypeDef BNO08X_SendCommand(uint8_t *data, uint16_t len);
HAL_StatusTypeDef BNO08X_ReadData(uint8_t *data, uint16_t *len);

void BNO08X_Init(void)
{
  // Según datasheet: PS1=0, PS0=0 para modo I2C (ver Figura 1-5)
  // El datasheet dice: "These pins should be tied to ground to select the I2C interface"
  HAL_GPIO_WritePin(BNO08X_PS0_Port, BNO08X_PS0_Pin, GPIO_PIN_RESET);  // LOW
  HAL_GPIO_WritePin(BNO08X_PS1_Port, BNO08X_PS1_Pin, GPIO_PIN_RESET);  // LOW
  HAL_Delay(10);
    
  HAL_UART_Transmit(&huart2, (uint8_t*)"PS pins configured for I2C\r\n", 28, HAL_MAX_DELAY);
  HAL_Delay(100);
    
  // Reset del BNO08X (NRST es active low)
  HAL_GPIO_WritePin(BNO08X_RST_Port, BNO08X_RST_Pin, GPIO_PIN_RESET);  // Reset assert
  HAL_Delay(100);
  HAL_GPIO_WritePin(BNO08X_RST_Port, BNO08X_RST_Pin, GPIO_PIN_SET);    // Reset deassert
  HAL_Delay(500); // Esperar a que el sensor inicie completamente
    
  HAL_UART_Transmit(&huart2, (uint8_t*)"BNO08x Reset Complete\r\n", 24, HAL_MAX_DELAY);
}

/* Simple I2C bus scanner: prints found 7-bit addresses over UART */
void I2C_Scan(void)
{
  char msg[80];
  int found = 0;
  HAL_UART_Transmit(&huart2, (uint8_t*)"Starting I2C bus scan...\r\n", 24, HAL_MAX_DELAY);
  for (uint8_t addr = 1; addr < 127; addr++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(addr << 1), 1, 10) == HAL_OK) {
      int l = sprintf(msg, "I2C device found at 0x%02X\r\n", addr);
      HAL_UART_Transmit(&huart2, (uint8_t*)msg, l, HAL_MAX_DELAY);
      found++;
    }
  }
  if (!found) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"No I2C devices found\r\n", 23, HAL_MAX_DELAY);
  } else {
    int l = sprintf(msg, "I2C scan complete (%d devices)\r\n", found);
    HAL_UART_Transmit(&huart2, (uint8_t*)msg, l, HAL_MAX_DELAY);
  }
}

HAL_StatusTypeDef BNO08X_SendCommand(uint8_t *data, uint16_t len)
{
  char msg[50];
  int msg_len = sprintf(msg, "Sending I2C cmd (%d bytes)...\r\n", len);
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, msg_len, HAL_MAX_DELAY);
  // Esperar a que H_INT se asevere (activo bajo) como hacen las librerías Arduino
  // Si no responde, hacer reset del dispositivo
  int wait_ok = 0;
  for (int i = 0; i < 500; i++) { // 500 ms timeout
    if (HAL_GPIO_ReadPin(BNO08X_INT_Port, BNO08X_INT_Pin) == GPIO_PIN_RESET) { wait_ok = 1; break; }
    HAL_Delay(1);
  }
  if (!wait_ok) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"H_INT timeout before TX, resetting BNO08x\r\n", 39, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(BNO08X_RST_Port, BNO08X_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(BNO08X_RST_Port, BNO08X_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(300);
    return HAL_ERROR;
  }

  HAL_StatusTypeDef ret = HAL_I2C_Master_Transmit(&hi2c1, BNO08X_I2C_ADDR << 1, data, len, 1000);
  if(ret != HAL_OK) {
    i2c_error = 1;
    char err_msg[60];
    int err_len = sprintf(err_msg, "I2C TX Error: 0x%02X\r\n", ret);
    HAL_UART_Transmit(&huart2, (uint8_t*)err_msg, err_len, HAL_MAX_DELAY);
    return ret;
  }
  HAL_UART_Transmit(&huart2, (uint8_t*)"I2C TX OK\r\n", 11, HAL_MAX_DELAY);
  HAL_Delay(20);
  return ret;
}

HAL_StatusTypeDef BNO08X_ReadData(uint8_t *data, uint16_t *len)
{
    uint8_t header[4];
    HAL_StatusTypeDef ret;
    
    // Esperar H_INT (activo bajo) antes de leer, como en la librería Arduino
    int wait_ok = 0;
    for (int i = 0; i < 500; i++) { // 500 ms
      if (HAL_GPIO_ReadPin(BNO08X_INT_Port, BNO08X_INT_Pin) == GPIO_PIN_RESET) { wait_ok = 1; break; }
      HAL_Delay(1);
    }
    if (!wait_ok) {
      HAL_UART_Transmit(&huart2, (uint8_t*)"H_INT timeout before RX header\r\n", 30, HAL_MAX_DELAY);
      return HAL_ERROR;
    }

    // Leer header (4 bytes)
    ret = HAL_I2C_Master_Receive(&hi2c1, BNO08X_I2C_ADDR << 1, header, 4, 1000);
    if(ret != HAL_OK) {
        i2c_error = 1;
        HAL_UART_Transmit(&huart2, (uint8_t*)"I2C RX Header Error\r\n", 21, HAL_MAX_DELAY);
        return ret;
    }
    
    // Parsear longitud del payload (bytes 2 y 3)
    *len = ((uint16_t)header[2] | ((uint16_t)header[3] << 8)) & 0x0FFF;
    
    if(*len > 0 && *len <= 255) {
        // Leer payload
        ret = HAL_I2C_Master_Receive(&hi2c1, BNO08X_I2C_ADDR << 1, data, *len, 100);
        if(ret != HAL_OK) {
            i2c_error = 1;
            HAL_UART_Transmit(&huart2, (uint8_t*)"I2C RX Payload Error\r\n", 22, HAL_MAX_DELAY);
        }
    }
    
    return ret;
}

void BNO08X_SetFeature(uint8_t report_id, uint16_t interval_ms)
{
  char msg[60];
  int msg_len = sprintf(msg, "Setting feature 0x%02X, interval %dms\r\n", report_id, interval_ms);
  HAL_UART_Transmit(&huart2, (uint8_t*)msg, msg_len, HAL_MAX_DELAY);
    
  // Comando para habilitar reportes según SHTP protocol
  // Estructura: [longitud_payload, comando(0xFD), report_id, flags, sensibilidad, intervalo...]
  uint8_t cmd[21];
  uint16_t i = 0;
    
  // Byte 0: Longitud del payload (sin incluir este byte)
  cmd[i++] = 20;  // Los siguientes 20 bytes
    
    // Byte 1: Comando "Set Feature" = 0xFD
    cmd[i++] = 0xFD;
    
    // Byte 2: Report ID a habilitar
    cmd[i++] = report_id;
    
    // Byte 3: Feature flags (0 = sin cambios especiales)
    cmd[i++] = 0x00;
    
    // Byte 4: Change sensitivity (0 = no cambiar sensibilidad)
    cmd[i++] = 0x00;
    
    // Bytes 5-8: Reporting interval en microsegundos (little-endian)
    uint32_t interval_us = interval_ms * 1000;
    cmd[i++] = (interval_us) & 0xFF;
    cmd[i++] = (interval_us >> 8) & 0xFF;
    cmd[i++] = (interval_us >> 16) & 0xFF;
    cmd[i++] = (interval_us >> 24) & 0xFF;
    
    // Bytes 9-12: Batch interval (0 para no usar batching)
    cmd[i++] = 0x00;
    cmd[i++] = 0x00;
    cmd[i++] = 0x00;
    cmd[i++] = 0x00;
    
    // Bytes 13-20: Sensor-specific config (relleno con ceros)
    for(int j = 0; j < 8; j++) {
        cmd[i++] = 0x00;
    }
    
    if(BNO08X_SendCommand(cmd, i) == HAL_OK) {
        char msg[60];
        int len = sprintf((char*)msg, "Feature 0x%02X enabled (%dms)\r\n", report_id, interval_ms);
        HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
    } else {
        HAL_UART_Transmit(&huart2, (uint8_t*)"Failed to enable feature\r\n", 26, HAL_MAX_DELAY);
    }
}

void BNO08X_ReadSensorData(void)
{
    uint8_t payload[256];
    uint16_t len = 0;
    int16_t accel_x, accel_y, accel_z;
    int16_t gyro_x, gyro_y, gyro_z;
    float ax, ay, az, gx, gy, gz;
    
    // Leer datos del sensor
    if(BNO08X_ReadData(payload, &len) != HAL_OK) {
        return;
    }
    
    if(len < 14) {
        return; // Datos insuficientes
    }
    
    // El primer byte es el report ID
    uint8_t report_id = payload[0];
    
    // Verificar si el reporte contiene datos válidos (byte 1 = status/flags)
    uint8_t status = payload[1];
    
    // Parsear aceleración (bytes 2-7, little-endian, formato Q14)
    // Para acelerómetro: X=bytes 2-3, Y=bytes 4-5, Z=bytes 6-7
    accel_x = (int16_t)(payload[2] | (payload[3] << 8));
    accel_y = (int16_t)(payload[4] | (payload[5] << 8));
    accel_z = (int16_t)(payload[6] | (payload[7] << 8));
    
    // Parsear giroscopio (bytes 8-13, little-endian, formato Q14)
    // Para giroscopio: X=bytes 8-9, Y=bytes 10-11, Z=bytes 12-13
    gyro_x = (int16_t)(payload[8] | (payload[9] << 8));
    gyro_y = (int16_t)(payload[10] | (payload[11] << 8));
    gyro_z = (int16_t)(payload[12] | (payload[13] << 8));
    
    // Convertir a unidades físicas
    // Formato Q14: dividir por 16384 (2^14) para obtener el valor real
    ax = (accel_x / 16384.0f) * 9.81f;  // Convertir a m/s²
    ay = (accel_y / 16384.0f) * 9.81f;
    az = (accel_z / 16384.0f) * 9.81f;
    
    gx = gyro_x / 16384.0f;  // Radianes/segundo
    gy = gyro_y / 16384.0f;
    gz = gyro_z / 16384.0f;
    
    int len_uart = sprintf((char*)buf, 
        "ID:0x%02X | ACC(g)=%.3f,%.3f,%.3f | GYR(rad/s)=%.4f,%.4f,%.4f\r\n",
        report_id, 
        ax/9.81f, ay/9.81f, az/9.81f,  // Mostrar en g también
        gx, gy, gz);
    HAL_UART_Transmit(&huart2, buf, len_uart, HAL_MAX_DELAY);
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  HAL_UART_Transmit(&huart2, (uint8_t*)"[A] MX_USART2_UART_Init OK\r\n", 26, HAL_MAX_DELAY);
  MX_I2C1_Init();
  HAL_UART_Transmit(&huart2, (uint8_t*)"[B] MX_I2C1_Init OK\r\n", 23, HAL_MAX_DELAY);
  HAL_Delay(50);
  
  // Print reset reason flags
  uint32_t reset_flags = RCC->CSR;
  char rst_msg[128];
  int rst_len = 0;
  rst_len = sprintf(rst_msg, "Reset CSR=0x%08lX\r\n", reset_flags);
  HAL_UART_Transmit(&huart2, (uint8_t*)rst_msg, rst_len, HAL_MAX_DELAY);
  // Clear reset flags so subsequent boots report fresh
  __HAL_RCC_CLEAR_RESET_FLAGS();
  /* USER CODE BEGIN 2 */

  HAL_UART_Transmit(&huart2, (uint8_t*)"=== BNO08X IMU Sensor Test ===\r\n", 33, HAL_MAX_DELAY);
  HAL_Delay(500);

  HAL_UART_Transmit(&huart2, (uint8_t*)"[1] Calling BNO08X_Init()...\r\n", 30, HAL_MAX_DELAY);
  BNO08X_Init();
  HAL_UART_Transmit(&huart2, (uint8_t*)"[2] BNO08X_Init() completed\r\n", 30, HAL_MAX_DELAY);
  
  HAL_Delay(1000);
  /* Run I2C scan after configuring PS pins and resetting the sensor */
  I2C_Scan();
  HAL_Delay(100);
  
  HAL_UART_Transmit(&huart2, (uint8_t*)"[3] Setting ACCEL feature...\r\n", 31, HAL_MAX_DELAY);
  BNO08X_SetFeature(SHTP_REPORT_BASE_ACCEL, 20);
  HAL_Delay(200);
  
  HAL_UART_Transmit(&huart2, (uint8_t*)"[4] Setting GYRO feature...\r\n", 30, HAL_MAX_DELAY);
  BNO08X_SetFeature(SHTP_REPORT_BASE_GYRO, 20);
  HAL_Delay(200);
  
  HAL_UART_Transmit(&huart2, (uint8_t*)"[5] Ready to read data...\r\n", 27, HAL_MAX_DELAY);

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); // Solo para ver que MCU corre
    HAL_Delay(100); // pausa para no saturar el LED
    
    // Verificar si hay datos del sensor listos
    if(sensor_data_ready)
    {
        sensor_data_ready = 0;       // reset flag
        BNO08X_ReadSensorData();     // enviar datos por UART aquí
    }

  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}


/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|D7_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, D6_Pin|D5_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : LD2_Pin D7_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|D7_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : D6_Pin D5_Pin */
  GPIO_InitStruct.Pin = D6_Pin|D5_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : D2_Pin */
  GPIO_InitStruct.Pin = D2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(D2_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* Habilitar interrupción EXTI para D2 (PA10 en tu caso) */
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);  // Prioridad 0, subprioridad 0
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if(GPIO_Pin == BNO08X_INT_Pin)
    {
    	sensor_data_ready = 1;  // solo marcar que hay datos listos
    }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
