/**
 * @file mymath.h
 * @brief 数学计算相关函数头文件
 */
#ifndef __MYMATH_H
#define __MYMATH_H

#include "main.h"

/**
 * @brief 卡尔曼滤波器结构体
 */
typedef struct {
    float x_hat;    // 状态估计值
    float p;        // 估计误差协方差
    float Q;        // 过程噪声协方差
    float R;        // 测量噪声协方差
    float K;        // 卡尔曼增益
} KalmanFilter;

/**
 * @brief  初始化卡尔曼滤波器
 * @param  kf: 卡尔曼滤波器结构体指针
 * @param  Q: 过程噪声协方差(推荐值0.001-0.1)
 * @param  R: 测量噪声协方差(推荐值0.1-5)
 * @retval None
 */
void KalmanFilter_Init(KalmanFilter *kf, float Q, float R);

/**
 * @brief  卡尔曼滤波器更新
 * @param  kf: 卡尔曼滤波器结构体指针
 * @param  measurement: 测量值
 * @retval int16_t 滤波后的值
 */
int16_t KalmanFilter_Update(KalmanFilter *kf, int16_t measurement);

#endif /* __MYMATH_H */
