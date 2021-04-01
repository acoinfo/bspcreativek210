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
** ��   ��   ��: k210_dmac.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 18 ��
**
** ��        ��: DMA ����������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include "driver/clock/k210_clock.h"
#include "driver/dmac/k210_dma.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/dmac.h"
#include "KendryteWare/include/fpioa.h"
#include "KendryteWare/include/plic.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define __DMA_CHANNEL_NR         (6)
#define __DMA_CHANNEL_SEM_FMT    "sem_dma%d"
#define __DMA_CHANNEL_ISR_FMT    "dma%d_isr"
/*********************************************************************************************************
  FIX DMA ���������Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t                  DMAC_ulPhyAddrBase;                         /*  �����ַ����ַ              */
    BOOL                    DMAC_bIsInit;                               /*  DMA �������Ƿ��ѱ���ʼ��    */
    UINT32                  DMAC_uiAxiMaster1Use;                       /*  ��ͨ��ʹ�� axi master 1 ?   */
    UINT32                  DMAC_uiAxiMaster2Use;                       /*  ��ͨ��ʹ�� axi master 2 ?   */
}__K210_DMA_CONTROLLER, *__PK210_DMA_CONTROLLER;
/*********************************************************************************************************
  FIX DMA ͨ�����Ͷ���
*********************************************************************************************************/
typedef struct {
    __PK210_DMA_CONTROLLER  DMAC_pController;
    UINT32                  DMAC_uiAxiMaster;                           /*  ��ͨ��ʹ���ĸ�axi master    */
    UINT                    DMAC_uiChannel;                             /*  ��ͨ����ͨ����              */
    BOOL                    DMAC_bIsInit;                               /*  ��ͨ���Ƿ��Ѿ���ʼ��        */
    LW_SPINLOCK_DEFINE(DMAC_splLock);
    ULONG                   DMAC_ulCompVector;                          /*  DMA ����ж�����            */
    UINT                    DMAC_uiCompIntPriority;                     /*  DMA ����ж����ȼ�          */
    dmac_transfer_flow_t    DMAC_flowControl;                           /*  ���ز�������δʹ��          */
} __K210_DMA_CHANNEL, *__PK210_DMA_CHANNEL;
/*********************************************************************************************************
  ȫ�ֱ���
*********************************************************************************************************/
__K210_DMA_CONTROLLER        _G_k210DmaController   =   {DMAC_BASE_ADDR};

static __K210_DMA_CHANNEL    _G_k210DmaChannels[__DMA_CHANNEL_NR] = {
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  0,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA0_INTERRUPT),
      },
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  1,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA1_INTERRUPT),
      },
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  2,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA2_INTERRUPT),
      },
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  3,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA3_INTERRUPT),
      },
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  4,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA4_INTERRUPT),
      },
      {
              .DMAC_pController   =  &_G_k210DmaController,
              .DMAC_uiChannel     =  5,
              .DMAC_ulCompVector  =  KENDRYTE_PLIC_VECTOR(IRQN_DMA5_INTERRUPT),
      },
};
/*********************************************************************************************************
** ��������: __k210DmaReset
** ��������: ��ʼ�� DMA ��������ֹͣ
** �䡡��  : uiChannel         DMA ͨ����
**           pDmaFuncs         DMA ��������
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __k210DmaReset (UINT   uiChannel, PLW_DMA_FUNCS  pDmaFuncs)
{
    dmac_chen_u_t            channelConfig;
    __PK210_DMA_CONTROLLER   pController  =  &_G_k210DmaController;

    volatile dmac_t *const   dmac         =  (dmac_t *)pController->DMAC_ulPhyAddrBase;
    volatile dmac_channel_t *dma          =  (volatile dmac_channel_t *)dmac->channel + uiChannel;

    channelConfig.data = readq(&dmac->chen);

    switch (uiChannel) {
    case DMAC_CHANNEL0:
        channelConfig.dmac_chen.ch1_en = 0;
        channelConfig.dmac_chen.ch1_en_we = 0;
        break;
    case DMAC_CHANNEL1:
        channelConfig.dmac_chen.ch2_en = 0;
        channelConfig.dmac_chen.ch2_en_we = 0;
        break;
    case DMAC_CHANNEL2:
        channelConfig.dmac_chen.ch3_en = 0;
        channelConfig.dmac_chen.ch3_en_we = 0;
        break;
    case DMAC_CHANNEL3:
        channelConfig.dmac_chen.ch4_en = 0;
        channelConfig.dmac_chen.ch4_en_we = 0;
        break;
    case DMAC_CHANNEL4:
        channelConfig.dmac_chen.ch5_en = 0;
        channelConfig.dmac_chen.ch5_en_we = 0;
        break;
    case DMAC_CHANNEL5:
        channelConfig.dmac_chen.ch6_en = 0;
        channelConfig.dmac_chen.ch6_en_we = 0;
        break;
    default:
        break;
    }

    writeq(channelConfig.data, &dmac->chen);

    dma->intclear      = 0xFFFFFFFF;
    dmac->com_intclear = 0xFFFFFFFF;
}
/*********************************************************************************************************
** ��������: __k210DmaTransact
** ��������: ��ʼ��һ�� DMA ����
** �䡡��  : uiChannel         DMA ͨ����
**           pDmaFuncs         DMA ��������
**           pDmaTrans         DMA �������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210DmaTransact (UINT                 uiChannel,
                               PLW_DMA_FUNCS        pDmaFuncs,
                               PLW_DMA_TRANSACTION  pDmaTrans)
{
    __PK210_DMA_CONTROLLER    pController = &_G_k210DmaController;
    __PK210_DMA_CHANNEL       pDmaChannel = &_G_k210DmaChannels[uiChannel];
    UINT32                    intReg;
    UINT32                    uiPriority;

    dmac_ch_cfg_u_t           cfg_u;
    sysctl_dma_select_t       request;
    volatile dmac_t          *dmac = (volatile dmac_t *)pController->DMAC_ulPhyAddrBase;
    volatile dmac_channel_t  *dma  = (volatile dmac_channel_t *)dmac->channel + uiChannel;

    if (uiChannel >= __DMA_CHANNEL_NR) {
        return  (PX_ERROR);
    }

    if (!pDmaTrans) {
        return  (PX_ERROR);
    }

    if (pDmaTrans->DMAT_pucSrcAddress == LW_NULL || pDmaTrans->DMAT_pucDestAddress == LW_NULL) {
        return  (PX_ERROR);
    }

    /*
     * ���� DMA ����Դ
     */
    request = (pDmaTrans->DMAT_ulOption >> DMA_REQUEST_SOURCE_SHIFT) & DMA_REQUEST_SOURCE_MASK;
    if (request >= SYSCTL_DMA_SELECT_MAX) {
        printk("__k210DmaTransact: error DMAT_ulOption!\r\n");
        return  (PX_ERROR);
    }
    sysctl_dma_select(uiChannel, request);

    /*
     * ͨ�����ȼ�����
     */
    uiPriority = (pDmaTrans->DMAT_iTransMode >> DMA_TRANS_PRIORITY_SHIFT) & DMA_TRANS_PRIORITY_MASK;
    if (((dmac->chen & (1 << uiChannel)) != 0) || (uiPriority > 7 || uiPriority < 0)) {
        printk("__k210DmaTransact: error priority!\r\n");
        return  (PX_ERROR);
    }

    LW_SPIN_LOCK_QUICK(&pDmaChannel->DMAC_splLock, &intReg);            /*    ����������                */

    cfg_u.data = readq(&dma->cfg);
    cfg_u.ch_cfg.ch_prior = uiPriority;
    writeq(cfg_u.data, &dma->cfg);

    API_InterVectorEnable(pDmaChannel->DMAC_ulCompVector);              /*    ʹ�� DMA ��������ж�     */

    dmac_set_single_mode((dmac_channel_number_t)uiChannel,
                         (const void *)pDmaTrans->DMAT_pucSrcAddress,
                         (void *)pDmaTrans->DMAT_pucDestAddress,
                         (dmac_address_increment_t)pDmaTrans->DMAT_iSrcAddrCtl,
                         (dmac_address_increment_t)pDmaTrans->DMAT_iDestAddrCtl,
                         ((pDmaTrans->DMAT_iTransMode >> DMA_BURST_SIZE_SHIFT)  & DMA_BURST_SIZE_MASK),
                         ((pDmaTrans->DMAT_iTransMode >> DMA_TRANS_WIDTH_SHIFT) & DMA_TRANS_WIDTH_MASK),
                         (size_t)pDmaTrans->DMAT_stDataBytes);

    LW_SPIN_UNLOCK_QUICK(&pDmaChannel->DMAC_splLock, intReg);           /*    ����������                */

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __k210DmaGetStatus
** ��������: ��� DMA ��ǰ״̬
** �䡡��  : uiChannel         DMA ͨ����
**           pDmaFuncs         DMA ��������
** �䡡��  : 0:����  1:æ  PX_ERROR:����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210DmaGetStatus (UINT   uiChannel, PLW_DMA_FUNCS  pDmaFuncs)
{
    __PK210_DMA_CHANNEL  pDmaChannel = &_G_k210DmaChannels[0];

    if (uiChannel >= __DMA_CHANNEL_NR) {
        return  (PX_ERROR);
    }

    if (dmac_check_channel_busy((dmac_channel_number_t)(pDmaChannel->DMAC_uiChannel))) {                                                            /*  ���״̬                    */
        return  (LW_DMA_STATUS_BUSY);
    } else {
        return  (LW_DMA_STATUS_IDLE);
    }
}
/*********************************************************************************************************
** ��������: dmaCompletionIsr
** ��������: DMA ����жϷ�������
** �䡡��  : pDmaChannel     DMA ������
**           ulVector          �ж�����
** �䡡��  : �жϷ���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static irqreturn_t dmaCompletionIsr (__PK210_DMA_CHANNEL  pDmaChannel, ULONG  ulVector)
{
    __PK210_DMA_CONTROLLER   pController = &_G_k210DmaController;

    volatile dmac_t         *dmac = (volatile dmac_t *)pController->DMAC_ulPhyAddrBase;
    volatile dmac_channel_t *dma  = (volatile dmac_channel_t *)dmac->channel + pDmaChannel->DMAC_uiChannel;

    if ((dma->intstatus & 0x2) == LW_FALSE) {
        _PrintFormat("dmaCompletionIsr(): enter with no irq happened.\r\n");
        return  (LW_IRQ_NONE);
    }

    dma->intclear      = 0xFFFFFFFF;
    dmac->com_intclear = 0xFFFFFFFF;

    /*
     * DMA ������ɺ���жϴ�����
     */
    API_DmaContext(pDmaChannel->DMAC_uiChannel);

    return  (LW_IRQ_HANDLED);
}
/*********************************************************************************************************
** ��������: __k210DmacHwInit
** ��������: ��ʼ�� DMA Ӳ��������
** �䡡��  : pDmaChannel     DMA ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210DmacHwInit (__PK210_DMA_CHANNEL  pDmaChannel)
{
    __PK210_DMA_CONTROLLER  pController = &_G_k210DmaController;

    /*
     *  The following things has been done in this interface.
     *  ==> reset dmac
     *  ==> clear common register interrupt
     *  ==> disable dmac and disable interrupt
     *  ==> disable all channel before configure
     */
    if (!pController->DMAC_bIsInit) {
        dmac_init();
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __k210DmaInit
** ��������: ��ʼ�� DMA ������
** �䡡��  : pDmaChannel     DMA ������
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210DmaInit (__PK210_DMA_CHANNEL  pDmaChannel)
{
    CHAR    cBuffer[64];

    if (!pDmaChannel->DMAC_bIsInit) {

        LW_SPIN_INIT(&pDmaChannel->DMAC_splLock);

        if (__k210DmacHwInit(pDmaChannel) != ERROR_NONE) {
            printk(KERN_ERR "__k210DmaInit(): failed to init!\n");
            goto  __error_handle;
        }

        API_InterVectorDisable(pDmaChannel->DMAC_ulCompVector);

        /*
         * Register DMA Channel Controller transfer interrupt-isr.
         */
        snprintf(cBuffer, sizeof(cBuffer), __DMA_CHANNEL_ISR_FMT, pDmaChannel->DMAC_uiChannel);

        API_InterVectorConnect(pDmaChannel->DMAC_ulCompVector, (PINT_SVR_ROUTINE)dmaCompletionIsr,
                               (PVOID)pDmaChannel, cBuffer);

        /*
         * Setting the priority for DMA completion interrupt prio.
         */
        API_InterVectorSetPriority(pDmaChannel->DMAC_ulCompVector, BSP_CFG_DMAC_COMP_INT_PRIO);

        pDmaChannel->DMAC_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);

__error_handle:

    return  (PX_ERROR);
}
/*********************************************************************************************************
  AM335X ������ DMA ��������
*********************************************************************************************************/
static LW_DMA_FUNCS  _G_k210DmaFuncs = {
    __k210DmaReset,
    __k210DmaTransact,
    __k210DmaGetStatus,
};
/*********************************************************************************************************
** ��������: dmaGetFuncs
** ��������: ��ȡ DMA ��������
** �䡡��  : uiChannel       DMA ͨ����
**           pulMaxBytes     ������ֽ���
** �䡡��  : ָ�� DMA ͨ������������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
PLW_DMA_FUNCS  dmaGetFuncs (UINT  uiChannel, ULONG  *pulMaxBytes)
{
    __PK210_DMA_CHANNEL  pDmaChannel;

    if (uiChannel >= __DMA_CHANNEL_NR) {
        printk(KERN_ERR "dmaGetFuncs(): DMA channel invalid!\n");
        return  (LW_NULL);
    }

    pDmaChannel = &_G_k210DmaChannels[uiChannel];

    if (__k210DmaInit(pDmaChannel) != ERROR_NONE) {
        return  (LW_NULL);
    }

    if (pulMaxBytes) {
        *pulMaxBytes = DMA_MAX_TRANS_SIZE;                              /*  ������Ҫ�����ֲ�����޸�    */
    }

    return  (&_G_k210DmaFuncs);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
