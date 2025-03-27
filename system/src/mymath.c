/**
 * @file mymath.c
 * @brief 数学计算相关函数实现
 */
#include "mymath.h"
#include <math.h>

/**
 * @brief  初始化卡尔曼滤波器
 * @param  kf: 卡尔曼滤波器结构体指针
 * @param  Q: 过程噪声协方差(推荐值0.001-0.1)
 * @param  R: 测量噪声协方差(推荐值0.1-5)
 * @retval None
 */
void KalmanFilter_Init(KalmanFilter *kf, float Q, float R)
{
    kf->Q = Q;
    kf->R = R;
    kf->p = 1.0f;    // 初始估计误差协方差
    kf->x_hat = 0.0f; // 初始状态估计值
}

/**
 * @brief  卡尔曼滤波器更新
 * @param  kf: 卡尔曼滤波器结构体指针
 * @param  measurement: 测量值
 * @retval int16_t 滤波后的值
 */
int16_t KalmanFilter_Update(KalmanFilter *kf, int16_t measurement)
{
    /* 预测阶段 */
    float x_hat_minus = kf->x_hat;  // 预测状态估计(系统动态模型为恒值)
    float p_minus = kf->p + kf->Q;  // 预测估计误差协方差

    /* 更新阶段 */
    kf->K = p_minus / (p_minus + kf->R); // 计算卡尔曼增益
    kf->x_hat = x_hat_minus + kf->K * (measurement - x_hat_minus); // 更新后验估计
    kf->p = (1 - kf->K) * p_minus;      // 更新后验误差协方差

    return (int16_t)kf->x_hat; // 返回滤波结果
}
