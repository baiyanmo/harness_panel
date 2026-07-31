#include "globals.h"
#include "serial_bridge.h"
#include "mqtt_handler.h"
#include <settings.h>

static TaskHandle_t serialTaskHandle = nullptr;

// ========================== 串口任务（Core 1 独立运行）==========================
void serialTask(void *pvParameters)
{
  String line = "";
  while (true)
  {
    while (Serial2.available())
    {
      char c = Serial2.read();
      if (c == '\n')
      {
        line.trim();
        if (line.length() > 0)
        {
          log_i("STM32 ← 串口: %s", line.c_str());
          if (!line.startsWith("cmd=") && mqtt_configured && mqtt.connected())
          {
            // 发布到 /set topic，防止巴法云将消息推回给自己
            mqtt.publish((mqtt_topic + "/set").c_str(), line.c_str());
          }
        }
        line = "";
      }
      else
      {
        line += c;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

// ========================== 初始化 ==========================
void serial_bridge_setup()
{
#if ENABLE_SERIAL_BRIDGE
  Serial2.begin(STM32_BAUD, SERIAL_8N1, STM32_RX_PIN, STM32_TX_PIN);
  xTaskCreatePinnedToCore(
    serialTask,
    "serial",
    4096,
    nullptr,
    1,
    &serialTaskHandle,
    1   // Core 1（避免影响 Core 0 的 WiFi 协议栈）
  );
  log_i("串口桥接已启用: RX=GPIO%d  TX=GPIO%d  baud=%d", STM32_RX_PIN, STM32_TX_PIN, STM32_BAUD);
#else
  log_i("串口桥接已禁用，仅 WiFi + MQTT 模式");
#endif
}
