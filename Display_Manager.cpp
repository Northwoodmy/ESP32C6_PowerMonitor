#include "Display_Manager.h"
#include <time.h>

// 静态成员初始化
lv_obj_t* DisplayManager::mainScreen = nullptr;
lv_obj_t* DisplayManager::currentScreen = nullptr;

// WiFi错误UI组件
lv_obj_t* DisplayManager::wifiErrorTitle = nullptr;
lv_obj_t* DisplayManager::wifiErrorMessage = nullptr;
lv_obj_t* DisplayManager::wifiErrorContainer = nullptr;

// 时间显示UI组件
lv_obj_t* DisplayManager::timeContainer = nullptr;
lv_obj_t* DisplayManager::timeLabel = nullptr;
lv_obj_t* DisplayManager::dateLabel = nullptr;

// 电源监控UI组件
lv_obj_t* DisplayManager::powerMonitorContainer = nullptr;
lv_obj_t* DisplayManager::ui_title = nullptr;
lv_obj_t* DisplayManager::ui_total_label = nullptr;
lv_obj_t* DisplayManager::ui_port_labels[MAX_PORTS] = {nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t* DisplayManager::ui_power_values[MAX_PORTS] = {nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t* DisplayManager::ui_power_bars[MAX_PORTS] = {nullptr, nullptr, nullptr, nullptr, nullptr};
lv_obj_t* DisplayManager::ui_total_bar = nullptr;
lv_obj_t* DisplayManager::ui_wifi_status = nullptr;

// 扫描界面UI组件
lv_obj_t* DisplayManager::scanContainer = nullptr;
lv_obj_t* DisplayManager::scanLabel = nullptr;
lv_obj_t* DisplayManager::scanStatus = nullptr;

// 状态标志初始化
bool DisplayManager::apScreenActive = false;
bool DisplayManager::wifiErrorScreenActive = false;
bool DisplayManager::timeScreenActive = false;
bool DisplayManager::powerMonitorScreenActive = false;
bool DisplayManager::scanScreenActive = false;
bool DisplayManager::dataError = false;
unsigned long DisplayManager::screenSwitchTime = 0;

// AP配置UI组件
lv_obj_t* DisplayManager::apContainer = nullptr;
lv_obj_t* DisplayManager::apTitle = nullptr;
lv_obj_t* DisplayManager::apContent = nullptr;

SemaphoreHandle_t DisplayManager::lvgl_mutex = nullptr;

// 添加静态变量记录上次时间
static int lastHour = -1;
static int lastMin = -1;
static int lastSec = -1;

void DisplayManager::init() {
    // 创建LVGL互斥锁
    lvgl_mutex = xSemaphoreCreateMutex();
    if (lvgl_mutex == nullptr) {
        printf("[Display] Failed to create LVGL mutex\n");
        return;
    }
    
    // 创建主屏幕
    createMainScreen();
}

void DisplayManager::createMainScreen() {
    if (mainScreen == nullptr) {
        takeLvglLock();
        mainScreen = lv_obj_create(NULL);
        lv_obj_set_style_bg_color(mainScreen, lv_color_black(), 0);
        currentScreen = mainScreen;
        lv_scr_load(mainScreen);
        giveLvglLock();
        printf("[Display] Main screen created successfully\n");
    }
}

void DisplayManager::hideAllContainers() {
    // 注意：此函数假设调用者已经获取了LVGL锁
    if (apContainer) lv_obj_add_flag(apContainer, LV_OBJ_FLAG_HIDDEN);
    if (wifiErrorContainer) lv_obj_add_flag(wifiErrorContainer, LV_OBJ_FLAG_HIDDEN);
    if (timeContainer) lv_obj_add_flag(timeContainer, LV_OBJ_FLAG_HIDDEN);
    if (powerMonitorContainer) lv_obj_add_flag(powerMonitorContainer, LV_OBJ_FLAG_HIDDEN);
    if (scanContainer) lv_obj_add_flag(scanContainer, LV_OBJ_FLAG_HIDDEN);
    
    // 同时重置所有屏幕状态标志，确保状态一致性
    apScreenActive = false;
    wifiErrorScreenActive = false;
    timeScreenActive = false;
    powerMonitorScreenActive = false;
    scanScreenActive = false;
}

void DisplayManager::createWiFiErrorScreen() {
    printf("[Display] Creating WiFi error screen\n");
    
    takeLvglLock();
    
    if (wifiErrorScreenActive) {
        printf("[Display] WiFi error screen already active\n");
        giveLvglLock();
        return;
    }
    
    // 检查状态一致性
    if (!isValidScreenState()) {
        printf("[Display] Invalid screen state detected, resetting...\n");
        resetAllScreenStates();
        takeLvglLock(); // 重新获取锁，因为resetAllScreenStates会释放锁
    }
    
    hideAllContainers();
    
    // 如果容器不存在，创建它
    if (wifiErrorContainer == nullptr) {
        wifiErrorContainer = lv_obj_create(mainScreen);
        lv_obj_set_size(wifiErrorContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(wifiErrorContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(wifiErrorContainer, 0, 0);
        
        // 创建错误标题
        wifiErrorTitle = lv_label_create(wifiErrorContainer);
        lv_label_set_text(wifiErrorTitle, "WiFi Connection Failed");
        lv_obj_set_style_text_color(wifiErrorTitle, lv_color_make(0xFF, 0x00, 0x00), 0);
        lv_obj_set_style_text_font(wifiErrorTitle, &lv_font_montserrat_24, 0);
        lv_obj_align(wifiErrorTitle, LV_ALIGN_TOP_MID, 0, 30);
        
        // 创建提示信息
        wifiErrorMessage = lv_label_create(wifiErrorContainer);
        lv_label_set_text(wifiErrorMessage, "Please check your WiFi settings\nRetrying connection...");
        lv_obj_set_style_text_color(wifiErrorMessage, lv_color_white(), 0);
        lv_obj_set_style_text_font(wifiErrorMessage, &lv_font_montserrat_16, 0);
        lv_obj_set_style_text_align(wifiErrorMessage, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_align(wifiErrorMessage, LV_ALIGN_CENTER, 0, 0);
    }
    
    // 显示WiFi错误容器
    lv_obj_clear_flag(wifiErrorContainer, LV_OBJ_FLAG_HIDDEN);
    wifiErrorScreenActive = true;
    
    giveLvglLock();
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
}

void DisplayManager::createTimeScreen() {
    printf("[Display] Creating time screen\n");
    
    takeLvglLock();
    
    if (timeScreenActive) {
        printf("[Display] Time screen already active\n");
        giveLvglLock();
        return;
    }
    
    hideAllContainers();
    
    // 如果容器不存在，创建它
    if (timeContainer == nullptr) {
        timeContainer = lv_obj_create(mainScreen);
        lv_obj_set_size(timeContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(timeContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(timeContainer, 0, 0);
        
        // 创建背景图案容器
        lv_obj_t* bgContainer = lv_obj_create(timeContainer);
        lv_obj_set_size(bgContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(bgContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(bgContainer, 0, 0);
        lv_obj_clear_flag(bgContainer, LV_OBJ_FLAG_SCROLLABLE);

        // 创建外部装饰圆环
        lv_obj_t* outerCircle = lv_obj_create(bgContainer);
        lv_obj_set_size(outerCircle, 150, 150);
        lv_obj_set_style_radius(outerCircle, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(outerCircle, lv_color_hex(0x111111), 0);
        lv_obj_set_style_border_width(outerCircle, 1, 0);
        lv_obj_set_style_border_color(outerCircle, lv_color_hex(0x333333), 0);
        lv_obj_align(outerCircle, LV_ALIGN_CENTER, 0, 0);
        
        // 创建中间圆环
        lv_obj_t* circle1 = lv_obj_create(bgContainer);
        lv_obj_set_size(circle1, 120, 120);
        lv_obj_set_style_radius(circle1, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(circle1, lv_color_hex(0x222222), 0);
        lv_obj_set_style_border_width(circle1, 2, 0);
        lv_obj_set_style_border_color(circle1, lv_color_hex(0x444444), 0);
        lv_obj_align(circle1, LV_ALIGN_CENTER, 0, 0);
        
        // 创建内部圆环
        lv_obj_t* circle2 = lv_obj_create(bgContainer);
        lv_obj_set_size(circle2, 100, 100);
        lv_obj_set_style_radius(circle2, LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_bg_color(circle2, lv_color_hex(0x111111), 0);
        lv_obj_set_style_border_width(circle2, 1, 0);
        lv_obj_set_style_border_color(circle2, lv_color_hex(0x333333), 0);
        lv_obj_align(circle2, LV_ALIGN_CENTER, 0, 0);

        // 创建装饰性弧线
        for(int i = 0; i < 4; i++) {
            lv_obj_t* arc = lv_arc_create(bgContainer);
            lv_obj_set_size(arc, 160, 160);
            lv_arc_set_rotation(arc, i * 90);
            lv_arc_set_bg_angles(arc, 0, 60);
            lv_arc_set_angles(arc, 0, 60);
            lv_obj_set_style_arc_color(arc, lv_color_hex(0x222222), LV_PART_MAIN);
            lv_obj_set_style_arc_color(arc, lv_color_hex(0x0066FF), LV_PART_INDICATOR);
            lv_obj_set_style_arc_width(arc, 2, LV_PART_MAIN);
            lv_obj_set_style_arc_width(arc, 2, LV_PART_INDICATOR);
            lv_obj_align(arc, LV_ALIGN_CENTER, 0, 0);
        }
        
        // 创建时间标签
        timeLabel = lv_label_create(timeContainer);
        lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, LV_PART_MAIN);
        lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
        lv_obj_set_width(timeLabel, LV_PCT(100));
        lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, 0);

        // 创建日期标签
        dateLabel = lv_label_create(timeContainer);
        lv_obj_set_style_text_color(dateLabel, lv_color_hex(0x888888), LV_PART_MAIN);
        lv_obj_set_style_text_font(dateLabel, &lv_font_montserrat_16, LV_PART_MAIN);
        lv_obj_align(dateLabel, LV_ALIGN_CENTER, 0, 40);
        lv_label_set_text(dateLabel, ""); // 设置初始空文本
        
        // 创建装饰性点
        for(int i = 0; i < 12; i++) {
            // 主要刻度点
            lv_obj_t* dot = lv_obj_create(bgContainer);
            lv_obj_set_size(dot, i % 3 == 0 ? 6 : 4, i % 3 == 0 ? 6 : 4);
            lv_obj_set_style_radius(dot, LV_RADIUS_CIRCLE, 0);
            lv_obj_set_style_bg_color(dot, i % 3 == 0 ? lv_color_hex(0x0066FF) : lv_color_hex(0x666666), 0);
            lv_obj_set_style_border_width(dot, 0, 0);
            
            float angle = i * 30 * 3.14159f / 180;
            int x = 70 * cos(angle);
            int y = 70 * sin(angle);
            lv_obj_align(dot, LV_ALIGN_CENTER, x, y);
            
            // 外圈装饰点
            if(i % 3 == 0) {
                lv_obj_t* outerDot = lv_obj_create(bgContainer);
                lv_obj_set_size(outerDot, 3, 3);
                lv_obj_set_style_radius(outerDot, LV_RADIUS_CIRCLE, 0);
                lv_obj_set_style_bg_color(outerDot, lv_color_hex(0x0066FF), 0);
                lv_obj_set_style_border_width(outerDot, 0, 0);
                
                x = 85 * cos(angle);
                y = 85 * sin(angle);
                lv_obj_align(outerDot, LV_ALIGN_CENTER, x, y);
            }
        }
    }
    
    // 显示时间容器
    lv_obj_clear_flag(timeContainer, LV_OBJ_FLAG_HIDDEN);
    timeScreenActive = true;
    screenSwitchTime = millis();
    
    giveLvglLock();
    
    // 更新时间显示
    updateTimeScreen();
    
    // 设置为低亮度
    setScreenBrightness(BRIGHTNESS_DIM);
}

void DisplayManager::createPowerMonitorScreen() {
    printf("[Display] Creating power monitor screen\n");
    
    takeLvglLock();
    
    if (powerMonitorScreenActive) {
        printf("[Display] Power monitor screen already active\n");
        giveLvglLock();
        return;
    }
    
    hideAllContainers();
    
    // 如果容器不存在，创建它
    if (powerMonitorContainer == nullptr) {
        powerMonitorContainer = lv_obj_create(mainScreen);
        lv_obj_set_size(powerMonitorContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(powerMonitorContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(powerMonitorContainer, 0, 0);
        
        // 创建电源监控内容
        if (!createPowerMonitorContent()) {
            printf("[Display] Failed to create power monitor content\n");
            giveLvglLock();
            return;
        }
    }
    
    // 显示电源监控容器
    lv_obj_clear_flag(powerMonitorContainer, LV_OBJ_FLAG_HIDDEN);
    powerMonitorScreenActive = true;
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
    
    giveLvglLock();
    printf("[Display] Power monitor screen created successfully\n");
}

void DisplayManager::createScanScreen() {
    printf("[Display] Creating scan screen\n");
    
    takeLvglLock();
    
    if (scanScreenActive) {
        printf("[Display] Scan screen already active\n");
        giveLvglLock();
        return;
    }
    
    hideAllContainers();
    
    // 如果容器不存在，创建它
    if (scanContainer == nullptr) {
        scanContainer = lv_obj_create(mainScreen);
        lv_obj_set_size(scanContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(scanContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(scanContainer, 0, 0);
        
        // 创建扫描标题
        scanLabel = lv_label_create(scanContainer);
        lv_label_set_text(scanLabel, "Looking for cp02...");
        lv_obj_set_style_text_color(scanLabel, lv_color_hex(0xFFFFFF), 0);
        lv_obj_set_style_text_font(scanLabel, &lv_font_montserrat_24, 0);
        lv_obj_align(scanLabel, LV_ALIGN_CENTER, 0, -30);
        
        // 创建状态文本
        scanStatus = lv_label_create(scanContainer);
        lv_label_set_text(scanStatus, "Using mDNS to find cp02 device");
        lv_obj_set_style_text_color(scanStatus, lv_color_hex(0x00FF00), 0);
        lv_obj_set_style_text_font(scanStatus, &lv_font_montserrat_20, 0);
        lv_obj_set_style_text_align(scanStatus, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(scanStatus, 240);
        lv_label_set_long_mode(scanStatus, LV_LABEL_LONG_WRAP);
        lv_obj_align(scanStatus, LV_ALIGN_CENTER, 0, 30);
    }
    
    // 显示扫描容器
    lv_obj_clear_flag(scanContainer, LV_OBJ_FLAG_HIDDEN);
    scanScreenActive = true;
    
    giveLvglLock();
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
}

// 删除屏幕函数修改为隐藏对应容器
void DisplayManager::deleteWiFiErrorScreen() {
    if (wifiErrorContainer != nullptr) {
        takeLvglLock();
        lv_obj_add_flag(wifiErrorContainer, LV_OBJ_FLAG_HIDDEN);
        wifiErrorScreenActive = false;
        giveLvglLock();
    }
}

void DisplayManager::deleteTimeScreen() {
    if (timeContainer != nullptr) {
        takeLvglLock();
        lv_obj_add_flag(timeContainer, LV_OBJ_FLAG_HIDDEN);
        timeScreenActive = false;
        giveLvglLock();
    }
}

void DisplayManager::deletePowerMonitorScreen() {
    if (powerMonitorContainer != nullptr) {
        takeLvglLock();
        lv_obj_add_flag(powerMonitorContainer, LV_OBJ_FLAG_HIDDEN);
        powerMonitorScreenActive = false;
        giveLvglLock();
    }
}

void DisplayManager::deleteScanScreen() {
    if (scanContainer != nullptr) {
        takeLvglLock();
        lv_obj_add_flag(scanContainer, LV_OBJ_FLAG_HIDDEN);
        scanScreenActive = false;
        giveLvglLock();
    }
}

void DisplayManager::createAPScreen(const char* ssid, const char* ip) {
    printf("[Display] Creating AP screen\n");
    
    takeLvglLock();
    
    if (apScreenActive) {
        printf("[Display] AP screen already active\n");
        giveLvglLock();
        return;
    }
    
    hideAllContainers();
    
    // 如果容器不存在，创建它
    if (apContainer == nullptr) {
        apContainer = lv_obj_create(mainScreen);
        lv_obj_set_size(apContainer, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(apContainer, lv_color_black(), 0);
        lv_obj_set_style_border_width(apContainer, 0, 0);
        
        // 创建屏幕内容
        createAPScreenContent(ssid, ip);
    }
    
    // 显示AP容器
    lv_obj_clear_flag(apContainer, LV_OBJ_FLAG_HIDDEN);
    apScreenActive = true;
    
    giveLvglLock();
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
}

void DisplayManager::createAPScreenContent(const char* ssid, const char* ip) {
    // 创建标题
    apTitle = lv_label_create(apContainer);
    lv_label_set_text(apTitle, "WiFi Setup");
    lv_obj_align(apTitle, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_text_color(apTitle, lv_color_white(), 0);
    lv_obj_set_style_text_font(apTitle, &lv_font_montserrat_20, 0);
    
    // 创建容器来组织内容
    apContent = lv_obj_create(apContainer);
    lv_obj_set_size(apContent, 280, 80);
    lv_obj_align(apContent, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(apContent, lv_color_black(), 0);
    lv_obj_set_style_border_width(apContent, 0, 0);
    lv_obj_set_style_pad_all(apContent, 0, 0);
    
    // 创建SSID信息
    lv_obj_t* ssidLabel = lv_label_create(apContent);
    lv_obj_set_style_text_font(ssidLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ssidLabel, lv_color_white(), 0);
    String ssidText = String("Network: ") + ssid;
    lv_label_set_text(ssidLabel, ssidText.c_str());
    lv_obj_align(ssidLabel, LV_ALIGN_TOP_MID, 0, 0);
    
    // 创建IP信息
    lv_obj_t* ipLabel = lv_label_create(apContent);
    lv_obj_set_style_text_font(ipLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ipLabel, lv_color_white(), 0);
    String ipText = String("Setup URL: ") + ip;
    lv_label_set_text(ipLabel, ipText.c_str());
    lv_obj_align(ipLabel, LV_ALIGN_TOP_MID, 0, 40);
}

void DisplayManager::deleteAPScreen() {
    if (apContainer != nullptr) {
        takeLvglLock();
        lv_obj_add_flag(apContainer, LV_OBJ_FLAG_HIDDEN);
        apScreenActive = false;
        giveLvglLock();
    }
}

bool DisplayManager::isAPScreenActive() {
    return apScreenActive;
}

bool DisplayManager::isWiFiErrorScreenActive() {
    return currentScreen == wifiErrorContainer;
}

bool DisplayManager::isTimeScreenActive() {
    return timeScreenActive;
}

void DisplayManager::updateTimeScreen() {
    if (!timeScreenActive || !timeLabel) return;
    
    // 获取当前时间
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    // 检查时间是否发生变化
    if (timeinfo.tm_hour == lastHour && 
        timeinfo.tm_min == lastMin && 
        timeinfo.tm_sec == lastSec) {
        return;  // 时间没有变化，不更新显示
    }
    
    // 更新记录的时间
    lastHour = timeinfo.tm_hour;
    lastMin = timeinfo.tm_min;
    lastSec = timeinfo.tm_sec;
    
    takeLvglLock();
    
    // 格式化时间字符串
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour,
             timeinfo.tm_min, 
             timeinfo.tm_sec);
    
    // 更新时间显示
    lv_label_set_text(timeLabel, timeStr);

    // 更新日期显示
    if (dateLabel != nullptr) {
        char dateStr[32];
        snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d",
                timeinfo.tm_year + 1900,
                timeinfo.tm_mon + 1,
                timeinfo.tm_mday);
        lv_label_set_text(dateLabel, dateStr);
    }
    
    giveLvglLock();
}

bool DisplayManager::createPowerMonitorContent() {
    // 先清除所有现有内容
    if (powerMonitorContainer != nullptr) {
        lv_obj_clean(powerMonitorContainer);
    }

    // 标题
    ui_title = lv_label_create(powerMonitorContainer);
    if (ui_title == nullptr) return false;
    
    lv_label_set_text(ui_title, "Power Monitor");
    lv_obj_set_style_text_color(ui_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_title, LV_ALIGN_TOP_MID, -20, -10);
    
    // WiFi状态
    ui_wifi_status = lv_label_create(powerMonitorContainer);
    if (ui_wifi_status == nullptr) return false;
    
    lv_label_set_text(ui_wifi_status, "WiFi");
    lv_obj_set_style_text_color(ui_wifi_status, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_wifi_status, LV_ALIGN_TOP_RIGHT, -10, -10);
    
    // 布局参数
    uint8_t start_y = 15;
    uint8_t item_height = 22;
    
    // 为每个端口创建UI元素
    for (int i = 0; i < MAX_PORTS; i++) {
        // 端口名称标签
        ui_port_labels[i] = lv_label_create(powerMonitorContainer);
        if (ui_port_labels[i] == nullptr) return false;
        
        lv_label_set_text_fmt(ui_port_labels[i], "%s:", portInfos[i].name);
        lv_obj_set_style_text_color(ui_port_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(ui_port_labels[i], LV_ALIGN_TOP_LEFT, -5, start_y + i * item_height);
        
        // 功率值标签
        ui_power_values[i] = lv_label_create(powerMonitorContainer);
        if (ui_power_values[i] == nullptr) return false;
        
        lv_label_set_text(ui_power_values[i], "0.00W");
        lv_obj_set_style_text_color(ui_power_values[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(ui_power_values[i], LV_ALIGN_TOP_LEFT, 30, start_y + i * item_height);
        
        // 功率进度条
        ui_power_bars[i] = lv_bar_create(powerMonitorContainer);
        if (ui_power_bars[i] == nullptr) return false;
        
        lv_obj_set_size(ui_power_bars[i], 200, 15);
        lv_obj_align(ui_power_bars[i], LV_ALIGN_TOP_RIGHT, -5, start_y + i * item_height);
        lv_bar_set_range(ui_power_bars[i], 0, 100);
        lv_bar_set_value(ui_power_bars[i], 0, LV_ANIM_OFF);
        
        // 设置进度条样式
        lv_obj_set_style_bg_color(ui_power_bars[i], lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_power_bars[i], lv_color_hex(0x88FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(ui_power_bars[i], LV_GRAD_DIR_HOR, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(ui_power_bars[i], lv_color_hex(0xFF8800), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    
    // 总功率标签
    ui_total_label = lv_label_create(powerMonitorContainer);
    if (ui_total_label == nullptr) return false;
    
    lv_label_set_text(ui_total_label, "Total: 0W");
    lv_obj_set_style_text_color(ui_total_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_total_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_total_label, LV_ALIGN_TOP_LEFT, -5, start_y + MAX_PORTS * item_height + 5);
    
    // 总功率进度条
    ui_total_bar = lv_bar_create(powerMonitorContainer);
    if (ui_total_bar == nullptr) return false;
    
    lv_obj_set_size(ui_total_bar, 200, 15);
    lv_obj_align(ui_total_bar, LV_ALIGN_TOP_RIGHT, -5, start_y + MAX_PORTS * item_height + 5);
    lv_bar_set_range(ui_total_bar, 0, 100);
    lv_bar_set_value(ui_total_bar, 0, LV_ANIM_OFF);
    
    // 设置总功率进度条背景色
    lv_obj_set_style_bg_color(ui_total_bar, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 设置进度条指示器颜色为绿黄色
    lv_obj_set_style_bg_color(ui_total_bar, lv_color_hex(0xf039fb), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
    // 启用水平渐变
    lv_obj_set_style_bg_grad_dir(ui_total_bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
    // 设置渐变终止颜色为红黄色
    lv_obj_set_style_bg_grad_color(ui_total_bar, lv_color_hex(0xfb3a39), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
    return true;
}

bool DisplayManager::isPowerMonitorScreenActive() {
    return powerMonitorScreenActive;
}

void DisplayManager::setScreenBrightness(uint8_t brightness) {
    Set_Backlight(brightness);
    printf("[Display] Brightness set to %d\n", brightness);
}

bool DisplayManager::isScanScreenActive() {
    return scanScreenActive;
}

void DisplayManager::updateScanStatus(const char* status) {
    if (scanStatus != nullptr) {
        takeLvglLock();
        lv_label_set_text(scanStatus, status);
        giveLvglLock();
    }
}

void DisplayManager::updatePowerMonitorScreen() {
    if (!powerMonitorScreenActive || powerMonitorContainer == nullptr) {
        return;
    }

    takeLvglLock();

    static char text_buf[128];
    
    // 更新每个端口的显示
    for (int i = 0; i < MAX_PORTS; i++) {
        if (ui_power_values[i] == nullptr) continue;

        // 启用标签的重着色功能
        lv_label_set_recolor(ui_power_values[i], true);

        if (dataError) {
            lv_label_set_text(ui_power_values[i], "#888888 --.-W#");
            if (ui_power_bars[i] != nullptr) {
                lv_bar_set_value(ui_power_bars[i], 0, LV_ANIM_OFF);
            }
            continue;
        }

        // 根据电压设置颜色
        const char* color_code;
        int voltage_mv = portInfos[i].voltage;
        
        if (voltage_mv > 21000) {
            color_code = "#FF00FF";
        } else if (voltage_mv > 16000) {
            color_code = "#FF0000";
        } else if (voltage_mv > 13000) {
            color_code = "#FF8800";
        } else if (voltage_mv > 10000) {
            color_code = "#FFFF00";
        } else if (voltage_mv > 6000) {
            color_code = "#00FF00";
        } else if (voltage_mv >= 0) {
            color_code = "#FFFFFF";
        } else {
            color_code = "#888888";
        }

        // 更新功率值
        int power_int = (int)(portInfos[i].power * 100);
        // 增加缓冲区大小检查，确保有足够空间
        int needed_size = strlen(color_code) + 20; // 颜色代码 + 数字 + 格式字符
        if (needed_size < sizeof(text_buf)) {
            snprintf(text_buf, sizeof(text_buf), "%s %d.%02dW#", 
                    color_code, 
                    power_int / 100, 
                    power_int % 100);
        } else {
            // 如果缓冲区不够，使用简化格式
            snprintf(text_buf, sizeof(text_buf), "#FFFFFF %d.%02dW#", 
                    power_int / 100, 
                    power_int % 100);
        }
        lv_label_set_text(ui_power_values[i], text_buf);

        // 更新进度条
        if (ui_power_bars[i] != nullptr) {
            int percent = (int)((portInfos[i].power / MAX_PORT_WATTS) * 100);
            if (portInfos[i].power > 0 && percent == 0) percent = 1;
            lv_bar_set_value(ui_power_bars[i], percent, LV_ANIM_ON);
        }
    }

    // 更新总功率显示
    if (ui_total_label != nullptr) {
        lv_label_set_recolor(ui_total_label, true);
        float totalPower = PowerMonitor_GetTotalPower();
        
        if (dataError) {
            lv_label_set_text(ui_total_label, "Total: #888888 --.-W#");
            if (ui_total_bar != nullptr) {
                lv_bar_set_value(ui_total_bar, 0, LV_ANIM_ON);
            }
        } else {
            int total_power_int = (int)(totalPower * 100);
            snprintf(text_buf, sizeof(text_buf), 
                    "Total: #FFFFFF %d.%02dW#",
                    total_power_int / 100,
                    total_power_int % 100);
            lv_label_set_text(ui_total_label, text_buf);

            if (ui_total_bar != nullptr) {
                // 计算百分比
                int totalPercent = (int)((totalPower / MAX_POWER_WATTS) * 100);
                if (totalPower > 0 && totalPercent == 0) totalPercent = 1;
                if (totalPercent > 100) totalPercent = 100;
                 // 设置进度条值
                lv_bar_set_value(ui_total_bar, totalPercent, LV_ANIM_ON);
            }
        }
    }

    giveLvglLock();
}

void DisplayManager::takeLvglLock() {
    if (lvgl_mutex == nullptr) {
        printf("[Display] Error: LVGL mutex not initialized, recreating...\n");
        // 如果互斥锁未初始化，尝试重新创建
        lvgl_mutex = xSemaphoreCreateMutex();
        if (lvgl_mutex == nullptr) {
            printf("[Display] Fatal: Failed to create LVGL mutex\n");
            return;
        }
        printf("[Display] LVGL mutex recreated successfully\n");
    }
    
    // 检查当前任务是否已经持有锁（可选的调试信息）
    #ifdef DEBUG_DISPLAY_LOCKS
    TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
    printf("[Display] Task %p acquiring LVGL lock\n", currentTask);
    #endif
    
    xSemaphoreTake(lvgl_mutex, portMAX_DELAY);
    
    #ifdef DEBUG_DISPLAY_LOCKS
    printf("[Display] Task %p acquired LVGL lock\n", currentTask);
    #endif
}

void DisplayManager::giveLvglLock() {
    if (lvgl_mutex != nullptr) {
        #ifdef DEBUG_DISPLAY_LOCKS
        TaskHandle_t currentTask = xTaskGetCurrentTaskHandle();
        printf("[Display] Task %p releasing LVGL lock\n", currentTask);
        #endif
        
        xSemaphoreGive(lvgl_mutex);
        
        #ifdef DEBUG_DISPLAY_LOCKS
        printf("[Display] Task %p released LVGL lock\n", currentTask);
        #endif
    } else {
        printf("[Display] Warning: Attempting to give uninitialized mutex\n");
    }
}

void DisplayManager::handleLvglTask() {
    takeLvglLock();
    lv_timer_handler();
    giveLvglLock();
}

bool DisplayManager::isValidScreenState() {
    // 检查是否有多个屏幕同时处于活动状态
    int activeScreenCount = 0;
    if (apScreenActive) activeScreenCount++;
    if (wifiErrorScreenActive) activeScreenCount++;
    if (timeScreenActive) activeScreenCount++;
    if (powerMonitorScreenActive) activeScreenCount++;
    if (scanScreenActive) activeScreenCount++;
    
    if (activeScreenCount > 1) {
        printf("[Display] Warning: Multiple screens active simultaneously (%d)\n", activeScreenCount);
        return false;
    }
    
    return true;
}

void DisplayManager::resetAllScreenStates() {
    printf("[Display] Emergency: Resetting all screen states\n");
    
    takeLvglLock();
    
    // 强制重置所有状态标志
    apScreenActive = false;
    wifiErrorScreenActive = false;
    timeScreenActive = false;
    powerMonitorScreenActive = false;
    scanScreenActive = false;
    
    // 隐藏所有容器
    if (apContainer) lv_obj_add_flag(apContainer, LV_OBJ_FLAG_HIDDEN);
    if (wifiErrorContainer) lv_obj_add_flag(wifiErrorContainer, LV_OBJ_FLAG_HIDDEN);
    if (timeContainer) lv_obj_add_flag(timeContainer, LV_OBJ_FLAG_HIDDEN);
    if (powerMonitorContainer) lv_obj_add_flag(powerMonitorContainer, LV_OBJ_FLAG_HIDDEN);
    if (scanContainer) lv_obj_add_flag(scanContainer, LV_OBJ_FLAG_HIDDEN);
    
    giveLvglLock();
    
    printf("[Display] All screen states reset\n");
} 