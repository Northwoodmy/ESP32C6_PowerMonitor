#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>

class NetworkScanner {
public:
    // 扫描网络并返回找到的第一个提供metrics的设备URL
    static bool findMetricsServer(String& outUrl, bool printLog = false);
    
private:
    static bool testMetricsEndpoint(const String& ip, bool printLog);
}; 