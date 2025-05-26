/**
 * @file     Power_Monitor.cpp
 * @author   Claude AI
 * @version  V1.0
 * @date     2024-10-30
 * @brief    Power Monitor Module Implementation
 */

#include "Power_Monitor.h"
#include "Display_Manager.h"
#include <WiFi.h>
#include "Wireless.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "Config_Manager.h"
#include "Network_Scanner.h"
#include <time.h>

// 声明外部常量引用
extern const int MAX_POWER_WATTS;
extern const int MAX_PORT_WATTS;
extern const char* DATA_URL;
extern const int REFRESH_INTERVAL;

#define LV_SPRINTF_CUSTOM 1

// 全局变量
PortInfo portInfos[MAX_PORTS];
static float totalPower = 0.0f;  // 内部静态变量，只在本模块中可见
bool dataError = false;  // 数据错误标志
static bool scanUIActive = false;

// 数据获取任务句柄
TaskHandle_t monitorTaskHandle = NULL;

// 数据队列
QueueHandle_t dataQueue = NULL;

// NTP服务器配置
const char* NTP_SERVER = "ntp.aliyun.com";  // 阿里云NTP服务器
const char* TZ_INFO = "CST-8";              // 中国时区
const long  GMT_OFFSET_SEC = 8 * 3600;      // 时区偏移（北京时间：GMT+8）
const int   DAYLIGHT_OFFSET_SEC = 0;        // 夏令时偏移（中国不使用夏令时）

// NTP同步间隔（30分钟）
const unsigned long NTP_SYNC_INTERVAL = 30 * 60 * 1000;
static unsigned long lastNTPSync = 0;

// NTP时间同步函数
void syncTimeWithNTP() {
    printf("[Time] Synchronizing time with NTP server...\n");
    
    // 配置时区
    setenv("TZ", TZ_INFO, 1);
    tzset();
    
    // 配置NTP服务器
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC, NTP_SERVER);
    
    // 等待时间同步
    int retry = 0;
    const int maxRetries = 5;
    time_t now = 0;
    struct tm timeinfo = { 0 };
    
    while (timeinfo.tm_year < (2024 - 1900) && retry < maxRetries) {
        printf("[Time] Waiting for NTP sync... (%d/%d)\n", retry + 1, maxRetries);
        vTaskDelay(pdMS_TO_TICKS(1000));
        time(&now);
        localtime_r(&now, &timeinfo);
        retry++;
    }
    
    if (timeinfo.tm_year >= (2024 - 1900)) {
        printf("[Time] Time synchronized: %04d-%02d-%02d %02d:%02d:%02d\n",
               timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
               timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        lastNTPSync = millis();
    } else {
        printf("[Time] NTP sync failed after %d attempts\n", maxRetries);
    }
}

// 检查是否需要同步时间
void checkAndSyncTime() {
    unsigned long currentMillis = millis();
    
    // 检查是否需要进行定期同步
    if (currentMillis - lastNTPSync >= NTP_SYNC_INTERVAL) {
        if (WiFi.status() == WL_CONNECTED) {
            syncTimeWithNTP();
        }
    }
}

// 初始化电源监控
void PowerMonitor_Init() {
    // 初始化端口信息
    for (int i = 0; i < MAX_PORTS; i++) {
        portInfos[i].id = i;
        portInfos[i].state = 0;
        portInfos[i].fc_protocol = 0;
        portInfos[i].current = 0;
        portInfos[i].voltage = 0;
        portInfos[i].power = 0.0f;
    }
    
    // 设置端口名称
    portInfos[0].name = "A1";
    portInfos[1].name = "C1";
    portInfos[2].name = "C2";
    portInfos[3].name = "C3";
    portInfos[4].name = "C4";
    
    // 创建数据队列
    dataQueue = xQueueCreate(1, sizeof(PowerData));
    
    // 创建UI
    DisplayManager::createPowerMonitorScreen();
    
    // 启动监控任务
    PowerMonitor_Start();
}

// 监控任务
void PowerMonitor_Task(void* parameter) {
    HTTPClient http;
    String payload;
    bool lastWiFiState = false;
    uint32_t wifiRetryTime = 0;
    const uint32_t WIFI_RETRY_INTERVAL = 5000;
    uint32_t lastScanTime = 0;
    const uint32_t SCAN_RETRY_INTERVAL = 30000;
    bool isScanning = false;
    
    while (true) {
        bool currentWiFiState = WiFi.status() == WL_CONNECTED;
        
        // WiFi状态发生变化
        if (currentWiFiState != lastWiFiState) {
            if (currentWiFiState) {
                printf("[Monitor] WiFi connected\n");
                syncTimeWithNTP();
                // WiFi连接后，显示电源监控界面
                DisplayManager::createPowerMonitorScreen();
                vTaskDelay(pdMS_TO_TICKS(1000));
            } else {
                printf("[Monitor] WiFi disconnected\n");
                dataError = true;
            }
            lastWiFiState = currentWiFiState;
        }
        
        // 检查并同步时间
        if (currentWiFiState) {
            checkAndSyncTime();
        }
        
        // 检查WiFi连接
        if (!currentWiFiState) {
            uint32_t currentTime = millis();
            if (currentTime - wifiRetryTime >= WIFI_RETRY_INTERVAL) {
                printf("[Monitor] Trying to reconnect WiFi...\n");
                WiFi.reconnect();
                wifiRetryTime = currentTime;
            }
            
            dataError = true;
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }
        
        // WiFi已连接，获取数据
        String url = ConfigManager::getMonitorUrl();
        
        http.begin(url);
        int httpCode = http.GET();
        
        if (httpCode > 0 && httpCode == HTTP_CODE_OK) {
            payload = http.getString();
            totalPower = 0.0f;
            
            // 逐行解析数据
            int position = 0;
            while (position < payload.length()) {
                int lineEnd = payload.indexOf('\n', position);
                if (lineEnd == -1) lineEnd = payload.length();
                
                String line = payload.substring(position, lineEnd);
                position = lineEnd + 1;
                
                // 解析电流数据
                if (line.startsWith("ionbridge_port_current{id=")) {
                    // 提取端口ID
                    int idStart = line.indexOf("\"") + 1;
                    int idEnd = line.indexOf("\"", idStart);
                    int portId = line.substring(idStart, idEnd).toInt();
                    
                    // 提取电流值
                    int valueStart = line.indexOf("}") + 1;
                    int current = line.substring(valueStart).toInt();
                    
                    // 更新端口电流
                    if (portId >= 0 && portId < MAX_PORTS) {
                        portInfos[portId].current = current;
                    }
                }
                // 解析电压数据
                else if (line.startsWith("ionbridge_port_voltage{id=")) {
                    // 提取端口ID
                    int idStart = line.indexOf("\"") + 1;
                    int idEnd = line.indexOf("\"", idStart);
                    int portId = line.substring(idStart, idEnd).toInt();
                    
                    // 提取电压值
                    int valueStart = line.indexOf("}") + 1;
                    int voltage = line.substring(valueStart).toInt();
                    
                    // 更新端口电压
                    if (portId >= 0 && portId < MAX_PORTS) {
                        portInfos[portId].voltage = voltage;
                    }
                }
                // 解析状态数据
                else if (line.startsWith("ionbridge_port_state{id=")) {
                    // 提取端口ID
                    int idStart = line.indexOf("\"") + 1;
                    int idEnd = line.indexOf("\"", idStart);
                    int portId = line.substring(idStart, idEnd).toInt();
                    
                    // 提取状态值
                    int valueStart = line.indexOf("}") + 1;
                    int state = line.substring(valueStart).toInt();
                    
                    // 更新端口状态
                    if (portId >= 0 && portId < MAX_PORTS) {
                        portInfos[portId].state = state;
                    }
                }
                // 解析协议数据
                else if (line.startsWith("ionbridge_port_fc_protocol{id=")) {
                    // 提取端口ID
                    int idStart = line.indexOf("\"") + 1;
                    int idEnd = line.indexOf("\"", idStart);
                    int portId = line.substring(idStart, idEnd).toInt();
                    
                    // 提取协议值
                    int valueStart = line.indexOf("}") + 1;
                    int protocol = line.substring(valueStart).toInt();
                    
                    // 更新端口协议
                    if (portId >= 0 && portId < MAX_PORTS) {
                        portInfos[portId].fc_protocol = protocol;
                    }
                }
            }
            
            // 计算每个端口的功率
            for (int i = 0; i < MAX_PORTS; i++) {
                // 功率 = 电流(mA) * 电压(mV) / 1000000 (转换为W)
                portInfos[i].power = (portInfos[i].current * portInfos[i].voltage) / 1000000.0f;
                totalPower += portInfos[i].power;
            }
            
            // 更新UI
            if (DisplayManager::isPowerMonitorScreenActive()) {
                DisplayManager::updatePowerMonitorScreen();
            }
            
            dataError = false;
            
            // 如果之前在扫描，现在恢复了连接，则切换回电源监控界面
            if (isScanning) {
                DisplayManager::deleteScanScreen();
                DisplayManager::createPowerMonitorScreen();
                isScanning = false;
            }
        } else {
            dataError = true;
            printf("[Monitor] Failed to fetch data, HTTP code: %d\n", httpCode);
            
            uint32_t currentTime = millis();
            if (currentTime - lastScanTime >= SCAN_RETRY_INTERVAL) {
                if (!isScanning) {
                    // 显示扫描界面
                    DisplayManager::createScanScreen();
                    DisplayManager::updateScanStatus("Searching for devices...");
                    isScanning = true;
                }
                
                printf("[Monitor] Trying to find new metrics server...\n");
                String newUrl;
                
                if (NetworkScanner::findMetricsServer(newUrl, true)) {
                    printf("[Monitor] Found new metrics server, updating URL to: %s\n", newUrl.c_str());
                    ConfigManager::saveMonitorUrl(newUrl.c_str());
                    
                    // 更新扫描状态并等待
                    DisplayManager::updateScanStatus("Device found! Connecting...");
                    vTaskDelay(pdMS_TO_TICKS(2000));
                    
                    // 切换回电源监控界面
                    DisplayManager::deleteScanScreen();
                    DisplayManager::createPowerMonitorScreen();
                    isScanning = false;
                } else {
                    // 更新扫描状态
                    DisplayManager::updateScanStatus("No devices found, retrying...");
                }
                
                lastScanTime = currentTime;
            }
        }
        
        http.end();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// 启动监控任务
void PowerMonitor_Start() {
    if (monitorTaskHandle == NULL) {
        xTaskCreate(
            PowerMonitor_Task,    // 任务函数
            "MonitorTask",        // 任务名称
            16284,                 // 堆栈大小
            NULL,                 // 任务参数
            1,                    // 任务优先级
            &monitorTaskHandle    // 任务句柄
        );
    }
}

// 停止监控任务
void PowerMonitor_Stop() {
    if (monitorTaskHandle != NULL) {
        vTaskDelete(monitorTaskHandle);
        monitorTaskHandle = NULL;
    }
}

// 获取总功率
float PowerMonitor_GetTotalPower() {
    return totalPower;
}