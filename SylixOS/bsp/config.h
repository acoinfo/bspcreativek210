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
** ��   ��   ��: config.h
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: Ŀ��ϵͳ����.
*********************************************************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

/*********************************************************************************************************
  ������������
*********************************************************************************************************/

#include "../../config.h"

/*********************************************************************************************************
  �弶�������
*********************************************************************************************************/

#include "config_k210.h"

/*********************************************************************************************************
  ��������ѡ��
*********************************************************************************************************/

/*********************************************************************************************************
  PLL���
*********************************************************************************************************/

#define configCPU_CLOCK_HZ    ((unsigned long)160000000)
#define PLL1_OUTPUT_FREQ      (160000000UL)
#define PLL2_OUTPUT_FREQ      (45158400UL)

/*********************************************************************************************************
  �ж����
*********************************************************************************************************/

#include "config/cpu/cpu_cfg.h"

#if LW_CFG_RISCV_M_LEVEL == 1
#define BSP_CFG_TICK_VECTOR             IRQ_M_TIMER
#define BSP_CFG_IPI_VECTOR              IRQ_M_SOFT
#define BSP_CFG_EXT_VECTOR              IRQ_M_EXT
#else
#define BSP_CFG_TICK_VECTOR             IRQ_S_TIMER
#define BSP_CFG_IPI_VECTOR              IRQ_S_SOFT
#define BSP_CFG_EXT_VECTOR              IRQ_S_EXT
#endif

#define KENDRYTE_CLINT_BASE             (0x02000000)
#define KENDRYTE_CLINT_SIZE             (0x10000)
#define KENDRYTE_CLINT_VECTOR_NR        (14)
#define KENDRYTE_CLINT_TIMEBASE_FREQ    ((unsigned long)160000000 / 50)

#define KENDRYTE_PLIC_BASE              (0x0C000000)
#define KENDRYTE_PLIC_SIZE              (0x0400000)
#define KENDRYTE_PLIC_DEV_NR            (65)

#define KENDRYTE_PLIC_VECTOR_BASE       KENDRYTE_CLINT_VECTOR_NR
#define KENDRYTE_PLIC_IRQ(vector)       ((vector) - KENDRYTE_PLIC_VECTOR_BASE)
#define KENDRYTE_PLIC_VECTOR(irq)       ((irq)    + KENDRYTE_PLIC_VECTOR_BASE)

/*********************************************************************************************************
  �ж����ȼ�����
*********************************************************************************************************/

#define BSP_CFG_UARTHS_INT_PRIO         (3)                             /* Under TileLink               */
#define BSP_CFG_GPIOHS_INT_PRIO         (5)

#define BSP_CFG_FFT_INT_PRIO            (4)                             /* Under AXI 64 bit             */
#define BSP_CFG_FFT_INT_PRIO            (4)

#define BSP_CFG_DMAC_COMP_INT_PRIO      (1)                             /* Under AHB 32 bit             */
#define BSP_CFG_DMAC_ERR_INT_PRIO       (2)

#define BSP_CFG_GPIO_INT_PRIO           (1)                             /* Under APB1 32 bit            */
#define BSP_CFG_UART1_INT_PRIO          (3)
#define BSP_CFG_UART2_INT_PRIO          (3)
#define BSP_CFG_UART3_INT_PRIO          (3)
#define BSP_CFG_I2S0_INT_PRIO           (2)
#define BSP_CFG_I2S1_INT_PRIO           (2)
#define BSP_CFG_I2S2_INT_PRIO           (2)
#define BSP_CFG_I2C0_INT_PRIO           (2)
#define BSP_CFG_I2C1_INT_PRIO           (2)
#define BSP_CFG_I2C2_INT_PRIO           (2)
#define BSP_CFG_SHA256_INT_PRIO         (2)
#define BSP_CFG_TIMER0_INT_PRIO         (0)
#define BSP_CFG_TIMER1_INT_PRIO         (0)
#define BSP_CFG_TIMER2_INT_PRIO         (0)

#define BSP_CFG_DVP_INT_PRIO            (5)                             /* Under APB2 32 bit            */
#define BSP_CFG_AES_INT_PRIO            (5)

#define BSP_CFG_SPI0_INT_PRIO           (5)                             /*  Under APB3 32 bit           */
#define BSP_CFG_SPI1_INT_PRIO           (5)
#define BSP_CFG_SPI3_INT_PRIO           (5)

/*********************************************************************************************************
  ���Դ��� UARTHS ���
*********************************************************************************************************/

#define KENDRYTE_UART0_BASE      (0x38000000)
#define KENDRYTE_UART0_VECTOR    KENDRYTE_PLIC_VECTOR(33)

/*********************************************************************************************************
  FPIOA ��� (GPIO + GPIOHS)
*********************************************************************************************************/

#define KENDRYTE_GPIO_NR         (8)
#define KENDRYTE_GPIOHS_NR       (32)

/*********************************************************************************************************
  ���Դ�ļ����ɼ�����������ѡ��
*********************************************************************************************************/

#ifndef ASSEMBLY

#endif                                                                  /*  ASSEMBLY                    */

#endif                                                                  /*  __CONFIG_H                  */
/*********************************************************************************************************
  END
*********************************************************************************************************/
