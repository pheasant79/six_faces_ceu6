/**
 * @file menu.h
 * @brief 菜单系统头文件 - 定义菜单结构
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

/* 系统参数定义 */
#define REFRESH_RATE_MS    100     // 刷新率(ms)
#define WARNING_BLINK_MS   300     // 警告闪烁间隔时间
#define ANIMATION_DURATION_MS 280  // 动画持续时间毫秒
#define SCREEN_WIDTH       128     // OLED屏幕宽度像素
#define SCREEN_HEIGHT      64      // OLED屏幕高度

/* 工艺阶段定义 - 压力机模拟使用 */
typedef enum {
    STAGE_READY,     // 准备阶段
    STAGE_CALIBRATION, // 校准阶段
    STAGE_PRESS_FAST, // 加压阶段（已合并一次加压和二次加压）
    STAGE_HEAT,      // 加热阶段
    STAGE_HOLD,      // 保温保压阶段
    STAGE_COOL,      // 冷却阶段
    STAGE_RELEASE,   // 卸压阶段
    STAGE_COMPLETE   // 完成阶段
} ProcessStage;

/* 外部声明 - 用于中断处理 */
extern ProcessStage current_stage;     // 当前工艺阶段

/**
 * @brief 系统处理状态枚举
 */
typedef enum {
    PROCESS_IDLE,       // 空闲状态
    PROCESS_HEATING,    // 加热过程中
    PROCESS_PRESSING,   // 加压过程中
    PROCESS_TRANSFORM,  // 转化过程中
    PROCESS_COOLING,    // 冷却过程中
    PROCESS_ERROR       // 错误状态
} ProcessState;

/**
 * @brief 系统告警级别枚举
 */
typedef enum {
    ALERT_NONE,         // 无告警
    ALERT_WARNING,      // 警告
    ALERT_CRITICAL      // 严重
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
 * @brief 工艺处理参数结构定义
 */
typedef struct {
    float pressure[6];      // 各阶段压力设定值
    float temperature;      // 温度值
    float progress;         // 工艺进度百分比(0-100%)
    uint32_t runtime;       // 运行总时间(秒)
    ProcessState state;     // 处理状态
    DisplayMode mode;       // 显示模式
} ProcessParams;

/* 菜单项结构 */
typedef struct {
    const char *text;      // 菜单项显示文本
    int (*handler)(void);  // 菜单项处理函数指针
} MenuItem;

/* 菜单函数声明 - 只保留实际使用的函数 */

/* ===== 主菜单和基本导航 ===== */
int menu1(void);             // 开机动画和主菜单选择，返回选择的菜单编号
int menu2(void);             // 二级菜单，根据选择进入不同的功能设置界面
int menu_process(void);      // 工艺流程监控，显示当前生产过程及3D模型
int menu_history(void);      // 历史记录菜单，查看过去的生产数据和系统日志

/* ===== 三级菜单函数组 ===== */
int menu3_PressureSet(void);    // 压力参数设置界面
int menu3_TemperatureSet(void); // 温度参数设置界面
int menu3_TimeSet(void);        // 时间参数设置界面
int menu3_ProcessSelect(void);  // 工艺选择界面，不同工艺过程
int menu3_DataRecord(void);     // 数据记录界面，查看历史生产数据
int menu3_SystemConfig(void);   // 系统配置界面，各种系统参数设置
int menu_motor_status(void);    // 电机状态界面，控制电机运行和参数设定
int menu3_MotorCalibration(void); // 电机校准界面

/* ===== 四级菜单函数组 ===== */
int menu4_Process1(void);       // 工艺1参数设置
int menu4_Process2(void);       // 工艺2参数设置
int menu4_Process3(void);       // 工艺3参数设置
int menu4_CalibrationP(void);   // 压力传感器校准
int menu4_CalibrationT(void);   // 温度传感器校准
int menu4_AlarmConfig(void);    // 报警设置，包括报警阈值和方式
int menu4_SystemInfo(void);     // 系统信息界面，显示版本等信息

#endif /* __MENU_H__ */
