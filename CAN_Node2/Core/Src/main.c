/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : CAN Node2 - Receiver
  ******************************************************************************
  */
/* USER CODE END Header */
#include "main.h"
/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

SPI_HandleTypeDef hspi1;
UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);

/* USER CODE BEGIN 0 */
void MCP2515_Select(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
}
void MCP2515_Deselect(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}
uint8_t MCP2515_ReadRegister(uint8_t address) {
    uint8_t cmd = 0x03, result = 0;
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, &address, 1, 100);
    HAL_SPI_Receive(&hspi1, &result, 1, 100);
    MCP2515_Deselect();
    return result;
}
void MCP2515_WriteRegister(uint8_t address, uint8_t data) {
    uint8_t cmd = 0x02;
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Transmit(&hspi1, &address, 1, 100);
    HAL_SPI_Transmit(&hspi1, &data, 1, 100);
    MCP2515_Deselect();
}
void MCP2515_Reset(void) {
    uint8_t cmd = 0xC0;
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    MCP2515_Deselect();
    HAL_Delay(10);
}
void MCP2515_Init(void) {
    MCP2515_Reset();
    MCP2515_WriteRegister(0x2A, 0x00);
    MCP2515_WriteRegister(0x29, 0x90);
    MCP2515_WriteRegister(0x28, 0x01);
    MCP2515_WriteRegister(0x20, 0x00);
    MCP2515_WriteRegister(0x21, 0x00);
    MCP2515_WriteRegister(0x24, 0x00);
    MCP2515_WriteRegister(0x25, 0x00);
    MCP2515_WriteRegister(0x60, 0x64);
    MCP2515_WriteRegister(0x70, 0x60);
    MCP2515_WriteRegister(0x0F, 0x00); // Normal模式
    HAL_Delay(10);
}
uint8_t MCP2515_Receive(uint16_t *id, uint8_t *data, uint8_t *len) {
    uint8_t canintf = MCP2515_ReadRegister(0x2C);
    if (!(canintf & 0x03)) return 0;
    uint8_t cmd = (canintf & 0x01) ? 0x90 : 0x94;
    uint8_t buf[13] = {0};
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, buf, 13, 100);
    MCP2515_Deselect();
    *id = ((uint16_t)buf[0] << 3) | (buf[1] >> 5);
    *len = buf[4] & 0x0F;
    for (uint8_t i = 0; i < *len; i++) {
        data[i] = buf[5 + i];
    }
    uint8_t clr[3] = {0x05, 0x2C, (uint8_t)(canintf & 0x01 ? ~0x01 : ~0x02)};
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, clr, 3, 100);
    MCP2515_Deselect();
    return 1;
}
/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();

  /* USER CODE BEGIN 2 */
  MCP2515_Deselect();
  HAL_Delay(100);
  MCP2515_Init();

  char msg[80];
  uint8_t canstat = MCP2515_ReadRegister(0x0E);
  sprintf(msg, "Node2 Ready CANSTAT=0x%02X\r\n", canstat);
  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  uint16_t rx_id;
  uint8_t rx_data[8], rx_len;

  while (1)
  {
    /* USER CODE END WHILE */
    /* USER CODE BEGIN 3 */
    if (MCP2515_Receive(&rx_id, rx_data, &rx_len)) {
        if (rx_id == 0x101) {
            int16_t t = ((int16_t)rx_data[0] << 8) | rx_data[1];
            sprintf(msg, "RX 0x101 Temp=%d.%d C\r\n", t/10, t%10);
        } else if (rx_id == 0x102) {
            int16_t v = ((int16_t)rx_data[0] << 8) | rx_data[1];
            sprintf(msg, "RX 0x102 Volt=%d.%02d V\r\n", v/100, v%100);
        } else if (rx_id == 0x103) {
            sprintf(msg, "RX 0x103 Status=0x%02X ErrCode=0x%02X\r\n",
                    rx_data[0], rx_data[1]);
        } else {
            sprintf(msg, "RX 0x%03X unknown\r\n", rx_id);
        }
        HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
    }
  }
  /* USER CODE END 3 */
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) Error_Handler();
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) Error_Handler();
}

static void MX_SPI1_Init(void)
{
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK) Error_Handler();
}

static void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK) Error_Handler();
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
  GPIO_InitStruct.Pin = GPIO_PIN_0;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

/* USER CODE BEGIN 4 */
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line) {}
#endif
