# 六面顶步进电机驱动使用手册

<div align="center">
<img src="https://img.shields.io/badge/STM32-F4-blue" alt="STM32 F4">
<img src="https://img.shields.io/badge/步进电机-驱动库-green" alt="步进电机驱动库">
<img src="https://img.shields.io/badge/版本-1.0-orange" alt="版本">
</div>

---

## 📑 目录

- [六面顶步进电机驱动使用手册](#六面顶步进电机驱动使用手册)
  - [📑 目录](#-目录)
  - [1. 简介](#1-简介)
  - [2. 硬件资源分配](#2-硬件资源分配)
    - [电机引脚分配](#电机引脚分配)
    - [定时器分配](#定时器分配)
  - [3. 使用前准备](#3-使用前准备)
  - [4. 基本使用方法](#4-基本使用方法)
  - [5. 实用示例](#5-实用示例)
  - [6. 常见问题与解决方案](#6-常见问题与解决方案)
  - [7. 电机参数说明](#7-电机参数说明)
  - [8. 使用注意事项](#8-使用注意事项)
  - [9. 高级定制](#9-高级定制)

---

<a id="1-简介"></a>
## 1. 简介

<div style="padding: 15px; border-left: 4px solid #5cb85c; background-color: #f1f1f1;">
本驱动库提供了对三个步进电机的完整控制能力，适用于STM32F4系列微控制器。驱动采用定时器中断方式控制步进电机，每个电机通过4个GPIO引脚控制四相（A/B/C/D）输出，采用低电平有效的控制方式。本手册将帮助您了解如何在项目中使用这些API来控制电机。
</div>

---

<a id="2-硬件资源分配"></a>
## 2. 硬件资源分配

### 电机引脚分配
<table>
  <tr>
    <th colspan="2">电机1 (TIM2)</th>
    <th colspan="2">电机2 (TIM3)</th>
    <th colspan="2">电机3 (TIM4)</th>
  </tr>
  <tr>
    <td>motor1_A</td><td>PA10</td>
    <td>motor2_A</td><td>PA0</td>
    <td>motor3_A</td><td>PB3</td>
  </tr>
  <tr>
    <td>motor1_B</td><td>PA11</td>
    <td>motor2_B</td><td>PA1</td>
    <td>motor3_B</td><td>PB4</td>
  </tr>
  <tr>
    <td>motor1_C</td><td>PA12</td>
    <td>motor2_C</td><td>PA2</td>
    <td>motor3_C</td><td>PB5</td>
  </tr>
  <tr>
    <td>motor1_D</td><td>PA15</td>
    <td>motor2_D</td><td>PA3</td>
    <td>motor3_D</td><td>PB6</td>
  </tr>
</table>

### 定时器分配
- **TIM2**: 电机1控制
- **TIM3**: 电机2控制
- **TIM4**: 电机3控制
- **TIM5**: 按键扫描(2ms中断)

---

<a id="3-使用前准备"></a>
## 3. 使用前准备

<details>
<summary><b>👉 点击展开初始化流程</b></summary>

开始使用电机库前，您需要确保：

1. 已正确初始化相关硬件：
   ```c
   // 必要的系统初始化
   HAL_Init();
   SystemClock_Config();
   
   // 初始化外设
   MX_GPIO_Init();
   MX_TIM2_Init();
   MX_TIM3_Init();
   MX_TIM4_Init();
   MX_TIM5_Init();
   
   // 启动TIM5用于按键扫描(如需要)
   HAL_TIM_Base_Start_IT(&htim5);
   
   // 初始化所有电机
   Motor_InitAll();
   ```
</details>

---

<a id="4-基本使用方法"></a>
## 4. 基本使用方法

<div style="display: flex; flex-wrap: wrap; gap: 20px;">

<div style="flex: 1; min-width: 300px; border: 1px solid #ddd; padding: 15px; border-radius: 5px;">
<h3>4.1 启动和停止电机</h3>

```c
// 启动指定ID的电机(ID范围: 0-2)
Motor_Run(0);    // 启动电机1
Motor_Run(1);    // 启动电机2
Motor_Run(2);    // 启动电机3

// 停止指定的电机
Motor_Stop(0);   // 停止电机1

// 紧急停止所有电机
Motor_EmergencyStop();
```
</div>

<div style="flex: 1; min-width: 300px; border: 1px solid #ddd; padding: 15px; border-radius: 5px;">
<h3>4.2 设置电机方向</h3>

```c
// 设置电机1为顺时针方向
Motor_SetDir(0, DIR_CW);

// 设置电机2为逆时针方向
Motor_SetDir(1, DIR_CCW);
```
</div>

</div>

<div style="display: flex; flex-wrap: wrap; gap: 20px; margin-top: 20px;">

<div style="flex: 1; min-width: 300px; border: 1px solid #ddd; padding: 15px; border-radius: 5px;">
<h3>4.3 调整电机速度</h3>

```c
// 设置电机1速度为500步/秒
Motor_SetSpeed(0, 500);

// 设置电机2速度为200步/秒
Motor_SetSpeed(1, 200);
```
</div>

<div style="flex: 1; min-width: 300px; border: 1px solid #ddd; padding: 15px; border-radius: 5px;">
<h3>4.4 获取电机状态和步数</h3>

```c
// 检查电机是否在运行
if (Motor_GetState(0) == MOTOR_RUNNING) {
    // 电机1正在运行
}

// 获取电机当前步数
uint32_t steps = Motor_GetStepCount(0);
```
</div>

</div>

---

<a id="5-实用示例"></a>
## 5. 实用示例

<details>
<summary><b>5.1 简单的电机控制示例</b></summary>

```c
// 初始化电机
Motor_InitAll();

// 设置电机1参数
Motor_SetDir(0, DIR_CW);      // 设置方向为顺时针
Motor_SetSpeed(0, 500);       // 设置速度为500步/秒
Motor_Run(0);                 // 启动电机1

// 运行5秒
HAL_Delay(5000);

// 改变方向
Motor_SetDir(0, DIR_CCW);     // 设置方向为逆时针

// 再运行5秒
HAL_Delay(5000);

// 停止电机
Motor_Stop(0);
```
</details>

<details>
<summary><b>5.2 多电机协同工作</b></summary>

```c
// 初始化所有电机
Motor_InitAll();

// 配置电机1和电机2
Motor_SetDir(0, DIR_CW);
Motor_SetDir(1, DIR_CCW);
Motor_SetSpeed(0, 400);
Motor_SetSpeed(1, 400);

// 同时启动两个电机
Motor_Run(0);
Motor_Run(1);

// 运行一段时间
HAL_Delay(3000);

// 调整电机1速度
Motor_SetSpeed(0, 200);

// 再运行一段时间
HAL_Delay(3000);

// 停止所有电机
Motor_EmergencyStop();
```
</details>

<details>
<summary><b>5.3 步数控制示例</b></summary>

```c
// 使电机运转指定步数
void RotateMotorSteps(uint8_t motor_id, uint32_t steps, Motor_Dir direction) {
    uint32_t initial_steps;
    uint32_t target_steps;
    
    // 设置方向
    Motor_SetDir(motor_id, direction);
    
    // 记录初始步数
    initial_steps = Motor_GetStepCount(motor_id);
    
    // 计算目标步数
    if (direction == DIR_CW) {
        target_steps = initial_steps + steps;
    } else {
        target_steps = initial_steps - steps;
    }
    
    // 启动电机
    Motor_Run(motor_id);
    
    // 等待直到达到目标步数
    while (1) {
        uint32_t current_steps = Motor_GetStepCount(motor_id);
        
        if (direction == DIR_CW && current_steps >= target_steps) {
            break;
        } else if (direction == DIR_CCW && current_steps <= target_steps) {
            break;
        }
        
        // 防止CPU占用过高
        HAL_Delay(10);
    }
    
    // 停止电机
    Motor_Stop(motor_id);
}

// 使用示例
void example(void) {
    // 初始化电机
    Motor_InitAll();
    
    // 电机1顺时针旋转1000步
    RotateMotorSteps(0, 1000, DIR_CW);
    
    HAL_Delay(1000);
    
    // 电机1逆时针旋转500步
    RotateMotorSteps(0, 500, DIR_CCW);
}
```
</details>

---

<a id="6-常见问题与解决方案"></a>
## 6. 常见问题与解决方案

<div style="background-color: #f8f9fa; padding: 15px; border-radius: 5px; margin-bottom: 20px;">
<h3>6.1 电机无法启动</h3>

1. **检查电源**: 确保步进电机的电源电压适当
2. **检查接线**: 确认A/B/C/D四相连接正确
3. **检查代码**: 确认已正确调用`Motor_InitAll()`
4. **检查返回值**: 调用`Motor_Run()`后检查返回值确认没有错误

```c
Motor_Status status = Motor_Run(0);
if (status != MOTOR_OK) {
    // 处理错误情况
    switch (status) {
        case MOTOR_ERR_INVALID_ID:
            // 无效的电机ID
            break;
        case MOTOR_ERR_TIM_START:
            // 定时器启动错误
            break;
        // 处理其他错误...
    }
}
```
</div>

<div style="background-color: #f8f9fa; padding: 15px; border-radius: 5px; margin-bottom: 20px;">
<h3>6.2 电机转动不稳定</h3>

1. **调整速度**: 尝试降低电机速度
   ```c
   Motor_SetSpeed(0, 200); // 设置较低的速度
   ```

2. **检查电源**: 确保电源能提供足够电流

3. **更改相位表**: 如需修改相位顺序，需修改`Motor_PhaseTables`数组
</div>

<div style="background-color: #f8f9fa; padding: 15px; border-radius: 5px;">
<h3>6.3 修改电机最大/最小速度</h3>

如需修改电机的速度范围限制，可在初始化后修改电机结构体内的max_speed和min_speed字段：

```c
// 初始化电机
Motor_InitAll();

// 修改电机1的速度范围
motors[0].max_speed = 2000; // 最大速度改为2000步/秒
motors[0].min_speed = 5;    // 最小速度改为5步/秒
```
</div>

---

<a id="7-电机参数说明"></a>
## 7. 电机参数说明

<div style="display: flex; flex-wrap: wrap; gap: 20px;">

<div style="flex: 1; min-width: 300px; border: 1px solid #eaeaea; padding: 15px; border-radius: 5px; background-color: #fcfcfc;">
<h3>7.1 电机状态码</h3>

```c
typedef enum {
    MOTOR_OK = 0,                 // 正常
    MOTOR_ERR_INVALID_ID,         // 无效电机ID
    MOTOR_ERR_SPEED_RANGE,        // 速度超出范围
    MOTOR_ERR_TIM_START,          // 定时器启动错误
    MOTOR_ERR_ALREADY_RUNNING,    // 电机已在运行
    MOTOR_ERR_NOT_RUNNING         // 电机未运行
} Motor_Status;
```
</div>

<div style="flex: 1; min-width: 300px; border: 1px solid #eaeaea; padding: 15px; border-radius: 5px; background-color: #fcfcfc;">
<h3>7.2 电机运行状态</h3>

```c
typedef enum {
    MOTOR_STOP = 0,               // 停止
    MOTOR_RUNNING,                // 运行中
    MOTOR_ERROR                   // 错误
} Motor_State;
```
</div>

</div>

<div style="margin-top: 20px; border: 1px solid #eaeaea; padding: 15px; border-radius: 5px; background-color: #fcfcfc;">
<h3>7.3 电机旋转方向</h3>

```c
typedef enum {
    DIR_NONE = 0,                 // 无方向
    DIR_CW,                       // 顺时针
    DIR_CCW                       // 逆时针
} Motor_Dir;
```
</div>

---

<a id="8-使用注意事项"></a>
## 8. 使用注意事项

<div style="border-left: 4px solid #f0ad4e; padding-left: 15px;">
<ol>
<li><strong>速度限制</strong>: 电机速度存在最大和最小限制，超出范围将返回错误</li>
<li><strong>电机ID范围</strong>: 电机ID范围为0-2，对应三个电机实例</li>
<li><strong>初始化顺序</strong>: 必须先初始化GPIO和定时器，再调用<code>Motor_InitAll()</code></li>
<li><strong>中断处理</strong>: 电机驱动依赖于定时器中断，请确保不要在中断处理中调用耗时函数</li>
<li><strong>资源占用</strong>: 电机驱动使用TIM2/TIM3/TIM4资源，避免在其他地方重复配置这些定时器</li>
</ol>
</div>

---

<a id="9-高级定制"></a>
## 9. 高级定制

<details>
<summary><b>9.1 修改定时器配置</b></summary>

如果需要修改定时器配置，应该在修改后重新初始化电机：

```c
// 修改定时器配置
// ...

// 然后重新初始化电机
Motor_InitAll();
```
</details>

<details>
<summary><b>9.2 添加更多电机支持</b></summary>

要添加更多电机支持，需要修改以下几个部分：

1. 修改`MOTOR_NUM`定义
2. 扩展相位表`Motor_PhaseTables`数组
3. 扩展`motors`数组
4. 增加相应的定时器中断处理
</details>

<details>
<summary><b>9.3 实现加减速</b></summary>

以下是一个简单的加减速实现示例：

```c
// 实现电机加速过程
void Motor_Accelerate(uint8_t id, uint32_t start_speed, uint32_t target_speed, uint32_t accel_time_ms) {
    uint32_t steps = (target_speed - start_speed) * accel_time_ms / 1000;
    uint32_t delay_ms = accel_time_ms / steps;
    uint32_t current_speed = start_speed;
    
    // 设置初始速度
    Motor_SetSpeed(id, current_speed);
    Motor_Run(id);
    
    // 逐步加速
    for (uint32_t i = 0; i < steps; i++) {
        current_speed += (target_speed - start_speed) / steps;
        Motor_SetSpeed(id, current_speed);
        HAL_Delay(delay_ms);
    }
    
    // 设置最终目标速度
    Motor_SetSpeed(id, target_speed);
}
```
</details>

---

<div align="center">
<p>© 2025 六面顶步进电机驱动库 | 文档版本 v1.0</p>
</div> 