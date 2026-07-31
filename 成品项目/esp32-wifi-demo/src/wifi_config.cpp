#include "globals.h"
#include "wifi_config.h"
#include "mqtt_handler.h"
#include <WiFi.h>
#include <ESPmDNS.h>

// ========================== WiFi 启动 ==========================
void start_ap()
{
  log_i("启动热点: %s", AP_SSID);
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASSWORD);
  dns_server.start(53, "*", WiFi.softAPIP());
  log_i("热点地址: http://192.168.4.1");
}

// ========================== STA 连接 ==========================
void try_connect_sta_bg(const String &ssid, const String &pass)
{
  log_i("尝试连接 WiFi: %s", ssid.c_str());
  WiFi.setHostname(HOSTNAME);
  WiFi.begin(ssid.c_str(), pass.c_str());
  state      = State::STA_CONNECTING;
  sta_start_ms = millis();
}

void on_sta_connected()
{
  state = State::STA_CONNECTED;
  log_i("STA 已连接 IP: %s,  AP 仍在线: %s",
        WiFi.localIP().toString().c_str(),
        WiFi.softAPIP().toString().c_str());
  if (MDNS.begin(HOSTNAME))
  {
    MDNS.addService("http", "tcp", 80);
    log_i("mDNS: http://%s.local", HOSTNAME);
  }
  // STA 通网后立即尝试 MQTT
  try_mqtt_connect();
}

void on_sta_disconnected()
{
  log_w("WiFi 断开");
  state = State::AP_MODE;
  mqtt.disconnect();
}

// ========================== 扫描 WiFi API ==========================
static void handle_scan()
{
  int n = WiFi.scanNetworks(false, true);
  String json = "[";
  for (int i = 0; i < n; i++)
  {
    if (i > 0) json += ",";
    int rssi = WiFi.RSSI(i);
    String enc;
    switch (WiFi.encryptionType(i))
    {
      case WIFI_AUTH_OPEN:         enc = "开放"; break;
      case WIFI_AUTH_WEP:          enc = "WEP"; break;
      case WIFI_AUTH_WPA_PSK:      enc = "WPA"; break;
      case WIFI_AUTH_WPA2_PSK:     enc = "WPA2"; break;
      case WIFI_AUTH_WPA_WPA2_PSK: enc = "WPA/WPA2"; break;
      case WIFI_AUTH_WPA2_ENTERPRISE: enc = "WPA2企业"; break;
      case WIFI_AUTH_WPA3_PSK:     enc = "WPA3"; break;
      case WIFI_AUTH_WPA2_WPA3_PSK: enc = "WPA2/WPA3"; break;
      default:                     enc = "未知"; break;
    }
    String bars;
    if (rssi >= -50) bars = "====";
    else if (rssi >= -60) bars = "===_";
    else if (rssi >= -70) bars = "==__";
    else bars = "=___";

    json += "{\"ssid\":\"" + String(WiFi.SSID(i)) + "\",";
    json += "\"rssi\":" + String(rssi) + ",";
    json += "\"bars\":\"" + bars + "\",";
    json += "\"enc\":\"" + enc + "\",";
    json += "\"open\":" + String(WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? "true" : "false");
    json += "}";
  }
  json += "]";
  web_server.send(200, "application/json", json);
  for (int i = 0; i < n; i++) WiFi.scanDelete();
}

// ========================== 配网页面 ==========================
static void handle_ap_root()
{
  web_server.send(200, "text/html; charset=utf-8", R"RAW(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESP32 配网</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;display:flex;justify-content:center;align-items:center}
.card{background:#16213e;border-radius:16px;padding:32px;max-width:400px;width:90%;box-shadow:0 8px 32px rgba(0,0,0,.3)}
h1{font-size:1.4rem;margin-bottom:8px;color:#4ecca3}
.sub{color:#a0a0b0;font-size:.85rem;margin-bottom:24px}
label{display:block;font-size:.85rem;color:#a0a0b0;margin-bottom:4px;margin-top:16px}
input[type=text],input[type=password]{width:100%;padding:12px;border:1px solid #1f3460;border-radius:8px;background:#0f3460;color:#eee;font-size:1rem;outline:none}
input:focus{border-color:#4ecca3}
.select-wrap{position:relative;margin-top:4px}
.select-wrap select{width:100%;padding:12px;border:1px solid #1f3460;border-radius:8px;background:#0f3460;color:#eee;font-size:1rem;outline:none;appearance:none;cursor:pointer}
.select-wrap::after{content:' v';position:absolute;right:14px;top:50%;transform:translateY(-50%);color:#a0a0b0;pointer-events:none;font-size:1.2rem}
.select-wrap option{padding:10px;font-size:.95rem}
.scan-row{display:flex;gap:8px;align-items:center}
.scan-row label{flex:1;margin-top:0}
.btn-scan{padding:12px 16px;border:none;border-radius:8px;background:#0f3460;color:#4ecca3;font-size:.85rem;font-weight:600;cursor:pointer;white-space:nowrap;margin-top:16px}
.btn-scan:disabled{opacity:.5;cursor:not-allowed}
.scan-status{font-size:.8rem;color:#a0a0b0;margin-top:8px;min-height:1.2em}
button[type=submit]{margin-top:24px;width:100%;padding:14px;border:none;border-radius:8px;background:#e94560;color:#fff;font-size:1.1rem;font-weight:600;cursor:pointer}
.hint{margin-top:12px;font-size:.75rem;color:#666;text-align:center}
</style>
</head>
<body>
<div class="card">
<h1>ESP32 配网</h1>
<div class="sub">配置路由器 & 巴法云密钥</div>
<form method="POST" action="/connect" id="form">
<div class="scan-row"><label>WiFi 名称</label><button type="button" class="btn-scan" onclick="scan()">扫描</button></div>
<input type="text" name="ssid" id="ssid" placeholder="手动输入或从下方选择" required>
<div class="select-wrap"><select id="ap-list" onchange="onSelect()"><option value="">-- 点击扫描搜索周围 WiFi --</option></select></div>
<div class="scan-status" id="status"></div>
<label>WiFi 密码</label>
<input type="password" name="pass" id="pass" placeholder="请输入密码">
<label>巴法云密钥 (UID)</label>
<input type="text" name="uid" id="uid" placeholder="巴法云控制台获取">
<button type="submit">保存并连接</button>
</form>
<div class="hint">凭据保存在 ESP32 中，断电不丢失</div>
</div>
<script>
function $(id){return document.getElementById(id)}
function onSelect(){
  var s=$('ap-list'),v=s.value,o=s.options[s.selectedIndex];
  if(v){$('ssid').value=v;$('pass').placeholder=o.dataset.open==='true'?'开放网络无需密码':'请输入密码';if(o.dataset.open==='true')$('pass').value=''}
}
function scan(){
  var b=$('btn-scan'),s=$('ap-list'),t=$('status');
  b.disabled=true;b.textContent='扫描中...';t.textContent='';s.innerHTML='<option>扫描中...</option>';
  fetch('/scan').then(function(r){return r.json()}).then(function(l){
    s.innerHTML='<option value="">-- 选择 WiFi ('+l.length+'个) --</option>';
    l.forEach(function(a){var o=document.createElement('option');o.value=a.ssid;o.textContent=a.bars+' '+a.ssid+' ('+Math.abs(a.rssi)+'dBm '+a.enc+')';o.dataset.open=a.open;s.appendChild(o)});
    t.textContent='扫描到 '+l.length+' 个 WiFi'
  }).catch(function(){t.textContent='扫描失败请重试'}).finally(function(){b.disabled=false;b.textContent='扫描'})
}
setTimeout(scan,500)
</script>
</body>
</html>)RAW");
}

// ========================== 提交配网 ==========================
static void handle_connect()
{
  if (!web_server.hasArg("ssid") || web_server.arg("ssid").length() == 0)
  {
    web_server.sendHeader("Location", "/", true);
    web_server.send(302);
    return;
  }
  String uid = web_server.arg("uid");
  save_creds(web_server.arg("ssid"), web_server.arg("pass"), uid);

  // 更新 UID → 生成 topic → 尝试 MQTT
  mqtt_uid   = uid;
  mqtt_topic = BEMFA_TOPIC;

  try_connect_sta_bg(web_server.arg("ssid"), web_server.arg("pass"));

  web_server.send(200, "text/html; charset=utf-8", R"RAW(<!DOCTYPE html>
<html lang="zh-CN">
<head><meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>连接中</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;display:flex;justify-content:center;align-items:center}
.card{background:#16213e;border-radius:16px;padding:32px;max-width:400px;width:90%;text-align:center;box-shadow:0 8px 32px rgba(0,0,0,.3)}
h1{color:#4ecca3;margin-bottom:16px}
.spinner{display:inline-block;width:40px;height:40px;border:3px solid #1f3460;border-top-color:#4ecca3;border-radius:50%;animation:spin .8s linear infinite;margin-bottom:16px}
@keyframes spin{to{transform:rotate(360deg)}}
p{color:#a0a0b0;margin-bottom:4px}
</style></head>
<body><div class="card">
<h1>连接中...</h1>
<div class="spinner"></div>
<p>正在尝试连接路由器</p>
<p style="font-size:.8rem;color:#666">热点仍在运行</p>
</div>
<script>
setInterval(function(){fetch('/').then(function(r){if(!r.url.includes('192.168.4.1'))location.href=r.url})},2000)
</script></body></html>)RAW");
}

// ========================== 状态页面 ==========================
static void handle_sta_root()
{
  String html = R"RAW(<!DOCTYPE html>
<html lang="zh-CN">
<head>
<meta charset="UTF-8"><meta name="viewport" content="width=device-width,initial-scale=1.0">
<title>ESP32 状态</title>
<style>
*{margin:0;padding:0;box-sizing:border-box}
body{font-family:-apple-system,sans-serif;background:#1a1a2e;color:#eee;min-height:100vh;display:flex;justify-content:center;align-items:center}
.card{background:#16213e;border-radius:16px;padding:32px;max-width:420px;width:90%;box-shadow:0 8px 32px rgba(0,0,0,.3)}
h1{font-size:1.3rem;margin-bottom:24px;color:#0f3460;background:#4ecca3;display:inline-block;padding:4px 14px;border-radius:8px}
.row{display:flex;justify-content:space-between;padding:10px 0;border-bottom:1px solid #1f3460}
.lbl{color:#a0a0b0}.val{font-weight:600}
.ok{color:#4ecca3}.weak{color:#f0a500}.err{color:#e94560}
form{margin-top:24px}
.btn{padding:10px 24px;border:none;border-radius:8px;background:#e94560;color:#fff;font-size:.9rem;font-weight:600;cursor:pointer}
.btn.warn{background:#f0a500;color:#1a1a2e}
.mqtt{display:inline-block;padding:2px 8px;border-radius:4px;font-size:.75rem}
.mqtt.ok{background:#4ecca3;color:#1a1a2e}
.mqtt.err{background:#e94560;color:#fff}
</style>
</head>
<body>
<div class="card">
<h1>ESP32 状态</h1>)RAW";

  // WiFi 信号
  int rssi = WiFi.RSSI();
  auto rc = [](int r){ return r>=-60?"ok":(r>=-70?"weak":"err"); };
  auto rt = [](int r)->const char*{
    if(r>=-50)return"优秀";if(r>=-60)return"良好";if(r>=-70)return"一般";if(r>=-80)return"较差";return"很弱";
  };

  html += "<div class='row'><span class='lbl'>状态</span><span class='val ok'>已连接 + 热点在线</span></div>";
  html += "<div class='row'><span class='lbl'>路由器</span><span class='val'>"+WiFi.SSID()+"</span></div>";
  html += "<div class='row'><span class='lbl'>信号</span><span class='val "+String(rc(rssi))+"'>"+String(rssi)+" dBm ("+rt(rssi)+")</span></div>";
  html += "<div class='row'><span class='lbl'>路由器 IP</span><span class='val ok'>"+WiFi.localIP().toString()+"</span></div>";
  html += "<div class='row'><span class='lbl'>热点 IP</span><span class='val ok'>"+WiFi.softAPIP().toString()+"</span></div>";
  html += "<div class='row'><span class='lbl'>热点名称</span><span class='val'>"+String(AP_SSID)+"</span></div>";

  // MQTT 状态
  String mqttStatus = mqtt.connected()
    ? "<span class='mqtt ok'>已连接</span>"
    : (mqtt_uid.length() == 0
      ? "<span class='mqtt err'>未配置 UID</span>"
      : "<span class='mqtt err'>未连接</span>");
  html += "<div class='row'><span class='lbl'>巴法云 MQTT</span><span class='val'>"+mqttStatus+"</span></div>";

  if (mqtt_topic.length() > 0)
    html += "<div class='row'><span class='lbl'>设备 Topic</span><span class='val ok'>"+mqtt_topic+"</span></div>";

  // 运行时间
  unsigned long sec=millis()/1000;
  String up;
  if(sec<60)up=String(sec)+"秒";
  else if(sec<3600)up=String(sec/60)+"分"+String(sec%60)+"秒";
  else up=String(sec/3600)+"时"+String((sec%3600)/60)+"分";
  html += "<div class='row'><span class='lbl'>运行时间</span><span class='val'>"+up+"</span></div>";

  html += R"RAW(
<form method="POST" action="/reset"><button type="submit" class="btn warn">重置 WiFi</button></form>
</div></body></html>)RAW";

  web_server.send(200, "text/html; charset=utf-8", html);
}

// ========================== 重置 ==========================
static void handle_reset()
{
  clear_creds();
  mqtt_uid = "";
  mqtt_topic = "";
  mqtt_configured = false;
  mqtt.disconnect();
  WiFi.disconnect(true, true);
  state = State::AP_MODE;
  web_server.sendHeader("Location", "/", true);
  web_server.send(302);
}

// ========================== 首页分发 ==========================
static void handle_home()
{
  if (state == State::STA_CONNECTED)
    handle_sta_root();
  else
    handle_ap_root();
}

// ========================== 对外接口 ==========================
void web_setup()
{
  web_server.on("/", HTTP_GET, handle_home);
  web_server.on("/connect", HTTP_POST, handle_connect);
  web_server.on("/scan", HTTP_GET, handle_scan);
  web_server.on("/reset", HTTP_POST, handle_reset);
  web_server.onNotFound([]() { web_server.sendHeader("Location", "/", true); web_server.send(302); });
  web_server.begin();
}

void web_loop()
{
  dns_server.processNextRequest();
  web_server.handleClient();
}
