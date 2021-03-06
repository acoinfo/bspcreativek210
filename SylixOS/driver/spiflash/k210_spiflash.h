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
** 文   件   名: k210_spiflash.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 8 日
**
** 描        述: SPI Flash (w25qxx) 驱动头文件
*********************************************************************************************************/
#ifndef __K210_SPIFLASH_H
#define __K210_SPIFLASH_H
/*********************************************************************************************************
** 函数名称: spiFlashDevCreate
** 功能描述: 向适配器挂载SPI接口设备
** 输　入  : pSpiBusName     SPI适配器名称
**           uiSSPin         GPIO 序号
** 输　出  :
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  spiFlashDevCreate(PCHAR  pSpiBusName, UINT  uiSSPin);
/*********************************************************************************************************
** 函数名称: spiFlashDrvInstall
** 功能描述: SPI_NOR 驱动安装
** 输  入  : uiOffsetByts   实际使用时, 偏移(保留)的字节数
** 输  出  : ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  spiFlashDrvInstall(CPCHAR  pcName, UINT  ulOffsetBytes);

#endif                                                                  /*      __K210_SPIFLASH_H       */
/*********************************************************************************************************
  END
*********************************************************************************************************/
