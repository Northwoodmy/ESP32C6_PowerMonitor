#include "Display_Manager.h"
#include <time.h>

// 静态成员初始化
lv_obj_t* DisplayManager::apScreen = nullptr;
lv_obj_t* DisplayManager::currentScreen = nullptr;
lv_obj_t* DisplayManager::wifiErrorScreen = nullptr;
lv_obj_t* DisplayManager::powerMonitorScreen = nullptr;

// UI组件 - 电源监控
lv_obj_t* DisplayManager::ui_title = nullptr;
lv_obj_t* DisplayManager::ui_total_label = nullptr;
lv_obj_t* DisplayManager::ui_port_labels[MAX_PORTS] = {nullptr};
lv_obj_t* DisplayManager::ui_power_values[MAX_PORTS] = {nullptr};
lv_obj_t* DisplayManager::ui_power_bars[MAX_PORTS] = {nullptr};
lv_obj_t* DisplayManager::ui_total_bar = nullptr;
lv_obj_t* DisplayManager::ui_wifi_status = nullptr;

// 状态标志初始化
bool DisplayManager::apScreenActive = false;
bool DisplayManager::wifiErrorScreenActive = false;
bool DisplayManager::timeScreenActive = false;
bool DisplayManager::powerMonitorScreenActive = false;
bool DisplayManager::dataError = false;
lv_obj_t* DisplayManager::timeLabel = nullptr;
unsigned long DisplayManager::screenSwitchTime = 0;

void DisplayManager::init() {

}

void DisplayManager::createAPScreen(const char* ssid, const char* ip) {
    if (apScreen != nullptr) {
        deleteAPScreen();
    }
    
    // 创建AP配置屏幕
    apScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(apScreen, lv_color_black(), 0);
    
    // 创建屏幕内容
    createAPScreenContent(ssid, ip);
    
    // 切换到AP屏幕
    currentScreen = apScreen;
    lv_scr_load(apScreen);
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
}

void DisplayManager::createAPScreenContent(const char* ssid, const char* ip) {
    // 创建标题
    lv_obj_t* title = lv_label_create(apScreen);
    lv_label_set_text(title, "WiFi Setup");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 20);  // 顶部居中
    lv_obj_set_style_text_color(title, lv_color_white(), 0);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    
    // 创建容器来组织内容
    lv_obj_t* cont = lv_obj_create(apScreen);
    lv_obj_set_size(cont, 280, 80);  // 保持宽度足够显示内容
    lv_obj_align(cont, LV_ALIGN_TOP_MID, 0, 60);  // 居中对齐
    lv_obj_set_style_bg_color(cont, lv_color_black(), 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    
    // 创建SSID信息
    lv_obj_t* ssidLabel = lv_label_create(cont);
    lv_obj_set_style_text_font(ssidLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ssidLabel, lv_color_white(), 0);
    String ssidText = String("Network: ") + ssid;
    lv_label_set_text(ssidLabel, ssidText.c_str());
    lv_obj_align(ssidLabel, LV_ALIGN_TOP_MID, 0, 0);  // 容器内顶部居中
    
    // 创建IP信息
    lv_obj_t* ipLabel = lv_label_create(cont);
    lv_obj_set_style_text_font(ipLabel, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(ipLabel, lv_color_white(), 0);
    String ipText = String("Setup URL: ") + ip;
    lv_label_set_text(ipLabel, ipText.c_str());
    lv_obj_align(ipLabel, LV_ALIGN_TOP_MID, 0, 40);  // 容器内居中，距顶部40px
}

void DisplayManager::deleteAPScreen() {
    if (apScreen != nullptr) {
        lv_obj_del(apScreen);
        apScreen = nullptr;
    }
}

bool DisplayManager::isAPScreenActive() {
    return currentScreen == apScreen;
}

void DisplayManager::createWiFiErrorScreen() {
    if (wifiErrorScreen != nullptr) {
        deleteWiFiErrorScreen();
    }
    
    // 创建WiFi错误屏幕
    wifiErrorScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifiErrorScreen, lv_color_black(), 0);
    
    // 创建错误标题
    lv_obj_t* title = lv_label_create(wifiErrorScreen);
    lv_label_set_text(title, "WiFi Connection Failed");
    lv_obj_set_style_text_color(title, lv_color_make(0xFF, 0x00, 0x00), 0);  // 红色
    lv_obj_set_style_text_font(title, &lv_font_montserrat_24, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 30);
    
    // 创建提示信息
    lv_obj_t* message = lv_label_create(wifiErrorScreen);
    lv_label_set_text(message, "Please check your WiFi settings\nRetrying connection...");
    lv_obj_set_style_text_color(message, lv_color_white(), 0);
    lv_obj_set_style_text_font(message, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(message, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(message, LV_ALIGN_CENTER, 0, 0);
    
    // 切换到错误屏幕
    currentScreen = wifiErrorScreen;
    lv_scr_load(wifiErrorScreen);
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
}

void DisplayManager::deleteWiFiErrorScreen() {
    if (wifiErrorScreen != nullptr) {
        lv_obj_del(wifiErrorScreen);
        wifiErrorScreen = nullptr;
    }
}

bool DisplayManager::isWiFiErrorScreenActive() {
    return currentScreen == wifiErrorScreen;
}

void DisplayManager::createTimeScreen() {
    printf("[Display] Creating time screen\n");
    
    // 先删除其他屏幕
    if (powerMonitorScreenActive) {
        deletePowerMonitorScreen();
    }
    
    if (timeScreenActive) {
        printf("[Display] Time screen already active\n");
        return;
    }
    
    // 创建时间显示屏幕
    lv_obj_t* screen = lv_obj_create(NULL);
    if (screen == nullptr) {
        printf("[Display] Failed to create time screen\n");
        return;
    }
    
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    // 创建时间标签
    timeLabel = lv_label_create(screen);
    if (timeLabel == nullptr) {
        printf("[Display] Failed to create time label\n");
        lv_obj_del(screen);
        return;
    }
    
    lv_obj_set_style_text_color(timeLabel, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    lv_obj_set_style_text_font(timeLabel, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_align(timeLabel, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN);
    lv_obj_set_width(timeLabel, LV_PCT(100));
    lv_obj_align(timeLabel, LV_ALIGN_CENTER, 0, 0);
    
    // 更新时间显示
    updateTimeScreen();
    
    // 加载屏幕
    lv_scr_load(screen);
    currentScreen = screen;
    timeScreenActive = true;
    screenSwitchTime = millis();
    
    // 设置为低亮度
    setScreenBrightness(BRIGHTNESS_DIM);
    
    printf("[Display] Time screen created successfully\n");
}

void DisplayManager::deleteTimeScreen() {
    printf("[Display] Deleting time screen\n");
    
    if (timeScreenActive && timeLabel != nullptr) {
        lv_obj_t* screen = lv_obj_get_parent(timeLabel);
        if (screen != nullptr) {
            lv_obj_clean(screen);
            lv_obj_del(screen);
        }
        timeLabel = nullptr;
        timeScreenActive = false;
    }
    
    printf("[Display] Time screen deleted\n");
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
    
    // 格式化时间字符串，使用更大的间距
    char timeStr[32];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour,
             timeinfo.tm_min, 
             timeinfo.tm_sec);
    
    // 更新显示
    lv_label_set_text(timeLabel, timeStr);
}

// 创建电源监控屏幕
void DisplayManager::createPowerMonitorScreen() {
    printf("[Display] Creating power monitor screen\n");
    
    // 先删除其他屏幕
    if (timeScreenActive) {
        deleteTimeScreen();
    }
    
    if (powerMonitorScreen != nullptr) {
        deletePowerMonitorScreen();
    }
    
    // 创建屏幕
    powerMonitorScreen = lv_obj_create(NULL);
    if (powerMonitorScreen == nullptr) {
        printf("[Display] Failed to create power monitor screen\n");
        return;
    }
    
    lv_obj_set_style_bg_color(powerMonitorScreen, lv_color_hex(0x000000), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // 创建内容
    if (!createPowerMonitorContent()) {
        printf("[Display] Failed to create power monitor content\n");
        lv_obj_del(powerMonitorScreen);
        powerMonitorScreen = nullptr;
        return;
    }
    
    // 切换到电源监控屏幕
    lv_scr_load(powerMonitorScreen);
    currentScreen = powerMonitorScreen;
    powerMonitorScreenActive = true;
    
    // 设置为正常亮度
    setScreenBrightness(BRIGHTNESS_NORMAL);
    
    printf("[Display] Power monitor screen created successfully\n");
}

bool DisplayManager::createPowerMonitorContent() {
    // 标题
    ui_title = lv_label_create(powerMonitorScreen);
    if (ui_title == nullptr) return false;
    
    lv_label_set_text(ui_title, "Power Monitor");
    lv_obj_set_style_text_color(ui_title, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_title, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_title, LV_ALIGN_TOP_MID, 0, 5);
    
    // WiFi状态
    ui_wifi_status = lv_label_create(powerMonitorScreen);
    if (ui_wifi_status == nullptr) return false;
    
    lv_label_set_text(ui_wifi_status, "WiFi");
    lv_obj_set_style_text_color(ui_wifi_status, lv_color_hex(0x00FF00), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_wifi_status, LV_ALIGN_TOP_RIGHT, -10, 5);
    
    // 布局参数
    uint8_t start_y = 30;
    uint8_t item_height = 22;
    
    // 为每个端口创建UI元素
    for (int i = 0; i < MAX_PORTS; i++) {
        // 端口名称标签
        ui_port_labels[i] = lv_label_create(powerMonitorScreen);
        if (ui_port_labels[i] == nullptr) return false;
        
        lv_label_set_text_fmt(ui_port_labels[i], "%s:", portInfos[i].name);
        lv_obj_set_style_text_color(ui_port_labels[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(ui_port_labels[i], LV_ALIGN_TOP_LEFT, 10, start_y + i * item_height);
        
        // 功率值标签
        ui_power_values[i] = lv_label_create(powerMonitorScreen);
        if (ui_power_values[i] == nullptr) return false;
        
        lv_label_set_text(ui_power_values[i], "0.00W");
        lv_obj_set_style_text_color(ui_power_values[i], lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_align(ui_power_values[i], LV_ALIGN_TOP_LEFT, 45, start_y + i * item_height);
        
        // 功率进度条
        ui_power_bars[i] = lv_bar_create(powerMonitorScreen);
        if (ui_power_bars[i] == nullptr) return false;
        
        lv_obj_set_size(ui_power_bars[i], 200, 15);
        lv_obj_align(ui_power_bars[i], LV_ALIGN_TOP_RIGHT, -10, start_y + i * item_height);
        lv_bar_set_range(ui_power_bars[i], 0, 100);
        lv_bar_set_value(ui_power_bars[i], 0, LV_ANIM_OFF);
        
        // 设置进度条样式
        lv_obj_set_style_bg_color(ui_power_bars[i], lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(ui_power_bars[i], lv_color_hex(0x88FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_dir(ui_power_bars[i], LV_GRAD_DIR_HOR, LV_PART_INDICATOR | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_grad_color(ui_power_bars[i], lv_color_hex(0xFF8800), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    }
    
    // 总功率标签
    ui_total_label = lv_label_create(powerMonitorScreen);
    if (ui_total_label == nullptr) return false;
    
    lv_label_set_text(ui_total_label, "Total: 0W");
    lv_obj_set_style_text_color(ui_total_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_total_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_align(ui_total_label, LV_ALIGN_TOP_LEFT, 10, start_y + MAX_PORTS * item_height + 5);
    
    // 总功率进度条
    ui_total_bar = lv_bar_create(powerMonitorScreen);
    if (ui_total_bar == nullptr) return false;
    
    lv_obj_set_size(ui_total_bar, 200, 15);
    lv_obj_align(ui_total_bar, LV_ALIGN_TOP_RIGHT, -10, start_y + MAX_PORTS * item_height + 5);
    lv_bar_set_range(ui_total_bar, 0, 100);
    lv_bar_set_value(ui_total_bar, 0, LV_ANIM_OFF);
    
    // 设置总功率进度条样式
    lv_obj_set_style_bg_color(ui_total_bar, lv_color_hex(0x444444), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_total_bar, lv_color_hex(0x88FF00), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_dir(ui_total_bar, LV_GRAD_DIR_HOR, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_grad_color(ui_total_bar, lv_color_hex(0xFF8800), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    
    return true;
}

void DisplayManager::deletePowerMonitorScreen() {
    printf("[Display] Deleting power monitor screen\n");
    
    if (powerMonitorScreen != nullptr) {
        // 删除所有子对象
        lv_obj_clean(powerMonitorScreen);
        
        // 删除屏幕对象
        lv_obj_del(powerMonitorScreen);
        powerMonitorScreen = nullptr;
        
        // 重置所有UI组件指针
        ui_title = nullptr;
        ui_total_label = nullptr;
        ui_total_bar = nullptr;
        ui_wifi_status = nullptr;
        
        for (int i = 0; i < MAX_PORTS; i++) {
            ui_port_labels[i] = nullptr;
            ui_power_values[i] = nullptr;
            ui_power_bars[i] = nullptr;
        }
        
        powerMonitorScreenActive = false;
    }
    
    printf("[Display] Power monitor screen deleted\n");
}

// 更新电源监控屏幕
void DisplayManager::updatePowerMonitorScreen() {
    if (!powerMonitorScreenActive || powerMonitorScreen == nullptr) {
        return;
    }

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
        snprintf(text_buf, sizeof(text_buf), "%s %d.%02dW#", 
                color_code, 
                power_int / 100, 
                power_int % 100);
        lv_label_set_text(ui_power_values[i], text_buf);

        // 更新进度条
        if (ui_power_bars[i] != nullptr) {
            int percent = (int)((portInfos[i].power / MAX_PORT_WATTS) * 100);
            if (portInfos[i].power > 0 && percent == 0) percent = 1;
            lv_bar_set_value(ui_power_bars[i], percent, LV_ANIM_OFF);
        }
    }

    // 更新总功率显示
    if (ui_total_label != nullptr) {
        lv_label_set_recolor(ui_total_label, true);
        float totalPower = PowerMonitor_GetTotalPower();
        
        if (dataError) {
            lv_label_set_text(ui_total_label, "Total: #888888 --.-W#");
            if (ui_total_bar != nullptr) {
                lv_bar_set_value(ui_total_bar, 0, LV_ANIM_OFF);
            }
        } else {
            int total_power_int = (int)(totalPower * 100);
            snprintf(text_buf, sizeof(text_buf), 
                    "Total: #FFFFFF %d.%02dW#",
                    total_power_int / 100,
                    total_power_int % 100);
            lv_label_set_text(ui_total_label, text_buf);

            if (ui_total_bar != nullptr) {
                int totalPercent = (int)((totalPower / MAX_POWER_WATTS) * 100);
                if (totalPower > 0 && totalPercent == 0) totalPercent = 1;
                lv_bar_set_value(ui_total_bar, totalPercent, LV_ANIM_OFF);
            }
        }
    }
}

bool DisplayManager::isPowerMonitorScreenActive() {
    return powerMonitorScreenActive;
}

void DisplayManager::setScreenBrightness(uint8_t brightness) {
    Set_Backlight(brightness);
    printf("[Display] Brightness set to %d\n", brightness);
} 