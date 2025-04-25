/**
 * @file menu.h
 * @brief �˵�ϵͳͷ�ļ� - ����˵��ṹ
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

/* ϵͳ�������� */
#define REFRESH_RATE_MS    100     // ˢ����(ms)
#define WARNING_BLINK_MS   300     // ������˸���ʱ��
#define ANIMATION_DURATION_MS 280  // ��������ʱ�����
#define SCREEN_WIDTH       128     // OLED��Ļ��������
#define SCREEN_HEIGHT      64      // OLED��Ļ�߶�

/* ���ս׶ζ��� - ѹ����ģ��ʹ�� */
typedef enum {
    STAGE_READY,     // ׼���׶�
    STAGE_CALIBRATION, // У׼�׶�
    STAGE_PRESS_FAST, // ��ѹ�׶Σ��Ѻϲ�һ�μ�ѹ�Ͷ��μ�ѹ��
    STAGE_HEAT,      // ���Ƚ׶�
    STAGE_HOLD,      // ���±�ѹ�׶�
    STAGE_COOL,      // ��ȴ�׶�
    STAGE_RELEASE,   // жѹ�׶�
    STAGE_COMPLETE   // ��ɽ׶�
} ProcessStage;

/* �ⲿ���� - �����жϴ��� */
extern ProcessStage current_stage;     // ��ǰ���ս׶�

/**
 * @brief ϵͳ����״̬ö��
 */
typedef enum {
    PROCESS_IDLE,       // ����״̬
    PROCESS_HEATING,    // ���ȹ�����
    PROCESS_PRESSING,   // ��ѹ������
    PROCESS_TRANSFORM,  // ת��������
    PROCESS_COOLING,    // ��ȴ������
    PROCESS_ERROR       // ����״̬
} ProcessState;

/**
 * @brief ϵͳ�澯����ö��
 */
typedef enum {
    ALERT_NONE,         // �޸澯
    ALERT_WARNING,      // ����
    ALERT_CRITICAL      // ����
} AlertLevel;

/**
 * @brief ��ʾģʽö��
 */
typedef enum {
    DISPLAY_MODE_NORMAL,    // ����ģʽ
    DISPLAY_MODE_NIGHT,     // ҹ��ģʽ
    DISPLAY_MODE_DEMO       // ��ʾģʽ
} DisplayMode;

/**
 * @brief ���մ��������ṹ����
 */
typedef struct {
    float pressure[6];      // ���׶�ѹ���趨ֵ
    float temperature;      // �¶�ֵ
    float progress;         // ���ս��Ȱٷֱ�(0-100%)
    uint32_t runtime;       // ������ʱ��(��)
    ProcessState state;     // ����״̬
    DisplayMode mode;       // ��ʾģʽ
} ProcessParams;

/* �˵���ṹ */
typedef struct {
    const char *text;      // �˵�����ʾ�ı�
    int (*handler)(void);  // �˵��������ָ��
} MenuItem;

/* �˵��������� - ֻ����ʵ��ʹ�õĺ��� */

/* ===== ���˵��ͻ������� ===== */
int menu1(void);             // �������������˵�ѡ�񣬷���ѡ��Ĳ˵����
int menu2(void);             // �����˵�������ѡ����벻ͬ�Ĺ������ý���
int menu_process(void);      // �������̼�أ���ʾ��ǰ�������̼�3Dģ��
int menu_history(void);      // ��ʷ��¼�˵����鿴��ȥ���������ݺ�ϵͳ��־

/* ===== �����˵������� ===== */
int menu3_PressureSet(void);    // ѹ���������ý���
int menu3_TemperatureSet(void); // �¶Ȳ������ý���
int menu3_TimeSet(void);        // ʱ��������ý���
int menu3_ProcessSelect(void);  // ����ѡ����棬��ͬ���չ���
int menu3_DataRecord(void);     // ���ݼ�¼���棬�鿴��ʷ��������
int menu3_SystemConfig(void);   // ϵͳ���ý��棬����ϵͳ��������
int menu_motor_status(void);    // ���״̬���棬���Ƶ�����кͲ����趨
int menu3_MotorCalibration(void); // ���У׼����

/* ===== �ļ��˵������� ===== */
int menu4_Process1(void);       // ����1��������
int menu4_Process2(void);       // ����2��������
int menu4_Process3(void);       // ����3��������
int menu4_CalibrationP(void);   // ѹ��������У׼
int menu4_CalibrationT(void);   // �¶ȴ�����У׼
int menu4_AlarmConfig(void);    // �������ã�����������ֵ�ͷ�ʽ
int menu4_SystemInfo(void);     // ϵͳ��Ϣ���棬��ʾ�汾����Ϣ

#endif /* __MENU_H__ */
