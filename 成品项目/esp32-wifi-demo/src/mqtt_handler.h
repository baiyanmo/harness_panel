#pragma once

// 尝试连接 MQTT（无 uid 则跳过）
void try_mqtt_connect();

// MQTT 连接
void mqtt_connect();

// MQTT 发布消息到云端
void mqtt_publish(const String &msg);

// MQTT 状态轮询（在 loop 中调用）
void mqtt_loop();
