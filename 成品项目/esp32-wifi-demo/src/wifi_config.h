#pragma once

// 启动热点（AP 模式）
void start_ap();

// 后台尝试连接路由器
void try_connect_sta_bg(const String &ssid, const String &pass);

// WiFi 连接成功/断开回调
void on_sta_connected();
void on_sta_disconnected();

// Web 服务器路由注册
void web_setup();

// Web 服务器状态轮询（在 loop 中调用）
void web_loop();
