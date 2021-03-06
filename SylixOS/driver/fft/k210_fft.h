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
** 文   件   名: k210_fft.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 22 日
**
** 描        述: FFT 快速傅里叶变换加速器 驱动头文件
*********************************************************************************************************/
#ifndef __K210_FFT_H
#define __K210_FFT_H
/*********************************************************************************************************
  包含IOCTL命令定义相关头文件
*********************************************************************************************************/
#include <sys/ioccom.h>
/*********************************************************************************************************
  Kendryte K210 FFT 命令配置宏
*********************************************************************************************************/
#define START_FFT_TRANSACTION       _IO('f',0x01)
/*********************************************************************************************************
  包含相关结构体类型的定义: fft_direction_t
*********************************************************************************************************/
#include "KendryteWare/include/fft.h"
/*********************************************************************************************************
 傅立叶变换计算事务结构体 (注意: 为方便调用裸机库，该结构体命名不遵循 SylixOS 命名规范)
*********************************************************************************************************/
typedef struct {
    fft_direction_t     direction;
    const UINT64       *input;
    UINT64             *output;
    size_t              point_num;
} fft_transaction_t;
/*********************************************************************************************************
** 函数名称: k210FFTDrv
** 功能描述: 安装 FFT 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210FFTDrv(VOID);
/*********************************************************************************************************
** 函数名称: k210FFTDevAdd
** 功能描述: 创建 FFT 设备
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210FFTDevAdd(VOID);

#endif                                                                  /*    __K210_FFT_H              */
/*********************************************************************************************************
  END
*********************************************************************************************************/
