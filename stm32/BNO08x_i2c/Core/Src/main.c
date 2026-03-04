/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
// BNO08x interrupt flag (used by BNO driver)
// Definition placed here so other modules (driver, ISR) can extern it.
volatile uint8_t BNO_Ready = 0;
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
#include "gpio.h"
#include "BNO08x/BNO_08x_I2C.h"
#include "BNO08x/bno_board.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define BNO08X_I2C_ADDR 0x4A << 1  // Dirección I2C del BNO08x (7-bit + HAL)
#define REPORT_ARVR_STABILIZED 0x05
#define REPORT_GYRO_INTEGRATED 0x06

/* Typedef -------------------------------------------------------------------*/
typedef struct {
    float yaw;
    float pitch;
    float roll;
} euler_t;

typedef struct {
    float real;
    float i;
    float j;
    float k;
} quaternion_t;

/* External handles ----------------------------------------------------------*/
extern I2C_HandleTypeDef hi2c1;
extern UART_HandleTypeDef huart2;

/* Global Variables ----------------------------------------------------------*/
euler_t ypr;

/* Prototypes ---------------------------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART2_UART_Init(void);

int BNO08x_Init(void);
int BNO08x_EnableReport(uint8_t reportID, uint32_t interval_us);
int BNO08x_ReadQuaternion(quaternion_t *q);
void QuaternionToEuler(quaternion_t *q, euler_t *ypr, int degrees);
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
volatile uint8_t BNO_Ready = 0;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void) ;
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void I2C_Scan(void);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

char msg[128];
int len;

void pollBNO08x(void)
{
    HAL_StatusTypeDef st;

    // Intentar inicializar el sensor (polling)
    st = BNO_Init();
    if(st != HAL_OK)
    {
        HAL_UART_Transmit(&huart2, (uint8_t*)"BNO_Init() falló, intentando REQUEST...\r\n", 41, HAL_MAX_DELAY);
        if(BNO_requestProductID() != HAL_OK)
        {
            HAL_UART_Transmit(&huart2, (uint8_t*)"BNO08x NO responde\n", 20, HAL_MAX_DELAY);
            return;
        }
    }

    // Leer datos disponibles en polling
    if(BNO_dataAvailable() == HAL_OK)
    {
        uint8_t sensorId = BNO_getSensorEventID();

        if(sensorId == ROTATION_VECTOR)
        {
            BNO_RotationVectorWAcc_t rv = getRotationVector();
            len = snprintf(msg, sizeof(msg), "RV: I=%.4f J=%.4f K=%.4f W=%.4f Acc=%.4f\r\n",
                           rv.I, rv.J, rv.K, rv.Real, rv.Accuracy);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
        }
        else if(sensorId == ACCELEROMETER)
        {
            BNO_Accelerometer_t acc = getaccelerometer();
            len = snprintf(msg, sizeof(msg), "ACC: X=%.3f Y=%.3f Z=%.3f\r\n",
                           acc.X, acc.Y, acc.Z);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
        }
        else if(sensorId == GYROSCOPE_CALIBRATED)
        {
            BNO_Gyroscope_t gyro = getGyroscope();
            len = snprintf(msg, sizeof(msg), "GYR: X=%.3f Y=%.3f Z=%.3f\r\n",
                           gyro.X, gyro.Y, gyro.Z);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
        }
    }
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
  // Inicializar UART ANTES que reloj y todo lo demás
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  HAL_UART_Init(&huart2);
  
  const char* testMsg = "\r\n*** UART TEST ***\r\n";
  HAL_UART_Transmit(&huart2, (uint8_t*)testMsg, 21, HAL_MAX_DELAY);
  
  // Print reset reason (helps detect watchdog / brown-out / pin reset loops)
  char rstMsg[128];
  int rstLen = 0;
  if(__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: POR/PDR\r\n");
  } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: PIN reset\r\n");
  } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: Software reset\r\n");
  } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: Independent WDG\r\n");
  } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: Window WDG\r\n");
  } else if(__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST)) {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: Low-Power reset\r\n");
  } else {
    rstLen = snprintf(rstMsg, sizeof(rstMsg), "Reset reason: Unknown\r\n");
  }
  if(rstLen > 0) HAL_UART_Transmit(&huart2, (uint8_t*)rstMsg, rstLen, HAL_MAX_DELAY);
  // Clear reset flags so subsequent boots reflect new events
  __HAL_RCC_CLEAR_RESET_FLAGS();
  // brief pause to observe serial output and avoid immediate reboot flooding
  HAL_Delay(200);

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  I2C_Scan();
  HAL_Delay(1000); // 1 segundo entre scans
  char msg[100];

     if (BNO08x_Init() != HAL_OK) {
         sprintf(msg, "Failed to init BNO08x\r\n");
         HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
         while(1);
     }
     sprintf(msg, "BNO08x Initialized!\r\n");
     HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);

     // Activar reporte de rotación estabilizada (Euler)
     BNO08x_EnableReport(REPORT_ARVR_STABILIZED, 5000);

     quaternion_t q;
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

	  if (BNO08x_ReadQuaternion(&q) == HAL_OK) {
	              QuaternionToEuler(&q, &ypr, 1);
	              sprintf(msg, "Yaw: %.2f, Pitch: %.2f, Roll: %.2f\r\n", ypr.yaw, ypr.pitch, ypr.roll);
	              HAL_UART_Transmit(&huart2, (uint8_t*)msg, strlen(msg), HAL_MAX_DELAY);
	    HAL_Delay(10); // pequeña pausa para no saturar I2C/UART
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
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 50;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
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

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
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
  
  // Habilitar reloj de I2C1
  __HAL_RCC_I2C1_CLK_ENABLE();
  
  // Habilitar GPIOB para los pines I2C (SCL y SDA)
  __HAL_RCC_GPIOB_CLK_ENABLE();
  
  // Configurar pines PB6 (SCL) y PB7 (SDA) como open-drain alternates
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  
  GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;  // PB8 (SCL), PB9 (SDA)
  GPIO_InitStruct.Mode = GPIO_MODE_AF_OD;          // Alternate function, Open-Drain
  GPIO_InitStruct.Pull = GPIO_PULLUP;              // Pull-up enabled
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF4_I2C1;       // AF4 is I2C1 for these pins
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 83;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 4294967295;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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
  HAL_GPIO_WritePin(GPIOA, LD2_Pin|RST_imu_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, PS1_imu_Pin|PS0_imu_Pin|BOOT_imu_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : LD2_Pin RST_imu_Pin */
  GPIO_InitStruct.Pin = LD2_Pin|RST_imu_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PS1_imu_Pin PS0_imu_Pin BOOT_imu_Pin */
  GPIO_InitStruct.Pin = PS1_imu_Pin|PS0_imu_Pin|BOOT_imu_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : INT_imu_Pin */
  GPIO_InitStruct.Pin = INT_imu_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(INT_imu_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */


void I2C_Scan(void)
{
    char msg[64];
    int len;

    HAL_UART_Transmit(&huart2, (uint8_t*)"\r\n--- I2C SCAN ---\r\n", 18, HAL_MAX_DELAY);

    for(uint8_t addr = 1; addr < 127; addr++)
    {
        HAL_StatusTypeDef st =
            HAL_I2C_IsDeviceReady(&hi2c1, addr << 1, 3, 10);

        if(st == HAL_OK)
        {
            len = snprintf(msg, sizeof(msg),
                          "Dispositivo encontrado en 0x%02X\r\n", addr);
            HAL_UART_Transmit(&huart2, (uint8_t*)msg, len, HAL_MAX_DELAY);
        }
    }

    HAL_UART_Transmit(&huart2, (uint8_t*)"--- FIN SCAN ---\r\n", 18, HAL_MAX_DELAY);
}


/* Functions -----------------------------------------------------------------*/

// Inicializa el sensor (resetea y verifica conexión)
int BNO08x_Init(void) {
    uint8_t whoami;
    if (HAL_I2C_Mem_Read(&hi2c1, BNO08X_I2C_ADDR, 0x00, 1, &whoami, 1, 100) != HAL_OK)
        return HAL_ERROR;  // No responde

    // Aquí puedes agregar reset si tu placa lo tiene conectado
    return HAL_OK;
}

// Configura un reporte en el sensor
int BNO08x_EnableReport(uint8_t reportID, uint32_t interval_us) {
    // Paquete de ejemplo (dependiendo del protocolo SH2)
    // Para Arduino la librería lo hace por ti; aquí debes armar el paquete de bytes según datasheet
    // Por simplicidad asumimos que el sensor ya viene con ARVR activo
    return HAL_OK;
}

// Lee quaternion del sensor (simplificado)
int BNO08x_ReadQuaternion(quaternion_t *q) {
    uint8_t buf[12];
    if (HAL_I2C_Mem_Read(&hi2c1, BNO08X_I2C_ADDR, 0x00, 1, buf, 12, 10) != HAL_OK)
        return HAL_ERROR;

    // Convertir bytes a float (little-endian 4 bytes por componente)
    memcpy(&q->real, buf + 0, 4);
    memcpy(&q->i,    buf + 4, 4);
    memcpy(&q->j,    buf + 8, 4);
    memcpy(&q->k,    buf + 12, 4);
    return HAL_OK;
}

// Convierte quaternion a Euler
void QuaternionToEuler(quaternion_t *q, euler_t *ypr, int degrees) {
    float sqr = q->real*q->real;
    float sqi = q->i*q->i;
    float sqj = q->j*q->j;
    float sqk = q->k*q->k;

    ypr->yaw   = atan2f(2.0f * (q->i*q->j + q->k*q->real), (sqi - sqj - sqk + sqr));
    ypr->pitch = asinf(-2.0f * (q->i*q->k - q->j*q->real) / (sqi + sqj + sqk + sqr));
    ypr->roll  = atan2f(2.0f * (q->j*q->k + q->i*q->real), (-sqi - sqj + sqk + sqr));

    if (degrees) {
        ypr->yaw   *= 180.0f / 3.14159265f;
        ypr->pitch *= 180.0f / 3.14159265f;
        ypr->roll  *= 180.0f / 3.14159265f;
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
