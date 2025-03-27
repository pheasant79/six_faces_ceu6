/**
 * @file menu.h
 * @brief 六面顶菜单系统头文件 - 碳转石墨过程可视化
 */
#ifndef __MENU_H__
#define __MENU_H__

#include "main.h"

/* UI配置常量 */
#define REFRESH_RATE_MS 500      // 数据刷新率
#define WARNING_BLINK_MS 300     // 警告闪烁周期
#define ANIMATION_DURATION_MS 280 // 动画持续时间
#define SCREEN_WIDTH 128         // OLED屏幕宽度
#define SCREEN_HEIGHT 64         // OLED屏幕高度

/* 警报级别枚举 */
typedef enum {
    ALERT_NONE,         // 无警报
    ALERT_WARNING,      // 警告
    ALERT_CRITICAL      // 危险
} AlertLevel;

/* 工艺状态枚举 */
typedef enum {
    PROCESS_IDLE,       // 待机
    PROCESS_HEATING,    // 加热阶段
    PROCESS_PRESSING,   // 加压阶段
    PROCESS_TRANSFORM,  // 转化阶段
    PROCESS_COOLING,    // 冷却阶段
    PROCESS_ERROR      // 错误状态
} ProcessState;

/* 显示模式枚举 */
typedef enum {
    DISPLAY_MODE_NORMAL,    // 正常模式
    DISPLAY_MODE_NIGHT,     // 夜间模式
    DISPLAY_MODE_DEMO       // 演示模式
} DisplayMode;

/* 系统参数结构体 */
typedef struct {
    float pressure;         // 当前压力值
    float temperature;      // 当前温度值
    AlertLevel alert;       // 警报级别
    uint16_t alert_count;   // 警报计数
    ProcessState state;     // 工艺状态
    DisplayMode mode;       // 显示模式
    uint8_t is_calibrated;  // 校准状态
} SystemParams;

/* 六面顶压力数据结构 */
typedef struct {
    float pressure[6];      // 六个面的压力值
    float temperature;      // 温度值
    float progress;         // 转化进度(0-100%)
    uint32_t runtime;       // 运行时间(秒)
    ProcessState state;     // 工艺状态
    DisplayMode mode;       // 显示模式
} ProcessParams;

/* 3D模型顶点结构 */
typedef struct {
    int8_t x, y, z;        // 3D坐标
    uint8_t transformed;    // 转化状态
} Vertex;

/* 菜单项结构体 */
typedef struct {
    const char *text;      // 菜单项文本
    int (*handler)(void);  // 菜单项处理函数
} MenuItem;

/* 函数声明 */
// 主菜单函数
int menu1(void);
int menu_process(void);
int menu_main(void);
int menu2(void);
int menu_settings(void);
int menu_history(void);

// 工艺控制函数
int process_start(void);
int process_stop(void);
int process_pause(void);
void process_update(void);

// 显示函数
void display_3d_model(void);
void display_pressure_bars(void);
void display_temperature(void);
void display_progress(void);
void display_status(void);
void display_animation_frame(void);

// 交互函数
void handle_touch_input(void);
void update_parameters(void);
void save_history_data(void);

// 动画效果
void animate_transformation(void);
void show_warning_effect(void);
void update_particle_effects(void);

// 系统功能
void update_system_state(void);
void toggle_night_mode(void);

// 菜单处理函数
int menu3_PressureSet(void);
int menu3_TemperatureSet(void);
int menu3_TimeSet(void);
int menu3_ProcessSelect(void);
int menu3_DataRecord(void);
int menu3_SystemConfig(void);
int menu4_Process1(void);
int menu4_Process2(void);
int menu4_Process3(void);
int menu4_CalibrationP(void);
int menu4_CalibrationT(void);
int menu4_AlarmConfig(void);
int menu4_SystemInfo(void);
int menu_motor_status(void);

#endif /* __MENU_H__ */
