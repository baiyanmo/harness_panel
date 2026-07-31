#include "globals.h"
#include "mqtt_handler.h"
#include <settings.h>

// 上行 topic（加 /set 后缀，巴法云不会把消息推回给自己）
static String mqtt_uplink_topic;

// ========================== MQTT 回调 ==========================
static void mqtt_callback(char *topic, byte *payload, unsigned int length)
{
  // 从巴法云收到的消息 → 串口发给 STM32
  String msg((char *)payload, length);
  log_i("MQTT ← 云端 [%s]: %s", topic, msg);

  // 发到串口（STM32）
  Serial2.println(msg);
}

// ========================== MQTT 连接 ==========================
void mqtt_connect()
{
  if (!mqtt_configured) return;
  if (mqtt.connected())  return;

  // 上行用 /set 后缀（防回环：发给所有人，不推给自己）
  mqtt_uplink_topic = mqtt_topic + "/set";

  log_i("MQTT 连接 %s:%d  uid=%s  down=%s  up=%s",
        BEMFA_MQTT_SERVER, BEMFA_MQTT_PORT,
        mqtt_uid.c_str(), mqtt_topic.c_str(), mqtt_uplink_topic.c_str());

  mqtt.setServer(BEMFA_MQTT_SERVER, BEMFA_MQTT_PORT);
  mqtt.setCallback(mqtt_callback);
  mqtt.setKeepAlive(BEMFA_MQTT_KEEPALIVE);

  // 巴法云 MQTT: client_id = uid
  if (mqtt.connect(mqtt_uid.c_str()))
  {
    log_i("MQTT 已连接 keepalive=%d", BEMFA_MQTT_KEEPALIVE);
    // 订阅下行 topic（接收云端指令）
    mqtt.subscribe(mqtt_topic.c_str());
    log_i("MQTT 订阅: %s", mqtt_topic.c_str());
    log_i("MQTT 上行: %s", mqtt_uplink_topic.c_str());
  }
  else
  {
    log_w("MQTT 连接失败, rc=%d", mqtt.state());
  }
}

// ========================== 尝试连接 ==========================
void try_mqtt_connect()
{
  if (mqtt_uid.length() == 0)
  {
    log_i("MQTT 未配置 uid，跳过");
    return;
  }
  mqtt_configured = true;
  mqtt_connect();
}

// ========================== 发布消息（/set 防回环）==========================
void mqtt_publish(const String &msg)
{
  if (!mqtt.connected())
  {
    log_w("MQTT 未连接，丢弃消息: %s", msg.c_str());
    return;
  }
  if (mqtt.publish(mqtt_uplink_topic.c_str(), msg.c_str()))
  {
    log_i("MQTT → 云端 [%s]: %s", mqtt_uplink_topic.c_str(), msg.c_str());
  }
  else
  {
    log_w("MQTT 发布失败");
  }
}

// ========================== 状态轮询 ==========================
void mqtt_loop()
{
  if (!mqtt_configured) return;

  if (!mqtt.connected())
  {
    // 每 5 秒重试一次
    static unsigned long last_retry = 0;
    if (millis() - last_retry > 5000)
    {
      last_retry = millis();
      mqtt_connect();
    }
  }
  mqtt.loop();
}
