/**
 * @file menu.c
 * @brief 六面顶菜单系统实现 - 碳转石墨过程可视化
 */
#include "menu.h"

/* 全局变量 */
static uint8_t KeyNum;                    // 按键值存储
static SystemParams g_sys_params = {0};   // 系统参数
static ProcessParams g_process_params = {0};     // 工艺参数
static Particle particles[MAX_PARTICLES]; // 粒子效果数据

/* ===== 内部函数声明 ===== */
// 显示函数
static void display_3d_model(void);         // 显示3D立方体模型
static void display_pressure_bars(void);    // 显示六面压力条形图
static void display_temperature(void);      // 显示温度计和温度值
static void display_progress(void);         // 显示进度条和百分比
static void display_status_bar(void);       // 显示系统状态栏
static void display_animation_frame(void);  // 显示动画帧
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select); // 显示菜单项
static void display_value_with_animation(float value, uint8_t x, uint8_t y); // 带动画效果显示数值

// 交互与数据处理函数
static void handle_touch_input(void);    // 处理触摸屏输入
static void update_parameters(void);     // 更新系统参数
static void save_history_data(void);     // 保存历史数据
static void process_update(void);        // 更新工艺参数

// 特效函数
static void animate_transformation(void);  // 石墨转化过程动画
static void show_warning_effect(void);     // 警告状态闪烁效果
static void update_particle_effects(void); // 粒子效果动画
static void init_particles(void);          // 初始化粒子效果

// 系统功能函数
static void update_system_state(void);     // 更新系统状态，检查警报条件
static void toggle_night_mode(void);       // 切换夜间/正常显示模式

/**
 * @brief  显示菜单项
 * @param  items 菜单项数组
 * @param  count 菜单项数量
 * @param  select 当前选中项
 * @retval None
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
    OLED_Update();
}

/**
 * @brief 更新系统状态
 */
static void update_system_state(void) {
    static uint32_t last_check = 0;
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - last_check >= REFRESH_RATE_MS) {
        // 更新压力和温度
        g_sys_params.pressure = Get_Pressure();
        g_sys_params.temperature = Get_Temperature();
        
        // 检查警报条件
        if(g_sys_params.pressure > MAX_PRESSURE * 0.9f) {
            g_sys_params.alert = ALERT_CRITICAL;
            g_sys_params.alert_count++;
            show_warning_effect();
        } else if(g_sys_params.pressure > MAX_PRESSURE * 0.8f) {
            g_sys_params.alert = ALERT_WARNING;
        } else {
            g_sys_params.alert = ALERT_NONE;
        }
        
        // 更新电机状态 - 检查所有电机
        Motor_State status0 = Motor_GetState(0);
        Motor_State status1 = Motor_GetState(1);
        Motor_State status2 = Motor_GetState(2);
        
        // 如果任一电机状态为MOTOR_ERROR，则系统状态为错误
        if(status0 == MOTOR_ERROR || status1 == MOTOR_ERROR || status2 == MOTOR_ERROR) {
            g_sys_params.motor_status = MOTOR_ERROR;
        }
        // 如果任一电机状态为MOTOR_RUNNING，则系统状态为运行
        else if(status0 == MOTOR_RUNNING || status1 == MOTOR_RUNNING || status2 == MOTOR_RUNNING) {
            g_sys_params.motor_status = MOTOR_RUNNING;
        }
        // 否则系统状态为停止
        else {
            g_sys_params.motor_status = MOTOR_STOP;
        }
        
        last_check = current_time;
    }
}

/**
 * @brief 显示状态栏
 */
static void display_status_bar(void) {
    char status_text[16];
    const char *state_str[] = {"IDLE", "HEAT", "PRESS", "HOLD", "COOL", "ERR"};
    
    // 显示系统状态
    snprintf(status_text, sizeof(status_text), "%s", state_str[g_sys_params.state]);
    OLED_ShowString(0, 0, status_text, OLED_8X16);
    
    // 显示警报计数
    if(g_sys_params.alert_count > 0) {
        snprintf(status_text, sizeof(status_text), "!%d", g_sys_params.alert_count);
        OLED_ShowString(SCREEN_WIDTH-24, 0, status_text, OLED_8X16);
    }
}

/**
 * @brief 带动画效果显示数值
 */
static void display_value_with_animation(float value, uint8_t x, uint8_t y) {
    static uint8_t animation_step = 0;
    char value_str[16];
    
    // 根据警报级别选择显示效果
    if(g_sys_params.alert == ALERT_CRITICAL) {
        if(animation_step++ % 2) {
            // OLED_InvertRect功能暂未实现
            // OLED_InvertRect(x, y, x+48, y+16);
        }
    }
    
    snprintf(value_str, sizeof(value_str), "%.1f", value);
    OLED_ShowString(x, y, value_str, OLED_8X16);
}

/**
 * @brief 初始化粒子效果
 */
static void init_particles(void) {
    for(int i = 0; i < MAX_PARTICLES; i++) {
        particles[i].active = 0;
    }
}

/**
 * @brief 更新粒子效果
 */
static void update_particle_effects(void) {
    static uint32_t last_particle = 0;
    uint32_t current_time = HAL_GetTick();
    
    // 每100ms创建新粒子
    if(current_time - last_particle > 100) {
        for(int i = 0; i < MAX_PARTICLES; i++) {
            if(!particles[i].active) {
                // 使用(rand() % 21 - 10)确保范围正好是-10到10
                particles[i].x = SCREEN_WIDTH/2 + (rand() % 21 - 10);
                particles[i].y = SCREEN_HEIGHT/2 + (rand() % 21 - 10);
                particles[i].dx = (rand() % 3 - 1);
                particles[i].dy = (rand() % 3 - 1);
                particles[i].active = 1;
                break;
            }
        }
        last_particle = current_time;
    }
    
    // 更新粒子位置
    for(int i = 0; i < MAX_PARTICLES; i++) {
        if(particles[i].active) {
            particles[i].x += particles[i].dx;
            particles[i].y += particles[i].dy;
            
            // 检查边界
            if(particles[i].x < 0 || particles[i].x >= SCREEN_WIDTH ||
               particles[i].y < 0 || particles[i].y >= SCREEN_HEIGHT) {
                particles[i].active = 0;
            } else {
                OLED_DrawPoint(particles[i].x, particles[i].y);
            }
        }
    }
}

/**
 * @brief 绘制3D模型
 */
static void display_3d_model(void) {
    // 定义立方体顶点（移到函数内部，因为只在这里使用）
    static const Vertex cube_vertices[] = {
        {-10, -10, -10, 0},  // 0: 左下前
        {10, -10, -10, 0},   // 1: 右下前
        {10, 10, -10, 0},    // 2: 右上前
        {-10, 10, -10, 0},   // 3: 左上前
        {-10, -10, 10, 0},   // 4: 左下后
        {10, -10, 10, 0},    // 5: 右下后
        {10, 10, 10, 0},     // 6: 右上后
        {-10, 10, 10, 0}     // 7: 左上后
    };
    
    int i;
    for(i = 0; i < 8; i++) {
        int x = cube_vertices[i].x + 64;
        int y = cube_vertices[i].y + 32;
        OLED_DrawPoint(x, y);
    }
}

/**
 * @brief 显示压力条形图
 */
static void display_pressure_bars(void) {
    int i;
    char buf[32];
    for(i = 0; i < 6; i++) {
        int bar_height = (int)(g_process_params.pressure[i] * 20.0f);
        // 绘制压力条
        OLED_DrawLine(10 + i*20, 63-bar_height, 10 + i*20, 63);
        OLED_DrawLine(10 + i*20, 63, 25 + i*20, 63);
        OLED_DrawLine(25 + i*20, 63, 25 + i*20, 63-bar_height);
        OLED_DrawLine(10 + i*20, 63-bar_height, 25 + i*20, 63-bar_height);
        
        // 显示压力值
        snprintf(buf, sizeof(buf), "%.1f", g_process_params.pressure[i]);
        OLED_ShowString(8 + i*20, 52, buf, 12);
    }
}

/**
 * @brief 显示温度计
 */
static void display_temperature(void) {
    const uint8_t x = SCREEN_WIDTH - 20;
    const uint8_t y = 10;
    const uint8_t height = 40;
    
    // 温度计外框
    OLED_DrawLine(x, y, x+10, y);
    OLED_DrawLine(x, y, x, y+height);
    OLED_DrawLine(x+10, y, x+10, y+height);
    OLED_DrawLine(x, y+height, x+10, y+height);
    
    // 温度液柱 - 确保浮点数运算
    uint8_t temp_height = (uint8_t)(((float)g_process_params.temperature / MAX_TEMPERATURE) * height);
    for(uint8_t i = 0; i < temp_height; i++) {
        OLED_DrawLine(x+2, y+height-i, x+8, y+height-i);
    }
    
    // 显示温度值
    OLED_Printf(x-30, y, OLED_8X16, "%.1fC", g_process_params.temperature);
}

/**
 * @brief 显示进度条
 */
static void display_progress(void) {
    const uint8_t progress_width = 100;
    const uint8_t y = SCREEN_HEIGHT - 10;
    
    // 进度条外框
    OLED_DrawLine(14, y-8, 114, y-8);
    OLED_DrawLine(14, y, 114, y);
    OLED_DrawLine(14, y-8, 14, y);
    OLED_DrawLine(114, y-8, 114, y);
    
    // 进度填充 - 确保浮点数运算
    uint8_t fill_width = (uint8_t)(((float)g_process_params.progress / 100.0f) * progress_width);
    for(uint8_t i = 0; i < fill_width; i++) {
        OLED_DrawLine(14+i, y-7, 14+i, y-1);
    }
    
    // 显示百分比 - 可显示小数点
    OLED_Printf(70, y-7, OLED_8X16, "%.1f%%", g_process_params.progress);
}

/**
 * @brief 切换夜间模式
 */
static void toggle_night_mode(void) {
    static DisplayMode mode = DISPLAY_MODE_NORMAL;
    mode = (mode == DISPLAY_MODE_NIGHT) ? DISPLAY_MODE_NORMAL : DISPLAY_MODE_NIGHT;
    
    // OLED对比度控制功能暂未实现
    /*if(mode == DISPLAY_MODE_NIGHT) {
        OLED_SetContrast(0x20); // 降低亮度
    } else {
        OLED_SetContrast(0xFF); // 恢复正常亮度
    }*/
}

/**
 * @brief 警告效果显示
 */
static void show_warning_effect(void) {
    // 实现警告效果的代码
    // 暂时留空，实际可以实现闪烁或声音提示
}

/**
 * @brief  主菜单界面
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
    
    // 初始化粒子效果 - 只在主菜单需要调用一次
    init_particles();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                OLED_Clear();
                OLED_Update();
                return items[select].handler();
        }
        
        display_menu_items(items, count, select);
    }
}

/**
 * @brief  工艺过程界面
 * @retval int 返回值说明
 */
int menu_process(void)
{
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu1();
        
        OLED_Clear();
        
        // 显示状态栏
        display_status_bar();
        
        // 显示3D模型
        display_3d_model();
        
        // 显示压力条形图
        display_pressure_bars();
        
        // 显示温度
        display_temperature();
        
        // 显示进度
        display_progress();
        
        // 更新粒子效果
        update_particle_effects();
        
        OLED_Update();
    }
}

/**
 * @brief  参数设置界面
 * @note   选择要设置的参数类型
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
        {"电机状态", menu_motor_status}
    };
    uint8_t count = sizeof(main_menu_items) / sizeof(main_menu_items[0]);
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                OLED_Clear();
                OLED_Update();
                return main_menu_items[select].handler();
                
            case 4: // 返回
                OLED_Clear();
                OLED_Update();
                return menu1();
        }
        
        // 显示菜单项
        display_menu_items(main_menu_items, count, select);
    }
}

/**
 * @brief  设置菜单界面
 * @retval int 返回值说明
 */
int menu_settings(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem settings_items[] = {
        {"显示设置", NULL},
        {"声音设置", NULL},
        {"语言设置", NULL},
        {"关于系统", menu4_SystemInfo}
    };
    uint8_t count = sizeof(settings_items) / sizeof(settings_items[0]);
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                if(settings_items[select].handler != NULL) {
                    return settings_items[select].handler();
                }
                break;
                
            case 4: // 返回
                return menu1();
        }
        
        display_menu_items(settings_items, count, select);
    }
}

/**
 * @brief  历史记录菜单界面
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
        {"导出数据", NULL}
    };
    uint8_t count = sizeof(history_items) / sizeof(history_items[0]);
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                if(history_items[select].handler != NULL) {
                    // 历史记录功能暂未实现
                    OLED_Clear();
                    OLED_ShowString(0, 0, "历史记录", OLED_8X16);
                    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
                    OLED_Update();
                    HAL_Delay(1000);
                }
                break;
                
            case 4: // 返回
                return menu1();
        }
        
        display_menu_items(history_items, count, select);
    }
}

/**
 * @brief  主菜单界面 - 老接口，保持兼容性
 * @retval int 返回值说明
 */
int menu_main(void)
{
    // 为兼容保留，实际调用menu1
    return menu1();
}

/**
 * @brief  工艺选择界面
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
        {"自定义", NULL}
    };
    uint8_t count = sizeof(process_items) / sizeof(process_items[0]);
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                if(process_items[select].handler != NULL) {
                    return process_items[select].handler();
                }
                break;
                
            case 4: // 返回
                return menu2();
        }
        
        display_menu_items(process_items, count, select);
    }
}

/**
 * @brief  系统设置界面
 * @retval int 返回值说明
 */
int menu3_SystemConfig(void)
{
    uint8_t select = 0;
    // 在函数内部定义菜单项
    static const MenuItem config_items[] = {
        {"压力校准", menu4_CalibrationP},
        {"温度校准", menu4_CalibrationT},
        {"报警设置", menu4_AlarmConfig},
        {"系统信息", menu4_SystemInfo}
    };
    uint8_t count = sizeof(config_items) / sizeof(config_items[0]);
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 上
                select = (select > 0) ? (select - 1) : (count - 1);
                break;
                
            case 2: // 下
                select = (select < (count - 1)) ? (select + 1) : 0;
                break;
                
            case 3: // 确认
                if(config_items[select].handler != NULL) {
                    return config_items[select].handler();
                }
                break;
                
            case 4: // 返回
                return menu2();
        }
        
        display_menu_items(config_items, count, select);
    }
}

/**
 * @brief  数据记录界面
 * @retval int 返回值说明
 */
int menu3_DataRecord(void)
{
    // 数据记录功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "数据记录", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu2();
    }
}

/**
 * @brief  压力设置界面
 * @retval int 返回值说明
 */
int menu3_PressureSet(void)
{
    float pressure = Get_Pressure();
    uint8_t select = 0;
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 增加
                pressure += (select == 0) ? 1.0f : 0.1f;
                if(pressure > MAX_PRESSURE) pressure = MAX_PRESSURE;
                break;
                
            case 2: // 减小
                pressure -= (select == 0) ? 1.0f : 0.1f;
                if(pressure < 0) pressure = 0;
                break;
                
            case 3: // 切换整数/小数
                select = !select;
                break;
                
            case 4: // 返回
                return menu2();
                
            case 5: // 确认设置
                if(Set_Pressure(pressure) == PRESSURE_OK) {
                    return menu2();
                }
                break;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置压力:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f GPa", pressure);
        OLED_ShowString(0, 32, select ? "精细调节" : "粗略调节", OLED_8X16);
        OLED_Update();
    }
}

/**
 * @brief  温度设置界面
 * @retval int 返回值说明
 */
int menu3_TemperatureSet(void)
{
    float temperature = Get_Temperature();
    uint8_t select = 0;
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 增加
                temperature += (select == 0) ? 10.0f : 1.0f;
                if(temperature > MAX_TEMPERATURE) temperature = MAX_TEMPERATURE;
                break;
                
            case 2: // 减小
                temperature -= (select == 0) ? 10.0f : 1.0f;
                if(temperature < 0) temperature = 0;
                break;
                
            case 3: // 切换整数/小数
                select = !select;
                break;
                
            case 4: // 返回
                return menu2();
                
            case 5: // 确认设置
                if(Set_Temperature(temperature) == TEMP_OK) {
                    return menu2();
                }
                break;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置温度:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f C", temperature);
        OLED_ShowString(0, 32, select ? "精细调节" : "粗略调节", OLED_8X16);
        OLED_Update();
    }
}

/**
 * @brief  时间设置界面
 * @retval int 返回值说明
 */
int menu3_TimeSet(void)
{
    uint8_t hours = 0, minutes = 0;
    uint8_t select = 0; // 0:hours, 1:minutes
    
    while(1)
    {
        KeyNum = Key_GetNum();
        
        switch(KeyNum)
        {
            case 1: // 增加
                if(select == 0) {
                    hours = (hours + 1) % 24;
                } else {
                    minutes = (minutes + 1) % 60;
                }
                break;
                
            case 2: // 减小
                if(select == 0) {
                    hours = (hours > 0) ? (hours - 1) : 23;
                } else {
                    minutes = (minutes > 0) ? (minutes - 1) : 59;
                }
                break;
                
            case 3: // 切换时/分
                select = !select;
                break;
                
            case 4: // 返回
                return menu2();
                
            case 5: // 确认设置
                // 设置时间的代码待实现
                return menu2();
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置时间:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%02d:%02d", hours, minutes);
        OLED_ShowString(0, 32, select ? "设置分钟" : "设置小时", OLED_8X16);
        OLED_Update();
    }
}

/**
 * @brief  工艺1参数设置
 * @retval int 返回值说明
 */
int menu4_Process1(void)
{
    // 工艺1参数设置待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 1", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_ProcessSelect();
    }
}

/**
 * @brief  工艺2参数设置
 * @retval int 返回值说明
 */
int menu4_Process2(void)
{
    // 工艺2参数设置待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 2", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_ProcessSelect();
    }
}

/**
 * @brief  工艺3参数设置
 * @retval int 返回值说明
 */
int menu4_Process3(void)
{
    // 工艺3参数设置待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "工艺 3", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_ProcessSelect();
    }
}

/**
 * @brief  压力校准界面
 * @retval int 返回值说明
 */
int menu4_CalibrationP(void)
{
    // 压力校准功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "压力校准", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_SystemConfig();
    }
}

/**
 * @brief  温度校准界面
 * @retval int 返回值说明
 */
int menu4_CalibrationT(void)
{
    // 温度校准功能待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "温度校准", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_SystemConfig();
    }
}

/**
 * @brief  报警设置界面
 * @retval int 返回值说明
 */
int menu4_AlarmConfig(void)
{
    // 报警设置界面待实现
    OLED_Clear();
    OLED_ShowString(0, 0, "报警设置", OLED_8X16);
    OLED_ShowString(0, 16, "功能未实现", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) return menu3_SystemConfig();
    }
}

/**
 * @brief  系统信息界面
 * @retval int 返回值说明
 */
int menu4_SystemInfo(void)
{
    // 系统信息
    OLED_Clear();
    OLED_ShowString(0, 0, "系统信息", OLED_8X16);
    OLED_ShowString(0, 16, "硬件: v1.0", OLED_8X16);
    OLED_ShowString(0, 32, "软件: v1.2", OLED_8X16);
    OLED_ShowString(0, 48, "序列号: 2023112301", OLED_8X16);
    OLED_Update();
    
    while(1)
    {
        KeyNum = Key_GetNum();
        if(KeyNum == 4) 
            return menu3_SystemConfig();
    }
}

/**
 * @brief  步进电机状态显示界面
 * @retval int 返回值说明
 */
int menu_motor_status(void)
{
    // 电机参数
    static struct {
        float feed_x;  // X轴进给量
        float feed_y;  // Y轴进给量
        float feed_z;  // Z轴进给量
        float speed_x; // X轴速度
        float speed_y; // Y轴速度
        float speed_z; // Z轴速度
    } motor_params = {
        .feed_x = 0.0f,
        .feed_y = 0.0f,
        .feed_z = 0.0f,
        .speed_x = 100.0f,
        .speed_y = 100.0f,
        .speed_z = 100.0f
    };
    
    uint8_t select = 0;  // 0-5: X进给、Y进给、Z进给、X速度、Y速度、Z速度
    uint8_t edit_mode = 0;
    float *current_value = &motor_params.feed_x;
    
    while(1)
    {
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
                    
                case 3: // 进入编辑模式
                    edit_mode = 1;
                    break;
                    
                case 4: // 返回
                    return menu2();
            }
        } else {
            switch(KeyNum)
            {
                case 1: // 增加
                    if(select < 3) { // 进给量设置
                        *current_value += 0.1f;
                        if(*current_value > MAX_FEED) *current_value = MAX_FEED;
                    } else { // 速度设置
                        *current_value += 10.0f;
                        if(*current_value > MAX_SPEED) *current_value = MAX_SPEED;
                    }
                    break;
                    
                case 2: // 减小
                    if(select < 3) { // 进给量设置
                        *current_value -= 0.1f;
                        if(*current_value < 0) *current_value = 0;
                    } else { // 速度设置
                        *current_value -= 10.0f;
                        if(*current_value < 0) *current_value = 0;
                    }
                    break;
                    
                case 3: // 确认
                    edit_mode = 0;
                    break;
                    
                case 4: // 取消
                    edit_mode = 0;
                    break;
            }
        }
        
        // 显示界面
        OLED_Clear();
        
        // 标题
        OLED_ShowString(0, 0, "电机状态", OLED_8X16);
        
        // 显示三个电机的进给量
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
        
        // 显示速度值（在进给量右侧）
        OLED_Printf(64, 16, OLED_8X16, "%.1f%%", motor_params.speed_x);
        if(select == 3 && edit_mode) OLED_ShowString(120, 16, "*", OLED_8X16);
        else if(select == 3) OLED_ShowString(120, 16, ">", OLED_8X16);
        
        OLED_Printf(64, 32, OLED_8X16, "%.1f%%", motor_params.speed_y);
        if(select == 4 && edit_mode) OLED_ShowString(120, 32, "*", OLED_8X16);
        else if(select == 4) OLED_ShowString(120, 32, ">", OLED_8X16);
        
        OLED_Printf(64, 48, OLED_8X16, "%.1f%%", motor_params.speed_z);
        if(select == 5 && edit_mode) OLED_ShowString(120, 48, "*", OLED_8X16);
        else if(select == 5) OLED_ShowString(120, 48, ">", OLED_8X16);
        
        OLED_Update();
    }
}
