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
** ��   ��   ��: videoAsync.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 20 ��
**
** ��        ��: sccb ��������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"

#include "../sccbBusDev.h"
#include "driver/clock/k210_clock.h"
#include "KendryteWare/include/dvp.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  SCCB ͨ���ṹ����
*********************************************************************************************************/
#define SCCB_PHYADDR_BASE   0x50430000                                  /*  DVP register base address   */
#define SCCB_REG_CFG        0x50430014                                  /*  SCCB ���ƼĴ���             */
#define SCCB_REG_DATA       0x50430018                                  /*  SCCB ��/д���ݼĴ���        */
#define __NO_IRQ_VECTOR     (-1)
/*********************************************************************************************************
  SCCB ͨ���ṹ����
*********************************************************************************************************/
typedef struct {
    LW_VIDEOBUS_FUNCS   SCCB_Funcs;                                     /*  ����Ϊ��һ��                */
    CHAR               *SCCB_pcBusName;                                 /*  ��������                    */
    INT                 SCCB_iCh;                                       /*  ͨ����                      */
    addr_t              SCCB_ulPhyAddr;                                 /*  �Ĵ�������ַ                */
    UINT32              SCCB_regAddressWidth;                           /*  �Ĵ���λ��                  */
    ULONG               SCCB_ulRxDmaCh;                                 /*  ����������                  */
    ULONG               SCCB_ulTxDmaCh;                                 /*  ����������                  */
    INT32               SCCB_ulRxVector;                                /*  ����������                  */
    INT32               SCCB_ulTxVector;                                /*  ����������                  */
    ULONG               SCCB_ulBusClk;                                  /*  ��ǰʱ����Ƶ��              */
} SCCB_VIDEO_CHANNEL;
/*********************************************************************************************************
** ��������: __sccbTx
** ��������: ������Ƶ��Ϣ
** ��    ��: pvideoBusAdapter   ��Ƶ����������
**           pvideoBusmsg       ��Ƶ��Ϣ
**           iNum               ��Ϣ����
** ��    ��: ERROR_CODE
*********************************************************************************************************/
static INT  __sccbTx (PLW_VIDEOBUS_ADAPTER  pvideoBusAdapter,
                      PLW_VIDEOBUS_MESSAGE  pvideoBusmsg,
                      INT                   iNum)
{
    PLW_VIDEOBUS_MESSAGE  pMsg          = pvideoBusmsg;
    UINT16               *pucRegId;
    UINT8                *pucRegValue;

    for (INT index = 0; index < iNum; index++) {

        pucRegId     =  (UINT16 *)pMsg->VIDEOMSG_pucRegIdTable;
        pucRegValue  =  (UINT8  *)pMsg->VIDEOMSG_pucRegValueTable;

        if (!pucRegId || !pucRegValue) {
            printk("__sccbTx(): invalid pvideoBusmsg.\n");
            return  (index);
        }

        for (INT i = 0; i < pMsg->VIDEOMSG_uiLen; ++i) {

            dvp_sccb_send_data(pMsg->VIDEOMSG_uiSlaveAddress, *pucRegId, *pucRegValue);

            ++pucRegId;
            ++pucRegValue;
        }

        ++pMsg;
    }

    return  (iNum);
}
/*********************************************************************************************************
** ��������: __sccbRx
** ��������: ������Ƶ��Ϣ
** ��    ��: pvideoBusAdapter   ��Ƶ����������
**           pvideoBusmsg       ��Ƶ��Ϣ
**           iNum               ��Ϣ����
** ��    ��: ERROR_CODE
*********************************************************************************************************/
static INT  __sccbRx (PLW_VIDEOBUS_ADAPTER  pvideoBusAdapter,
                     PLW_VIDEOBUS_MESSAGE  pvideoBusmsg,
                     INT                   iNum)
{
    PLW_VIDEOBUS_MESSAGE  pMsg          = pvideoBusmsg;
    UINT16               *pucRegId;
    UINT8                *pucRegValue;

    for (INT index = 0; index < iNum; index++) {

        pucRegId     =  (UINT16 *)pMsg->VIDEOMSG_pucRegIdTable;
        pucRegValue  =  (UINT8  *)pMsg->VIDEOMSG_pucRegValueTable;

        if (!pucRegId || !pucRegValue) {
            printk("__sccbRx(): invalid pvideoBusmsg.\n");
            return  (index);
        }

        for (INT i = 0; i < pMsg->VIDEOMSG_uiLen; ++i) {

            //dvp_sccb_send_data(pMsg->VIDEOMSG_uiSlaveAddress, *pucRegId, *pucRegValue);
            *pucRegValue = dvp_sccb_receive_data(pMsg->VIDEOMSG_uiSlaveAddress, *pucRegId);

            ++pucRegId;
            ++pucRegValue;
        }

        ++pMsg;
    }

    return  (iNum);
}
/*********************************************************************************************************
** ��������: __sccbCtrl
** ��������: ������Ƶ����
** ��    ��: pvideoBusAdapter   ��Ƶ����������
**           iCmd               ��������
**           lParam             ���Ʋ���
** ��    ��: ERROR_CODE
*********************************************************************************************************/
static INT  __sccbCtrl (PLW_VIDEOBUS_ADAPTER   pvideoBusAdapter,
                       INT                    iCmd,
                       LONG                   lParam)
{
    /*
     * TODO  nothing need to do now.
     */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: sccbDrv
** ��������: ���� sai �豸
** ��    ��: NONE
** ��    ��: ERROR_CODE
*********************************************************************************************************/
INT  sccbDrv (INT  iChannel)
{
    static  SCCB_VIDEO_CHANNEL  sccbVideoBus = {
        .SCCB_Funcs = {
            __sccbTx,
            __sccbRx,
            __sccbCtrl,
        },
        .SCCB_pcBusName    = "/bus/video/sccb0",
        .SCCB_iCh          = 0,
        .SCCB_ulPhyAddr    = SCCB_PHYADDR_BASE,
        .SCCB_ulRxDmaCh    = 0,
        .SCCB_ulTxDmaCh    = 1,
        .SCCB_ulRxVector   = __NO_IRQ_VECTOR,
        .SCCB_ulTxVector   = __NO_IRQ_VECTOR,
    };

    fpioa_set_function(10, FUNC_SCCB_SCLK);                             /*  ���ų�ʼ��                  */
    fpioa_set_function(9, FUNC_SCCB_SDA);

    API_VideoBusAdapterCreate(sccbVideoBus.SCCB_pcBusName, &sccbVideoBus.SCCB_Funcs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
