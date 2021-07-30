/*
 * board_1.cpp
 *
 *  Created on: Jun 28, 2021
 *      Author: peter
 */

#include <bsp/bsp.hpp>
#include <stm32l4xx_hal.h>
#include <stdio.h>


#define CS_PORT                   GPIOA
#define CS_PIN                    GPIO_PIN_2

#define CAN_RST_PORT              GPIOA
#define CAN_RST_PIN               GPIO_PIN_4

#define SCK_PORT                  GPIOA
#define SCK_PIN                   GPIO_PIN_5

#define MISO_PORT                 GPIOA
#define MISO_PIN                  GPIO_PIN_6

#define MOSI_PORT                 GPIOA
#define MOSI_PIN                  GPIO_PIN_7

#define UART_TX_PORT              GPIOA
#define UART_TX_PIN               GPIO_PIN_9

#define UART_RX_PORT              GPIOA
#define UART_RX_PIN               GPIO_PIN_10

#define CAN_IRQ_PORT              GPIOB
#define CAN_IRQ_PIN               GPIO_PIN_0


typedef struct
{
  GPIO_TypeDef *port;
  GPIO_InitTypeDef gpio;
  GPIO_PinState init;
} GPIO;

static const GPIO __gpios[] = {
    {CS_PORT, {CS_PIN, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, GPIO_SPEED_HIGH, 0}, GPIO_PIN_SET},
    {SCK_PORT, {SCK_PIN, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_HIGH, GPIO_AF5_SPI1}, GPIO_PIN_SET},
    {MISO_PORT, {MISO_PIN, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_HIGH, GPIO_AF5_SPI1}, GPIO_PIN_SET},
    {MOSI_PORT, {MOSI_PIN, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_SPEED_HIGH, GPIO_AF5_SPI1}, GPIO_PIN_SET},
    {UART_TX_PORT, {UART_TX_PIN, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_LOW, GPIO_AF7_USART1}, GPIO_PIN_RESET},
    {UART_RX_PORT, {UART_RX_PIN, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_SPEED_LOW, GPIO_AF7_USART1}, GPIO_PIN_RESET},
    //{CAN_RST_PORT, {CAN_RST_PIN, GPIO_MODE_OUTPUT_OD, GPIO_NOPULL, GPIO_SPEED_LOW, 0}, GPIO_PIN_SET},
    //{CAN_IRQ_PORT, {CAN_IRQ_PIN, GPIO_MODE_IT_FALLING, GPIO_PULLUP, GPIO_SPEED_HIGH, 0}, GPIO_PIN_RESET},
};


SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;
TIM_HandleTypeDef htim6;

void gpio_pin_init();
void SystemClock_Config(void);
void Error_Handler();

uart_irq_callback usart_irq = nullptr;
can_irq_callback can_irq = nullptr;

void bsp_set_uart_irq_cb(uart_irq_callback cb)
{
  usart_irq = cb;
}

void bsp_set_can_irq_cb(can_irq_callback cb)
{
  can_irq = cb;
}

void bsp_init()
{
  // To redirect printf() to serial, disable buffering first
  //setvbuf(stdout, NULL, _IONBF, 0);
  //setvbuf(stderr, NULL, _IONBF, 0);

  GPIO_InitTypeDef GPIO_InitStruct = {0};

  HAL_Init();
  SystemClock_Config();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_TIM6_CLK_ENABLE();

  gpio_pin_init();


  // SPI

  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
    {
      Error_Handler();
    }
  __HAL_SPI_ENABLE(&hspi1);

  HAL_NVIC_EnableIRQ(EXTI0_IRQn);


  // UART
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
    {
      Error_Handler();
    }

  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_SetPriority(USART1_IRQn, 1, 0);
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);

  // TIM6
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim6.Instance = TIM6;
  htim6.Init.Prescaler = 79;
  htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim6.Init.Period = 0;
  htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
    {
      Error_Handler();
    }

  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim6, &sMasterConfig) != HAL_OK)
    {
      Error_Handler();
    }
}

void gpio_pin_init()
{

  for ( unsigned i = 0; i < sizeof __gpios / sizeof(GPIO); ++i )
    {
      const GPIO* io = &__gpios[i];
      HAL_GPIO_Init(io->port, (GPIO_InitTypeDef*)&io->gpio);
      if ( io->gpio.Mode == GPIO_MODE_OUTPUT_PP || io->gpio.Mode == GPIO_MODE_OUTPUT_OD )
        {
          HAL_GPIO_WritePin(io->port, io->gpio.Pin, io->init);
        }

    }
}

bool bsp_is_isr()
{
  return __get_IPSR();
}

void bsp_mcp_2515_select()
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_RESET);
}

void bsp_mcp_2515_unselect()
{
  HAL_GPIO_WritePin(CS_PORT, CS_PIN, GPIO_PIN_SET);
}

uint8_t bsp_spi_transfer(uint8_t byte)
{
  uint8_t result = 0;
  HAL_SPI_TransmitReceive(&hspi1, &byte, &result, 1, 10);
  return result;
}

void bsp_delay_us(uint32_t us)
{
  TIM6->CNT = 0;
  HAL_TIM_Base_Start(&htim6);
  while ( TIM6->CNT < us )
    ;
  HAL_TIM_Base_Stop(&htim6);
}

extern "C" {

  int _write(int fd, char* ptr, int len)
  {
    HAL_StatusTypeDef hstatus;

    hstatus = HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, HAL_MAX_DELAY);
    if (hstatus == HAL_OK)
      return len;
    else
      return -1;
  }

  // delay() and millis() for compatibility with Arduino

  void delay(uint32_t ms)
  {
    HAL_Delay(ms);
  }

  uint32_t millis()
  {
    return HAL_GetTick();
  }

  void USART1_IRQHandler(void)
  {
    if ( __HAL_UART_GET_IT(&huart1, UART_IT_RXNE) )
      {
        __HAL_UART_CLEAR_IT(&huart1, UART_IT_RXNE);
        char c = USART1->RDR;
        if ( usart_irq )
          usart_irq(c);
      }
  }

  void SysTick_Handler(void)
  {
    HAL_IncTick();
  }

  void EXTI0_IRQHandler(void)
  {
    if ( __HAL_GPIO_EXTI_GET_IT(CAN_RST_PIN) != RESET )
      {
        __HAL_GPIO_EXTI_CLEAR_IT(CAN_RST_PIN);
        if ( can_irq )
          can_irq();
      }

  }

}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure the main internal regulator output voltage
   */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
      Error_Handler();
    }
  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;

#if defined(STM32L432xx) || defined(STM32L431xx)
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
#endif

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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
      Error_Handler();
    }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }
}


void HAL_MspInit(void)
{
  /* USER CODE BEGIN MspInit 0 */

  /* USER CODE END MspInit 0 */

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_RCC_PWR_CLK_ENABLE();

  HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

  /**
   * Some of these interrupts will be managed and configured in FreeRTOS
   */

  /* System interrupt init*/
  /* MemoryManagement_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(MemoryManagement_IRQn, 0, 0);
  /* BusFault_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(BusFault_IRQn, 0, 0);
  /* UsageFault_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(UsageFault_IRQn, 0, 0);
  /* SVCall_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SVCall_IRQn, 10, 0);
  /* DebugMonitor_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DebugMonitor_IRQn, 0, 0);
  /* PendSV_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(PendSV_IRQn, 10, 0);
  /* SysTick_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);

  /* USER CODE BEGIN MspInit 1 */

  /* USER CODE END MspInit 1 */
}



/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  asm("bkpt 0");
  while (1)
    {
    }
  /* USER CODE END Error_Handler_Debug */
}

