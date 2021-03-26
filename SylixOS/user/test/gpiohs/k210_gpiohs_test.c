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
** 文   件   名: k210_gpiohs_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 26 日
**
** 描        述: K210 处理器 GPIOHS 驱动测试程序
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include "sys/select.h"
#include "sys/gpiofd.h"
#include "driver/gpiohs/k210_gpiohs.h"

#include "KendryteWare/include/gpiohs.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define PIN_LED         (25)
#define PIN_KEY         (26)
#define GPIO_LED        KENDRYTE_GET_GPIOHS_ID(3)
#define GPIO_KEY        KENDRYTE_GET_GPIOHS_ID(2)
/*********************************************************************************************************
** 函数名称: irq_gpiohs2
** 功能描述: GPIOHS 中断服务函数
** 输　入  : gp         中断参数
**           irq        中断号
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
irqreturn_t irq_gpiohs2(void* gp, unsigned int irq)
{
    if (API_GpioSvrIrq(GPIO_KEY)) {
        _PrintFormat("IRQ The PIN is %d\r\n", API_GpioGetValue(GPIO_KEY));
    }

    API_GpioClearIrq(GPIO_KEY);

    return (LW_IRQ_HANDLED);
}
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
    ULONG         irqVector;
    UINT          value = 1;
    UINT          io_number;
    INT           iValue;

    fpioa_set_function(PIN_LED, FUNC_GPIOHS3);
    io_number = fpioa_get_io_by_function(FUNC_GPIOHS3);
    printf("gpiohs-test: io_number = %d\n", io_number);

    API_GpioRequestOne(GPIO_LED, LW_GPIOF_OUT_INIT_LOW, "Kendryte K210 GPIOHS");
    API_GpioDirectionOutput(GPIO_LED, value);

    fpioa_set_function(PIN_KEY, FUNC_GPIOHS2);
    io_number = fpioa_get_io_by_function(FUNC_GPIOHS2);
    printf("gpiohs-test: io_number = %d\n", io_number);

    API_GpioRequestOne(GPIO_KEY, LW_GPIOF_OUT_INIT_LOW, "Kendryte K210 GPIOHS");
    API_GpioDirectionInput(GPIO_KEY);

    irqVector = API_GpioSetupIrq(GPIO_KEY, 0, GPIO_PE_FALLING);
    if (irqVector == LW_VECTOR_INVALID) {
        printf("error gpio\n");
    }
    printf("irqVector = %ld\n", irqVector);

    API_InterVectorConnect(irqVector, (PINT_SVR_ROUTINE)irq_gpiohs2, LW_NULL, "key_isr");
    API_InterVectorEnable(irqVector);

    while (1) {

        sleep(1);
        API_GpioSetValue(GPIO_LED, value = !value);

        iValue = API_GpioGetValue(GPIO_KEY);
        printf("The PIN is %d\n", iValue);
    }

    API_GpioFree(GPIO_LED);
    API_GpioFree(GPIO_KEY);

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: gpiohsTestStart
** 功能描述: GPIOHS 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  gpiohsTestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test_extern_gpio, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
