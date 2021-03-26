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
** ��   ��   ��: k210_clint.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 18 ��
**
** ��        ��: CLIC �жϿ���������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "config.h"
#include "k210_clint.h"
#include "driver/fix_arch_def.h"
/*********************************************************************************************************
 �Ĵ�����ַ��ƫ�ƶ���
*********************************************************************************************************/
#define CLINT_MSIP           (0x0000)                                   /* Reg msip address offset      */
#define CLINT_MSIP_SIZE      (0x4)
#define CLINT_MTIMECMP       (0x4000)                                   /* Reg mtimecmp address offset  */
#define CLINT_MTIMECMP_SIZE  (0x8)
#define CLINT_MTIME          (0xBFF8)                                   /* Reg mtime address offset     */
#define CLINT_MTIME_SIZE     (0x8)
#define CLINT_MAX_HARTS      (4095)                                     /* Max number of cores          */
#define CLINT_NUM_HARTS      (2)                                        /* Real number of cores         */
#define CLINT_CLOCK_DIV      (50)                                       /* Clock freq division factor   */
/*********************************************************************************************************
 * CLINT Object ����
*********************************************************************************************************/
struct clint_msip_t {                                                   /* MSIP Registers               */
    uint32_t            msip : 1;                                       /* Bit 0 is msip                */
    uint32_t            zero : 31;                                      /* Bits [32:1] is 0             */
} __attribute__((packed, aligned(4)));

typedef uint64_t        clint_mtimecmp_t;                               /* MTIMECMP Registers           */

typedef uint64_t        clint_mtime_t;                                  /* Timer Registers              */

struct clint_t {                                                        /* Coreplex-Local INTerrupts    */
    struct clint_msip_t  msip[CLINT_MAX_HARTS];                         /* 0x0000 to 0x3FF8, Reg msip   */
    uint32_t             resv0;                                         /* Resverd space, do not use    */
    clint_mtimecmp_t     mtimecmp[CLINT_MAX_HARTS];                     /* 0x4000 to 0xBFF0, mtimecmp   */
    clint_mtime_t        mtime;                                         /* 0xBFF8, Time Register        */
} __attribute__((packed, aligned(4)));
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
volatile struct clint_t* const clint = (volatile struct clint_t*)CLINT_BASE_ADDR;
/*********************************************************************************************************
** ��������: clintMTimeCmpSet
** ��������: ���� Machine Mode ��ָ���˵ıȽ���mtimecmp�е�ֵ
** ��  ��  : hartid       ָ���˵�hartid
**           value        Ҫ���õ�ֵ
** ��  ��  : ERROR_CODE   0: �ɹ�, -1: hartid����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintMTimeCmpSet (UINT uiHartId, UINT uiValue)
{
    if (uiHartId >= CLINT_NUM_HARTS) {
        return -1;
    }

    KN_SMP_WMB();
    clint->mtimecmp[uiHartId] = uiValue;

    return 0;
}
/*********************************************************************************************************
** ��������: clintMTimeCmpGet
** ��������: ��� Machine Mode ��ָ���˵ıȽ���mtimecmp�е�ֵ
** ��  ��  : hartid       ָ���˵�hartid
** ��  ��  : ָ���˱Ƚ���mtimecmp�е�ֵ, �������-1
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintMTimeCmpGet (UINT uiHartId)
{
    if (uiHartId >= CLINT_NUM_HARTS) {
        return -1;
    }

    return (clint->mtimecmp[uiHartId]);
}
/*********************************************************************************************************
** ��������: clintMTimeGet
** ��������: ��� Machine Mode �µļ�����mtime��ֵ
** ��  ��  : NONE
** ��  ��  : ������mtime��ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintMTimeGet (VOID)
{
    return (clint->mtime);
}
/*********************************************************************************************************
** ��������: clintIpiInit
** ��������: ��ʼ�� Machine Mode �µ�����ж�ʹ��λ
** ��  ��  : NONE
** ��  ��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintIpiInit (VOID)
{
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** ��������: clintIpiEnable
** ��������: ʹ�� Machine Mode �µ�����ж�
** ��  ��  : NONE
** ��  ��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintIpiEnable (VOID)
{
    /* Enable interrupts in general */
    set_csr(mstatus, MSTATUS_MIE);

    /* Set the Machine-Software bit in MIE */
    set_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** ��������: clintIpiDisable
** ��������: ��ֹ Machine Mode �µ�����ж�
** ��  ��  : NONE
** ��  ��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintIpiDisable (VOID)
{
    /* Clear the Machine-Software bit in MIE */
    clear_csr(mie, MIP_MSIP);

    return 0;
}
/*********************************************************************************************************
** ��������: clintIpiSend
** ��������: Machine Mode ����ָ���˷��ͺ˼��ж�
** ��  ��  : core_id  �˵�hartid
** ��  ��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintIpiSend (UINT uiCoreId)
{
    if (uiCoreId >= CLINT_NUM_HARTS) {
        return -1;
    }

    KN_SMP_WMB();
    clint->msip[uiCoreId].msip = 1;

    return 0;
}
/*********************************************************************************************************
** ��������: clintIpiClear
** ��������: ��� Machine Mode ��ָ���˵�����ж�
** ��  ��  : core_id        ָ���˵�hartid
** ��  ��  : ERROR_CODE     1: �ɹ�, 0: û���ж�, -1: core_id ����
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT clintIpiClear(UINT uiCoreId)
{
    if (uiCoreId >= CLINT_NUM_HARTS) {
        return -1;
    }

    if (clint->msip[uiCoreId].msip) {
        KN_SMP_WMB();
        clint->msip[uiCoreId].msip = 0;
        return 1;
    }

    return 0;
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
