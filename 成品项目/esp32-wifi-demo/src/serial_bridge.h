#pragma once

// 初始化串口桥接 + 启动 FreeRTOS 任务
void serial_bridge_setup();

// 串口任务函数（被 serial_bridge_setup 内部创建）
void serialTask(void *pvParameters);
