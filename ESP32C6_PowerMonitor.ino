/**
 ******************************************************************************
 * @file     LVGL_Arduino.ino
 * @author   YPW
 * @version  V1.8
 * @date     2025-05-02
 * @brief    功率监控应用程序
 * @license  MIT
 * @copyright Copyright (c) 2025, YPW
 ******************************************************************************
 * 
 * 应用程序说明：实现CP-02电源监控显示功能
 *
 * 硬件资源和引脚分配：
 * 1. 显示器接口 --> 查看Display_ST7789.h配置
 * 2. 无线模块接口 --> 查看Wireless.h配置
 * 
 ******************************************************************************
 * 
 * 开发平台：ESP32
 * 开发文档：https://www.waveshare.net/wiki/ESP32-S3-LCD-1.47
 *
 ******************************************************************************
 */
#include <Arduino.h>
#include "Display_ST7789.h"
#include "LVGL_Driver.h"
#include "Wireless.h"
#include "RGB_lamp.h"
#include "Power_Monitor.h"
#include "Config_Manager.h"
#include "Display_Manager.h"

// 电源监控配置
const int MAX_POWER_WATTS = 160;    // 最大总功率 160W
const int MAX_PORT_WATTS = 140;     // 每个端口最大功率 140W
const int REFRESH_INTERVAL = 500;   // 刷新间隔 (ms)

// 任务计时器
unsigned long lastWiFiCheck = 0;
const unsigned long WIFI_CHECK_INTERVAL = 1000;   // WiFi状态检查间隔 (ms)

// 系统状态
bool systemInitialized = false;
bool displayInitialized = false;
bool powerMonitorInitialized = false;
bool lastRGBState = false;

// 屏幕切换相关
const unsigned long SCREEN_SWITCH_DELAY = 60000;  // 1分钟延迟
unsigned long lastPowerCheckTime = 0;
const unsigned long POWER_CHECK_INTERVAL = 1000;  // 每秒检查一次功率

// RGB控制任务相关
TaskHandle_t rgbTaskHandle = NULL;
const uint32_t RGB_TASK_STACK_SIZE = 2048;
const UBaseType_t RGB_TASK_PRIORITY = 1;
const unsigned long RGB_UPDATE_INTERVAL = 20;     // RGB更新间隔 (ms)

// RGB控制任务
void rgbControlTask(void* parameter) {
    printf("[RGB] Task started\n");
    unsigned long lastUpdate = 0;
    bool lastState = ConfigManager::isRGBEnabled();
    
    while(1) {
        unsigned long currentMillis = millis();
        bool currentState = ConfigManager::isRGBEnabled();
        
        // 检查RGB灯状态变化
        if (currentState != lastState) {
            if (!currentState) {
                RGB_Lamp_Off();
                printf("[RGB] RGB lamp disabled\n");
            } else {
                printf("[RGB] RGB lamp enabled\n");
            }
            lastState = currentState;
        }
        
        // 更新RGB灯效果
        if (currentState && (currentMillis - lastUpdate >= RGB_UPDATE_INTERVAL)) {
            RGB_Lamp_Loop(1);
            lastUpdate = currentMillis;
        }
        
        // 任务延时
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// 系统初始化函数
bool initializeSystem() {
    printf("\n[System] Starting system initialization...\n");
    delay(100);
    
    // 初始化显示相关组件
    printf("[Display] Initializing display components...\n");
    LCD_Init();  // 直接调用，不检查返回值
    delay(100);
    
    Set_Backlight(40);
    delay(50);
    
    Lvgl_Init();  // 直接调用，不检查返回值
    delay(100);
    
    DisplayManager::init();
    displayInitialized = true;
    printf("[Display] Display initialization complete\n");
    delay(100);

    // 初始化配置管理器
    printf("[Config] Starting configuration manager...\n");
    ConfigManager::begin();
    delay(100);

    // 初始化RGB灯
    printf("[RGB] Initializing RGB lamp...\n");
    RGB_Lamp_Init();  // 初始化RGB灯数据
    lastRGBState = ConfigManager::isRGBEnabled();
    if (!lastRGBState) {
        RGB_Lamp_Off();
        printf("[RGB] RGB lamp disabled\n");
    } else {
        printf("[RGB] RGB lamp enabled\n");
    }
    
    // 创建RGB控制任务
    xTaskCreate(
        rgbControlTask,          // 任务函数
        "RGBControlTask",        // 任务名称
        RGB_TASK_STACK_SIZE,     // 堆栈大小
        NULL,                    // 任务参数
        RGB_TASK_PRIORITY,       // 任务优先级
        &rgbTaskHandle           // 任务句柄
    );
    
    delay(100);
    printf("[System] System initialization complete\n");
    return true;
}

void checkAndUpdateScreen() {
    if (!powerMonitorInitialized) return;
    
    unsigned long currentMillis = millis();
    if (currentMillis - lastPowerCheckTime < POWER_CHECK_INTERVAL) return;
    lastPowerCheckTime = currentMillis;
    
    float totalPower = PowerMonitor_GetTotalPower();
    static bool lowPowerTimerStarted = false;
    static unsigned long lowPowerStartTime = 0;
    
    if (totalPower < 1.0) {
        if (!lowPowerTimerStarted) {
            lowPowerStartTime = currentMillis;
            lowPowerTimerStarted = true;
        } else if (currentMillis - lowPowerStartTime >= SCREEN_SWITCH_DELAY) {
            // 如果低功率持续1分钟，切换到时间显示
            if (!DisplayManager::isTimeScreenActive()) {
                DisplayManager::createTimeScreen();
            }
            DisplayManager::updateTimeScreen();  // 更新时间显示
        }
    } else if (totalPower > 2.0) {
        lowPowerTimerStarted = false;
        if (DisplayManager::isTimeScreenActive()) {
            DisplayManager::deleteTimeScreen();
            DisplayManager::createPowerMonitorScreen();  // 切换回电源监控屏幕
        }
    }
}

void setup()
{
    // 等待系统稳定
    delay(1000);
    
    // 执行系统初始化
    systemInitialized = initializeSystem();
    
    if (!systemInitialized) {
        printf("[System] System initialization failed, restarting...\n");
        delay(3000);
        ESP.restart();
    }
}

void loop()
{
    // 如果系统未初始化，不执行任何操作
    if (!systemInitialized) {
        delay(1000);
        return;
    }

    unsigned long currentMillis = millis();
    
    // 处理LVGL任务
    if (displayInitialized) {
        lv_timer_handler();
    }
    
    // 检查并更新屏幕显示
    checkAndUpdateScreen();
    
    // 定期检查WiFi状态
    if (currentMillis - lastWiFiCheck >= WIFI_CHECK_INTERVAL) {
        bool wifiReady = ConfigManager::isConfigured() && ConfigManager::isConnected();
        
        if (wifiReady && !powerMonitorInitialized) {
            printf("[Power] Initializing power monitor...\n");
            PowerMonitor_Init();
            powerMonitorInitialized = true;
            printf("[Power] Power monitor initialized successfully\n");
            printf("[WiFi] Connected to: %s\n", ConfigManager::getSSID().c_str());
            printf("[WiFi] IP address: %s\n", WiFi.localIP().toString().c_str());
        } else if (!wifiReady && powerMonitorInitialized) {
            powerMonitorInitialized = false;
            printf("[System] Power monitor stopped due to WiFi disconnection\n");
            if (ConfigManager::isConfigured()) {
                printf("[WiFi] Connection lost, will retry automatically\n");
            }
        }
        
        lastWiFiCheck = currentMillis;
    }
    
    // 给其他任务一些执行时间
    delay(5);  // 短暂延时，防止看门狗复位
}
