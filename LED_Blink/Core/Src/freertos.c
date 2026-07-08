#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
#include "mcp2515.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern UART_HandleTypeDef huart1;

osMutexId_t spiMutexHandle;
osMessageQueueId_t uartQueueHandle;

static uint8_t can_open = 0;
static uint8_t uart_rx_byte;
static char cmd_buf[32];
static int cmd_pos = 0;
static uint8_t cmd_ready = 0;
static char pending_cmd[32];

void StartDefaultTask(void *argument);
void StartCANTxTask(void *argument);
void StartUARTLogTask(void *argument);
void StartMonitorTask(void *argument);

void MX_FREERTOS_Init(void) {
    spiMutexHandle = osMutexNew(NULL);
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    HAL_UART_Transmit(&huart1, (uint8_t*)"STACK OVERFLOW: ", 16, 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)pcTaskName, strlen(pcTaskName), 100);
    HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 2, 100);
    while(1) {}
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        uint8_t byte = uart_rx_byte;
        if (byte == '\r' || byte == '\n') {
            if (cmd_pos > 0) {
                cmd_buf[cmd_pos] = 0;
                memcpy(pending_cmd, cmd_buf, cmd_pos + 1);
                cmd_ready = 1;
                cmd_pos = 0;
                memset(cmd_buf, 0, sizeof(cmd_buf));
            }
        } else if (cmd_pos < 31) {
            cmd_buf[cmd_pos++] = byte;
        }

        HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);
    }
}

void slcan_parse(char *cmd)
{
    char debug[40];
    sprintf(debug, "CMD:[%s]\r\n", cmd);
    HAL_UART_Transmit(&huart1, (uint8_t*)debug, strlen(debug), 100);

    uint8_t response = '\r';

    if (cmd[0] == 'S') {
        HAL_UART_Transmit(&huart1, &response, 1, 100);
    }
    else if (cmd[0] == 'O') {
        can_open = 1;
        HAL_UART_Transmit(&huart1, &response, 1, 100);
    }
    else if (cmd[0] == 'C') {
        can_open = 0;
        HAL_UART_Transmit(&huart1, &response, 1, 100);
    }
    else if (cmd[0] == 't' && can_open) {
        uint16_t id = 0;
        uint8_t dlc = 0;
        uint8_t data[8] = {0};

        char id_str[4] = {cmd[1], cmd[2], cmd[3], 0};
        id = (uint16_t)strtol(id_str, NULL, 16);

        dlc = cmd[4] - '0';
        if (dlc > 8) dlc = 8;

        for (int i = 0; i < dlc; i++) {
            char byte_str[3] = {cmd[5 + i*2], cmd[6 + i*2], 0};
            data[i] = (uint8_t)strtol(byte_str, NULL, 16);
        }

        char tx_debug[40];
        sprintf(tx_debug, "TX ID=0x%03X DLC=%d\r\n", id, dlc);
        HAL_UART_Transmit(&huart1, (uint8_t*)tx_debug, strlen(tx_debug), 100);

        osMutexAcquire(spiMutexHandle, osWaitForever);
        MCP2515_SendFrame(id, data, dlc);
        osMutexRelease(spiMutexHandle);

        HAL_UART_Transmit(&huart1, &response, 1, 100);
    }
    else if (cmd[0] == 'F') {
        osMutexAcquire(spiMutexHandle, osWaitForever);
        uint8_t eflg = MCP2515_ReadRegister(0x2D);
        osMutexRelease(spiMutexHandle);
        char resp[8];
        sprintf(resp, "F%02X\r", eflg);
        HAL_UART_Transmit(&huart1, (uint8_t*)resp, strlen(resp), 100);
    }
    else if (cmd[0] == 'V') {
        HAL_UART_Transmit(&huart1, (uint8_t*)"V0101\r", 6, 100);
    }
    else {
        uint8_t err = 0x07;
        HAL_UART_Transmit(&huart1, &err, 1, 100);
    }
}

void slcan_receive(void)
{
    uint8_t canintf = MCP2515_ReadRegister(0x2C);
    if (!(canintf & 0x03)) return;

    uint8_t cmd = (canintf & 0x01) ? 0x90 : 0x94;
    uint8_t buf[13] = {0};
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    HAL_SPI_Receive(&hspi1, buf, 13, 100);
    MCP2515_Deselect();

    uint16_t id = ((uint16_t)buf[0] << 3) | (buf[1] >> 5);
    uint8_t dlc = buf[4] & 0x0F;

    char slcan_msg[32];
    int pos = 0;
    pos += sprintf(slcan_msg + pos, "t%03X%d", id, dlc);
    for (int i = 0; i < dlc; i++) {
        pos += sprintf(slcan_msg + pos, "%02X", buf[5 + i]);
    }
    slcan_msg[pos++] = '\r';
    slcan_msg[pos] = 0;

    HAL_UART_Transmit(&huart1, (uint8_t*)slcan_msg, pos, 100);

    uint8_t clr[3] = {0x05, 0x2C, (uint8_t)(canintf & 0x01 ? ~0x01 : ~0x02)};
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, clr, 3, 100);
    MCP2515_Deselect();
}

void StartDefaultTask(void *argument)
{
    for(;;)
    {
        HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
        osDelay(500);
    }
}

void StartCANTxTask(void *argument)
{
    osDelay(200);

    osMutexAcquire(spiMutexHandle, osWaitForever);
    MCP2515_Init();
    osMutexRelease(spiMutexHandle);

    HAL_UART_Transmit(&huart1, (uint8_t*)"slcan gateway ready\r\n", 21, 100);

    HAL_UART_Receive_IT(&huart1, &uart_rx_byte, 1);

    for(;;)
    {

        if (cmd_ready) {
            cmd_ready = 0;
            slcan_parse(pending_cmd);
        }

        if (can_open) {
            osMutexAcquire(spiMutexHandle, osWaitForever);
            slcan_receive();
            osMutexRelease(spiMutexHandle);
        }

        osDelay(1);
    }
}

void StartUARTLogTask(void *argument)
{
    for(;;)
    {
        osDelay(1000);
    }
}

void StartMonitorTask(void *argument)
{
    for(;;)
    {
        osDelay(5000);
    }
}
