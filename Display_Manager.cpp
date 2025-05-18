#include "Display_Manager.h"

lv_obj_t* DisplayManager::apScreen = nullptr;
lv_obj_t* DisplayManager::currentScreen = nullptr;
lv_obj_t* DisplayManager::wifiErrorScreen = nullptr;

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