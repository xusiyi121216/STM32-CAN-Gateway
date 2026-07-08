#ifndef MCP2515_H
#define MCP2515_H

#include "main.h"

extern SPI_HandleTypeDef hspi1;
void MCP2515_Select(void);
void MCP2515_Deselect(void);
uint8_t MCP2515_ReadRegister(uint8_t address);
void MCP2515_WriteRegister(uint8_t address, uint8_t data);
void MCP2515_Reset(void);
void MCP2515_Init(void);
void MCP2515_SendFrame(uint16_t id, uint8_t *data, uint8_t len);
void PrintErrorStatus(uint32_t frame_count);

#endif
