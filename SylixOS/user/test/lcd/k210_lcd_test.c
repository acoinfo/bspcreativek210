/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k210_lcd_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 29 日
**
** 描        述: K210 处理器 LCD 驱动测试程序
*********************************************************************************************************/
#include "pthread.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/mman.h"
#include "driver/clock/k210_clock.h"
#include "system/device/graph/gmemDev.h"

#include "driver/clock/k210_clock.h"
#include "driver/lcd/k210_lcd.h"
#include "KendryteWare/include/fpioa.h"
#include "driver/lcd/lcd.h"
#include "driver/lcd/nt35310.h"
/*********************************************************************************************************
** 函数名称: io_set_power
** 功能描述: 设置对应 IO_BANK 的电压值
** 输　入  : VOID
** 输　出  : VOID
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID io_set_power(VOID)
{
    sysctl_power_mode_sel(SYSCTL_POWER_BANK1, POWER_V18);
}
/*********************************************************************************************************
** 函数名称: pthread_test
** 功能描述: LCD 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT                     iFbFd;
    LW_GM_SCRINFO           scrInfo;
    LW_GM_VARINFO           varInfo;
    INT                     iError;
    VOID                   *pvFrameBuffer;

    sysctl_spi0_dvp_data_set(1);
    io_set_power();

    while (1) {

        iFbFd = open("/dev/fb0", O_RDWR, 0666);                         /* 打开显示设备                 */
        if (iFbFd < 0) {
            printf("failed to open /dev/fb0\n");
            return  (LW_NULL);
        }

        iError = ioctl(iFbFd, LW_GM_GET_SCRINFO, &scrInfo);             /* 获得SCR信息                  */
        if (iError < 0) {
            printf("failed to get /dev/fb0 screen info\n");
            close(iFbFd);
            return  (LW_NULL);
        }

        iError = ioctl(iFbFd, LW_GM_GET_VARINFO, &varInfo);             /* 获得VAR信息                  */
        if (iError < 0) {
            printf("failed to get /dev/fb0 var info\n");
            close(iFbFd);
            return  (LW_NULL);
        }

        pvFrameBuffer = (PVOID)scrInfo.GMSI_pcMem;
        if (pvFrameBuffer == LW_NULL) {
            printf("failed to mmap /dev/fb0\n");
            close(iFbFd);
            return  (LW_NULL);
        }

#if 0
        while (1) {
            lib_memset(pvFrameBuffer, 0x00, scrInfo.GMSI_stMemSize);
            lcd_draw_picture(0, 0, 240, 320, pvFrameBuffer);

            sleep(5);

            lib_memset(pvFrameBuffer, 0xFF, scrInfo.GMSI_stMemSize);
            lcd_draw_picture(0, 0, 240, 320, pvFrameBuffer);

            sleep(5);
        }
#endif

#if 1
        lcd_clear(WHITE);
        lcd_draw_string(16, 40, "SylixOS", RED);
        lcd_draw_string(16, 80, "Kendryte K210", BLUE);
        while (1);
#endif

        close(iFbFd);
    }

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: lcdTestStart
** 功能描述: LCD 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  lcdTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
