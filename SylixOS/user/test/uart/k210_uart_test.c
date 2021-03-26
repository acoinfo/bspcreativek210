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
** ��   ��   ��: k210_sio_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 9 ��
**
** ��        ��: K210 ������ͨ�ô��� SIO ��������.
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
  ����
*********************************************************************************************************/
static void io_mux_init(void)
{
    fpioa_set_function(34, FUNC_UART1_RX);
    fpioa_set_function(35, FUNC_UART1_TX);
    fpioa_set_function(24, FUNC_GPIOHS3);
}
/*********************************************************************************************************
** ��������: pthread_test_recv
** ��������: SIO ���� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT              iFd;

    gpio_pin_value_t value = GPIO_PV_HIGH;
    int              num   = 0;
    char            *tag   = "hello world! abbbcd\n";
    char             input[20] = {0};

    /*
     * ע��: ʹ�øò��Գ���ʱ��Ӧʹ��һ���Ű����� RX(IO_34) �� TX(IO_35) �������ӡ�
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
** ��������: sioTestStart
** ��������: ���� SIO ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
