/**
 * @file menu.h
 * @brief 六面顶菜单系统头文件 - 碳转石墨过程可视化
 */
#ifndef __MENU_H__
#define __MENU_H__

#include "main.h"
#include "pressure.h"
#include "temperature.h"
#include "motor.h"
#include "oled.h"
#include "key.h"
#include "led.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdio.h>

/* 常量定义 */
#define REFRESH_RATE_MS    100     // 刷新率(ms)
#define MAX_PARTICLES      20      // 最大粒子数
#define WARNING_BLINK_MS   300     // 警告闪烁周期
#define ANIMATION_DURATION_MS 280  // 动画持续时间
#define SCREEN_WIDTH       128     // OLED屏幕宽度
#define SCREEN_HEIGHT      64      // OLED屏幕高度

/**
 * @brief 工艺状态枚举
 */
typedef enum {
    PROCESS_IDLE,       // 待机
    PROCESS_HEATING,    // 加热阶段
    PROCESS_PRESSING,   // 加压阶段
    PROCESS_TRANSFORM,  // 转化阶段
    PROCESS_COOLING,    // 冷却阶段
    PROCESS_ERROR       // 错误状态
} ProcessState;

/**
 * @brief 警报级别枚举
 */
typedef enum {
    ALERT_NONE,         // 无警报
    ALERT_WARNING,      // 警告
    ALERT_CRITICAL      // 危险
} AlertLevel;

/**
 * @brief 显示模式枚举
 */
typedef enum {
    DISPLAY_MODE_NORMAL,    // 正常模式
    DISPLAY_MODE_NIGHT,     // 夜间模式
    DISPLAY_MODE_DEMO       // 演示模式
} DisplayMode;

/**
 * @brief 粒子效果数据
 */
typedef struct {
    int8_t x, y;
    int8_t dx, dy;
    uint8_t active;
} Particle;

/**
 * @brief 3D模型顶点结构
 */
typedef struct {
    int8_t x, y, z;        // 3D坐标
    uint8_t transformed;    // 转化状态
} Vertex;

/**
 * @brief 系统参数结构体
 */
typedef struct {
    float pressure;         // 当前压力值
    float temperature;      // 当前温度值
    AlertLevel alert;       // 警报级别
    uint16_t alert_count;   // 警报计数
    ProcessState state;     // 工艺状态
    DisplayMode mode;       // 显示模式
    uint8_t is_calibrated;  // 校准状态
    uint8_t motor_status;   // 电机状态
} SystemParams;

/**
 * @brief 六面顶压力数据结构
 */
typedef struct {
    float pressure[6];      // 六个面的压力值
    float temperature;      // 温度值
    float progress;         // 转化进度(0-100%)
    uint32_t runtime;       // 运行时间(秒)
    ProcessState state;     // 工艺状态
    DisplayMode mode;       // 显示模式
} ProcessParams;

/* 菜单项结构体 */
typedef struct {
    const char *text;      // 菜单项文本
    int (*handler)(void);  // 菜单项处理函数
} MenuItem;

/* 函数声明 */

/* ===== 菜单系统入口函数 ===== */
int menu1(void);             // 系统主菜单入口，提供三个主选项
int menu_main(void);         // 向后兼容的主菜单入口点，调用menu1()

/* ===== 主菜单页面 ===== */
int menu_process(void);      // 工艺过程监控界面，显示参数和3D模型
int menu2(void);             // 参数设置菜单，选择设置参数类型
int menu_settings(void);     // 系统设置菜单，提供显示/声音等设置
int menu_history(void);      // 历史记录浏览菜单，查看过去工艺记录

/* ===== 二级菜单页面 ===== */
int menu3_PressureSet(void);    // 压力参数设置界面
int menu3_TemperatureSet(void); // 温度参数设置界面
int menu3_TimeSet(void);        // 时间参数设置界面
int menu3_ProcessSelect(void);  // 工艺选择界面，选择预设工艺方案
int menu3_DataRecord(void);     // 数据记录界面，查看和导出数据
int menu3_SystemConfig(void);   // 系统配置界面，传感器校准和系统设置
int menu_motor_status(void);    // 电机状态监控界面，三轴电机参数控制

/* ===== 三级菜单页面 ===== */
int menu4_Process1(void);       // 工艺1参数设置
int menu4_Process2(void);       // 工艺2参数设置
int menu4_Process3(void);       // 工艺3参数设置
int menu4_CalibrationP(void);   // 压力传感器校准
int menu4_CalibrationT(void);   // 温度传感器校准
int menu4_AlarmConfig(void);    // 警报配置，设置警报阈值
int menu4_SystemInfo(void);     // 系统信息，显示版本和硬件信息

/* ===== 工艺控制函数 ===== */
int process_start(void);      // 启动工艺过程
int process_stop(void);       // 停止工艺过程
int process_pause(void);      // 暂停工艺过程

#endif /* __MENU_H__ */
