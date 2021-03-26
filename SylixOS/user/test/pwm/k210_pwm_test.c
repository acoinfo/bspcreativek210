/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_pwm_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 3 ��
**
** ��        ��: K210 ������ PWM �������Գ���
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/pwm/k210_pwm.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
 ȫ�ֱ�������
*********************************************************************************************************/
#define TIMER_PWM           1
#define TIMER_PWM_CHN       0
/*********************************************************************************************************
** ��������: pthread_test
** ��������: PWM �������� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test (VOID  *pvArg)
{
    INT             iFd;
    pwm_ctrl_msg_t  myMsg;

    fpioa_set_function(24, FUNC_TIMER1_TOGGLE1);                        /*    IO_24  =====>   PWM1      */

    iFd = open("/dev/pwm1", O_RDWR, 0666);
    if (iFd < 0) {
        printf("failed to open /dev/pwm1\n");
        return  (LW_NULL);
    }

    myMsg.duty      =   0.5;
    myMsg.frequency =   200000;
    myMsg.uiChannel =   TIMER_PWM_CHN;
    ioctl(iFd, SET_PWM_ACTIVE_PERCENT, &myMsg);
    ioctl(iFd, SET_PWM_CHANNEL_ENABLE, TIMER_PWM_CHN);

    printf("turn light to more darkness\n");
    sleep(3);
    ioctl(iFd, SET_PWM_CHANNEL_DISABLE, TIMER_PWM_CHN);
    myMsg.duty      =   0.2;
    myMsg.frequency =   200000;
    myMsg.uiChannel =   TIMER_PWM_CHN;
    ioctl(iFd, SET_PWM_ACTIVE_PERCENT, &myMsg);
    ioctl(iFd, SET_PWM_CHANNEL_ENABLE, TIMER_PWM_CHN);

    printf("turn light to more brightness\n");
    sleep(3);
    ioctl(iFd, SET_PWM_CHANNEL_DISABLE, TIMER_PWM_CHN);
    myMsg.duty      =   0.8;
    myMsg.frequency =   200000;
    myMsg.uiChannel =   TIMER_PWM_CHN;
    ioctl(iFd, SET_PWM_ACTIVE_PERCENT, &myMsg);
    ioctl(iFd, SET_PWM_CHANNEL_ENABLE, TIMER_PWM_CHN);

    close(iFd);

    return  (NULL);
}
/*********************************************************************************************************
** ��������: pwmTestStart
** ��������: PWM ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  pwmTestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
