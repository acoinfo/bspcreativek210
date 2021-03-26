/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k210_pwm_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 3 日
**
** 描        述: K210 处理器 PWM 驱动测试程序
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
 全局变量定义
*********************************************************************************************************/
#define TIMER_PWM           1
#define TIMER_PWM_CHN       0
/*********************************************************************************************************
** 函数名称: pthread_test
** 功能描述: PWM 驱动测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
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
** 函数名称: pwmTestStart
** 功能描述: PWM 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
