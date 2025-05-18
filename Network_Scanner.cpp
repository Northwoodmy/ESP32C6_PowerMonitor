#include "Network_Scanner.h"

bool NetworkScanner::findMetricsServer(String& outUrl, bool printLog) {
    if (WiFi.status() != WL_CONNECTED) {
        if(printLog) printf("[Scanner] WiFi not connected\n");
        return false;
    }

    // 获取本机IP地址
    IPAddress localIP = WiFi.localIP();
    String baseIP = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";
    int localLastOctet = localIP[3];
    
    if(printLog) printf("[Scanner] Starting network scan from %s\n", baseIP.c_str());
    
    // 先扫描本机IP附近的范围（前后10个IP）
    int startNearby = max(1, localLastOctet - 10);
    int endNearby = min(254, localLastOctet + 10);
    
    // 扫描附近IP
    for(int i = startNearby; i <= endNearby; i++) {
        if(i == localLastOctet) continue; // 跳过本机IP
        
        String testIP = baseIP + String(i);
        if(testMetricsEndpoint(testIP, printLog)) {
            outUrl = testIP;
            if(printLog) printf("[Scanner] Found metrics server at IP: %s\n", outUrl.c_str());
            return true;
        }
    }
    
    // 如果附近没找到，扫描剩余的IP范围
    for(int i = 1; i <= 254; i++) {
        // 跳过已经扫描过的范围
        if(i >= startNearby && i <= endNearby) continue;
        
        String testIP = baseIP + String(i);
        if(testMetricsEndpoint(testIP, printLog)) {
            outUrl = testIP;
            if(printLog) printf("[Scanner] Found metrics server at IP: %s\n", outUrl.c_str());
            return true;
        }
    }
    
    if(printLog) printf("[Scanner] No metrics server found\n");
    return false;
}

bool NetworkScanner::testMetricsEndpoint(const String& ip, bool printLog) {
    if(printLog) printf("[Scanner] Testing IP: %s\n", ip.c_str());
    
    HTTPClient http;
    String url = "http://" + ip + "/metrics";
    
    http.begin(url);
    http.setTimeout(200); // 设置较短的超时时间，加快扫描速度
    
    int httpCode = http.GET();
    bool success = (httpCode == HTTP_CODE_OK);
    
    if(printLog) {
        if(success) {
            printf("[Scanner] Success: %s responded with metrics data\n", ip.c_str());
        } else {
            printf("[Scanner] Failed: %s (HTTP code: %d)\n", ip.c_str(), httpCode);
        }
    }
    
    http.end();
    return success;
} 