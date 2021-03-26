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
** 文   件   名: k210_sio_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 9 日
**
** 描        述: K210 处理器通用串口 SIO 驱动测试.
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include <stdio.h>
#include <string.h>
#include "uart.h"
#include "gpiohs.h"
#include "fpioa.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
static void io_mux_init(void)
{
    fpioa_set_function(34, FUNC_UART1_RX);
    fpioa_set_function(35, FUNC_UART1_TX);
    fpioa_set_function(24, FUNC_GPIOHS3);
}
/*********************************************************************************************************
** 函数名称: pthread_test_recv
** 功能描述: SIO 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT              iFd;

    gpio_pin_value_t value = GPIO_PV_HIGH;
    int              num   = 0;
    char            *tag   = "hello world! abbbcd\n";
    char             input[20] = {0};

    /*
     * 注意: 使用该测试程序时，应使用一根杜邦线向 RX(IO_34) 和 TX(IO_35) 端相连接。
     */

    io_mux_init();

    gpiohs_set_drive_mode(3, GPIO_DM_OUTPUT);
    gpiohs_set_pin(3, value);

    iFd = open("/dev/ttyS1", O_RDWR, 0);
    num = write(iFd, tag, strlen(tag));
    printf("\nwrite[%d]: %s", num, tag);
    num = read(iFd, input, strlen(tag));
    printf("read[%d]: %s\n", num, input);

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: sioTestStart
** 功能描述: 启动 SIO 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  uartTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, 0);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
