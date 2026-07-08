#include "mcp2515.h"
#include <stdio.h>
#include <string.h>

extern SPI_HandleTypeDef hspi1;
extern UART_HandleTypeDef huart1;

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
    // 波特率500kbps，时钟8MHz
    MCP2515_WriteRegister(0x2A, 0x00); // CNF3
    MCP2515_WriteRegister(0x29, 0x90); // CNF2
    MCP2515_WriteRegister(0x28, 0x01); // CNF1
    // 接收缓冲区配置，接收所有帧
    MCP2515_WriteRegister(0x20, 0x00); // RXM0SIDH mask
    MCP2515_WriteRegister(0x21, 0x00); // RXM0SIDL mask
    MCP2515_WriteRegister(0x24, 0x00); // RXM1SIDH mask
    MCP2515_WriteRegister(0x25, 0x00); // RXM1SIDL mask
    MCP2515_WriteRegister(0x60, 0x64); // RXB0CTRL: 接收所有帧，开启rollover
    MCP2515_WriteRegister(0x70, 0x60); // RXB1CTRL: 接收所有帧
    // 进入Normal模式
    MCP2515_WriteRegister(0x0F, 0x00);
    HAL_Delay(10);
}

void MCP2515_SendFrame(uint16_t id, uint8_t *data, uint8_t len) {
    // 等待TXB0空闲
    uint8_t txb0ctrl;
    uint32_t timeout = HAL_GetTick();
    do {
        txb0ctrl = MCP2515_ReadRegister(0x30);
        if (HAL_GetTick() - timeout > 50) break;
    } while (txb0ctrl & 0x08);
    // 写入帧ID和数据
    MCP2515_WriteRegister(0x31, (id >> 3) & 0xFF); // TXB0SIDH
    MCP2515_WriteRegister(0x32, (id << 5) & 0xFF); // TXB0SIDL
    MCP2515_WriteRegister(0x33, 0x00);              // TXB0EID8
    MCP2515_WriteRegister(0x34, 0x00);              // TXB0EID0
    MCP2515_WriteRegister(0x35, len & 0x0F);        // TXB0DLC
    for (uint8_t i = 0; i < len; i++) {
        MCP2515_WriteRegister(0x36 + i, data[i]);
    }
    // 发送请求
    uint8_t cmd = 0x81;
    MCP2515_Select();
    HAL_SPI_Transmit(&hspi1, &cmd, 1, 100);
    MCP2515_Deselect();
}

void PrintErrorStatus(uint32_t frame_count) {
    uint8_t tec    = MCP2515_ReadRegister(0x1C); // TEC寄存器
    uint8_t rec    = MCP2515_ReadRegister(0x1D); // REC寄存器
    uint8_t eflg   = MCP2515_ReadRegister(0x2D); // 错误标志寄存器
    uint8_t canstat = MCP2515_ReadRegister(0x0E); // 状态寄存器
    char msg[80];
    sprintf(msg, "[%lu] TEC=%d REC=%d EFLG=0x%02X CANSTAT=0x%02X\r\n",
            frame_count, tec, rec, eflg, canstat);
    HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 100);
}
