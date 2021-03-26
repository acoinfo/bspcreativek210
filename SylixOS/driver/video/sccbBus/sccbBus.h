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
** ��   ��   ��: sccbBus.h
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 19 ��
**
** ��        ��: sccb ����ģ��.
*********************************************************************************************************/

#ifndef __VIDEOBUS_H
#define __VIDEOBUS_H

/*********************************************************************************************************
  ����ü�֧��
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

/*********************************************************************************************************
  VIDEO ���ߴ��������Ϣ:  ���������ݴ�����������������׶Σ�ÿһ���׶ΰ���9λ���ݣ�����
                           ��8λΪ��Ҫ��������ݣ����λ����������������в�ͬ��ȡֵ.
*********************************************************************************************************/

typedef struct lw_videobus_message {
    UINT8               VIDEOMSG_uiSlaveAddress;                        /*  SCCB �����ϴӻ��ĵ�ַ       */
    UINT                VIDEOMSG_uiIdRegByteWidth;                      /*  �ӻ��ϼĴ�����ŵ��ֽڿ�    */
    UINT                VIDEOMSG_uiValueRegByteWidth;                   /*  �ӻ��ϼĴ������ֽڿ�        */
    UINT32              VIDEOMSG_uiLen;                                 /*  ����(��������С)            */
    UINT8              *VIDEOMSG_pucRegIdTable;                         /*  ��д������                  */
    UINT8              *VIDEOMSG_pucRegValueTable;                      /*  ��д������                  */
    VOIDFUNCPTR         VIDEOMSG_pfuncComplete;                         /*  ���������Ļص�����        */
    PVOID               VIDEOMSG_pvContext;                             /*  �ص���������                */
} LW_VIDEOBUS_MESSAGE;
typedef LW_VIDEOBUS_MESSAGE    *PLW_VIDEOBUS_MESSAGE;

/*********************************************************************************************************
  VIDEO ����������
*********************************************************************************************************/

struct lw_videobus_funcs;
typedef struct lw_videobus_adapter {
    LW_BUS_ADAPTER              VIDEOBUSADAPTER_pbusAdapter;            /*  ���߽ڵ�                    */
    struct lw_videobus_funcs   *VIDEOBUSADAPTER_pvideoBusfunc;          /*  ������������������          */
    
    LW_OBJECT_HANDLE            VIDEOBUSADAPTER_hBusLock;               /*  ���߲�����                  */
    
    LW_LIST_LINE_HEADER         VIDEOBUSADAPTER_plineDevHeader;         /*  �豸����                    */
} LW_VIDEOBUS_ADAPTER;
typedef LW_VIDEOBUS_ADAPTER    *PLW_VIDEOBUS_ADAPTER;

/*********************************************************************************************************
  VIDEO ���ߴ��亯����
*********************************************************************************************************/

typedef struct lw_videobus_funcs {
    INT             (*VIDEOBUSFUNC_pfuncMasterTx)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
                                                                        /*  ���������ݴ���              */
    INT             (*VIDEOBUSFUNC_pfuncMasterRx)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                  PLW_VIDEOBUS_MESSAGE   pvideoBusmsg,
                                                  INT                    iNum);
                                                                        /*  ���������ݴ���              */
    INT             (*VIDEOBUSFUNC_pfuncMasterCtrl)(PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                                                    INT                    iCmd,
                                                    LONG                   lArg);
                                                                        /*  ����������                  */
} LW_VIDEOBUS_FUNCS;
typedef LW_VIDEOBUS_FUNCS       *PLW_VIDEOBUS_FUNCS;

#endif                                                                  /*  LW_CFG_DEVICE_EN            */
#endif                                                                  /*  __VIDEOBUS_H                */
/*********************************************************************************************************
  END
*********************************************************************************************************/
