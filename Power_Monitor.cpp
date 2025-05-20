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

// UI组件
static lv_obj_t *ui_screen = nullptr;
static lv_obj_t *ui_title = nullptr;
static lv_obj_t *ui_total_label = nullptr;
static lv_obj_t *ui_port_labels[MAX_PORTS];
static lv_obj_t *ui_power_values[MAX_PORTS];
static lv_obj_t *ui_power_bars[MAX_PORTS];
static lv_obj_t *ui_total_bar = nullptr;
static lv_obj_t *ui_wifi_status = nullptr;
static lv_timer_t *refresh_timer = NULL;
static lv_timer_t *wifi_timer = NULL;

// 数据获取任务句柄
TaskHandle_t monitorTaskHandle = NULL;

// 数据队列
QueueHandle_t dataQueue = NULL;

// 扫描界面组件
static lv_obj_t *scan_screen = nullptr;
static lv_obj_t *scan_label = nullptr;
static lv_obj_t *scan_status = nullptr;

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
        delay(1000);
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
    
    // 创建扫描界面
    DisplayManager::createScanScreen();
    
    // 在扫描过程中更新状态
    DisplayManager::updateScanStatus("Searching for devices...");
    
    // 在扫描完成后
    DisplayManager::deleteScanScreen();
    
    while (true) {
        bool currentWiFiState = WiFi.status() == WL_CONNECTED;
        
        // WiFi状态发生变化
        if (currentWiFiState != lastWiFiState) {
            if (currentWiFiState) {
                printf("[Monitor] WiFi connected\n");
                syncTimeWithNTP();
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
        printf("[Monitor] Fetching data from: %s\n", url.c_str());
        
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
            
            // 只在显示电源监控屏幕时更新UI
            if (DisplayManager::isPowerMonitorScreenActive()) {
                DisplayManager::updatePowerMonitorScreen();
            }
            
            dataError = false;
            printf("[Monitor] Data updated successfully\n");
        } else {
            dataError = true;
            printf("[Monitor] Failed to fetch data, HTTP code: %d\n", httpCode);
            
            uint32_t currentTime = millis();
            if (currentTime - lastScanTime >= SCAN_RETRY_INTERVAL) {
                if (!isScanning) {
                    DisplayManager::createPowerMonitorScreen();  // 创建电源监控屏幕而不是扫描屏幕
                    isScanning = true;
                }
                
                printf("[Monitor] Trying to find new metrics server...\n");
                String newUrl;
                
                if (NetworkScanner::findMetricsServer(newUrl, true)) {
                    printf("[Monitor] Found new metrics server, updating URL to: %s\n", newUrl.c_str());
                    ConfigManager::saveMonitorUrl(newUrl.c_str());
                    vTaskDelay(pdMS_TO_TICKS(1000));
                    
                    DisplayManager::createPowerMonitorScreen();
                    isScanning = false;
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

// 更新UI
void PowerMonitor_UpdateUI() {
    if (ui_screen == nullptr) {
        return;  // 如果屏幕未初始化，直接返回
    }

    // 使用静态缓冲区避免栈溢出
    static char text_buf[128];
    
    // 更新每个端口的显示
    for (int i = 0; i < MAX_PORTS; i++) {
        if (ui_power_values[i] == nullptr) {
            continue;  // 跳过未初始化的标签
        }

        // 启用标签的重着色功能
        lv_label_set_recolor(ui_power_values[i], true);

        if (dataError) {
            // 数据错误时显示 "--"
            lv_label_set_text(ui_power_values[i], "#888888 --.-W#");
            if (ui_power_bars[i] != nullptr) {
                lv_bar_set_value(ui_power_bars[i], 0, LV_ANIM_OFF);
            }
            continue;
        }

        // 根据电压确定颜色代码
        const char* color_code;
        int voltage_mv = portInfos[i].voltage;
        
        // 设置电压对应的颜色代码，根据区间要求
        if (voltage_mv > 21000) {                        // 21V以上
            color_code = "#FF00FF";                      // 紫色
        } else if (voltage_mv > 16000 && voltage_mv <= 21000) { // 16V~21V
            color_code = "#FF0000";                      // 红色
        } else if (voltage_mv > 13000 && voltage_mv <= 16000) { // 13V~16V
            color_code = "#FF8800";                      // 橙色
        } else if (voltage_mv > 10000 && voltage_mv <= 13000) { // 10V~13V
            color_code = "#FFFF00";                      // 黄色
        } else if (voltage_mv > 6000 && voltage_mv <= 10000) {  // 6V~10V
            color_code = "#00FF00";                      // 绿色
        } else if (voltage_mv >= 0 && voltage_mv <= 6000) {     // 0V~6V
            color_code = "#FFFFFF";                      // 白色
        } else {
            color_code = "#888888";                      // 灰色（未识别电压）
        }

        // 更新功率值标签
        int power_int = (int)(portInfos[i].power * 100);
        memset(text_buf, 0, sizeof(text_buf));  // 清空缓冲区
        snprintf(text_buf, sizeof(text_buf), "%s %d.%02dW#", 
                color_code, 
                power_int / 100, 
                power_int % 100);

        // 更新标签文本
        lv_label_set_text(ui_power_values[i], text_buf);

        // 更新进度条
        if (ui_power_bars[i] != nullptr) {
            int percent = (int)((portInfos[i].power / MAX_PORT_WATTS) * 100);
            if (portInfos[i].power > 0 && percent == 0) {
                percent = 1;
            }
            lv_bar_set_value(ui_power_bars[i], percent, LV_ANIM_OFF);
        }
    }

    // 更新总功率显示
    if (ui_total_label != nullptr) {
        lv_label_set_recolor(ui_total_label, true);
        
        if (dataError) {
            lv_label_set_text(ui_total_label, "Total: #888888 --.-W#");
            if (ui_total_bar != nullptr) {
                lv_bar_set_value(ui_total_bar, 0, LV_ANIM_OFF);
            }
        } else {
            int total_power_int = (int)(totalPower * 100);
            memset(text_buf, 0, sizeof(text_buf));
            snprintf(text_buf, sizeof(text_buf), 
                    "Total: #FFFFFF %d.%02dW#",
                    total_power_int / 100,
                    total_power_int % 100);
            
            lv_label_set_text(ui_total_label, text_buf);

            if (ui_total_bar != nullptr) {
                int totalPercent = (int)((totalPower / MAX_POWER_WATTS) * 100);
                if (totalPower > 0 && totalPercent == 0) {
                    totalPercent = 1;
                }
                lv_bar_set_value(ui_total_bar, totalPercent, LV_ANIM_OFF);
            }
        }
    }
}

// 获取总功率
float PowerMonitor_GetTotalPower() {
    return totalPower;
}