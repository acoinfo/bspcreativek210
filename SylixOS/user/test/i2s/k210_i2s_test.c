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
** 文   件   名: k210_i2s_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 5 日
**
** 描        述: K210 处理器 I2S 驱动测试程序
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
 全局变量定义及声明
*********************************************************************************************************/
#define FRAME_LEN   128

uint32_t rx_buf[1024];
uint32_t g_index;
uint32_t g_tx_len;
/*********************************************************************************************************
** 函数名称: io_mux_init
** 功能描述: I2C fpioa 引脚映射 
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
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
** 函数名称: pthread_test
** 功能描述: I2C 驱动测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    UINT                iFdI2s_0, iFdI2s_2;
    INT                 iError;
    i2s_dev_mode_t      i2s_mode;
    i2s_dev_config_t    i2s_config;

    /*
     * 注意: 该测试程序移植自裸机测试程序，当前由于没有录音设备，
     *       所以无法使用该测试程序实现录音和播放的功能。
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
** 函数名称: i2sTestStart
** 功能描述: I2S 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
