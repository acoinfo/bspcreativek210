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
** 文   件   名: k210_gpio.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 22 日
**
** 描        述: GPIO 控制器驱动头文件
*********************************************************************************************************/
#ifndef __K210_GPIO_H
#define __K210_GPIO_H
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define K210_GPIO_BANK_NUM                (1)
#define K210_GPIO_PINS_PER_BANK           (8)
#define K210_GPIO_NUMBER(bank, gpio)      (K210_GPIO_PINS_PER_BANK * (bank) + (gpio))
/*********************************************************************************************************
** 函数名称: k210GpioDrv
** 功能描述: 安装 Kendryte K210 GPIO 驱动
** 输  入  : NONE
** 输  出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210GpioDrv(VOID);

#endif                                                                  /*       __K210_GPIO_H          */
/*********************************************************************************************************
  END
*********************************************************************************************************/
