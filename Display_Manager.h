#pragma once
#include <Arduino.h>
#include "lvgl.h"
#include "Power_Monitor.h"
#include "Display_ST7789.h"  // 添加显示器控制头文件

class DisplayManager {
public:
    static void init();
    static void createAPScreen(const char* ssid, const char* ip);
    static void deleteAPScreen();
    static void showMonitorScreen();
    static bool isAPScreenActive();
    static void createWiFiErrorScreen();
    static void deleteWiFiErrorScreen();
    static bool isWiFiErrorScreenActive();
    
    // 时间显示相关函数
    static void createTimeScreen();
    static void deleteTimeScreen();
    static bool isTimeScreenActive();
    static void updateTimeScreen();
    
    // 电源监控相关
    static void createPowerMonitorScreen();
    static void deletePowerMonitorScreen();
    static void updatePowerMonitorScreen();
    static bool isPowerMonitorScreenActive();
    
    // 屏幕亮度设置
    static const uint8_t BRIGHTNESS_NORMAL = 42;  // 正常亮度
    static const uint8_t BRIGHTNESS_DIM = 8;      // 时间显示时的低亮度
    
private:
    static lv_obj_t* apScreen;
    static lv_obj_t* monitorScreen;
    static lv_obj_t* currentScreen;
    static lv_obj_t* wifiErrorScreen;
    static lv_obj_t* powerMonitorScreen;
    static bool apScreenActive;
    static bool wifiErrorScreenActive;
    static bool timeScreenActive;
    static bool powerMonitorScreenActive;
    static lv_obj_t* timeLabel;
    static unsigned long screenSwitchTime;  // 用于记录屏幕切换时间
    static lv_obj_t* ui_title;
    static lv_obj_t* ui_total_label;
    static lv_obj_t* ui_port_labels[MAX_PORTS];
    static lv_obj_t* ui_power_values[MAX_PORTS];
    static lv_obj_t* ui_power_bars[MAX_PORTS];
    static lv_obj_t* ui_total_bar;
    static lv_obj_t* ui_wifi_status;
    static bool dataError;  // 添加数据错误标志
    static void createAPScreenContent(const char* ssid, const char* ip);
    static bool createPowerMonitorContent();
    static void setScreenBrightness(uint8_t brightness);  // 添加亮度控制函数
}; 