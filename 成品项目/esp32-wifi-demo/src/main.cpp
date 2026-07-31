#include <Arduino.h>
#include "globals.h"
#include "wifi_config.h"
#include "mqtt_handler.h"
#include "serial_bridge.h"
#include <settings.h>

#define FORCE_AP_PIN 0  // GPIO0 长按清空凭据/恢复出厂

// ========================== setup ==========================
void setup()
{
  Serial.begin(115200);
  delay(300);

  // 1. 串口桥接（连 STM32）
  serial_bridge_setup();

  // 2. 启动热点
  start_ap();

  // 3. GPIO0 长按清空凭据
  pinMode(FORCE_AP_PIN, INPUT_PULLUP);
  if (digitalRead(FORCE_AP_PIN) == LOW)
  {
    log_i("GPIO0 按下，清空凭据");
    clear_creds();
  }

  // 4. MQTT topic
  mqtt_topic = BEMFA_TOPIC;
  log_i("MQTT topic: %s", mqtt_topic.c_str());

  // 5. 加载凭据，尝试后台连路由器
  String saved_ssid, saved_pass, saved_uid;
  if (load_creds(saved_ssid, saved_pass, saved_uid))
  {
    mqtt_uid = saved_uid;
    log_i("发现凭据: %s  uid 已配置", saved_ssid.c_str());
    try_connect_sta_bg(saved_ssid, saved_pass);
  }
  else
  {
    log_i("无凭据，等待配网");
  }

  // 6. Web 服务器
  web_setup();

  log_i("热点 %s 已就绪: http://192.168.4.1", AP_SSID);
}

// ========================== loop ==========================
void loop()
{
  // WiFi 状态处理
  if (state == State::STA_CONNECTING)
  {
    if (WiFi.status() == WL_CONNECTED)
      on_sta_connected();
    else if (millis() - sta_start_ms > WIFI_CONNECT_TIMEOUT_MS)
    {
      log_w("连接超时，热点仍在线");
      state = State::AP_MODE;
    }
  }
  else if (state == State::STA_CONNECTED)
  {
    if (WiFi.status() != WL_CONNECTED)
      on_sta_disconnected();
  }

  mqtt_loop();
  web_loop();
}
