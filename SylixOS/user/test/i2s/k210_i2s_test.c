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
** ��   ��   ��: k210_i2s_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 5 ��
**
** ��        ��: K210 ������ I2S �������Գ���
*********************************************************************************************************/
#include "pthread.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "KendryteWare/include/i2s.h"
#include "KendryteWare/include/fpioa.h"
#include "driver/clock/k210_clock.h"
#include "driver/i2s/k210_i2s.h"
/*********************************************************************************************************
 ȫ�ֱ������弰����
*********************************************************************************************************/
#define FRAME_LEN   128

uint32_t rx_buf[1024];
uint32_t g_index;
uint32_t g_tx_len;
/*********************************************************************************************************
** ��������: io_mux_init
** ��������: I2C fpioa ����ӳ�� 
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static void io_mux_init(){

    fpioa_set_function(36, FUNC_I2S0_IN_D0);
    fpioa_set_function(37, FUNC_I2S0_WS);
    fpioa_set_function(38, FUNC_I2S0_SCLK);

    fpioa_set_function(33, FUNC_I2S2_OUT_D1);
    fpioa_set_function(35, FUNC_I2S2_SCLK);
    fpioa_set_function(34, FUNC_I2S2_WS);
}
/*********************************************************************************************************
** ��������: pthread_test
** ��������: I2C �������� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    UINT                iFdI2s_0, iFdI2s_2;
    INT                 iError;
    i2s_dev_mode_t      i2s_mode;
    i2s_dev_config_t    i2s_config;

    /*
     * ע��: �ò��Գ�����ֲ��������Գ��򣬵�ǰ����û��¼���豸��
     *       �����޷�ʹ�øò��Գ���ʵ��¼���Ͳ��ŵĹ��ܡ�
     */

    io_mux_init();
    printf("I2S0 receive , I2S2 play...\n");

    g_index  = 0;
    g_tx_len = 0;

    iFdI2s_0 = open("/dev/i2s_0", O_RDWR, 0666);
    if (iFdI2s_0 < 0) {
        printf("failed to open /dev/fb0\n");
        return  (LW_NULL);
    }

    iFdI2s_2 = open("/dev/i2s_2", O_RDWR, 0666);
    if (iFdI2s_2 < 0) {
        printf("failed to open /dev/fb0\n");
        return  (LW_NULL);
    }

    //i2s_init(I2S_DEVICE_0, I2S_RECEIVER, 0x3);
    i2s_mode.rxtx_mode    = I2S_RECEIVER;
    i2s_mode.data_channel = 0;
    i2s_mode.channel_mask = 0x3;
    iError = ioctl(iFdI2s_0, IOCTL_I2S_INSTALL_DEV, &i2s_mode);
    if (iError < 0) printf("iFdI2s_0: IOCTL_I2S_INSTALL_DEV: failed! \n");

    //i2s_init(I2S_DEVICE_2, I2S_TRANSMITTER, 0xC);
    i2s_mode.rxtx_mode    = I2S_TRANSMITTER;
    i2s_mode.data_channel = 1;
    i2s_mode.channel_mask = 0xC;
    iError = ioctl(iFdI2s_2, IOCTL_I2S_INSTALL_DEV, &i2s_mode);
    if (iError < 0) printf("iFdI2s_2: IOCTL_I2S_INSTALL_DEV: failed! \n");

    //i2s_rx_channel_config(I2S_DEVICE_0, I2S_CHANNEL_0, ...)
    i2s_config.channel_num       =  I2S_CHANNEL_0;
    i2s_config.word_length       =  RESOLUTION_16_BIT;
    i2s_config.word_select_size  =  SCLK_CYCLES_32;
    i2s_config.trigger_level     =  TRIGGER_LEVEL_4;
    i2s_config.word_mode         =  STANDARD_MODE;
    iError = ioctl(iFdI2s_0, IOCTL_I2S_CHAN_CONFIG, &i2s_config);
    if (iError < 0) printf("iFdI2s_0: IOCTL_I2S_CHAN_CONFIG: failed! \n");

    //i2s_tx_channel_config(I2S_DEVICE_2, I2S_CHANNEL_1, ...)
    i2s_config.channel_num       =  I2S_CHANNEL_1;
    i2s_config.word_length       =  RESOLUTION_16_BIT;
    i2s_config.word_select_size  =  SCLK_CYCLES_32;
    i2s_config.trigger_level     =  TRIGGER_LEVEL_4;
    i2s_config.word_mode         =  RIGHT_JUSTIFYING_MODE;
    iError = ioctl(iFdI2s_2, IOCTL_I2S_CHAN_CONFIG, &i2s_config);
    if (iError < 0) printf("iFdI2s_2: IOCTL_I2S_CHAN_CONFIG: failed! \n");

    //i2s_receive_data_dma(I2S_DEVICE_0, &rx_buf[g_index], FRAME_LEN * 2, DMAC_CHANNEL1);
    read(iFdI2s_0, &rx_buf[g_index], FRAME_LEN * 2);

    printf("goto capture and render.\n");

    while (1) {
        g_index += (FRAME_LEN*2);

        if (g_index >= 1023) {
            g_index = 0;
        }

        //i2s_receive_data_dma(I2S_DEVICE_0, &rx_buf[g_index], FRAME_LEN * 2, DMAC_CHANNEL1);
        read(iFdI2s_0, &rx_buf[g_index], FRAME_LEN * 2);

        if (g_index - g_tx_len >= FRAME_LEN || g_tx_len - g_index >= (1023 - FRAME_LEN * 2)) {

            //i2s_send_data_dma(I2S_DEVICE_2, &rx_buf[g_tx_len], FRAME_LEN * 2, DMAC_CHANNEL0);
            write(iFdI2s_2, &rx_buf[g_tx_len], FRAME_LEN * 2);

            g_tx_len += (FRAME_LEN * 2);

            if (g_tx_len >= 1023) {
                g_tx_len = 0;
            }
        }
    }

    return  (NULL);
}
/*********************************************************************************************************
** ��������: i2sTestStart
** ��������: I2S ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  i2sTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
