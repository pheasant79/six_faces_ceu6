/**
 * @file menu.c
 * @brief 系统实现 - 模拟压力机
 */
#include "menu.h"

/* 全局变量 */
static uint8_t KeyNum;                    // 按键值存储
static SystemParams g_sys_params = {0};   // 系统参数
static ProcessParams g_process_params = {0};     // 工艺参数
static Particle particles[MAX_PARTICLES]; // 粒子效果数据
static uint8_t oled_update_needed = 1;    // OLED更新标志位，1表示需要更新

/* ===== 内部函数声明 ===== */
// 显示
static void display_3d_model(void);         // 显示3D模型
static void display_pressure_bars(void);    // 显示压力条形图
static void display_temperature(void);      // 显示温度计和温度值
static void display_progress(void);         // 显示进度条和百分比
static void display_status_bar(void);       // 显示系统状态栏
//static void display_animation_frame(void);  // 显示动画帧
static void display_menu_items(const MenuItem *items, uint8_t count, uint8_t select); // 显示菜单项
#if 0  // 未使用的函数，注释掉以消除警告
static void display_value_with_animation(float value, uint8_t x, uint8_t y); // 显示有效数据值
#endif

// 处理触摸输入
//static void handle_touch_input(void);    // 处理触摸输入
//static void update_parameters(void);     // 更新系统参数
//static void save_history_data(void);     // 保存历史数据
//static void process_update(void);        // 处理工艺参数

// 有效数据
//static void animate_transformation(void);  // 模拟变换固定
#if 0
static void show_warning_effect(void);     // 显示状态警告效果
#endif
static void update_particle_effects(void); // 更新粒子效果
static void init_particles(void);          // 初始化粒子效果

// 系统功能
#if 0  // 未使用的函数，注释掉以消除警告
static void update_system_state(void);     // 更新系统状态
static void toggle_night_mode(void);       // 切换白天/夜晚显示模式
#endif

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
 * @brief 显示状态栏
 */
static void display_status_bar(void) {
    char status_text[16];
    const char *state_str[] = {"IDLE", "HEAT", "PRESS", "HOLD", "COOL", "ERR"};
    
    // 显示系统状态
    snprintf(status_text, sizeof(status_text), "%s", state_str[g_sys_params.state]);
    OLED_ShowString(0, 0, status_text, OLED_8X16);
    
    // 显示报警次数
    if(g_sys_params.alert_count > 0) {
        snprintf(status_text, sizeof(status_text), "!%d", g_sys_params.alert_count);
        OLED_ShowString(SCREEN_WIDTH-24, 0, status_text, OLED_8X16);
    }
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
    
    // 每100ms产生新粒子
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
            
            // 超出边界
            if(particles[i].x < 0 || particles[i].x >= SCREEN_WIDTH ||
               particles[i].y < 0 || particles[i].y >= SCREEN_HEIGHT) {
                particles[i].active = 0;
            } else {
                OLED_DrawPoint(particles[i].x, particles[i].y);
            }
        }
    }
    
    oled_update_needed = 1; // 粒子效果更新后需要刷新屏幕
}

/**
 * @brief 显示3D模型（使用线而不是点）
 */
static void display_3d_model(void) {
    // 立方体的顶点（移到函数内部，只在函数中使用）
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
    
    // 使用线绘制立方体
    // 前面
    int x1, y1, x2, y2;
    
    // 立方体中心点平移到屏幕中心
    const int center_x = 64;
    const int center_y = 32;
    
    // 前面
    x1 = cube_vertices[0].x + center_x; y1 = cube_vertices[0].y + center_y;
    x2 = cube_vertices[1].x + center_x; y2 = cube_vertices[1].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[1].x + center_x; y1 = cube_vertices[1].y + center_y;
    x2 = cube_vertices[2].x + center_x; y2 = cube_vertices[2].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[2].x + center_x; y1 = cube_vertices[2].y + center_y;
    x2 = cube_vertices[3].x + center_x; y2 = cube_vertices[3].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[3].x + center_x; y1 = cube_vertices[3].y + center_y;
    x2 = cube_vertices[0].x + center_x; y2 = cube_vertices[0].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    // 后面
    x1 = cube_vertices[4].x + center_x; y1 = cube_vertices[4].y + center_y;
    x2 = cube_vertices[5].x + center_x; y2 = cube_vertices[5].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[5].x + center_x; y1 = cube_vertices[5].y + center_y;
    x2 = cube_vertices[6].x + center_x; y2 = cube_vertices[6].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[6].x + center_x; y1 = cube_vertices[6].y + center_y;
    x2 = cube_vertices[7].x + center_x; y2 = cube_vertices[7].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[7].x + center_x; y1 = cube_vertices[7].y + center_y;
    x2 = cube_vertices[4].x + center_x; y2 = cube_vertices[4].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    // 连接前后面
    x1 = cube_vertices[0].x + center_x; y1 = cube_vertices[0].y + center_y;
    x2 = cube_vertices[4].x + center_x; y2 = cube_vertices[4].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[1].x + center_x; y1 = cube_vertices[1].y + center_y;
    x2 = cube_vertices[5].x + center_x; y2 = cube_vertices[5].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[2].x + center_x; y1 = cube_vertices[2].y + center_y;
    x2 = cube_vertices[6].x + center_x; y2 = cube_vertices[6].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    x1 = cube_vertices[3].x + center_x; y1 = cube_vertices[3].y + center_y;
    x2 = cube_vertices[7].x + center_x; y2 = cube_vertices[7].y + center_y;
    OLED_DrawLine(x1, y1, x2, y2);
    
    oled_update_needed = 1; // 标记需要更新显示
}

/**
 * @brief 显示压力条形图
 */
static void display_pressure_bars(void) {
    int i;
    char buf[32];
    for(i = 0; i < 6; i++) {
        int bar_height = (int)(g_process_params.pressure[i] * 20.0f);
        // 显示压力条
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
    
    // 温度计
    OLED_DrawLine(x, y, x+10, y);
    OLED_DrawLine(x, y, x, y+height);
    OLED_DrawLine(x+10, y, x+10, y+height);
    OLED_DrawLine(x, y+height, x+10, y+height);
    
    // 温度液 - 确保线性关系
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
    
    // 显示进度条
    OLED_DrawLine(14, y-8, 114, y-8);
    OLED_DrawLine(14, y, 114, y);
    OLED_DrawLine(14, y-8, 14, y);
    OLED_DrawLine(114, y-8, 114, y);
    
    // 进度 - 确保线性关系
    uint8_t fill_width = (uint8_t)(((float)g_process_params.progress / 100.0f) * progress_width);
    for(uint8_t i = 0; i < fill_width; i++) {
        OLED_DrawLine(14+i, y-7, 14+i, y-1);
    }
    
    // 显示百分比 - 显示小数
    OLED_Printf(70, y-7, OLED_8X16, "%.1f%%", g_process_params.progress);
}

/**
 * @brief 显示状态警告效果
 */
#if 0
static void show_warning_effect(void) {
    // 实现状态警告效果的代码
    // 暂时延迟，实际可以实现警告效果显示
}
#endif

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
    
    // 初始化粒子效果 - 只在主菜单需要调用一次
    static uint8_t particles_initialized = 0;
    if (!particles_initialized) {
        init_particles();
        particles_initialized = 1;
    }
    
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
 * @brief  压力设置菜单入口
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
            case 1: // 上
                pressure += (select == 0) ? 1.0f : 0.1f;
                if(pressure > MAX_PRESSURE) pressure = MAX_PRESSURE;
                break;
                
            case 2: // 下
                pressure -= (select == 0) ? 1.0f : 0.1f;
                if(pressure < 0) pressure = 0;
                break;
                
            case 3: // 切换单位
                select = !select;
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
                
            case 5: // 确认设置
                if(Set_Pressure(pressure) == PRESSURE_OK) {
                    return 0; // 直接返回0，不调用上一级函数
                }
                break;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置压力:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f GPa", pressure);
        OLED_ShowString(0, 32, select ? "高精度" : "低精度", OLED_8X16);
        OLED_Update(); // 每次参数变化后直接更新显示
    }
}

/**
 * @brief  温度设置菜单入口
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
            case 1: // 上
                temperature += (select == 0) ? 10.0f : 1.0f;
                if(temperature > MAX_TEMPERATURE) temperature = MAX_TEMPERATURE;
                break;
                
            case 2: // 下
                temperature -= (select == 0) ? 10.0f : 1.0f;
                if(temperature < 0) temperature = 0;
                break;
                
            case 3: // 切换单位
                select = !select;
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
                
            case 5: // 确认设置
                if(Set_Temperature(temperature) == TEMP_OK) {
                    return 0; // 直接返回0，不调用上一级函数
                }
                break;
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置温度:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%.1f C", temperature);
        OLED_ShowString(0, 32, select ? "高精度" : "低精度", OLED_8X16);
        OLED_Update(); // 每次参数变化后直接更新显示
    }
}

/**
 * @brief  时间设置菜单入口
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
            case 1: // 上
                if(select == 0) {
                    hours = (hours + 1) % 24;
                } else {
                    minutes = (minutes + 1) % 60;
                }
                break;
                
            case 2: // 下
                if(select == 0) {
                    hours = (hours > 0) ? (hours - 1) : 23;
                } else {
                    minutes = (minutes > 0) ? (minutes - 1) : 59;
                }
                break;
                
            case 3: // 切换单位
                select = !select;
                break;
                
            case 4: // 返回
                return 0; // 直接返回0，不调用上一级函数
                
            case 5: // 确认设置
                // 时间设置的逻辑实现
                return 0; // 直接返回0，不调用上一级函数
        }
        
        OLED_Clear();
        OLED_ShowString(0, 0, "设置时间:", OLED_8X16);
        OLED_Printf(0, 16, OLED_8X16, "%02d:%02d", hours, minutes);
        OLED_ShowString(0, 32, select ? "设置分钟" : "设置小时", OLED_8X16);
        OLED_Update(); // 每次参数变化后直接更新显示
    }
}

/**
 * @brief  工艺过程界面 - 启动和监控工艺全流程
 * @retval int 返回值说明
 */
int menu_process(void)
{
    // 工艺阶段定义
    typedef enum {
        STAGE_READY,     // 准备阶段
        STAGE_CALIBRATION, // 校准阶段
        STAGE_PRESS_FAST, // 一次加压阶段（快速加压）
        STAGE_PRESS_PRECISE, // 二次加压阶段（精确加压）
        STAGE_HEAT,      // 加热阶段
        STAGE_HOLD,      // 保温保压阶段
        STAGE_COOL,      // 冷却阶段
        STAGE_RELEASE,   // 卸压阶段
        STAGE_COMPLETE   // 完成阶段
    } ProcessStage;
    
    // 系统状态定义 - 与全局状态定义保持一致
    ProcessStage current_stage = STAGE_READY; // 当前工艺阶段
    uint32_t stage_start_time = 0;
    // 各阶段持续时间(ms): 准备, 校准, 一次加压, 二次加压, 加热, 保压保温, 冷却, 卸压, 完成
    uint32_t stage_duration[] = {0, 1000, 8000, 10000, 15000, 20000, 10000, 5000, 0}; 
    float target_temperature = 200.0f; // 目标温度
    float target_pressure = 5.0f;      // 目标压力
    float current_temperature = 25.0f;  // 当前温度
    float current_pressure = 0.0f;      // 当前压力
    uint8_t motors_running = 0;         // 电机运行状态
    uint8_t process_confirmed = 0;      // 工艺确认标志
    
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
                    g_sys_params.state = PROCESS_HEATING; // 更新系统状态
                    
                    // 启动所有电机，设置为反向旋转进行校准
                    for(uint8_t i = 0; i < 3; i++) {
                        Motor_SetDir(i, DIR_CCW);    // 设置方向为逆时针（反方向）
                        Motor_SetSpeed(i, 100);      // 设置校准速度
                        Motor_Run(i);                // 启动电机
                    }
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
                    
                    // 显示停止信息
                    OLED_Clear();
                    OLED_ShowString(0, 0, "紧急停止", OLED_8X16);
                    OLED_ShowString(0, 16, "所有电机已停止", OLED_8X16);
                    OLED_Update();
                    HAL_Delay(1500);
                    
                    // 返回准备状态
                    current_stage = STAGE_READY;
                    g_sys_params.state = PROCESS_IDLE;
                    process_confirmed = 0;
                    oled_update_needed = 1;
                }
                break;
        }
        
        // 根据当前工艺阶段执行相应的逻辑
        if(motors_running) {
            uint32_t current_time = HAL_GetTick();
            uint32_t elapsed_time = current_time - stage_start_time;
            
            // 阶段转换逻辑
            switch(current_stage) {
                case STAGE_CALIBRATION:
                    // 校准阶段 - 电机反向旋转1秒回到原点
                    // 更新进度
                    g_process_params.progress = (float)elapsed_time / stage_duration[STAGE_CALIBRATION] * 10.0f;
                    
                    // 校准时间结束，进入一次加压阶段
                    if(elapsed_time >= stage_duration[STAGE_CALIBRATION]) {
                        current_stage = STAGE_PRESS_FAST;
                        stage_start_time = current_time;
                        g_sys_params.state = PROCESS_PRESSING;
                        
                        // 校准完成，调整电机方向和速度为快速加压
                        for(uint8_t i = 0; i < 3; i++) {
                            Motor_SetDir(i, DIR_CW);     // 设置方向为顺时针
                            Motor_SetSpeed(i, 500);      // 设置为高速
                            Motor_Run(i);                // 启动电机
                        }
                    }
                    break;
                    
                case STAGE_PRESS_FAST:
                    // 一次加压阶段 - 快速提升压力
                    // 模拟压力上升 (达到目标压力的70%)
                    current_pressure = 0.0f + (target_pressure * 0.7f) * 
                                      ((float)elapsed_time / stage_duration[STAGE_PRESS_FAST]);
                    
                    // 更新进度
                    g_process_params.progress = 10.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_PRESS_FAST] * 15.0f;
                    
                    // 阶段时间到，或者压力达到目标值的70%，进入二次加压阶段
                    if(elapsed_time >= stage_duration[STAGE_PRESS_FAST] || 
                       current_pressure >= target_pressure * 0.7f) {
                        current_stage = STAGE_PRESS_PRECISE;
                        stage_start_time = current_time;
                        
                        // 调整电机速度为低速，提高精度
                        for(uint8_t i = 0; i < 3; i++) {
                            Motor_SetSpeed(i, 100); // 降低速度，提高精度
                        }
                    }
                    break;
                    
                case STAGE_PRESS_PRECISE:
                    // 二次加压阶段 - 精确加压到目标值
                    // 模拟精确加压
                    current_pressure = target_pressure * 0.7f + (target_pressure * 0.3f) * 
                                      ((float)elapsed_time / stage_duration[STAGE_PRESS_PRECISE]);
                    
                    // 更新进度
                    g_process_params.progress = 25.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_PRESS_PRECISE] * 10.0f;
                    
                    // 阶段时间到，或者压力达到目标值，进入加热阶段
                    if(elapsed_time >= stage_duration[STAGE_PRESS_PRECISE] || 
                       current_pressure >= target_pressure) {
                        current_stage = STAGE_HEAT;
                        stage_start_time = current_time;
                        g_sys_params.state = PROCESS_HEATING;
                        
                        // 保持电机运行但只是为了维持压力，不再增加压力
                        for(uint8_t i = 0; i < 3; i++) {
                            Motor_SetSpeed(i, 50); // 设置为最低速度，仅维持压力
                        }
                    }
                    break;
                    
                case STAGE_HEAT:
                    // 加热阶段 - 温度上升，保持压力
                    // 模拟温度上升
                    current_temperature = 25.0f + (target_temperature - 25.0f) * 
                                         ((float)elapsed_time / stage_duration[STAGE_HEAT]);
                    
                    // 保持压力
                    current_pressure = target_pressure;
                    
                    // 更新进度
                    g_process_params.progress = 35.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_HEAT] * 15.0f;
                    
                    // 阶段时间到，或者温度达到目标值，进入保温保压阶段
                    if(elapsed_time >= stage_duration[STAGE_HEAT] || 
                       current_temperature >= target_temperature) {
                        current_stage = STAGE_HOLD;
                        stage_start_time = current_time;
                        g_sys_params.state = PROCESS_TRANSFORM;
                    }
                    break;
                    
                case STAGE_HOLD:
                    // 保温保压阶段 - 保持温度和压力
                    current_temperature = target_temperature;
                    current_pressure = target_pressure;
                    
                    // 更新进度
                    g_process_params.progress = 50.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_HOLD] * 20.0f;
                    
                    // 阶段时间到，进入冷却阶段
                    if(elapsed_time >= stage_duration[STAGE_HOLD]) {
                        current_stage = STAGE_COOL;
                        stage_start_time = current_time;
                        g_sys_params.state = PROCESS_COOLING;
                    }
                    break;
                    
                case STAGE_COOL:
                    // 冷却阶段 - 温度下降，保持压力
                    // 模拟温度下降
                    current_temperature = target_temperature - (target_temperature - 25.0f) * 
                                         ((float)elapsed_time / stage_duration[STAGE_COOL]);
                    
                    // 保持压力
                    current_pressure = target_pressure;
                    
                    // 更新进度
                    g_process_params.progress = 70.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_COOL] * 15.0f;
                    
                    // 阶段时间到，或者温度降到接近初始值，进入卸压阶段
                    if(elapsed_time >= stage_duration[STAGE_COOL] || 
                       current_temperature <= 30.0f) {
                        current_stage = STAGE_RELEASE;
                        stage_start_time = current_time;
                        g_sys_params.state = PROCESS_IDLE;
                        
                        // 开始卸压，电机反转
                        for(uint8_t i = 0; i < 3; i++) {
                            Motor_SetDir(i, DIR_CCW);    // 设置方向为逆时针（反方向）
                            Motor_SetSpeed(i, 300);      // 设置为中速卸压
                            Motor_Run(i);                // 启动电机
                        }
                    }
                    break;
                    
                case STAGE_RELEASE:
                    // 卸压阶段 - 释放压力
                    // 模拟压力下降
                    current_pressure = target_pressure - target_pressure * 
                                      ((float)elapsed_time / stage_duration[STAGE_RELEASE]);
                    
                    // 更新进度
                    g_process_params.progress = 85.0f + 
                                              (float)elapsed_time / stage_duration[STAGE_RELEASE] * 15.0f;
                    
                    // 阶段时间到，或者压力降到接近0，完成工艺
                    if(elapsed_time >= stage_duration[STAGE_RELEASE] || 
                       current_pressure <= 0.1f) {
                        current_stage = STAGE_COMPLETE;
                        g_sys_params.state = PROCESS_IDLE;
                        g_process_params.progress = 100.0f;
                        
                        // 停止所有电机
                        Motor_EmergencyStop();
                        motors_running = 0;
                        
                        // 显示完成信息
                        OLED_Clear();
                        OLED_ShowString(0, 0, "工艺完成", OLED_8X16);
                        OLED_ShowString(0, 16, "按任意键返回", OLED_8X16);
                        OLED_Update();
                        HAL_Delay(2000);
                    }
                    break;
                    
                default:
                    break;
            }
            
            // 更新系统参数用于显示
            g_process_params.temperature = current_temperature;
            g_process_params.pressure[0] = current_pressure;
        }
        
        // 更新显示
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
            // 每次循环都重绘界面，因为工艺参数需要实时更新
            OLED_Clear();
            
            // 显示状态栏
            display_status_bar();
            
            // 显示3D模型
            display_3d_model();
            
            // 显示阶段信息
            const char *stage_names[] = {"准备中", "校准中", "一次加压", "二次加压", 
                                         "加热中", "保温保压", "冷却中", "卸压中", "已完成"};
            OLED_ShowString(0, 0, (char*)stage_names[current_stage], OLED_8X16);
            
            // 显示压力条形图
            display_pressure_bars();
            
            // 显示温度
            display_temperature();
            
            // 显示进度
            display_progress();
            
            // 更新粒子效果 - 仅在工艺运行时显示
            if(motors_running) {
                update_particle_effects();
            }
            
            // 显示电机状态
            if(motors_running) {
                char motor_info[20];
                uint32_t steps = Motor_GetStepCount(0);
                snprintf(motor_info, sizeof(motor_info), "M:%lu", steps);
                OLED_ShowString(80, 0, motor_info, OLED_8X16);
            }
            
            // 直接更新，因为此界面内容总是在变化
            OLED_Update();
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
        {"系统状态", menu_motor_status}
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
 * @brief  主菜单入口 - 向后兼容的函数
 * @retval int 返回值说明
 */
int menu_main(void)
{
    menu1(); // 直接调用而不是返回
    return 0; // 永远不会执行到这里
}

/**
 * @brief 显示有效数据值
 * 注：此函数未使用，暂时注释掉以消除编译警告
 */
#if 0
static void display_value_with_animation(float value, uint8_t x, uint8_t y) {
    static uint8_t animation_step = 0;
    char value_str[16];
    
    // 根据状态选择显示效果
    if(g_sys_params.alert == ALERT_CRITICAL) {
        if(animation_step++ % 2) {
            // OLED_InvertRect未实现
            // OLED_InvertRect(x, y, x+48, y+16);
        }
    }
    
    snprintf(value_str, sizeof(value_str), "%.1f", value);
    OLED_ShowString(x, y, value_str, OLED_8X16);
}
#endif

/**
 * @brief 更新系统状态
 * 注：此函数未使用，暂时注释掉以消除编译警告
 */
#if 0
static void update_system_state(void) {
    static uint32_t last_check = 0;
    uint32_t current_time = HAL_GetTick();
    
    if(current_time - last_check >= REFRESH_RATE_MS) {
        // 读取压力和温度
        g_sys_params.pressure = Get_Pressure();
        g_sys_params.temperature = Get_Temperature();
        
        // 状态判断
        if(g_sys_params.pressure > MAX_PRESSURE * 0.9f) {
            g_sys_params.alert = ALERT_CRITICAL;
            g_sys_params.alert_count++;
            show_warning_effect();
        } else if(g_sys_params.pressure > MAX_PRESSURE * 0.8f) {
            g_sys_params.alert = ALERT_WARNING;
        } else {
            g_sys_params.alert = ALERT_NONE;
        }
        
        // 新状态 - 电机状态
        Motor_State status0 = Motor_GetState(0);
        Motor_State status1 = Motor_GetState(1);
        Motor_State status2 = Motor_GetState(2);
        
        // 如果有一个状态为MOTOR_ERROR则系统状态为错误
        if(status0 == MOTOR_ERROR || status1 == MOTOR_ERROR || status2 == MOTOR_ERROR) {
            g_sys_params.motor_status = MOTOR_ERROR;
        }
        // 如果有一个状态为MOTOR_RUNNING则系统状态为运行
        else if(status0 == MOTOR_RUNNING || status1 == MOTOR_RUNNING || status2 == MOTOR_RUNNING) {
            g_sys_params.motor_status = MOTOR_RUNNING;
        }
        // 系统状态为停止
        else {
            g_sys_params.motor_status = MOTOR_STOP;
        }
        
        last_check = current_time;
    }
}
#endif

/**
 * @brief 切换白天/夜晚显示模式
 * 注：此函数未使用，暂时注释掉以消除编译警告
 */
#if 0
static void toggle_night_mode(void) {
    static DisplayMode mode = DISPLAY_MODE_NORMAL;
    mode = (mode == DISPLAY_MODE_NIGHT) ? DISPLAY_MODE_NORMAL : DISPLAY_MODE_NIGHT;
    
    // OLED对比度自动调整未实现
    /*if(mode == DISPLAY_MODE_NIGHT) {
        OLED_SetContrast(0x20); // 自动调低
    } else {
        OLED_SetContrast(0xFF); // 自动调高
    }*/
}
#endif
