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
** 文   件   名: config.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 17 日
**
** 描        述: 目标系统配置.
*********************************************************************************************************/

#ifndef __CONFIG_K210_H
#define __CONFIG_K210_H

/*********************************************************************************************************
  使能 SPI-SDCard 或 SPI-Flash (注意: sdcard 和 flash 所使用的 spi 接口共用 30、32 号IO引脚)
*********************************************************************************************************/

#define KENDRYTE_VIDEO_ENABLE   (0)
#define KENDRYTE_SDCARD_ENABLE  (1)

#define KENDRYTE_FLASH_ENABLE   (0)
#define KENDRYTE_INSTALL_KPU    (0)

/*********************************************************************************************************
  寄存器基址定义 (参考自 kendryte-standalone-sdk 中的 platform.h )
*********************************************************************************************************/
                                                                        /* Under Coreplex               */
#define CLINT_BASE_ADDR         (0x02000000)
#define PLIC_BASE_ADDR          (0x0C000000)
                                                                        /* Under TileLink               */
#define UARTHS_BASE_ADDR        (0x38000000)
#define GPIOHS_BASE_ADDR        (0x38001000)
                                                                        /* Under AXI 64 bit             */
#define RAM_BASE_ADDR           (0x80000000)
#define RAM_SIZE                (6 * 1024 * 1024)

#define IO_BASE_ADDR            (0x40000000)
#define IO_SIZE                 (6 * 1024 * 1024)

#define AI_RAM_BASE_ADDR        (0x80600000)
#define AI_RAM_SIZE             (2 * 1024 * 1024)

#define AI_IO_BASE_ADDR         (0x40600000)
#define AI_IO_SIZE              (2 * 1024 * 1024)

#define AI_BASE_ADDR            (0x40800000)
#define AI_SIZE                 (12 * 1024 * 1024)

#define FFT_BASE_ADDR           (0x42000000)
#define FFT_SIZE                (4 * 1024 * 1024)

#define ROM_BASE_ADDR           (0x88000000)
#define ROM_SIZE                (128 * 1024)
                                                                        /* Under AHB 32 bit             */
#define DMAC_BASE_ADDR          (0x50000000)
                                                                        /* Under APB1 32 bit            */
#define GPIO_BASE_ADDR          (0x50200000)
#define UART1_BASE_ADDR         (0x50210000)
#define UART2_BASE_ADDR         (0x50220000)
#define UART3_BASE_ADDR         (0x50230000)
#define SPI_SLAVE_BASE_ADDR     (0x50240000)
#define I2S0_BASE_ADDR          (0x50250000)
#define I2S1_BASE_ADDR          (0x50260000)
#define I2S2_BASE_ADDR          (0x50270000)
#define I2C0_BASE_ADDR          (0x50280000)
#define I2C1_BASE_ADDR          (0x50290000)
#define I2C2_BASE_ADDR          (0x502A0000)
#define FPIOA_BASE_ADDR         (0x502B0000)
#define SHA256_BASE_ADDR        (0x502C0000)
#define TIMER0_BASE_ADDR        (0x502D0000)
#define TIMER1_BASE_ADDR        (0x502E0000)
#define TIMER2_BASE_ADDR        (0x502F0000)
                                                                        /* Under APB2 32 bit            */
#define WDT0_BASE_ADDR          (0x50400000)
#define WDT1_BASE_ADDR          (0x50410000)
#define OTP_BASE_ADDR           (0x50420000)
#define DVP_BASE_ADDR           (0x50430000)
#define SYSCTL_BASE_ADDR        (0x50440000)
#define AES_BASE_ADDR           (0x50450000)
#define RTC_BASE_ADDR           (0x50460000)
                                                                        /* Under APB3 32 bit            */
#define SPI0_BASE_ADDR          (0x52000000)
#define SPI1_BASE_ADDR          (0x53000000)
#define SPI3_BASE_ADDR          (0x54000000)

/*********************************************************************************************************
  PLINT 中断源定义 (参考自 kendryte-standalone-sdk 中的 plic.h )
*********************************************************************************************************/

#define PLIC_NUM_SOURCES        (IRQN_MAX - 1)                          /* IRQ number settings          */
#define PLIC_NUM_PRIORITIES     (7)
#define PLIC_NUM_HARTS          (2)                                     /* Real number of cores         */

/*********************************************************************************************************
  Fix API in atomic.h
*********************************************************************************************************/

#define atomic_set(ptr, val)    (*(volatile typeof(*(ptr))*)(ptr) = val)
#define atomic_read(ptr)        (*(volatile typeof(*(ptr))*)(ptr))
#define atomic_add(ptr, inc)    (*(volatile typeof(*(ptr))*)(ptr) += inc)

#endif                                                                  /*      __CONFIG_K210_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
