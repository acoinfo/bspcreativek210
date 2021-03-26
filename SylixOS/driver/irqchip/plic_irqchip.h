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
** ��   ��   ��: plic_irqchip.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 03 ��
**
** ��        ��: PLIC �жϿ���������
*********************************************************************************************************/

#ifndef __PLIC_IRQCHIP_H
#define __PLIC_IRQCHIP_H

typedef struct {
    PVOID            PLIC_pvBase;
    ULONG            PLIC_ulVectorBase;
    ULONG            PLIC_ulDevNr;
    BOOL             PLIC_bEnabled[LW_CFG_MAX_INTER_SRC];
    UINT             PLIC_uiPrio[LW_CFG_MAX_INTER_SRC];
    LW_CLASS_CPUSET  PLIC_irqCpuSet[LW_CFG_MAX_INTER_SRC];
} PLIC_IRQCHIP, *PPLIC_IRQCHIP;                                         /*  PLIC �жϿ�����             */

VOID   plicIntInit  (PPLIC_IRQCHIP  plic);
VOID   plicIntHandle(PPLIC_IRQCHIP  plic, BOOL  bPreemptive);

VOID   plicIntVectorEnable  (PPLIC_IRQCHIP  plic, ULONG  ulIrq);
VOID   plicIntVectorDisable (PPLIC_IRQCHIP  plic, ULONG  ulIrq);
BOOL   plicIntVectorIsEnable(PPLIC_IRQCHIP  plic, ULONG  ulIrq);

ULONG  plicIntVectorSetPriority(PPLIC_IRQCHIP  plic, ULONG  ulIrq, UINT  uiPrio);
ULONG  plicIntVectorGetPriority(PPLIC_IRQCHIP  plic, ULONG  ulIrq, UINT *puiPrio);

ULONG  plicIntVectorSetTarget(PPLIC_IRQCHIP  plic, ULONG  ulIrq, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset);
ULONG  plicIntVectorGetTarget(PPLIC_IRQCHIP  plic, ULONG  ulIrq, size_t  stSize,       PLW_CLASS_CPUSET  pcpuset);

#endif                                                                  /*  __PLIC_IRQCHIP_H            */
/*********************************************************************************************************
  END
*********************************************************************************************************/
