#include "globals.h"

// ========================== 全局对象定义 ==========================
Preferences prefs;
WebServer   web_server(80);
DNSServer   dns_server;
WiFiClient  wifi_client;
PubSubClient mqtt(wifi_client);

State state = State::AP_MODE;
unsigned long sta_start_ms = 0;

// ========================== MQTT 全局状态 ==========================
String mqtt_uid   = "";
String mqtt_topic = "";
bool   mqtt_configured = false;

// ========================== 凭据存储 ==========================
bool load_creds(String &ssid, String &pass, String &uid)
{
  prefs.begin("wifi", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  uid  = prefs.getString("uid", "");
  prefs.end();
  return ssid.length() > 0;
}

void save_creds(const String &ssid, const String &pass, const String &uid)
{
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.putString("uid",  uid);
  prefs.end();
}

void clear_creds()
{
  prefs.begin("wifi", false);
  prefs.clear();
  prefs.end();
}
