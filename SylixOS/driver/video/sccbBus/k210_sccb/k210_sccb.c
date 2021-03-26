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
** 文   件   名: videoAsync.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 20 日
**
** 描        述: sccb 总线驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "config.h"
#include "SylixOS.h"

#include "../sccbBusDev.h"
#include "driver/clock/k210_clock.h"
#include "KendryteWare/include/dvp.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  SCCB 通道结构定义
*********************************************************************************************************/
#define SCCB_PHYADDR_BASE   0x50430000                                  /*  DVP register base address   */
#define SCCB_REG_CFG        0x50430014                                  /*  SCCB 控制寄存器             */
#define SCCB_REG_DATA       0x50430018                                  /*  SCCB 读/写数据寄存器        */
#define __NO_IRQ_VECTOR     (-1)
/*********************************************************************************************************
  SCCB 通道结构定义
*********************************************************************************************************/
typedef struct {
    LW_VIDEOBUS_FUNCS   SCCB_Funcs;                                     /*  必须为第一个                */
    CHAR               *SCCB_pcBusName;                                 /*  总线名称                    */
    INT                 SCCB_iCh;                                       /*  通道号                      */
    addr_t              SCCB_ulPhyAddr;                                 /*  寄存器基地址                */
    UINT32              SCCB_regAddressWidth;                           /*  寄存器位宽                  */
    ULONG               SCCB_ulRxDmaCh;                                 /*  接收向量号                  */
    ULONG               SCCB_ulTxDmaCh;                                 /*  接收向量号                  */
    INT32               SCCB_ulRxVector;                                /*  接收向量号                  */
    INT32               SCCB_ulTxVector;                                /*  接收向量号                  */
    ULONG               SCCB_ulBusClk;                                  /*  当前时钟线频率              */
} SCCB_VIDEO_CHANNEL;
/*********************************************************************************************************
** 函数名称: __sccbTx
** 功能描述: 发送视频消息
** 输    入: pvideoBusAdapter   视频总线适配器
**           pvideoBusmsg       视频消息
**           iNum               消息数量
** 返    回: ERROR_CODE
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
** 函数名称: __sccbRx
** 功能描述: 接收视频消息
** 输    入: pvideoBusAdapter   视频总线适配器
**           pvideoBusmsg       视频消息
**           iNum               消息数量
** 返    回: ERROR_CODE
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
** 函数名称: __sccbCtrl
** 功能描述: 控制视频总线
** 输    入: pvideoBusAdapter   视频总线适配器
**           iCmd               控制命令
**           lParam             控制参数
** 返    回: ERROR_CODE
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
** 函数名称: sccbDrv
** 功能描述: 创建 sai 设备
** 输    入: NONE
** 返    回: ERROR_CODE
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

    fpioa_set_function(10, FUNC_SCCB_SCLK);                             /*  引脚初始化                  */
    fpioa_set_function(9, FUNC_SCCB_SDA);

    API_VideoBusAdapterCreate(sccbVideoBus.SCCB_pcBusName, &sccbVideoBus.SCCB_Funcs);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
