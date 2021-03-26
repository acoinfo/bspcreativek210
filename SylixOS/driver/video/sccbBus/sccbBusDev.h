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
** 文   件   名: sccbBusDev.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 19 日
**
** 描        述: sccb 总线挂载的设备结构.
*********************************************************************************************************/

#ifndef __VIDEOBUSDEV_H
#define __VIDEOBUSDEV_H

#include "sccbBus.h"                                                   /*  VIDEO 总线模型              */

/*********************************************************************************************************
  加入裁剪支持
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  VIDEO BUS 设备类型
*********************************************************************************************************/

typedef struct lw_videobus_device {
    PLW_VIDEOBUS_ADAPTER    VIDEOBUSDEV_pvideoBusAdapter;               /*  挂载的适配器                */
    LW_LIST_LINE            VIDEOBUSDEV_lineManage;                     /*  设备挂载链                  */
    atomic_t                VIDEOBUSDEV_atomicUsageCnt;                 /*  设备使用计数                */
    CHAR                    VIDEOBUSDEV_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  设备的名称                  */
} LW_VIDEOBUS_DEVICE;
typedef LW_VIDEOBUS_DEVICE           *PLW_VIDEOBUS_DEVICE;

/*********************************************************************************************************
  驱动程序可以使用的 API
  以下 API 只能在驱动程序中使用, 应用程序仅仅可以看到已经过 io 系统的 VIDEO BUS 设备.
*********************************************************************************************************/

LW_API INT                   API_VideoBusLibInit(VOID);

/*********************************************************************************************************
  VIDEO BUS 适配器基本操作
*********************************************************************************************************/

LW_API INT                   API_VideoBusAdapterCreate(CPCHAR               pcName,
                                                       PLW_VIDEOBUS_FUNCS   pvideoBusfunc);
LW_API INT                   API_VideoBusAdapterDelete(CPCHAR               pcName);
LW_API PLW_VIDEOBUS_ADAPTER  API_VideoBusAdapterGet(CPCHAR                  pcName);

/*********************************************************************************************************
  VIDEO BUS 设备基本操作
*********************************************************************************************************/

LW_API PLW_VIDEOBUS_DEVICE   API_VideoBusDeviceCreate(CPCHAR                  pcAdapterName,
                                                      CPCHAR                  pcDeviceName);
LW_API INT                   API_VideoBusDeviceDelete(PLW_VIDEOBUS_DEVICE     pvideoBusDevice);

LW_API INT                   API_VideoBusDeviceUsageInc(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceUsageDec(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceUsageGet(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);

/*********************************************************************************************************
  VIDEO BUS 设备总线控制权操作
*********************************************************************************************************/

LW_API INT                   API_VideoBusDeviceBusRequest(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceBusRelease(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);

/*********************************************************************************************************
  VIDEO BUS 设备传输控制操作
*********************************************************************************************************/

LW_API INT                   API_VideoBusDeviceTx(PLW_VIDEOBUS_DEVICE    pvideoBusDevice,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
LW_API INT                   API_VideoBusDeviceRx(PLW_VIDEOBUS_DEVICE    pvideoBusDevice,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
LW_API INT                   API_VideoBusDeviceCtrl(PLW_VIDEOBUS_DEVICE         pvideoBusDevice,
                                                    INT                         iCmd,
                                                    LONG                        lArg);

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __VIDEOBUSDEV_H             */
/*********************************************************************************************************
  END
*********************************************************************************************************/
