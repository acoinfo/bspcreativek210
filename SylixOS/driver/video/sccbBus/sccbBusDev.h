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
** ��   ��   ��: sccbBusDev.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 19 ��
**
** ��        ��: sccb ���߹��ص��豸�ṹ.
*********************************************************************************************************/

#ifndef __VIDEOBUSDEV_H
#define __VIDEOBUSDEV_H

#include "sccbBus.h"                                                   /*  VIDEO ����ģ��              */

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  VIDEO BUS �豸����
*********************************************************************************************************/

typedef struct lw_videobus_device {
    PLW_VIDEOBUS_ADAPTER    VIDEOBUSDEV_pvideoBusAdapter;               /*  ���ص�������                */
    LW_LIST_LINE            VIDEOBUSDEV_lineManage;                     /*  �豸������                  */
    atomic_t                VIDEOBUSDEV_atomicUsageCnt;                 /*  �豸ʹ�ü���                */
    CHAR                    VIDEOBUSDEV_cName[LW_CFG_OBJECT_NAME_SIZE]; /*  �豸������                  */
} LW_VIDEOBUS_DEVICE;
typedef LW_VIDEOBUS_DEVICE           *PLW_VIDEOBUS_DEVICE;

/*********************************************************************************************************
  �����������ʹ�õ� API
  ���� API ֻ��������������ʹ��, Ӧ�ó���������Կ����Ѿ��� io ϵͳ�� VIDEO BUS �豸.
*********************************************************************************************************/

LW_API INT                   API_VideoBusLibInit(VOID);

/*********************************************************************************************************
  VIDEO BUS ��������������
*********************************************************************************************************/

LW_API INT                   API_VideoBusAdapterCreate(CPCHAR               pcName,
                                                       PLW_VIDEOBUS_FUNCS   pvideoBusfunc);
LW_API INT                   API_VideoBusAdapterDelete(CPCHAR               pcName);
LW_API PLW_VIDEOBUS_ADAPTER  API_VideoBusAdapterGet(CPCHAR                  pcName);

/*********************************************************************************************************
  VIDEO BUS �豸��������
*********************************************************************************************************/

LW_API PLW_VIDEOBUS_DEVICE   API_VideoBusDeviceCreate(CPCHAR                  pcAdapterName,
                                                      CPCHAR                  pcDeviceName);
LW_API INT                   API_VideoBusDeviceDelete(PLW_VIDEOBUS_DEVICE     pvideoBusDevice);

LW_API INT                   API_VideoBusDeviceUsageInc(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceUsageDec(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceUsageGet(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);

/*********************************************************************************************************
  VIDEO BUS �豸���߿���Ȩ����
*********************************************************************************************************/

LW_API INT                   API_VideoBusDeviceBusRequest(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);
LW_API INT                   API_VideoBusDeviceBusRelease(PLW_VIDEOBUS_DEVICE   pvideoBusDevice);

/*********************************************************************************************************
  VIDEO BUS �豸������Ʋ���
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
