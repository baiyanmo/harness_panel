#pragma once

#include <Arduino.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <PubSubClient.h>
#include <settings.h>

// ========================== 状态机 ==========================
enum class State { STA_CONNECTING, STA_CONNECTED, AP_MODE };

// ========================== 全局对象 ==========================
extern Preferences prefs;
extern WebServer   web_server;
extern DNSServer   dns_server;
extern WiFiClient  wifi_client;
extern PubSubClient mqtt;

extern State state;
extern unsigned long sta_start_ms;

// ========================== MQTT 全局状态 ==========================
extern String mqtt_uid;
extern String mqtt_topic;
extern bool   mqtt_configured;

// ========================== 凭据存储 ==========================
bool load_creds(String &ssid, String &pass, String &uid);
void save_creds(const String &ssid, const String &pass, const String &uid);
void clear_creds();
