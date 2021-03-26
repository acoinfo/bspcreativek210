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
** ��   ��   ��: plic_irqchip.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 04 �� 03 ��
**
** ��        ��: PLIC �жϿ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <linux/compat.h>
#include "plic_irqchip.h"
/*********************************************************************************************************
 * From the RISC-V Privlidged Spec v1.10:
 *
 * Global interrupt sources are assigned small unsigned integer identifiers,
 * beginning at the value 1.  An interrupt ID of 0 is reserved to mean ��no
 * interrupt��.  Interrupt identifiers are also used to break ties when two or
 * more interrupt sources have the same assigned priority. Smaller values of
 * interrupt ID take precedence over larger values of interrupt ID.
 *
 * While the RISC-V supervisor spec doesn't define the maximum number of
 * devices supported by the PLIC, the largest number supported by devices
 * marked as 'riscv,plic0' (which is the only device type this driver supports,
 * and is the only extant PLIC as of now) is 1024.  As mentioned above, device
 * 0 is defined to be non-existant so this device really only supports 1023
 * devices.
*********************************************************************************************************/
#define MAX_DEVICES     1024
#define MAX_CONTEXTS    15872
/*********************************************************************************************************
 * The PLIC consists of memory-mapped control registers, with a memory map as
 * follows:
 *
 * base + 0x000000: Reserved (interrupt source 0 does not exist)
 * base + 0x000004: Interrupt source 1 priority
 * base + 0x000008: Interrupt source 2 priority
 * ...
 * base + 0x000FFC: Interrupt source 1023 priority
 * base + 0x001000: Pending 0
 * base + 0x001FFF: Pending
 * base + 0x002000: Enable bits for sources 0-31 on context 0
 * base + 0x002004: Enable bits for sources 32-63 on context 0
 * ...
 * base + 0x0020FC: Enable bits for sources 992-1023 on context 0
 * base + 0x002080: Enable bits for sources 0-31 on context 1
 * ...
 * base + 0x002100: Enable bits for sources 0-31 on context 2
 * ...
 * base + 0x1F1F80: Enable bits for sources 992-1023 on context 15871
 * base + 0x1F1F84: Reserved
 * ...              (higher context IDs would fit here, but wouldn't fit
 *                   inside the per-context priority vector)
 * base + 0x1FFFFC: Reserved
 * base + 0x200000: Priority threshold for context 0
 * base + 0x200004: Claim/complete for context 0
 * base + 0x200008: Reserved
 * ...
 * base + 0x200FFC: Reserved
 * base + 0x201000: Priority threshold for context 1
 * base + 0x201004: Claim/complete for context 1
 * ...
 * base + 0xFFE000: Priority threshold for context 15871
 * base + 0xFFE004: Claim/complete for context 15871
 * base + 0xFFE008: Reserved
 * ...
 * base + 0xFFFFFC: Reserved
*********************************************************************************************************/
/*********************************************************************************************************
  Each interrupt source has a priority register associated with it.
*********************************************************************************************************/
#define PRIORITY_BASE       0
#define PRIORITY_PER_ID     4
/*********************************************************************************************************
  Each hart context has a vector of interupt enable bits associated with it.
  There's one bit for each interrupt source.
*********************************************************************************************************/
#define ENABLE_BASE         0x2000
#define ENABLE_PER_HART     0x80
/*********************************************************************************************************
  Each hart context has a set of control registers associated with it.  Right
  now there's only two: a source priority threshold over which the hart will
  take an interrupt, and a register to claim interrupts.
*********************************************************************************************************/
#define CONTEXT_BASE        0x200000
#define CONTEXT_PER_HART    0x1000
#define CONTEXT_THRESHOLD   0
#define CONTEXT_CLAIM       4
/*********************************************************************************************************
  PLIC devices are named like 'riscv,plic0,%llx', this is enough space to
  store that name.
*********************************************************************************************************/
#define PLIC_DATA_NAME_SIZE 30
/*********************************************************************************************************
  Hart id to context id.
*********************************************************************************************************/
#if LW_CFG_RISCV_M_LEVEL > 0
#define HARTID_TO_CTXID(hartid)         (hartid)
#undef  RISCV_CPUID_TO_HARTID
#define RISCV_CPUID_TO_HARTID(cpuid)    (cpuid)
#else
#define HARTID_TO_CTXID(hartid)         ((hartid) + 1)
#endif                                                                  /*  LW_CFG_RISCV_M_LEVEL == 0   */
/*********************************************************************************************************
  Addressing helper functions.
*********************************************************************************************************/
static LW_INLINE
UINT32  *plic_enable_vector (PPLIC_IRQCHIP  plic, INT  iContextId)
{
    return  (plic->PLIC_pvBase + ENABLE_BASE + iContextId * ENABLE_PER_HART);
}

static LW_INLINE
UINT32  *plic_priority (PPLIC_IRQCHIP  plic, INT  iHwIrq)
{
    return  (plic->PLIC_pvBase + PRIORITY_BASE + iHwIrq * PRIORITY_PER_ID);
}

static LW_INLINE
UINT32  *plic_hart_threshold (PPLIC_IRQCHIP  plic, INT  iContextId)
{
    return  (plic->PLIC_pvBase + CONTEXT_BASE + CONTEXT_PER_HART * iContextId + CONTEXT_THRESHOLD);
}

static LW_INLINE
UINT32  *plic_hart_claim (PPLIC_IRQCHIP  plic, INT  iContextId)
{
    return  (plic->PLIC_pvBase + CONTEXT_BASE + CONTEXT_PER_HART * iContextId + CONTEXT_CLAIM);
}

/*
 * Handling an interrupt is a two-step process: first you claim the interrupt
 * by reading the claim register, then you complete the interrupt by writing
 * that source ID back to the same claim register.  This automatically enables
 * and disables the interrupt, so there's nothing else to do.
 */
static LW_INLINE
UINT32  plic_claim (PPLIC_IRQCHIP  plic, INT  iContextId)
{
    return  (readl(plic_hart_claim(plic, iContextId)));
}

static LW_INLINE
VOID  plic_complete (PPLIC_IRQCHIP  plic, INT  iContextId, UINT32  claim)
{
    writel(claim, plic_hart_claim(plic, iContextId));
}

/*
 * Explicit interrupt masking.
 */
static VOID  plic_disable (PPLIC_IRQCHIP  plic, INT  iContextId, INT  iHwIrq)
{
    UINT32  *reg  = plic_enable_vector(plic, iContextId) + (iHwIrq / 32);
    UINT32   mask = ~(1 << (iHwIrq % 32));

    writel(readl(reg) & mask, reg);
}

static VOID  plic_enable (PPLIC_IRQCHIP  plic, INT  iContextId, INT  iHwIrq)
{
    UINT32  *reg = plic_enable_vector(plic, iContextId) + (iHwIrq / 32);
    UINT32   bit = 1 << (iHwIrq % 32);

    writel(readl(reg) | bit, reg);
}
/*********************************************************************************************************
** ��������: plicIntInit
** ��������: ��ʼ�� PLIC �жϿ�����
** �䡡��  : plic          PLIC �жϿ�����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  plicIntInit (PPLIC_IRQCHIP  plic)
{
    ULONG  ulIrq;
    ULONG  ulCPUId;

    lib_bzero(plic->PLIC_bEnabled,  sizeof(plic->PLIC_bEnabled));

    for (ulIrq = 0; ulIrq <= plic->PLIC_ulDevNr; ulIrq++) {
        plic->PLIC_uiPrio[ulIrq]   = 1;
        plic->PLIC_bEnabled[ulIrq] = LW_FALSE;
        LW_CPU_ZERO(&plic->PLIC_irqCpuSet[ulIrq]);
        LW_CPU_SET(0, &plic->PLIC_irqCpuSet[ulIrq]);
    }

    for (ulCPUId = 0; ulCPUId < LW_NCPUS; ulCPUId++) {
        INT  iContextId = HARTID_TO_CTXID(RISCV_CPUID_TO_HARTID(ulCPUId));

        writel(0, plic_hart_threshold(plic, iContextId));
        for (ulIrq = 1; ulIrq <= plic->PLIC_ulDevNr; ulIrq++) {
            plic_disable(plic, iContextId, ulIrq);
        }
    }
}
/*********************************************************************************************************
** ��������: plicIntHandle
** ��������: ���� PLIC �жϿ������ж�
** �䡡��  : plic          PLIC �жϿ�����
**           bPreemptive   �Ƿ������ж�Ƕ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  plicIntHandle (PPLIC_IRQCHIP  plic, BOOL  bPreemptive)
{
    UINT32 uiWhat;
    INT    iContextId = HARTID_TO_CTXID(RISCV_CPUID_TO_HARTID(LW_CPU_GET_CUR_ID()));

    while ((uiWhat = plic_claim(plic, iContextId))) {
        archIntHandle(uiWhat + plic->PLIC_ulVectorBase, bPreemptive);
        plic_complete(plic, iContextId, uiWhat);
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorEnable
** ��������: ʹ��ָ�����ж�����
** ��  ��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  plicIntVectorEnable (PPLIC_IRQCHIP  plic, ULONG  ulIrq)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        VOID  *pvPriority = plic_priority(plic, ulIrq);
        ULONG  ulCPUId;

        plic->PLIC_bEnabled[ulIrq] = LW_TRUE;
        writel(plic->PLIC_uiPrio[ulIrq], pvPriority);
        for (ulCPUId = 0; ulCPUId < LW_NCPUS; ulCPUId++) {
            if (LW_CPU_ISSET(ulCPUId, &plic->PLIC_irqCpuSet[ulIrq])) {
                plic_enable(plic, HARTID_TO_CTXID(RISCV_CPUID_TO_HARTID(ulCPUId)), ulIrq);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorDisable
** ��������: ����ָ�����ж�����
** ��  ��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�����
** ��  ��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
VOID  plicIntVectorDisable (PPLIC_IRQCHIP  plic, ULONG  ulIrq)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        VOID  *pvPriority = plic_priority(plic, ulIrq);
        ULONG  ulCPUId;

        plic->PLIC_bEnabled[ulIrq] = LW_FALSE;
        writel(0, pvPriority);
        for (ulCPUId = 0; ulCPUId < LW_NCPUS; ulCPUId++) {
            if (LW_CPU_ISSET(ulCPUId, &plic->PLIC_irqCpuSet[ulIrq])) {
                plic_disable(plic, HARTID_TO_CTXID(RISCV_CPUID_TO_HARTID(ulCPUId)), ulIrq);
            }
        }
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorIsEnable
** ��������: ���ָ�����ж������Ƿ�ʹ��
** ��  ��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�����
** ��  ��  : LW_FALSE �� LW_TRUE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
BOOL  plicIntVectorIsEnable (PPLIC_IRQCHIP  plic, ULONG  ulIrq)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        return  (plic->PLIC_bEnabled[ulIrq]);
    } else {
        return  (LW_FALSE);
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorSetPriority
** ��������: ����ָ�����ж����������ȼ�
** ��  ��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�������
**           uiPrio        ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_PRIO > 0

ULONG  plicIntVectorSetPriority (PPLIC_IRQCHIP  plic, ULONG  ulIrq, UINT  uiPrio)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        VOID  *pvPriority = plic_priority(plic, ulIrq);

        plic->PLIC_uiPrio[ulIrq] = uiPrio;
        if (plic->PLIC_bEnabled[ulIrq]) {
            writel(uiPrio, pvPriority);
        }
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorGetPriority
** ��������: ��ȡָ�����ж����������ȼ�
** ��  ��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�������
**           puiPrio       ���ȼ�
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  plicIntVectorGetPriority (PPLIC_IRQCHIP  plic, ULONG  ulIrq, UINT  *puiPrio)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        *puiPrio = plic->PLIC_uiPrio[ulIrq];
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_INTER_PRIO > 0       */
/*********************************************************************************************************
** ��������: plicIntVectorSetTarget
** ��������: ����ָ�����ж�������Ŀ�� CPU
** �䡡��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�������
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_INTER_TARGET > 0

ULONG  plicIntVectorSetTarget (PPLIC_IRQCHIP  plic, ULONG  ulIrq, size_t  stSize, const PLW_CLASS_CPUSET  pcpuset)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        BOOL  bEnabled = plic->PLIC_bEnabled[ulIrq];

        if (bEnabled) {
            plicIntVectorDisable(plic, ulIrq);
        }

        lib_memcpy(&plic->PLIC_irqCpuSet[ulIrq], pcpuset, sizeof(LW_CLASS_CPUSET));

        if (bEnabled) {
            plicIntVectorEnable(plic, ulIrq);
        }

        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: plicIntVectorGetTarget
** ��������: ��ȡָ�����ж�������Ŀ�� CPU
** �䡡��  : plic          PLIC �жϿ�����
**           ulIrq         �ж�������
**           stSize        CPU ���뼯�ڴ��С
**           pcpuset       CPU ����
** �䡡��  : ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
ULONG  plicIntVectorGetTarget (PPLIC_IRQCHIP  plic, ULONG  ulIrq, size_t  stSize, PLW_CLASS_CPUSET  pcpuset)
{
    if (ulIrq <= plic->PLIC_ulDevNr) {
        lib_memcpy(pcpuset, &plic->PLIC_irqCpuSet[ulIrq], sizeof(LW_CLASS_CPUSET));
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}

#endif                                                                  /*  LW_CFG_INTER_TARGET > 0     */
/*********************************************************************************************************
  END
*********************************************************************************************************/
