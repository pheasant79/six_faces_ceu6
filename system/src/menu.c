/**
 * @file menu.c
 * @brief 系统实现 - 模拟压力机
 */
#include "menu.h"

/* ===================== 菜单系统相关变量 ===================== */
static uint8_t KeyNum;                    // 按键值存储
static uint8_t oled_update_needed = 1;    // OLED更新标志位，1表示需要更新

/* ===================== 温度控制相关 ===================== */
#define DEFAULT_TARGET_TEMPERATURE  200.0f  // 默认目标温度(°C)
#define DEFAULT_INITIAL_TEMPERATURE 25.0f   // 默认初始温度(°C)
static float g_target_temperature = DEFAULT_TARGET_TEMPERATURE;  // 目标温度

/* ===================== LED控制相关 ===================== */
#define LED_PWM_MAX_COUNT           20      // LED PWM周期计数最大值
static uint8_t g_led_pwm_duty = 0;        // LED PWM占空比(0-10)
static uint8_t g_led_pwm_counter = 0;     // LED PWM计数器
static uint8_t g_led_enabled = 0;         // LED启用状态，0表示禁用，1表示启用

/* ===================== 压力控制相关 ===================== */
#define DEFAULT_TARGET_PRESSURE     5.0f    // 默认目标压力(GPa)
#define MAX_PRESSURE                20.0f   // 最大压力值(GPa)
#define MIN_PRESSURE                0.0f    // 最小压力值(GPa)
#define STEP_COUNT_FULL_PRESSURE    5000    // 加压所需的步数
static float g_target_pressure = DEFAULT_TARGET_PRESSURE;       // 目标压力

/* ===================== 电机控制相关 ===================== */
#define CALIBRATION_SPEED           100     // 校准速度
#define NORMAL_SPEED                300     // 正常运行速度
uint8_t current_motor = 0;                 // 当前选择的电机ID
uint32_t initial_calibration_steps = 0;    // 校准阶段初始步数
/* 校准保存的步数值 - 各电机加压所需步数 */
static uint32_t calibrated_steps[MOTOR_NUM] = {STEP_COUNT_FULL_PRESSURE, STEP_COUNT_FULL_PRESSURE, STEP_COUNT_FULL_PRESSURE};

/* ===================== 工艺流程相关 ===================== */
#define MAX_STAGE_DURATION          18000   // 最大阶段持续时间
#define MIN_STAGE_DURATION          1000    // 最小阶段持续时间
#define DEFAULT_HOLD_DURATION       18000   // 默认保温保压时间(ms)
ProcessStage current_stage = STAGE_READY;  // 当前工艺阶段
static uint32_t g_stage_duration[8] = {0, 1000, 18000, 15000, DEFAULT_HOLD_DURATION, 10000, 5000, 0}; // 各阶段持续时间
static ProcessParams g_process_params = {0};     // 工艺参数

/* ===================== 外部变量声明 ===================== */
extern TIM_HandleTypeDef htim5;  // TIM5句柄，用于LED PWM控制
extern Motor motors[MOTOR_NUM];  // 从motor.c引入电机数组

/*=============================内部函数声明================================================*/

/* =====  ===== */
// 显示
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select); // 显示菜单项

// LED温度显示
static void update_temperature_led(float temperature, float target_temp, float initial_temp); // 根据温度更新LED亮度
static void reset_led_state(void);         // 重置LED状态

int menu3_MotorCalibration(void);

/*==============================================================================*/

/*======================================================================*/


/**
 * @brief  显示菜单项
 */
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select) {
    OLED_Clear();
    for(uint8_t i = 0; i < 4 && (select/4)*4 + i < count; i++) {
        uint8_t idx = (select/4)*4 + i;
        OLED_ShowString(0, i*16, (char*)items[idx].text, OLED_8X16);
		
		if(idx == select) {
			OLED_ShowString(120, i*16, ">", OLED_8X16);
        }
    }
    OLED_Update(); // 直接在此函数内更新OLED显示
    oled_update_needed = 0; // 重置标志位
}



/**
 * @brief  主菜单入口
 * @retval int 返回值说明
 */
int menu1(void)
{
    uint8_t select = 0;
    
    // 在函数内部定义菜单项
    static const MenuItem items[] = {
        {"工艺监控", menu_process},
        {"参数设置", menu2},
        {"历史记录", menu_history}
    };
    uint8_t count = sizeof(items) / sizeof(items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(items[select].handler != NULL) {
                    items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                }
                break;
        }
    }
}


/**
 * @brief  工艺过程界面 - 启动和监控工艺全流程
 * @retval int 返回值说明
 */
int menu_process(void)
{
    // 初始化当前工艺阶段
    current_stage = STAGE_READY;
    
    // 使用全局变量中保存的参数
    float target_temperature = g_target_temperature;  // 目标温度
    float target_pressure = g_target_pressure;       // 目标压力
    float current_temperature = DEFAULT_INITIAL_TEMPERATURE;  // 当前温度
    float current_pressure = 0.0f;      // 当前压力
    uint8_t motors_running = 0;         // 电机运行状态
    uint8_t process_confirmed = 0;      // 工艺确认标志
    
    // 使用全局变量中保存的时间参数
    uint32_t stage_duration[8];         // 各阶段持续时间
    memcpy(stage_duration, g_stage_duration, sizeof(stage_duration));
    
    // 时间相关变量
    uint32_t stage_start_time = 0;    // 当前阶段的开始时间戳
    uint32_t current_time = 0;        // 当前系统时间戳
    uint32_t elapsed_time = 0;        // 当前阶段已经过的时间
    uint32_t total_seconds = 0;       // 工艺总耗时（秒）
    uint32_t minutes = 0;             // 分钟数
    uint32_t seconds = 0;             // 秒数
    uint32_t remaining_time = 0;      // 当前阶段剩余时间
    
    // 进度相关变量
    float progress = 0.0f;            // 当前阶段总进度(0.0-1.0)
    float release_progress = 0.0f;    // 卸压总进度(0.0-1.0)
    float calib_progress_x = 0.0f;    // X轴校准进度(0.0-1.0)
    float calib_progress_y = 0.0f;    // Y轴校准进度(0.0-1.0)
    float calib_progress_z = 0.0f;    // Z轴校准进度(0.0-1.0)
    float calib_progress = 0.0f;      // 总校准进度
    float press_progress_x = 0.0f;    // X轴加压进度(0.0-1.0)
    float press_progress_y = 0.0f;    // Y轴加压进度(0.0-1.0)
    float press_progress_z = 0.0f;    // Z轴加压进度(0.0-1.0)
    float release_progress_x = 0.0f;  // X轴卸压进度(0.0-1.0)
    float release_progress_y = 0.0f;  // Y轴卸压进度(0.0-1.0)
    float release_progress_z = 0.0f;  // Z轴卸压进度(0.0-1.0)
    uint8_t bar_width = 0;            // 进度条显示宽度
    
    // 初始化电机目标步数数组
    for(uint8_t i = 0; i < MOTOR_NUM; i++) {
        motors[i].target_steps = 0; // 默认不限制步数
    }
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    // 清屏并显示欢迎界面
    OLED_Clear();
    OLED_ShowString(0, 0, "六面顶工艺控制", OLED_8X16);
    OLED_ShowString(0, 16, "按键3: 启动工艺", OLED_8X16);
    OLED_ShowString(0, 32, "按键4: 返回", OLED_8X16);
    OLED_ShowString(0, 48, "按键5: 紧急停止", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        //按键判断
        switch(KeyNum)
        {
            case 3: // 启动/确认
                if(current_stage == STAGE_READY && !process_confirmed) {
                    // 确认启动工艺
                    process_confirmed = 1;
                    OLED_Clear();
                    OLED_ShowString(0, 0, "确认启动工艺", OLED_8X16);
                    OLED_ShowString(0, 16, "按键3: 确认", OLED_8X16);
                    OLED_ShowString(0, 32, "按键4: 取消", OLED_8X16);
                    OLED_Update();
                } 
                else if(current_stage == STAGE_READY && process_confirmed) {
                    // 正式启动工艺流程，首先进入校准阶段
                    current_stage = STAGE_CALIBRATION;
                    stage_start_time = HAL_GetTick();
                    
                    // 重置LED状态，确保干净开始
                    reset_led_state();
                    
                    // 不用启动TIM5定时器，用于LED PWM控制,因为它已经在main里面开启了,用于按键扫描
                    g_led_enabled = 1; // 启用LED
                    
                    // 启动所有电机，设置为反向旋转进行校准
                    for(uint8_t i = 0; i < 3; i++) {
                        // 检查电机是否已经到达目标位置
                        if(Motor_GetStepCount(i) > 0) {
                            Motor_SetDir(i, DIR_CCW);    // 设置方向为逆时针（反方向）
                            Motor_SetSpeed(i, CALIBRATION_SPEED);      // 设置校准速度
                            
                            // 设置目标步数为0，表示需要减少到0
                            motors[i].target_steps = 0;
                            motors[i].step_control_enabled = 1; // 启用中断监控
                            
                            Motor_Run(i);                // 启动电机
                        }
                    }
                    
                    // 保存初始步数用于计算进度
                    initial_calibration_steps = Motor_GetStepCount(0);
                    
                    motors_running = 1;
                    
                    // 更新进度条初始值
                    g_process_params.progress = 0.0f;
                }
                break;
                
            case 4: // 返回
                if(process_confirmed && current_stage == STAGE_READY) {
                    // 取消确认，返回准备状态
                    process_confirmed = 0;
                    oled_update_needed = 1;
                } else if(!motors_running || current_stage == STAGE_COMPLETE) {
                    // 如果电机未运行或工艺已完成，则可以安全返回
                    return 0; // 直接返回0，不调用上一级函数
                } else {
                    // 提示用户工艺正在进行中
                    OLED_Clear();
                    OLED_ShowString(0, 0, "工艺正在进行中", OLED_8X16);
                    OLED_ShowString(0, 16, "请先紧急停止", OLED_8X16);
                    OLED_Update();
                    HAL_Delay(1500);
                    oled_update_needed = 1;
                }
                break;
                
            case 5: // 紧急停止
                if(motors_running) {
                    // 紧急停止所有电机
                    Motor_EmergencyStop();
                    motors_running = 0;
                    
                    // 重置所有电机的步数监控标志位
                    for(uint8_t i = 0; i < MOTOR_NUM; i++) {
                        motors[i].step_control_enabled = 0;
                        motors[i].target_steps = 0;
                    }
                    
                    // 重置LED状态
                    reset_led_state();
                    
                    // 显示停止信息
                    OLED_Clear();
                    OLED_ShowString(0, 0, "紧急停止", OLED_8X16);
                    OLED_ShowString(0, 16, "所有电机已停止", OLED_8X16);
                    OLED_Update();
                    HAL_Delay(1500);
                    
                    // 返回准备状态
                    current_stage = STAGE_READY;
                    process_confirmed = 0;
                    oled_update_needed = 1;
                }
                break;
        }
        
        // 根据当前工艺阶段执行相应的逻辑
        if(motors_running) {
            current_time = HAL_GetTick();
            elapsed_time = current_time - stage_start_time;
            
            // 计算进度 - 在界面渲染前先更新工艺进度
            switch(current_stage) {
                case STAGE_CALIBRATION:
                    // 校准阶段 - 所有电机反转回到原点
                    // 使用所有电机的步数计算进度
                    
                    // 计算所有电机的校准进度
                    calib_progress_x = (float)(initial_calibration_steps - Motor_GetStepCount(0)) / initial_calibration_steps;
                    calib_progress_y = (float)(initial_calibration_steps - Motor_GetStepCount(1)) / initial_calibration_steps;
                    calib_progress_z = (float)(initial_calibration_steps - Motor_GetStepCount(2)) / initial_calibration_steps;
                    
                    // 取最小值作为整体进度
                    calib_progress = calib_progress_x;
                    if(calib_progress_y < calib_progress) calib_progress = calib_progress_y;
                    if(calib_progress_z < calib_progress) calib_progress = calib_progress_z;
                    
                    if(calib_progress < 0.0f) calib_progress = 0.0f;
                    if(calib_progress > 1.0f) calib_progress = 1.0f;
                    
                    // 进度从0%到10%（在整个工艺中的比例）
                    g_process_params.progress = calib_progress * 10.0f;
                    
                    // 基于所有电机的步数判断校准是否完成
                    if(motors[0].state != MOTOR_RUNNING && 
                       motors[1].state != MOTOR_RUNNING &&
                       motors[2].state != MOTOR_RUNNING) {
                        // 自动进入加压阶段
                        current_stage = STAGE_PRESS_FAST;
                        stage_start_time = current_time;
                        
                        // 校准完成，所有电机设置为正向中速旋转进行加压
                        if(current_stage == STAGE_PRESS_FAST) {
                            for(uint8_t i = 0; i < 3; i++) {
                                // 检查电机是否已经到达目标位置
                                if(Motor_GetStepCount(i) < calibrated_steps[i]) {
                                    Motor_SetDir(i, DIR_CW);     // 设置方向为顺时针
                                    Motor_SetSpeed(i, NORMAL_SPEED);      // 设置中速，平衡速度和精度
                                    
                                    // 使用校准后保存的步数进行加压
                                    motors[i].target_steps = Motor_GetStepCount(i) + calibrated_steps[i];
                                    motors[i].step_control_enabled = 1; // 启用中断监控
                                    
                                    Motor_Run(i);                // 启动电机
                                }
                            }
                        }
                    }
                    break;
                
                case STAGE_PRESS_FAST:
                    // 加压阶段 - 提升压力至目标值
                    // 计算三个电机的加压进度，以最小进度作为整体进度
                    press_progress_x = (float)Motor_GetStepCount(0) / calibrated_steps[0];
                    press_progress_y = (float)Motor_GetStepCount(1) / calibrated_steps[1];
                    press_progress_z = (float)Motor_GetStepCount(2) / calibrated_steps[2];
                    
                    // 使用最小进度作为整体进度计算压力
                    progress = press_progress_x;
                    if(press_progress_y < progress) progress = press_progress_y;
                    if(press_progress_z < progress) progress = press_progress_z;
                    
                    // 模拟压力上升 - 基于整体进度
                    current_pressure = target_pressure * progress;
                    
                    // 更新进度
                    g_process_params.progress = 10.0f + progress * 25.0f;
                    
                    // 基于所有电机的步数判断加压是否完成
                    if(motors[0].state != MOTOR_RUNNING && 
                       motors[1].state != MOTOR_RUNNING &&
                       motors[2].state != MOTOR_RUNNING) {
                        // 自动进入加热阶段
                        current_stage = STAGE_HEAT;
                        stage_start_time = current_time;

                        // 更新LED状态以显示温度变化
                        g_led_enabled = 1;
                    }
                    break;
                    
                case STAGE_HEAT:
                    // 加热阶段 - 温度上升，保持压力
                    // 模拟温度上升（仍基于时间）
                    current_temperature = DEFAULT_INITIAL_TEMPERATURE + (target_temperature - DEFAULT_INITIAL_TEMPERATURE) * 
                                         ((float)elapsed_time / stage_duration[STAGE_HEAT]);
                    
                    // 保持压力
                    current_pressure = target_pressure;
                    
                    // 根据温度更新LED亮度
                    update_temperature_led(current_temperature, target_temperature, DEFAULT_INITIAL_TEMPERATURE);
                    
                    // 更新进度
                    g_process_params.progress = 35.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_HEAT] * 15.0f;
                    
                    // 阶段时间到，或者温度达到目标值，进入保温保压阶段
                    if(elapsed_time >= stage_duration[STAGE_HEAT] || 
                       current_temperature >= target_temperature) {
                        current_stage = STAGE_HOLD;
                        stage_start_time = current_time;

                    }
                    break;
                    
                case STAGE_HOLD:
                    // 保温保压阶段 - 保持温度和压力
                    current_temperature = target_temperature;
                    current_pressure = target_pressure;
                    
                    // 保持LED亮度最大
                    update_temperature_led(current_temperature, target_temperature, DEFAULT_INITIAL_TEMPERATURE);
                    
                    // 更新进度
                    g_process_params.progress = 50.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_HOLD] * 20.0f;
                    
                    // 阶段时间到，进入冷却阶段
                    if(elapsed_time >= stage_duration[STAGE_HOLD]) {
                        current_stage = STAGE_COOL;
                        stage_start_time = current_time;

                    }
                    break;
                    
                case STAGE_COOL:
                    // 冷却阶段 - 温度下降，保持压力
                    // 模拟温度下降（仍基于时间）
                    current_temperature = target_temperature - (target_temperature - DEFAULT_INITIAL_TEMPERATURE) * 
                                         ((float)elapsed_time / stage_duration[STAGE_COOL]);
                    
                    // 保持压力
                    current_pressure = target_pressure;
                    
                    
                    // 根据温度更新LED亮度
                    update_temperature_led(current_temperature, target_temperature, DEFAULT_INITIAL_TEMPERATURE);
                    
                    // 更新进度
                    g_process_params.progress = 70.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_COOL] * 15.0f;
                    
                    // 阶段时间到，或者温度降到接近初始值，进入卸压阶段
                    if(elapsed_time >= stage_duration[STAGE_COOL] || 
                       current_temperature <= 30.0f) {
                        current_stage = STAGE_RELEASE;
                        stage_start_time = current_time;

                        
                        // 启动卸压，让所有电机反向旋转
                        if(current_stage == STAGE_RELEASE) {
                            for(uint8_t i = 0; i < 3; i++) {
                                // 检查电机是否已经到达目标位置
                                if(Motor_GetStepCount(i) > 0) {
                                    Motor_SetDir(i, DIR_CCW);    // 设置方向为逆时针（反方向）
                                    Motor_SetSpeed(i, NORMAL_SPEED);      // 设置中速卸压
                                    
                                    // 目标步数为0（减少到0）
                                    motors[i].target_steps = 0;
                                    motors[i].step_control_enabled = 1; // 启用中断监控
                                    
                                    Motor_Run(i);                // 启动电机
                                }
                            }
                        }
                    }
                    break;
                    
                case STAGE_RELEASE:
                    // 卸压阶段 - 松开压力
                    // 计算各个电机的卸压进度
                    release_progress_x = (float)(calibrated_steps[0] - Motor_GetStepCount(0)) / calibrated_steps[0];
                    release_progress_y = (float)(calibrated_steps[1] - Motor_GetStepCount(1)) / calibrated_steps[1];
                    release_progress_z = (float)(calibrated_steps[2] - Motor_GetStepCount(2)) / calibrated_steps[2];
                    
                    // 取最小值作为整体进度
                    release_progress = release_progress_x;
                    if(release_progress_y < release_progress) release_progress = release_progress_y;
                    if(release_progress_z < release_progress) release_progress = release_progress_z;
                    
                    // 确保比例值在0-1之间
                    if(release_progress < 0.0f) release_progress = 0.0f;
                    if(release_progress > 1.0f) release_progress = 1.0f;
                    
                    // 压力从最大值逐渐减小到0 - 基于整体进度
                    current_pressure = target_pressure * (1.0f - release_progress);
                    
                    // 更新进度
                    g_process_params.progress = 85.0f + release_progress * 15.0f;
                    
                    // 判断卸压完成 - 检查所有电机
                    if(motors[0].state != MOTOR_RUNNING && 
                       motors[1].state != MOTOR_RUNNING &&
                       motors[2].state != MOTOR_RUNNING) {
                        // 自动进入完成阶段
                        current_stage = STAGE_COMPLETE;
                        
                        // 完成所有工艺流程
                        // 重置LED状态
                        reset_led_state();
                        
                        // 临时禁用定时器，完成后重新启用
                        HAL_TIM_Base_Stop_IT(&htim5);
                        
                        // 显示工艺完成界面
                        OLED_Clear();
                        OLED_ShowString(0, 0, "工艺流程完成", OLED_8X16);
                        OLED_ShowString(0, 16, "总耗时:", OLED_8X16);
                        total_seconds = (HAL_GetTick() - stage_start_time) / 1000;
                        minutes = total_seconds / 60;
                        seconds = total_seconds % 60;
                        OLED_Printf(56, 16, OLED_8X16, "%02lu:%02lu", minutes, seconds);
                        OLED_ShowString(0, 32, "按键4返回", OLED_8X16);
                        OLED_Update();
                        
                        current_stage = STAGE_READY;
                        process_confirmed = 0;
                        motors_running = 0; // 确保标记电机已停止
                        oled_update_needed = 1;
                        
                        HAL_Delay(2000);
                        return 0; // 直接返回上级菜单
                       
                    }
                    break;
                    
                default:
                    break;
            }
            
            // 更新系统参数用于显示
            g_process_params.temperature = current_temperature;
            g_process_params.pressure[0] = current_pressure;
        }
        
        // 界面显示
        if(!process_confirmed || (process_confirmed && current_stage == STAGE_READY)) {
            if(oled_update_needed) {
                // 显示欢迎界面
                OLED_Clear();
                OLED_ShowString(0, 0, "六面顶工艺控制", OLED_8X16);
                OLED_ShowString(0, 16, "按键3: 启动工艺", OLED_8X16);
                OLED_ShowString(0, 32, "按键4: 返回", OLED_8X16);
                OLED_ShowString(0, 48, "按键5: 紧急停止", OLED_8X16);
                OLED_Update();
                oled_update_needed = 0;
            }
        } else if(motors_running || current_stage == STAGE_COMPLETE) {
            // 每次循环都刷新界面，因为工艺参数需要实时更新
            OLED_Clear();
            
            // 根据不同工艺阶段显示不同的界面
            switch(current_stage) {
                case STAGE_CALIBRATION:
                    // === 校准阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "电机校准中", OLED_8X16);
                    
                    // 显示X、Y、Z三个电机的步数信息
                    OLED_ShowString(0, 16, "X:", OLED_8X16);
                    OLED_Printf(16, 16, OLED_8X16, "%lu/0", 
                              Motor_GetStepCount(0));
                    
                    OLED_ShowString(0, 32, "Y:", OLED_8X16);
                    OLED_Printf(16, 32, OLED_8X16, "%lu/0", 
                              Motor_GetStepCount(1));
                    
                    OLED_ShowString(0, 48, "Z:", OLED_8X16);
                    OLED_Printf(16, 48, OLED_8X16, "%lu/0", 
                              Motor_GetStepCount(2));
                    
                    // 右侧竖向进度条 - 取三个电机中进度最小的作为整体进度
                    OLED_DrawRectangle(120, 16, 8, 48, 0);
                    
                    // 计算三个电机的校准进度
                    calib_progress_x = (float)(initial_calibration_steps - Motor_GetStepCount(0)) / initial_calibration_steps;
                    calib_progress_y = (float)(initial_calibration_steps - Motor_GetStepCount(1)) / initial_calibration_steps;
                    calib_progress_z = (float)(initial_calibration_steps - Motor_GetStepCount(2)) / initial_calibration_steps;
                    
                    // 取最小值作为整体进度
                    calib_progress = calib_progress_x;
                    if(calib_progress_y < calib_progress) calib_progress = calib_progress_y;
                    if(calib_progress_z < calib_progress) calib_progress = calib_progress_z;
                    
                    // 规范化进度值
                    if(calib_progress < 0.0f) calib_progress = 0.0f;
                    if(calib_progress > 1.0f) calib_progress = 1.0f;
                    
                    // 计算进度条宽度
                    bar_width = (uint8_t)(calib_progress * 48);
                    if(bar_width > 48) bar_width = 48;
                    
                    for(uint8_t i = 0; i < bar_width; i++) {
                        OLED_DrawLine(121, 63 - i, 127, 63 - i);
                    }
                    
                    // 更新进度参数，用于其他地方显示
                    g_process_params.progress = calib_progress * 10.0f;
                    break;
                    
                case STAGE_PRESS_FAST:
                    // === 加压阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "加压中", OLED_8X16);
                    OLED_Printf(70, 0, OLED_8X16, "%.1fGPa", current_pressure);
                    
                    // 显示X、Y、Z三个电机的步数信息
                    OLED_ShowString(0, 16, "X:", OLED_8X16);
                    OLED_Printf(16, 16, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(0), calibrated_steps[0]);
                    
                    OLED_ShowString(0, 32, "Y:", OLED_8X16);
                    OLED_Printf(16, 32, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(1), calibrated_steps[1]);
                    
                    OLED_ShowString(0, 48, "Z:", OLED_8X16);
                    OLED_Printf(16, 48, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(2), calibrated_steps[2]);
                    
                    // 右侧竖向进度条 - 取三个电机中进度最小的作为整体进度
                    OLED_DrawRectangle(120, 16, 8, 48, 0);
                    
                    // 计算三个电机的加压进度
                    press_progress_x = (float)Motor_GetStepCount(0) / calibrated_steps[0];
                    press_progress_y = (float)Motor_GetStepCount(1) / calibrated_steps[1];
                    press_progress_z = (float)Motor_GetStepCount(2) / calibrated_steps[2];
                    
                    // 取最小值作为整体进度
                    progress = press_progress_x;
                    if(press_progress_y < progress) progress = press_progress_y;
                    if(press_progress_z < progress) progress = press_progress_z;
                    
                    // 规范化进度值
                    if(progress < 0.0f) progress = 0.0f;
                    if(progress > 1.0f) progress = 1.0f;
                    
                    // 计算进度条宽度
                    bar_width = (uint8_t)(progress * 48);
                    if(bar_width > 48) bar_width = 48;
                    
                    for(uint8_t i = 0; i < bar_width; i++) {
                        OLED_DrawLine(121, 63 - i, 127, 63 - i);
                    }
                    
                    // 更新整体进度
                    g_process_params.progress = 10.0f + progress * 25.0f;
                    break;
                    
                case STAGE_HEAT:
                    // === 加热阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "加热中", OLED_8X16);
                    
                    // 显示温度值
                    OLED_ShowString(0, 16, "温度:", OLED_8X16);
                    OLED_Printf(48, 16, OLED_8X16, "%.1f C", current_temperature);
                    
                    // 显示目标温度
                    OLED_ShowString(0, 32, "目标:", OLED_8X16);
                    OLED_Printf(48, 32, OLED_8X16, "%.1f C", target_temperature);
                    
                    // 显示温度进度条
                    OLED_DrawRectangle(0, 48, 100, 8, 0);
                    bar_width = (uint8_t)(((current_temperature - DEFAULT_INITIAL_TEMPERATURE) / (target_temperature - DEFAULT_INITIAL_TEMPERATURE)) * 100);
                    if(bar_width > 100) bar_width = 100;
                    for(uint8_t i = 0; i < bar_width; i++) {
                        OLED_DrawLine(i, 49, i, 55);
                    }
                    // 显示进度百分比
                    OLED_Printf(105, 48, OLED_8X16, "%d%%", (int)g_process_params.progress);
                    break;
                    
                case STAGE_HOLD:
                    // === 保温保压阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "保温保压中", OLED_8X16);
                    
                    // 显示温度值
                    OLED_ShowString(0, 16, "温度:", OLED_8X16);
                    OLED_Printf(48, 16, OLED_8X16, "%.1f C", current_temperature);
                    
                    // 显示压力值
                    OLED_ShowString(0, 32, "压力:", OLED_8X16);
                    OLED_Printf(48, 32, OLED_8X16, "%.1f GPa", current_pressure);
                    
                    // 显示剩余时间
                    remaining_time = (stage_duration[STAGE_HOLD] - elapsed_time) / 1000; // 秒
                    OLED_ShowString(0, 48, "剩余:", OLED_8X16);
                    OLED_Printf(48, 48, OLED_8X16, "%02lu:%02lu", remaining_time / 60, remaining_time % 60);
                    
                    // 显示进度百分比
                    OLED_Printf(105, 48, OLED_8X16, "%d%%", (int)g_process_params.progress);
                    break;
                    
                case STAGE_COOL:
                    // === 冷却阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "冷却中", OLED_8X16);
                    
                    // 显示温度值
                    OLED_ShowString(0, 16, "温度:", OLED_8X16);
                    OLED_Printf(48, 16, OLED_8X16, "%.1f C", current_temperature);
                    
                    // 显示目标温度
                    OLED_ShowString(0, 32, "目标:", OLED_8X16);
                    OLED_Printf(48, 32, OLED_8X16, "25.0 C");
                    
                    // 显示温度进度条
                    OLED_DrawRectangle(0, 48, 100, 8, 0);
                    bar_width = (uint8_t)(((current_temperature - DEFAULT_INITIAL_TEMPERATURE) / (target_temperature - DEFAULT_INITIAL_TEMPERATURE)) * 100);
                    if(bar_width > 100) bar_width = 100;
                    for(uint8_t i = 0; i < bar_width; i++) {
                        OLED_DrawLine(i, 49, i, 55);
                    }
                    // 显示进度百分比
                    OLED_Printf(105, 48, OLED_8X16, "%d%%", (int)g_process_params.progress);
                    break;
                    
                case STAGE_RELEASE:
                    // === 卸压阶段界面 ===
                    // 显示标题和状态
                    OLED_Clear();
                    OLED_ShowString(0, 0, "卸压中", OLED_8X16);
                    OLED_Printf(70, 0, OLED_8X16, "%.1fGPa", current_pressure);
                    
                    // 显示X、Y、Z三个电机的步数信息
                    OLED_ShowString(0, 16, "X:", OLED_8X16);
                    OLED_Printf(16, 16, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(0), 0);
                    
                    OLED_ShowString(0, 32, "Y:", OLED_8X16);
                    OLED_Printf(16, 32, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(1), 0);
                    
                    OLED_ShowString(0, 48, "Z:", OLED_8X16);
                    OLED_Printf(16, 48, OLED_8X16, "%lu/%lu", 
                              Motor_GetStepCount(2), 0);
                    
                    // 右侧竖向进度条 - 取三个电机中进度最小的作为整体进度
                    OLED_DrawRectangle(120, 16, 8, 48, 0);
                    
                    // 计算各个电机的卸压进度（从最大步数减小到0的比例）
                    release_progress_x = (float)(calibrated_steps[0] - Motor_GetStepCount(0)) / calibrated_steps[0];
                    release_progress_y = (float)(calibrated_steps[1] - Motor_GetStepCount(1)) / calibrated_steps[1];
                    release_progress_z = (float)(calibrated_steps[2] - Motor_GetStepCount(2)) / calibrated_steps[2];
                    
                    // 取最小值作为整体进度
                    release_progress = release_progress_x;
                    if(release_progress_y < release_progress) release_progress = release_progress_y;
                    if(release_progress_z < release_progress) release_progress = release_progress_z;
                    
                    // 规范化进度值
                    if(release_progress < 0.0f) release_progress = 0.0f;
                    if(release_progress > 1.0f) release_progress = 1.0f;
                    
                    // 计算进度条宽度
                    bar_width = (uint8_t)(release_progress * 48);
                    if(bar_width > 48) bar_width = 48;
                    
                    for(uint8_t i = 0; i < bar_width; i++) {
                        OLED_DrawLine(121, 63 - i, 127, 63 - i);
                    }
                    
                    // 更新整体进度
                    g_process_params.progress = 85.0f + release_progress * 15.0f;
                    
                    // 更新实时压力值 - 随着卸压进度增加，压力从最大值减小到0
                    current_pressure = target_pressure * (1.0f - release_progress);
                    break;
                    
                case STAGE_COMPLETE:
                    // 完成阶段显示
                    OLED_Clear();
                    OLED_ShowString(0, 0, "工艺完成", OLED_8X16);
                    OLED_ShowString(0, 16, "按键4: 返回", OLED_8X16);
                    OLED_Update();
                    
					current_stage = STAGE_READY;
					process_confirmed = 0;
					motors_running = 0; // 确保标记电机已停止
					oled_update_needed = 1;
				
					HAL_Delay(2000);
					return 0; // 直接返回上级菜单

                    
            }
            
            // 直接更新，因为工艺参数随时在变化
            OLED_Update();
        }
        
        /* 工艺流程控制示例 - 使用加压和卸压控制功能 */
        if(KeyNum == 1) { // 使用1代替KEY_OK，按照Key.h中可能的定义
            // 仅在非工艺进行中或工艺完成时响应按键1
            if(!motors_running || current_stage == STAGE_COMPLETE) {
                // 如果在准备阶段且已确认，则启动工艺
                if(current_stage == STAGE_READY && process_confirmed) {
                    // 正式启动工艺流程，重新设置为校准阶段
                    current_stage = STAGE_CALIBRATION;
                    // 此处不执行实际的电机控制，已在上面的过程确认部分完成
                } 
                // 如果在完成阶段，返回主菜单
                else if(current_stage == STAGE_COMPLETE) {
                    return menu1(); // 修正：原先是调用不存在的menu_main()
                }
            }
        }
    }
}


/**
 * @brief  参数设置菜单入口
 * @note  选择要进入的子菜单
 * @retval int 返回值说明
 */
int menu2(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem main_menu_items[] = {
        {"压力设置", menu3_PressureSet},
        {"温度设置", menu3_TemperatureSet},
        {"时间设置", menu3_TimeSet},
        {"工艺选择", menu3_ProcessSelect},
        {"数据记录", menu3_DataRecord},
        {"系统配置", menu3_SystemConfig},
        {"系统状态", menu_motor_status},
        {"电机校准", menu3_MotorCalibration}
    };
    uint8_t count = sizeof(main_menu_items) / sizeof(main_menu_items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(main_menu_items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(main_menu_items[select].handler != NULL) {
                    main_menu_items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                }
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
        }
    }
}


/**
 * @brief  参数设置菜单入口
 * @retval int 返回值说明
 */
int menu_settings(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem settings_items[] = {
        {"显示设置", NULL},
        {"参数设置", NULL},
        {"高级设置", NULL},
        {"系统信息", menu4_SystemInfo}
    };
    uint8_t count = sizeof(settings_items) / sizeof(settings_items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(settings_items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(settings_items[select].handler != NULL) {
                    settings_items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                }
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
        }
    }
}


/**
 * @brief  历史记录菜单入口
 * @retval int 返回值说明
 */
int menu_history(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem history_items[] = {
        {"记录 1", NULL},
        {"记录 2", NULL},
        {"记录 3", NULL},
        {"全部记录", NULL}
    };
    uint8_t count = sizeof(history_items) / sizeof(history_items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(history_items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(history_items[select].handler != NULL) {
                    history_items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                } else {
                    // 历史记录功能待实现
                    OLED_Clear();
                    OLED_ShowString(0, 0, "历史记录", OLED_8X16);
                    OLED_ShowString(0, 16, "暂无数据", OLED_8X16);
                    OLED_Update(); // 直接更新显示
                    HAL_Delay(1000);
                    oled_update_needed = 1; // 返回菜单时需要更新显示
                }
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
        }
    }
}


/**
 * @brief  温度设置菜单入口
 * @retval int 返回值说明
 */
int menu3_TemperatureSet(void)
{
    float temperature = g_target_temperature; // 使用全局温度值
    uint8_t select = 0;
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                temperature += (select == 0) ? 10.0f : 1.0f;
                if(temperature > MAX_TEMPERATURE) temperature = MAX_TEMPERATURE;
                break;
                
            case 2: // 下
                temperature -= (select == 0) ? 10.0f : 1.0f;
                if(temperature < 0) temperature = 0;
                break;
                
            case 3: // 切换粗调/精调
                select = !select;
                break;
                
            case 4: // 返回
                return 0;
                
            case 5: // 确认设置
				g_target_temperature = temperature; // 保存设置的温度值
			
				// 显示保存成功
				OLED_Clear();
				OLED_ShowString(0, 0, "温度设置成功", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "目标温度:%.1f", temperature);
				OLED_Update();
				HAL_Delay(1500);
			
				return 0;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置温度:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f C", temperature);
        OLED_ShowString(0, 32, select ? "精调模式" : "粗调模式", OLED_8X16);
        OLED_ShowString(0, 48, "按键5:确认", OLED_8X16);
        OLED_Update();
    }
}


/**
 * @brief  时间设置菜单入口
 * @retval int 返回值说明
 * @note   只设置保温保压时间，其他阶段时间由步数或固定值决定
 */
int menu3_TimeSet(void)
{
    uint8_t edit_mode = 0;    // 0:粗调, 1:精调
    uint32_t current_duration = g_stage_duration[STAGE_HOLD]; // 只获取保温保压阶段时间
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                if(edit_mode == 0) {
                    // 粗调模式，每次加1000ms (1秒)
                    current_duration += 1000;
                } else {
                    // 精调模式，每次加100ms (0.1秒)
                    current_duration += 100;
                }
                if(current_duration > MAX_STAGE_DURATION) current_duration = MAX_STAGE_DURATION;
                break;
                
            case 2: // 下
                if(edit_mode == 0) {
                    // 粗调模式，每次减1000ms (1秒)
                    current_duration = (current_duration > MIN_STAGE_DURATION) ? (current_duration - 1000) : MIN_STAGE_DURATION;
                } else {
                    // 精调模式，每次减100ms (0.1秒)
                    current_duration = (current_duration > MIN_STAGE_DURATION) ? (current_duration - 100) : MIN_STAGE_DURATION;
                }
                break;
                
            case 3: // 切换粗调/精调
                edit_mode = !edit_mode;
                break;
                
            case 4: // 返回
                return 0;
                
            case 5: // 确认设置
                g_stage_duration[STAGE_HOLD] = current_duration; // 保存保温保压时间
                
                // 显示保存成功
                OLED_Clear();
                OLED_ShowString(0, 0, "时间设置成功", OLED_8X16);
                OLED_ShowString(0, 16, "保温保压时间已更新", OLED_8X16);
                OLED_Update();
                HAL_Delay(1500);
                return 0;
        }
        
        OLED_Clear();
        
        // 显示当前设置
        OLED_ShowString(0, 0, "设置保温保压时间", OLED_8X16);
        
        // 显示时间（分:秒.毫秒）
        uint32_t minutes = current_duration / 60000;
        uint32_t seconds = (current_duration % 60000) / 1000;
        uint32_t milliseconds = current_duration % 1000;
        OLED_Printf(0, 16, OLED_8X16, "%02lu:%02lu.%03lu", minutes, seconds, milliseconds);
        
        // 显示调整模式
        OLED_ShowString(0, 32, edit_mode ? "精调模式" : "粗调模式", OLED_8X16);
        
        OLED_Update();
    }
}


/**
 * @brief  压力设置菜单入口
 * @retval int 返回值说明
 */
int menu3_PressureSet(void)
{
    float pressure = g_target_pressure; // 使用全局压力值
    uint8_t select = 0;
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                pressure += (select == 0) ? 1.0f : 0.1f;
                if(pressure > MAX_PRESSURE) pressure = MAX_PRESSURE;
                break;
                
            case 2: // 下
                pressure -= (select == 0) ? 1.0f : 0.1f;
                if(pressure < MIN_PRESSURE) pressure = MIN_PRESSURE;
                break;
                
            case 3: // 切换粗调/精调
                select = !select;
                break;
                
            case 4: // 返回
                return 0;
                
            case 5: // 确认设置
				g_target_pressure = pressure; // 保存设置的压力值
				
				// 显示保存成功
				OLED_Clear();
				OLED_ShowString(0, 0, "压力设置成功", OLED_8X16);
				OLED_Printf(0, 16, OLED_8X16, "目标压力:%.1fGPa", pressure);
				OLED_Update();
				HAL_Delay(1500);
				return 0;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置压力:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f GPa", pressure);
        OLED_ShowString(0, 32, select ? "精调模式" : "粗调模式", OLED_8X16);
        OLED_ShowString(0, 48, "按键5:确认", OLED_8X16);
        OLED_Update();
    }
}


/**
 * @brief  工艺选择菜单入口
 * @retval int 返回值说明
 */
int menu3_ProcessSelect(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem process_items[] = {
        {"工艺 1", menu4_Process1},
        {"工艺 2", menu4_Process2},
        {"工艺 3", menu4_Process3},
        {"自动", NULL}
    };
    uint8_t count = sizeof(process_items) / sizeof(process_items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(process_items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(process_items[select].handler != NULL) {
                    process_items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                }
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
        }
    }
}


/**
 * @brief  系统配置菜单入口
 * @retval int 返回值说明
 */
int menu3_SystemConfig(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem config_items[] = {
        {"压力校准", menu4_CalibrationP},
        {"温度校准", menu4_CalibrationT},
        {"报警配置", menu4_AlarmConfig},
        {"系统信息", menu4_SystemInfo}
    };
    uint8_t count = sizeof(config_items) / sizeof(config_items[0]);
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        if (oled_update_needed) {
            display_menu_items(config_items, count, select);
            // display_menu_items已经调用了OLED_Update并重置标志位
        }
        
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                oled_update_needed = 1; // 标记需要更新显示
                break;
                
            case 3: // 确认
                if(config_items[select].handler != NULL) {
                    config_items[select].handler(); // 直接调用，不使用return
                    oled_update_needed = 1; // 返回时标记需要更新显示
                }
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
        }
    }
}


/**
 * @brief  数据记录菜单入口
 * @retval int 返回值说明
 */
int menu3_DataRecord(void)
{
    // 数据记录功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "数据记录", OLED_8X16);
    OLED_ShowString(0, 16, "暂无数据", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return 0; // 直接返回0，不调用上一级函数
    }
}


/**
 * @brief  工艺1参数设置
 * @retval int 返回值说明
 */
int menu4_Process1(void)
{
    // 显示设置中...
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 1", OLED_8X16);
    OLED_ShowString(0, 16, "正在设置...", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 延时一秒
    HAL_Delay(1000);
    
    // 显示设置成功
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 1", OLED_8X16);
    OLED_ShowString(0, 16, "设置成功", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 再延时一秒
    HAL_Delay(1000);
	return 0;
}


/**
 * @brief  工艺2参数设置
 * @retval int 返回值说明
 */
int menu4_Process2(void)
{
    // 显示设置中...
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 2", OLED_8X16);
    OLED_ShowString(0, 16, "正在设置...", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 延时一秒
    HAL_Delay(1000);
    
    // 显示设置成功
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 2", OLED_8X16);
    OLED_ShowString(0, 16, "设置成功", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 再延时一秒
    HAL_Delay(1000);
	return 0;
}


/**
 * @brief  工艺3参数设置
 * @retval int 返回值说明
 */
int menu4_Process3(void)
{
    // 显示设置中...
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 3", OLED_8X16);
    OLED_ShowString(0, 16, "正在设置...", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 延时一秒
    HAL_Delay(1000);
    
    // 显示设置成功
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 3", OLED_8X16);
    OLED_ShowString(0, 16, "设置成功", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    // 再延时一秒
    HAL_Delay(1000);
	return 0;
}


/**
 * @brief  压力校准菜单入口
 * @retval int 返回值说明
 */
int menu4_CalibrationP(void)
{
    // 压力校准功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "压力校准", OLED_8X16);
    OLED_ShowString(0, 16, "暂无数据", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return 0; // 直接返回0，不调用上一级函数
    }
}


/**
 * @brief  温度校准菜单入口
 * @retval int 返回值说明
 */
int menu4_CalibrationT(void)
{
    // 温度校准功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "温度校准", OLED_8X16);
    OLED_ShowString(0, 16, "暂无数据", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return 0; // 直接返回0，不调用上一级函数
    }
}


/**
 * @brief  报警配置菜单入口
 * @retval int 返回值说明
 */
int menu4_AlarmConfig(void)
{
    // 报警配置功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "报警配置", OLED_8X16);
    OLED_ShowString(0, 16, "暂无数据", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return 0; // 直接返回0，不调用上一级函数
    }
}


/**
 * @brief  系统信息菜单入口
 * @retval int 返回值说明
 */
int menu4_SystemInfo(void)
{
    // 系统信息
    OLED_Clear();
    OLED_ShowString(0, 0, "系统信息", OLED_8X16);
    OLED_ShowString(0, 16, "硬件: v1.0", OLED_8X16);
    OLED_ShowString(0, 32, "软件: v1.2", OLED_8X16);
    OLED_ShowString(0, 48, "版本号: 2023112301", OLED_8X16);
    OLED_Update(); // 直接更新显示
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return 0; // 直接返回0，不调用上一级函数
    }
}


/**
 * @brief  系统状态显示菜单入口
 * @retval int 返回值说明
 */
int menu_motor_status(void)
{
    // 系统状态
    static struct {
        float feed_x;  // X方向
        float feed_y;  // Y方向
        float feed_z;  // Z方向
        float speed_x; // X速度
        float speed_y; // Y速度
        float speed_z; // Z速度
    } motor_params = {
        .feed_x = 0.0f,
        .feed_y = 0.0f,
        .feed_z = 0.0f,
        .speed_x = 100.0f,
        .speed_y = 100.0f,
        .speed_z = 100.0f
    };
    
    uint8_t select = 0;  // 0-5: X方向Y方向Z方向X速度Y速度Z速度
    uint8_t edit_mode = 0;
    float *current_value = &motor_params.feed_x;
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        // 显示系统状态需要实时更新，直接处理而不使用标记
        // 显示系统状态
        OLED_Clear();
        
        // 显示系统状态栏
        OLED_ShowString(0, 0, "系统状态", OLED_8X16);
        
        // 显示系统状态栏的子选项
        OLED_ShowString(0, 16, "X:", OLED_8X16);
        OLED_Printf(16, 16, OLED_8X16, "%.1fmm", motor_params.feed_x);
        if(select == 0 && edit_mode) OLED_ShowString(120, 16, "*", OLED_8X16);
        else if(select == 0) OLED_ShowString(120, 16, ">", OLED_8X16);
        
        OLED_ShowString(0, 32, "Y:", OLED_8X16);
        OLED_Printf(16, 32, OLED_8X16, "%.1fmm", motor_params.feed_y);
        if(select == 1 && edit_mode) OLED_ShowString(120, 32, "*", OLED_8X16);
        else if(select == 1) OLED_ShowString(120, 32, ">", OLED_8X16);
        
        OLED_ShowString(0, 48, "Z:", OLED_8X16);
        OLED_Printf(16, 48, OLED_8X16, "%.1fmm", motor_params.feed_z);
        if(select == 2 && edit_mode) OLED_ShowString(120, 48, "*", OLED_8X16);
        else if(select == 2) OLED_ShowString(120, 48, ">", OLED_8X16);
        
        // 显示速度值（在子选项中）
        OLED_Printf(64, 16, OLED_8X16, "%.1f%%", motor_params.speed_x);
        if(select == 3 && edit_mode) OLED_ShowString(120, 16, "*", OLED_8X16);
        else if(select == 3) OLED_ShowString(120, 16, ">", OLED_8X16);
        
        OLED_Printf(64, 32, OLED_8X16, "%.1f%%", motor_params.speed_y);
        if(select == 4 && edit_mode) OLED_ShowString(120, 32, "*", OLED_8X16);
        else if(select == 4) OLED_ShowString(120, 32, ">", OLED_8X16);
        
        OLED_Printf(64, 48, OLED_8X16, "%.1f%%", motor_params.speed_z);
        if(select == 5 && edit_mode) OLED_ShowString(120, 48, "*", OLED_8X16);
        else if(select == 5) OLED_ShowString(120, 48, ">", OLED_8X16);
        
        OLED_Update(); // 直接更新显示
        
        KeyNum = Key_GetNum();
        
        if(!edit_mode) {
            switch(KeyNum)
            {
                case 1: // 上
                    select = (select > 0) ? (select - 1) : 5;
                    switch(select) {
                        case 0: current_value = &motor_params.feed_x; break;
                        case 1: current_value = &motor_params.feed_y; break;
                        case 2: current_value = &motor_params.feed_z; break;
                        case 3: current_value = &motor_params.speed_x; break;
                        case 4: current_value = &motor_params.speed_y; break;
                        case 5: current_value = &motor_params.speed_z; break;
                    }
                    break;
                    
                case 2: // 下
                    select = (select < 5) ? (select + 1) : 0;
                    switch(select) {
                        case 0: current_value = &motor_params.feed_x; break;
                        case 1: current_value = &motor_params.feed_y; break;
                        case 2: current_value = &motor_params.feed_z; break;
                        case 3: current_value = &motor_params.speed_x; break;
                        case 4: current_value = &motor_params.speed_y; break;
                        case 5: current_value = &motor_params.speed_z; break;
                    }
                    break;
                    
                case 3: // 切换编辑模式
                    edit_mode = 1;
                    break;
                    
                case 4: // 返回
                    return 0; // 直接返回0，不调用上一级函数
            }
        } else {
            switch(KeyNum)
            {
                case 1: // 上
                    if(select < 3) { // 方向参数
                        *current_value += 0.1f;
                        if(*current_value > MAX_FEED) *current_value = MAX_FEED;
                    } else { // 速度参数
                        *current_value += 10.0f;
                        if(*current_value > MAX_SPEED) *current_value = MAX_SPEED;
                    }
                    break;
                    
                case 2: // 下
                    if(select < 3) { // 方向参数
                        *current_value -= 0.1f;
                        if(*current_value < 0) *current_value = 0;
                    } else { // 速度参数
                        *current_value -= 10.0f;
                        if(*current_value < 0) *current_value = 0;
                    }
                    break;
                    
                case 3: // 确认
                    edit_mode = 0;
                    break;
                    
                case 4: // 返回
                    edit_mode = 0;
                    break;
            }
        }
    }
}


/**
 * @brief  电机校准菜单入口
 * @retval int 返回值说明
 */
int menu3_MotorCalibration(void)
{
    typedef struct {
        uint32_t steps_x;    // X电机步数
        uint32_t steps_y;    // Y电机步数
        uint32_t steps_z;    // Z电机步数
        uint8_t motor_running; // 电机运行状态
    } CalibrationParams;
    
    static CalibrationParams calib_params = {
        .steps_x = 0,
        .steps_y = 0,
        .steps_z = 0,
        .motor_running = 0
    };
    
    uint8_t select = 0;      // 当前选择的电机: 0=X, 1=Y, 2=Z
    uint8_t edit_mode = 0;   // 编辑模式: 0=选择电机, 1=粗调整, 2=精调整
    uint8_t current_motor = 0; // 当前操作的电机ID
    
    // 首次进入时标记需要更新
    oled_update_needed = 1;
    
    while(1)
    {
        // 更新当前电机步数显示
        if (edit_mode != 0) {
            // 直接获取当前步数，不进行任何转换
            calib_params.steps_x = Motor_GetStepCount(0);
            calib_params.steps_y = Motor_GetStepCount(1);
            calib_params.steps_z = Motor_GetStepCount(2);
        }
        
        // 显示界面
        OLED_Clear();
        OLED_ShowString(0, 0, "电机校准", OLED_8X16);
        
        // 显示各电机步数
        OLED_ShowString(0, 16, "X:", OLED_8X16);
        OLED_Printf(16, 16, OLED_8X16, "%lu", calib_params.steps_x);
        if(select == 0) OLED_ShowString(120, 16, ">", OLED_8X16);
        
        OLED_ShowString(0, 32, "Y:", OLED_8X16);
        OLED_Printf(16, 32, OLED_8X16, "%lu", calib_params.steps_y);
        if(select == 1) OLED_ShowString(120, 32, ">", OLED_8X16);
        
        OLED_ShowString(0, 48, "Z:", OLED_8X16);
        OLED_Printf(16, 48, OLED_8X16, "%lu", calib_params.steps_z);
        if(select == 2) OLED_ShowString(120, 48, ">", OLED_8X16);
        
        // 显示当前模式和操作提示
        if (edit_mode == 0) {
            OLED_ShowString(80, 0, "选择", OLED_8X16);
            OLED_ShowString(60, 48, "5:工艺", OLED_8X16);
        } else if (edit_mode == 1) {
            OLED_ShowString(80, 0, "粗调", OLED_8X16);
            OLED_ShowString(60, 48, (calib_params.motor_running) ? "键停" : "1:+/2:-", OLED_8X16);
        } else if (edit_mode == 2) {
            OLED_ShowString(80, 0, "精调", OLED_8X16);
            OLED_ShowString(60, 48, "1:+1/2:-1", OLED_8X16);
        }
        
        // 直接更新，因为电机步数可能在实时变化
        OLED_Update();
        
        KeyNum = Key_GetNum();
        
        // 根据不同的模式处理按键
        if (edit_mode == 0) { // 选择电机模式
            switch(KeyNum)
            {
                case 1: // 上
                    select = (select > 0) ? (select - 1) : 2;
                    break;
                    
                case 2: // 下
                    select = (select < 2) ? (select + 1) : 0;
                    break;
                    
                case 3: // 确认选择电机
                    current_motor = select; // 记录当前选择的电机ID
                    edit_mode = 1; // 进入粗调模式
                    break;
                    
                case 4: // 返回
                    return 0; // 直接返回0，不调用上一级函数
                    
                case 5: // 跳转至工艺监控
                    // 显示确认消息
                    OLED_Clear();
                    OLED_ShowString(0, 0, "正在跳转至", OLED_8X16);
                    OLED_ShowString(0, 16, "工艺监控...", OLED_8X16);
                    OLED_Update();
                    HAL_Delay(1000); // 显示1秒
                    
                    // 直接调用工艺监控函数
                    return menu_process();
            }
        } else if (edit_mode == 1) { // 粗调模式
            switch(KeyNum)
            {
                case 1: // 电机前进/停止
                    if (!calib_params.motor_running) {
                        // 启动电机前进
                        Motor_SetDir(current_motor, DIR_CW);
                        Motor_SetSpeed(current_motor, CALIBRATION_SPEED);
                        Motor_Run(current_motor);
                        calib_params.motor_running = 1;
                    } else {
                        // 停止电机
                        Motor_Stop(current_motor);
                        calib_params.motor_running = 0;
                    }
                    break;
                    
                case 2: // 电机后退/停止
                    if (!calib_params.motor_running) {
                        // 启动电机后退
                        Motor_SetDir(current_motor, DIR_CCW);
                        Motor_SetSpeed(current_motor, CALIBRATION_SPEED);
                        Motor_Run(current_motor);
                        calib_params.motor_running = 1;
                    } else {
                        // 停止电机
                        Motor_Stop(current_motor);
                        calib_params.motor_running = 0;
                    }
                    break;
                    
                case 3: // 进入精调模式
                    // 如果电机在运行，先停止
                    if (calib_params.motor_running) {
                        Motor_Stop(current_motor);
                        calib_params.motor_running = 0;
                    }
                    edit_mode = 2; // 进入精调模式
                    break;
                    
                case 4: // 返回电机选择模式
                    // 如果电机在运行，先停止
                    if (calib_params.motor_running) {
                        Motor_Stop(current_motor);
                        calib_params.motor_running = 0;
                    }
                    edit_mode = 0; // 返回电机选择模式
                    break;
                    
                case 5: // 归零（零度校准）
                    // 如果电机在运行，先停止
                    if (calib_params.motor_running) {
                        Motor_Stop(current_motor);
                        calib_params.motor_running = 0;
                    }
                    
                    // 初始化电机步数
                    Motor_InitAll(); // 重新初始化电机状态和步数
                    
                    // 更新当前步数显示
                    calib_params.steps_x = Motor_GetStepCount(0);
                    calib_params.steps_y = Motor_GetStepCount(1);
                    calib_params.steps_z = Motor_GetStepCount(2);
                    break;
            }
        } else if (edit_mode == 2) { // 精调模式
            switch(KeyNum)
            {
                case 1: // 步数+1 - 不实际运行电机，只修改步数
                    // 获取当前步数并直接修改
                    if (current_motor == 0) {
                        calib_params.steps_x++;
                        // 更新电机实际步数
                        Motor_SetStepCount(0, calib_params.steps_x);
                    } else if (current_motor == 1) {
                        calib_params.steps_y++;
                        // 更新电机实际步数
                        Motor_SetStepCount(1, calib_params.steps_y);
                    } else if (current_motor == 2) {
                        calib_params.steps_z++;
                        // 更新电机实际步数
                        Motor_SetStepCount(2, calib_params.steps_z);
                    }
                    break;
                    
                case 2: // 步数-1 - 不实际运行电机，只修改步数
                    // 获取当前步数并直接修改，确保不小于0
                    if (current_motor == 0 && calib_params.steps_x > 0) {
                        calib_params.steps_x--;
                        // 更新电机实际步数
                        Motor_SetStepCount(0, calib_params.steps_x);
                    } else if (current_motor == 1 && calib_params.steps_y > 0) {
                        calib_params.steps_y--;
                        // 更新电机实际步数
                        Motor_SetStepCount(1, calib_params.steps_y);
                    } else if (current_motor == 2 && calib_params.steps_z > 0) {
                        calib_params.steps_z--;
                        // 更新电机实际步数
                        Motor_SetStepCount(2, calib_params.steps_z);
                    }
                    break;
                    
                case 3: // 确认当前步数
                    // 将当前步数存储到校准步数数组中
                    if (current_motor == 0) {
                        calibrated_steps[current_motor] = calib_params.steps_x;
                    } else if (current_motor == 1) {
                        calibrated_steps[current_motor] = calib_params.steps_y;
                    } else if (current_motor == 2) {
                        calibrated_steps[current_motor] = calib_params.steps_z;
                    }
                    
                    // 显示确认消息
                    OLED_Clear();
                    OLED_ShowString(0, 0, "步数已保存", OLED_8X16);
                    OLED_ShowString(0, 16, "用于加压校准", OLED_8X16);
                    OLED_Printf(0, 32, OLED_8X16, "电机%d: %lu步", current_motor+1, 
                              calibrated_steps[current_motor]);
                    OLED_Update();
                    HAL_Delay(1500); // 显示1.5秒
                    
                    edit_mode = 0; // 返回电机选择模式
                    break;
                    
                case 4: // 返回粗调模式
                    edit_mode = 1;
                    break;
                    
                case 5: // 归零（零度校准）
                    // 初始化电机步数
                    Motor_InitAll(); // 重新初始化电机状态和步数
                    
                    // 更新当前步数显示
                    calib_params.steps_x = Motor_GetStepCount(0);
                    calib_params.steps_y = Motor_GetStepCount(1);
                    calib_params.steps_z = Motor_GetStepCount(2);
                    break;
            }
        }
    }
}


/**
 * @brief 根据温度值设置LED PWM占空比
 * @param temperature 当前温度值
 * @param target_temp 目标温度
 * @param initial_temp 初始温度
 * @note 温度越接近目标温度，LED亮度越高
 */
static void update_temperature_led(float temperature, float target_temp, float initial_temp) {
    // 计算温度与目标温度的接近程度(0.0-1.0)
    float progress = 0.0f;
    
    if (temperature <= initial_temp) {
        // 低于或等于初始温度，不亮
        progress = 0.0f;
    } else if (temperature >= target_temp) {
        // 达到或超过目标温度，最亮
        progress = 1.0f;
    } else {
        // 按比例计算亮度
        progress = (temperature - initial_temp) / (target_temp - initial_temp);
    }
    
    // 将进度值转换为PWM占空比(0-LED_PWM_MAX_COUNT)
    g_led_pwm_duty = (uint8_t)(progress * LED_PWM_MAX_COUNT);
}

/**
 * @brief TIM5中断回调函数 - 用于软件模拟PWM控制LED
 * @note 此函数应该在TIM5中断回调中被调用
 */
void LED_PWM_Update(void) {
    // 如果LED被禁用，直接关闭LED并返回
    if (!g_led_enabled) {
        LED_TurnOff();
        return;
    }
    
    // 软件PWM实现：计数器从0到19循环
    g_led_pwm_counter = (g_led_pwm_counter + 1) % 20;
    
    // 如果计数器小于占空比，LED亮；否则LED灭
    if (g_led_pwm_counter < g_led_pwm_duty) {
        LED_TurnOn();
    } else {
        LED_TurnOff();
    }
}

/**
 * @brief 重置LED状态
 * @note 清除所有LED相关变量，确保LED完全关闭
 */
static void reset_led_state(void) {
    g_led_enabled = 0;     // 禁用LED
    g_led_pwm_duty = 0;    // 清除占空比
    g_led_pwm_counter = 0; // 重置计数器
    LED_TurnOff();         // 物理关闭LED
}
