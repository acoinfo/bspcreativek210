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
** 文   件   名: k210_gpio_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 25 日
**
** 描        述: K210 处理器 GPIO 驱动测试程序
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
  定义
*********************************************************************************************************/
#define __TARGET_FPIOA_IO_24       (24)
#define __USE_FUNC_GPIO_3          (3)
/*********************************************************************************************************
** 函数名称: pthread_test_led
** 功能描述: LED 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test_extern_gpio(VOID  *pvArg)
{
    UINT   io_number;

    fpioa_set_function(__TARGET_FPIOA_IO_24, FUNC_GPIO3);               /*  测试逻辑fpioa驱动接口       */
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
** 函数名称: gpioTestStart
** 功能描述: GPIO 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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


