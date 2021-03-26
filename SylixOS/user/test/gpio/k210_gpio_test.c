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
** ��   ��   ��: k210_gpio_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 25 ��
**
** ��        ��: K210 ������ GPIO �������Գ���
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include "sys/select.h"
#include "sys/gpiofd.h"
#include "driver/gpio/k210_gpio.h"

#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __TARGET_FPIOA_IO_24       (24)
#define __USE_FUNC_GPIO_3          (3)
/*********************************************************************************************************
** ��������: pthread_test_led
** ��������: LED ���� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test_extern_gpio(VOID  *pvArg)
{
    UINT   io_number;

    fpioa_set_function(__TARGET_FPIOA_IO_24, FUNC_GPIO3);               /*  �����߼�fpioa�����ӿ�       */
    io_number = fpioa_get_io_by_function(FUNC_GPIO3);
    printf("gpio-test: io_number = %d\n", io_number);

    API_GpioRequestOne(__USE_FUNC_GPIO_3, LW_GPIOF_OUT_INIT_HIGH, "Kendryte K210 GPIO");
    API_GpioDirectionOutput(__USE_FUNC_GPIO_3, 1);
    API_GpioSetPull(__USE_FUNC_GPIO_3, 0);

    while (1) {
        API_GpioSetValue(__USE_FUNC_GPIO_3, 0);
        API_TimeMSleep(500);

        API_GpioSetValue(__USE_FUNC_GPIO_3, 1);
        API_TimeMSleep(500);
    }

    API_GpioFree(__USE_FUNC_GPIO_3);

    return  (NULL);
}
/*********************************************************************************************************
** ��������: gpioTestStart
** ��������: GPIO ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  gpioTestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test_extern_gpio, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


