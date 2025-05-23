# CP02电源监控项目

## 项目概述
这是一个基于ESP32-C6开发的电源监控系统，主要用于实时监控和显示多个USB-C端口的供电状态。项目集成了WiFi配置、电源监控、RGB指示灯和LCD显示等功能模块。

## 硬件组成
1. 主控制器：ESP32-C6（单核处理器）
2. 显示屏：ST7789驱动的LCD屏幕
3. RGB指示灯
4. USB-C端口：5个监控端口（A1, C1, C2, C3, C4）

## 软件架构

### 1. 配置管理模块（Config_Manager）
- 负责WiFi网络配置
- 提供Web配置界面
- 管理系统参数存储
- 特点：
  - 支持AP模式配置
  - 配置数据持久化存储
  - 支持配置重置功能
  - WiFi连接状态管理

### 2. 显示管理模块（Display_Manager）
- 管理不同显示界面的切换
- 包含三个主要界面：
  - AP配置界面
  - 监控界面
  - WiFi错误界面
- 使用LVGL图形库实现UI

### 3. 电源监控模块（Power_Monitor）
- 实时监控各端口电源状态
- 功能特点：
  - 支持5个端口独立监控
  - 显示电压、电流、功率数据
  - 总功率计算和显示
  - 进度条显示功率使用情况
  - 异常状态提示

### 4. RGB指示灯模块（RGB_Lamp）
- 提供视觉反馈
- 可通过配置界面开启/关闭
- 支持动态效果

## 主要功能

### 1. WiFi配置功能
- 首次使用自动进入AP模式
- Web配置界面（http://设备IP）
- 支持：
  - WiFi网络设置
  - 监控服务器配置
  - RGB灯控制
  - 系统重置

### 2. 电源监控功能
- 实时监控参数：
  - 各端口电压
  - 各端口电流
  - 各端口功率
  - 总功率
- 安全限制：
  - 单端口最大功率：140W
  - 总功率上限：160W

### 3. 显示功能
- 实时数据显示
- 进度条可视化
- WiFi状态指示
- 异常状态提示

## 工作流程

### 1. 系统启动
- 初始化显示
- 检查配置状态
- 启动必要服务

### 2. 正常工作模式
- 连接WiFi网络
- 获取电源数据
- 更新显示界面
- 控制RGB指示

### 3. 配置模式
- 启动AP热点
- 提供Web配置界面
- 保存新配置
- 重启进入工作模式

## 异常处理

### 1. WiFi断开处理
- 显示错误界面
- 自动重连
- 状态指示

### 2. 数据异常处理
- 显示错误提示
- 继续尝试获取数据
- 保持界面响应

## 优化特点

### 1. 单任务设计
- 适应ESP32-C6单核特性
- 优化资源利用
- 提高系统稳定性

### 2. 界面优化
- 紧凑布局设计
- 直观的数据展示
- 清晰的状态反馈

### 3. 稳定性保障
- 看门狗保护
- 异常自动恢复
- 配置持久化

## 注意事项

### 1. 配置相关
- 首次使用需要配置WiFi
- 记录配置参数
- 可随时重置配置

### 2. 使用限制
- 注意功率限制
- 确保WiFi信号稳定
- 定期检查连接状态

### 3. 维护建议
- 定期检查系统状态
- 保持固件更新
- 注意环境温度

## 开发环境
- 开发平台：Arduino IDE
- 主要依赖：
  - LVGL图形库
  - ESP32 Arduino核心库
  - WiFi库
  - WebServer库
  - DNSServer库
  - Preferences库

## 许可证
本项目采用MIT许可证。详细信息请参见LICENSE文件。

## 贡献
欢迎提交问题报告和改进建议。如果您想为项目做出贡献，请遵循以下步骤：
1. Fork本仓库
2. 创建您的特性分支
3. 提交您的改动
4. 推送到您的分支
5. 创建Pull Request

## 更新日志
### V1.0.02 (2025-05-19)
#### UI系统稳定性更新
- 电源监控模块改进
  - 优化UI更新机制，提高显示稳定性
  - 改进电压区间判断逻辑，更精确的颜色显示
  - 增强数据错误状态处理
  - 添加UI组件空指针检查
  
- 内存管理优化
  - 使用静态缓冲区替代栈变量
  - 优化字符串处理逻辑
  - 增加缓冲区清理机制
  
- 显示安全性提升
  - 完善UI组件有效性验证
  - 改进错误状态显示逻辑
  - 优化电压颜色映射系统
  
- 代码质量改进
  - 增加详细的代码注释
  - 优化条件判断结构
  - 提高代码可维护性

### V1.0.00 (2025-05-18)
#### 初始版本发布
- 核心功能
  - 实现多端口USB-C电源监控系统
  - 支持5个独立监控端口（A1, C1, C2, C3, C4）
  - 实时显示电压、电流、功率数据
  - 总功率计算和显示功能
  
- 网络功能
  - WiFi配置管理系统
  - Web配置界面
  - AP模式自动配置
  - 设备自动发现功能
  
- 显示功能
  - 集成ST7789 LCD显示屏
  - LVGL图形库界面
  - 多界面切换系统
  - 实时数据可视化
  
- 指示系统
  - RGB指示灯控制
  - 状态可视化反馈
  - 可配置的灯光效果
  
- 安全特性
  - 单端口功率限制（140W）
  - 系统总功率保护（160W）
  - 异常状态监测和提示
  
- 系统优化
  - 单核处理器优化设计
  - 稳定性保障机制
  - 配置数据持久化
  - 自动故障恢复功能 