# STM32 CAN总线多节点通信与故障诊断系统

基于STM32F103C8T6 + MCP2515的CAN总线多节点通信与故障诊断系统，集成FreeRTOS实时操作系统和slcan USB-CAN网关。

## 项目架构

PC上位机（Python/WSL Ubuntu）通过slcan协议经串口连接STM32 Node1，Node1运行FreeRTOS，通过SPI驱动MCP2515控制器，经CANH/CANL与Node2通信。

## 核心功能

- **FreeRTOS多任务架构**：四任务并行（CAN发送/串口日志/系统监控/LED），SPI互斥量保护，优先级继承验证
- **CAN总线通信**：500kbps标准帧，三种传感器数据帧（温度0x101/电压0x102/状态位0x103）
- **slcan USB-CAN网关**：实现标准slcan协议，支持python-can工具链，PC可直接控制CAN总线
- **故障注入与检测**：断线场景Bus-Off自动恢复，TEC/REC错误计数实时监控
- **HIL自动化测试框架**：Python自动化测试脚本，覆盖正常通信/断线故障/自动恢复三个场景
- **Web实时Dashboard**：Flask + SocketIO + Chart.js实时显示温度/电压曲线和故障报警

## 硬件清单

| 器件 | 型号 | 数量 |
|------|------|------|
| MCU开发板 | STM32F103C8T6 Blue Pill | 2块 |
| CAN控制器模块 | MCP2515（板载TJA1050收发器） | 2个 |
| 调试烧录器 | ST-Link V2 | 1个 |
| USB转串口模块 | CH340 | 2个 |

## 软件环境

- STM32CubeIDE 2.1.1 + STM32CubeMX 6.17
- FreeRTOS 10.3.1 (CMSIS_V2)
- Python 3.x + python-can + Flask + Flask-SocketIO
- WSL Ubuntu 22.04

## 测试数据

| 测试项目 | 结果 |
|----------|------|
| 正常通信丢帧率 | 0.00% |
| 10ms高频压力测试（30秒） | PASS，268帧0丢帧 |
| 断线故障Bus-Off自动恢复 | TEC累积→Bus-Off→自动恢复 |
| FreeRTOS优先级继承验证 | 高优先级任务等待互斥量0ms |
| slcan网关全链路验证 | PC→STM32→CAN总线→Node2 |
