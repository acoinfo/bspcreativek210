/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                       SylixOS(TM)
**
**                               Copyright  All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_lcd_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 29 ��
**
** ��        ��: K210 ������ LCD �������Գ���
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
** ��������: io_set_power
** ��������: ���ö�Ӧ IO_BANK �ĵ�ѹֵ
** �䡡��  : VOID
** �䡡��  : VOID
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID io_set_power(VOID)
{
    sysctl_power_mode_sel(SYSCTL_POWER_BANK1, POWER_V18);
}
/*********************************************************************************************************
** ��������: pthread_test
** ��������: LCD ���� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
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

        iFbFd = open("/dev/fb0", O_RDWR, 0666);                         /* ����ʾ�豸                 */
        if (iFbFd < 0) {
            printf("failed to open /dev/fb0\n");
            return  (LW_NULL);
        }

        iError = ioctl(iFbFd, LW_GM_GET_SCRINFO, &scrInfo);             /* ���SCR��Ϣ                  */
        if (iError < 0) {
            printf("failed to get /dev/fb0 screen info\n");
            close(iFbFd);
            return  (LW_NULL);
        }

        iError = ioctl(iFbFd, LW_GM_GET_VARINFO, &varInfo);             /* ���VAR��Ϣ                  */
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
** ��������: lcdTestStart
** ��������: LCD ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
