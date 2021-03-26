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
** 文   件   名: k210_clock.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 18 日
**
** 描        述: 系统时钟控制器驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include "k210_clock.h"
#include <math.h>
/*********************************************************************************************************
 * 来自 kendryte-standalone-sdk v0.2
*********************************************************************************************************/

/**
 * @brief       Git short commit id
 *
 *              No. 0 Register (0x00)
 */
struct sysctl_git_id_t
{
    uint32_t git_id : 32;
} __attribute__((packed, aligned(4)));

/**
 * @brief       System clock base frequency
 *
 *              No. 1 Register (0x04)
 */
struct sysctl_clk_freq_t
{
    uint32_t clk_freq : 32;
} __attribute__((packed, aligned(4)));

/**
 * @brief       PLL0 controller
 *
 *              No. 2 Register (0x08)
 */
struct sysctl_pll0_t
{
    uint32_t clkr0 : 4;
    uint32_t clkf0 : 6;
    uint32_t clkod0 : 4;
    uint32_t bwadj0 : 6;
    uint32_t pll_reset0 : 1;
    uint32_t pll_pwrd0 : 1;
    uint32_t pll_intfb0 : 1;
    uint32_t pll_bypass0 : 1;
    uint32_t pll_test0 : 1;
    uint32_t pll_out_en0 : 1;
    uint32_t pll_test_en : 1;
    uint32_t reserved : 5;
} __attribute__((packed, aligned(4)));

/**
 * @brief       PLL1 controller
 *
 *              No. 3 Register (0x0c)
 */
struct sysctl_pll1_t
{
    uint32_t clkr1 : 4;
    uint32_t clkf1 : 6;
    uint32_t clkod1 : 4;
    uint32_t bwadj1 : 6;
    uint32_t pll_reset1 : 1;
    uint32_t pll_pwrd1 : 1;
    uint32_t pll_intfb1 : 1;
    uint32_t pll_bypass1 : 1;
    uint32_t pll_test1 : 1;
    uint32_t pll_out_en1 : 1;
    uint32_t reserved : 6;
} __attribute__((packed, aligned(4)));

/**
 * @brief       PLL2 controller
 *
 *              No. 4 Register (0x10)
 */
struct sysctl_pll2_t
{
    uint32_t clkr2 : 4;
    uint32_t clkf2 : 6;
    uint32_t clkod2 : 4;
    uint32_t bwadj2 : 6;
    uint32_t pll_reset2 : 1;
    uint32_t pll_pwrd2 : 1;
    uint32_t pll_intfb2 : 1;
    uint32_t pll_bypass2 : 1;
    uint32_t pll_test2 : 1;
    uint32_t pll_out_en2 : 1;
    uint32_t pll_ckin_sel2 : 2;
    uint32_t reserved : 4;
} __attribute__((packed, aligned(4)));

/**
 * @brief       PLL lock tester
 *
 *              No. 6 Register (0x18)
 */
struct sysctl_pll_lock_t
{
    uint32_t pll_lock0 : 2;
    uint32_t pll_slip_clear0 : 1;
    uint32_t test_clk_out0 : 1;
    uint32_t reserved0 : 4;
    uint32_t pll_lock1 : 2;
    uint32_t pll_slip_clear1 : 1;
    uint32_t test_clk_out1 : 1;
    uint32_t reserved1 : 4;
    uint32_t pll_lock2 : 2;
    uint32_t pll_slip_clear2 : 1;
    uint32_t test_clk_out2 : 1;
    uint32_t reserved2 : 12;
} __attribute__((packed, aligned(4)));

/**
 * @brief       AXI ROM detector
 *
 *              No. 7 Register (0x1c)
 */
struct sysctl_rom_error_t
{
    uint32_t rom_mul_error : 1;
    uint32_t rom_one_error : 1;
    uint32_t reserved : 30;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock select controller0
 *
 *              No. 8 Register (0x20)
 */
struct sysctl_clk_sel0_t
{
    uint32_t aclk_sel : 1;
    uint32_t aclk_divider_sel : 2;
    uint32_t apb0_clk_sel : 3;
    uint32_t apb1_clk_sel : 3;
    uint32_t apb2_clk_sel : 3;
    uint32_t spi3_clk_sel : 1;
    uint32_t timer0_clk_sel : 1;
    uint32_t timer1_clk_sel : 1;
    uint32_t timer2_clk_sel : 1;
    uint32_t reserved : 16;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock select controller1
 *
 *              No. 9 Register (0x24)
 */
struct sysctl_clk_sel1_t
{
    uint32_t spi3_sample_clk_sel : 1;
    uint32_t reserved0 : 30;
    uint32_t reserved1 : 1;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Central clock enable
 *
 *              No. 10 Register (0x28)
 */
struct sysctl_clk_en_cent_t
{
    uint32_t cpu_clk_en : 1;
    uint32_t sram0_clk_en : 1;
    uint32_t sram1_clk_en : 1;
    uint32_t apb0_clk_en : 1;
    uint32_t apb1_clk_en : 1;
    uint32_t apb2_clk_en : 1;
    uint32_t reserved : 26;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Peripheral clock enable
 *
 *              No. 11 Register (0x2c)
 */
struct sysctl_clk_en_peri_t
{
    uint32_t rom_clk_en : 1;
    uint32_t dma_clk_en : 1;
    uint32_t ai_clk_en : 1;
    uint32_t dvp_clk_en : 1;
    uint32_t fft_clk_en : 1;
    uint32_t gpio_clk_en : 1;
    uint32_t spi0_clk_en : 1;
    uint32_t spi1_clk_en : 1;
    uint32_t spi2_clk_en : 1;
    uint32_t spi3_clk_en : 1;
    uint32_t i2s0_clk_en : 1;
    uint32_t i2s1_clk_en : 1;
    uint32_t i2s2_clk_en : 1;
    uint32_t i2c0_clk_en : 1;
    uint32_t i2c1_clk_en : 1;
    uint32_t i2c2_clk_en : 1;
    uint32_t uart1_clk_en : 1;
    uint32_t uart2_clk_en : 1;
    uint32_t uart3_clk_en : 1;
    uint32_t aes_clk_en : 1;
    uint32_t fpioa_clk_en : 1;
    uint32_t timer0_clk_en : 1;
    uint32_t timer1_clk_en : 1;
    uint32_t timer2_clk_en : 1;
    uint32_t wdt0_clk_en : 1;
    uint32_t wdt1_clk_en : 1;
    uint32_t sha_clk_en : 1;
    uint32_t otp_clk_en : 1;
    uint32_t reserved : 1;
    uint32_t rtc_clk_en : 1;
    uint32_t reserved0 : 2;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Soft reset ctrl
 *
 *              No. 12 Register (0x30)
 */
struct sysctl_soft_reset_t
{
    uint32_t soft_reset : 1;
    uint32_t reserved : 31;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Peripheral reset controller
 *
 *              No. 13 Register (0x34)
 */
struct sysctl_peri_reset_t
{
    uint32_t rom_reset : 1;
    uint32_t dma_reset : 1;
    uint32_t ai_reset : 1;
    uint32_t dvp_reset : 1;
    uint32_t fft_reset : 1;
    uint32_t gpio_reset : 1;
    uint32_t spi0_reset : 1;
    uint32_t spi1_reset : 1;
    uint32_t spi2_reset : 1;
    uint32_t spi3_reset : 1;
    uint32_t i2s0_reset : 1;
    uint32_t i2s1_reset : 1;
    uint32_t i2s2_reset : 1;
    uint32_t i2c0_reset : 1;
    uint32_t i2c1_reset : 1;
    uint32_t i2c2_reset : 1;
    uint32_t uart1_reset : 1;
    uint32_t uart2_reset : 1;
    uint32_t uart3_reset : 1;
    uint32_t aes_reset : 1;
    uint32_t fpioa_reset : 1;
    uint32_t timer0_reset : 1;
    uint32_t timer1_reset : 1;
    uint32_t timer2_reset : 1;
    uint32_t wdt0_reset : 1;
    uint32_t wdt1_reset : 1;
    uint32_t sha_reset : 1;
    uint32_t reserved : 2;
    uint32_t rtc_reset : 1;
    uint32_t reserved0 : 2;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 0
 *
 *              No. 14 Register (0x38)
 */
struct sysctl_clk_th0_t
{
    uint32_t sram0_gclk_threshold : 4;
    uint32_t sram1_gclk_threshold : 4;
    uint32_t ai_gclk_threshold : 4;
    uint32_t dvp_gclk_threshold : 4;
    uint32_t rom_gclk_threshold : 4;
    uint32_t reserved : 12;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 1
 *
 *              No. 15 Register (0x3c)
 */
struct sysctl_clk_th1_t
{
    uint32_t spi0_clk_threshold : 8;
    uint32_t spi1_clk_threshold : 8;
    uint32_t spi2_clk_threshold : 8;
    uint32_t spi3_clk_threshold : 8;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 2
 *
 *              No. 16 Register (0x40)
 */
struct sysctl_clk_th2_t
{
    uint32_t timer0_clk_threshold : 8;
    uint32_t timer1_clk_threshold : 8;
    uint32_t timer2_clk_threshold : 8;
    uint32_t reserved : 8;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 3
 *
 *              No. 17 Register (0x44)
 */
struct sysctl_clk_th3_t
{
    uint32_t i2s0_clk_threshold : 16;
    uint32_t i2s1_clk_threshold : 16;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 4
 *
 *              No. 18 Register (0x48)
 */
struct sysctl_clk_th4_t
{
    uint32_t i2s2_clk_threshold : 16;
    uint32_t i2s0_mclk_threshold : 8;
    uint32_t i2s1_mclk_threshold : 8;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 5
 *
 *              No. 19 Register (0x4c)
 */
struct sysctl_clk_th5_t
{
    uint32_t i2s2_mclk_threshold : 8;
    uint32_t i2c0_clk_threshold : 8;
    uint32_t i2c1_clk_threshold : 8;
    uint32_t i2c2_clk_threshold : 8;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Clock threshold controller 6
 *
 *              No. 20 Register (0x50)
 */
struct sysctl_clk_th6_t
{
    uint32_t wdt0_clk_threshold : 8;
    uint32_t wdt1_clk_threshold : 8;
    uint32_t reserved0 : 8;
    uint32_t reserved1 : 8;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Miscellaneous controller
 *
 *              No. 21 Register (0x54)
 */
struct sysctl_misc_t
{
    uint32_t debug_sel : 6;
    uint32_t reserved0 : 4;
    uint32_t spi_dvp_data_enable : 1;
    uint32_t reserved1 : 21;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Peripheral controller
 *
 *              No. 22 Register (0x58)
 */
struct sysctl_peri_t
{
    uint32_t timer0_pause : 1;
    uint32_t timer1_pause : 1;
    uint32_t timer2_pause : 1;
    uint32_t timer3_pause : 1;
    uint32_t timer4_pause : 1;
    uint32_t timer5_pause : 1;
    uint32_t timer6_pause : 1;
    uint32_t timer7_pause : 1;
    uint32_t timer8_pause : 1;
    uint32_t timer9_pause : 1;
    uint32_t timer10_pause : 1;
    uint32_t timer11_pause : 1;
    uint32_t spi0_xip_en : 1;
    uint32_t spi1_xip_en : 1;
    uint32_t spi2_xip_en : 1;
    uint32_t spi3_xip_en : 1;
    uint32_t spi0_clk_bypass : 1;
    uint32_t spi1_clk_bypass : 1;
    uint32_t spi2_clk_bypass : 1;
    uint32_t i2s0_clk_bypass : 1;
    uint32_t i2s1_clk_bypass : 1;
    uint32_t i2s2_clk_bypass : 1;
    uint32_t jtag_clk_bypass : 1;
    uint32_t dvp_clk_bypass : 1;
    uint32_t debug_clk_bypass : 1;
    uint32_t reserved0 : 1;
    uint32_t reserved1 : 6;
} __attribute__((packed, aligned(4)));

/**
 * @brief       SPI sleep controller
 *
 *              No. 23 Register (0x5c)
 */
struct sysctl_spi_sleep_t
{
    uint32_t ssi0_sleep : 1;
    uint32_t ssi1_sleep : 1;
    uint32_t ssi2_sleep : 1;
    uint32_t ssi3_sleep : 1;
    uint32_t reserved : 28;
} __attribute__((packed, aligned(4)));

/**
 * @brief       Reset source status
 *
 *              No. 24 Register (0x60)
 */
struct sysctl_reset_status_t
{
    uint32_t reset_sts_clr : 1;
    uint32_t pin_reset_sts : 1;
    uint32_t wdt0_reset_sts : 1;
    uint32_t wdt1_reset_sts : 1;
    uint32_t soft_reset_sts : 1;
    uint32_t reserved : 27;
} __attribute__((packed, aligned(4)));

/**
 * @brief       DMA handshake selector
 *
 *              No. 25 Register (0x64)
 */
struct sysctl_dma_sel0_t
{
    uint32_t dma_sel0 : 6;
    uint32_t dma_sel1 : 6;
    uint32_t dma_sel2 : 6;
    uint32_t dma_sel3 : 6;
    uint32_t dma_sel4 : 6;
    uint32_t reserved : 2;
} __attribute__((packed, aligned(4)));

/**
 * @brief       DMA handshake selector
 *
 *              No. 26 Register (0x68)
 */
struct sysctl_dma_sel1_t
{
    uint32_t dma_sel5 : 6;
    uint32_t reserved : 26;
} __attribute__((packed, aligned(4)));

/**
 * @brief       IO Power Mode Select controller
 *
 *              No. 27 Register (0x6c)
 */
struct sysctl_power_sel_t
{
    uint32_t power_mode_sel0 : 1;
    uint32_t power_mode_sel1 : 1;
    uint32_t power_mode_sel2 : 1;
    uint32_t power_mode_sel3 : 1;
    uint32_t power_mode_sel4 : 1;
    uint32_t power_mode_sel5 : 1;
    uint32_t power_mode_sel6 : 1;
    uint32_t power_mode_sel7 : 1;
    uint32_t reserved : 24;
} __attribute__((packed, aligned(4)));

typedef union _sysctl_power_sel_u
{
    struct sysctl_power_sel_t sel;
    uint32_t data;
} sysctl_power_sel_u_t;


/**
 * @brief       System controller object
 *
 *              The System controller is a peripheral device mapped in the
 *              internal memory map, discoverable in the Configuration String.
 *              It is responsible for low-level configuration of all system
 *              related peripheral device. It contain PLL controller, clock
 *              controller, reset controller, DMA handshake controller, SPI
 *              controller, timer controller, WDT controller and sleep
 *              controller.
 */
struct sysctl_t
{
    /* No. 0 (0x00): Git short commit id */
    struct sysctl_git_id_t git_id;
    /* No. 1 (0x04): System clock base frequency */
    struct sysctl_clk_freq_t clk_freq;
    /* No. 2 (0x08): PLL0 controller */
    struct sysctl_pll0_t pll0;
    /* No. 3 (0x0c): PLL1 controller */
    struct sysctl_pll1_t pll1;
    /* No. 4 (0x10): PLL2 controller */
    struct sysctl_pll2_t pll2;
    /* No. 5 (0x14): Reserved */
    uint32_t resv5;
    /* No. 6 (0x18): PLL lock tester */
    struct sysctl_pll_lock_t pll_lock;
    /* No. 7 (0x1c): AXI ROM detector */
    struct sysctl_rom_error_t rom_error;
    /* No. 8 (0x20): Clock select controller0 */
    struct sysctl_clk_sel0_t clk_sel0;
    /* No. 9 (0x24): Clock select controller1 */
    struct sysctl_clk_sel1_t clk_sel1;
    /* No. 10 (0x28): Central clock enable */
    struct sysctl_clk_en_cent_t clk_en_cent;
    /* No. 11 (0x2c): Peripheral clock enable */
    struct sysctl_clk_en_peri_t clk_en_peri;
    /* No. 12 (0x30): Soft reset ctrl */
    struct sysctl_soft_reset_t soft_reset;
    /* No. 13 (0x34): Peripheral reset controller */
    struct sysctl_peri_reset_t peri_reset;
    /* No. 14 (0x38): Clock threshold controller 0 */
    struct sysctl_clk_th0_t clk_th0;
    /* No. 15 (0x3c): Clock threshold controller 1 */
    struct sysctl_clk_th1_t clk_th1;
    /* No. 16 (0x40): Clock threshold controller 2 */
    struct sysctl_clk_th2_t clk_th2;
    /* No. 17 (0x44): Clock threshold controller 3 */
    struct sysctl_clk_th3_t clk_th3;
    /* No. 18 (0x48): Clock threshold controller 4 */
    struct sysctl_clk_th4_t clk_th4;
    /* No. 19 (0x4c): Clock threshold controller 5 */
    struct sysctl_clk_th5_t clk_th5;
    /* No. 20 (0x50): Clock threshold controller 6 */
    struct sysctl_clk_th6_t clk_th6;
    /* No. 21 (0x54): Miscellaneous controller */
    struct sysctl_misc_t misc;
    /* No. 22 (0x58): Peripheral controller */
    struct sysctl_peri_t peri;
    /* No. 23 (0x5c): SPI sleep controller */
    struct sysctl_spi_sleep_t spi_sleep;
    /* No. 24 (0x60): Reset source status */
    struct sysctl_reset_status_t reset_status;
    /* No. 25 (0x64): DMA handshake selector */
    struct sysctl_dma_sel0_t dma_sel0;
    /* No. 26 (0x68): DMA handshake selector */
    struct sysctl_dma_sel1_t dma_sel1;
    /* No. 27 (0x6c): IO Power Mode Select controller */
    struct sysctl_power_sel_t power_sel;
    /* No. 28 (0x70): Reserved */
    uint32_t resv28;
    /* No. 29 (0x74): Reserved */
    uint32_t resv29;
    /* No. 30 (0x78): Reserved */
    uint32_t resv30;
    /* No. 31 (0x7c): Reserved */
    uint32_t resv31;
} __attribute__((packed, aligned(4)));
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define SYSCTRL_CLOCK_FREQ_IN0 (26000000UL)

const uint8_t get_select_pll2[] =
{
    [SYSCTL_SOURCE_IN0] = 0,
    [SYSCTL_SOURCE_PLL0] = 1,
    [SYSCTL_SOURCE_PLL1] = 2,
};

const uint8_t get_source_pll2[] =
{
    [0] = SYSCTL_SOURCE_IN0,
    [1] = SYSCTL_SOURCE_PLL0,
    [2] = SYSCTL_SOURCE_PLL1,
};

const uint8_t get_select_aclk[] =
{
    [SYSCTL_SOURCE_IN0] = 0,
    [SYSCTL_SOURCE_PLL0] = 1,
};

const uint8_t get_source_aclk[] =
{
    [0] = SYSCTL_SOURCE_IN0,
    [1] = SYSCTL_SOURCE_PLL0,
};
/*********************************************************************************************************
  SYSCTL 控制器对象指针
*********************************************************************************************************/
volatile struct sysctl_t *const sysctl = (volatile struct sysctl_t *)SYSCTL_BASE_ADDR;


uint32_t sysctl_get_git_id(void)
{
    return sysctl->git_id.git_id;
}

uint32_t sysctl_get_freq(void)
{
    return sysctl->clk_freq.clk_freq;
}

static void sysctl_reset_ctl(enum sysctl_reset_e reset, uint8_t rst_value)
{
    switch (reset)
    {
        case SYSCTL_RESET_SOC:
            sysctl->soft_reset.soft_reset = rst_value;
            break;
        case SYSCTL_RESET_ROM:
            sysctl->peri_reset.rom_reset = rst_value;
            break;
        case SYSCTL_RESET_DMA:
            sysctl->peri_reset.dma_reset = rst_value;
            break;
        case SYSCTL_RESET_AI:
            sysctl->peri_reset.ai_reset = rst_value;
            break;
        case SYSCTL_RESET_DVP:
            sysctl->peri_reset.dvp_reset = rst_value;
            break;
        case SYSCTL_RESET_FFT:
            sysctl->peri_reset.fft_reset = rst_value;
            break;
        case SYSCTL_RESET_GPIO:
            sysctl->peri_reset.gpio_reset = rst_value;
            break;
        case SYSCTL_RESET_SPI0:
            sysctl->peri_reset.spi0_reset = rst_value;
            break;
        case SYSCTL_RESET_SPI1:
            sysctl->peri_reset.spi1_reset = rst_value;
            break;
        case SYSCTL_RESET_SPI2:
            sysctl->peri_reset.spi2_reset = rst_value;
            break;
        case SYSCTL_RESET_SPI3:
            sysctl->peri_reset.spi3_reset = rst_value;
            break;
        case SYSCTL_RESET_I2S0:
            sysctl->peri_reset.i2s0_reset = rst_value;
            break;
        case SYSCTL_RESET_I2S1:
            sysctl->peri_reset.i2s1_reset = rst_value;
            break;
        case SYSCTL_RESET_I2S2:
            sysctl->peri_reset.i2s2_reset = rst_value;
            break;
        case SYSCTL_RESET_I2C0:
            sysctl->peri_reset.i2c0_reset = rst_value;
            break;
        case SYSCTL_RESET_I2C1:
            sysctl->peri_reset.i2c1_reset = rst_value;
            break;
        case SYSCTL_RESET_I2C2:
            sysctl->peri_reset.i2c2_reset = rst_value;
            break;
        case SYSCTL_RESET_UART1:
            sysctl->peri_reset.uart1_reset = rst_value;
            break;
        case SYSCTL_RESET_UART2:
            sysctl->peri_reset.uart2_reset = rst_value;
            break;
        case SYSCTL_RESET_UART3:
            sysctl->peri_reset.uart3_reset = rst_value;
            break;
        case SYSCTL_RESET_AES:
            sysctl->peri_reset.aes_reset = rst_value;
            break;
        case SYSCTL_RESET_FPIOA:
            sysctl->peri_reset.fpioa_reset = rst_value;
            break;
        case SYSCTL_RESET_TIMER0:
            sysctl->peri_reset.timer0_reset = rst_value;
            break;
        case SYSCTL_RESET_TIMER1:
            sysctl->peri_reset.timer1_reset = rst_value;
            break;
        case SYSCTL_RESET_TIMER2:
            sysctl->peri_reset.timer2_reset = rst_value;
            break;
        case SYSCTL_RESET_WDT0:
            sysctl->peri_reset.wdt0_reset = rst_value;
            break;
        case SYSCTL_RESET_WDT1:
            sysctl->peri_reset.wdt1_reset = rst_value;
            break;
        case SYSCTL_RESET_SHA:
            sysctl->peri_reset.sha_reset = rst_value;
            break;
        case SYSCTL_RESET_RTC:
            sysctl->peri_reset.rtc_reset = rst_value;
            break;

        default:
            break;
    }
}

void sysctl_reset(enum sysctl_reset_e reset)
{
    sysctl_reset_ctl(reset, 1);
    sysctl_reset_ctl(reset, 0);
}

static int sysctl_clock_bus_en(enum sysctl_clock_e clock, uint8_t en)
{
    /*
     * The timer is under APB0, to prevent apb0_clk_en1 and apb0_clk_en0
     * on same register, we split it to peripheral and central two
     * registers, to protect CPU close apb0 clock accidentally.
     *
     * The apb0_clk_en0 and apb0_clk_en1 have same function,
     * one of them set, the APB0 clock enable.
     */

    /* The APB clock should carefully disable */
    if (en)
    {
        switch (clock)
        {
            /*
             * These peripheral devices are under APB0
             * GPIO, UART1, UART2, UART3, SPI_SLAVE, I2S0, I2S1,
             * I2S2, I2C0, I2C1, I2C2, FPIOA, SHA256, TIMER0,
             * TIMER1, TIMER2
             */
            case SYSCTL_CLOCK_GPIO:
            case SYSCTL_CLOCK_SPI2:
            case SYSCTL_CLOCK_I2S0:
            case SYSCTL_CLOCK_I2S1:
            case SYSCTL_CLOCK_I2S2:
            case SYSCTL_CLOCK_I2C0:
            case SYSCTL_CLOCK_I2C1:
            case SYSCTL_CLOCK_I2C2:
            case SYSCTL_CLOCK_UART1:
            case SYSCTL_CLOCK_UART2:
            case SYSCTL_CLOCK_UART3:
            case SYSCTL_CLOCK_FPIOA:
            case SYSCTL_CLOCK_TIMER0:
            case SYSCTL_CLOCK_TIMER1:
            case SYSCTL_CLOCK_TIMER2:
            case SYSCTL_CLOCK_SHA:
                sysctl->clk_en_cent.apb0_clk_en = en;
                break;

            /*
             * These peripheral devices are under APB1
             * WDT, AES, OTP, DVP, SYSCTL
             */
            case SYSCTL_CLOCK_AES:
            case SYSCTL_CLOCK_WDT0:
            case SYSCTL_CLOCK_WDT1:
            case SYSCTL_CLOCK_OTP:
            case SYSCTL_CLOCK_RTC:
                sysctl->clk_en_cent.apb1_clk_en = en;
                break;

            /*
             * These peripheral devices are under APB2
             * SPI0, SPI1
             */
            case SYSCTL_CLOCK_SPI0:
            case SYSCTL_CLOCK_SPI1:
                sysctl->clk_en_cent.apb2_clk_en = en;
                break;

            default:
                break;
        }
    }

    return 0;
}

static int sysctl_clock_device_en(enum sysctl_clock_e clock, uint8_t en)
{
    switch (clock)
    {
        /*
         * These devices are PLL
         */
        case SYSCTL_CLOCK_PLL0:
            sysctl->pll0.pll_out_en0 = en;
            break;
        case SYSCTL_CLOCK_PLL1:
            sysctl->pll1.pll_out_en1 = en;
            break;
        case SYSCTL_CLOCK_PLL2:
            sysctl->pll2.pll_out_en2 = en;
            break;

        /*
         * These devices are CPU, SRAM, APB bus, ROM, DMA, AI
         */
        case SYSCTL_CLOCK_CPU:
            sysctl->clk_en_cent.cpu_clk_en = en;
            break;
        case SYSCTL_CLOCK_SRAM0:
            sysctl->clk_en_cent.sram0_clk_en = en;
            break;
        case SYSCTL_CLOCK_SRAM1:
            sysctl->clk_en_cent.sram1_clk_en = en;
            break;
        case SYSCTL_CLOCK_APB0:
            sysctl->clk_en_cent.apb0_clk_en = en;
            break;
        case SYSCTL_CLOCK_APB1:
            sysctl->clk_en_cent.apb1_clk_en = en;
            break;
        case SYSCTL_CLOCK_APB2:
            sysctl->clk_en_cent.apb2_clk_en = en;
            break;
        case SYSCTL_CLOCK_ROM:
            sysctl->clk_en_peri.rom_clk_en = en;
            break;
        case SYSCTL_CLOCK_DMA:
            sysctl->clk_en_peri.dma_clk_en = en;
            break;
        case SYSCTL_CLOCK_AI:
            sysctl->clk_en_peri.ai_clk_en = en;
            break;
        case SYSCTL_CLOCK_DVP:
            sysctl->clk_en_peri.dvp_clk_en = en;
            break;
        case SYSCTL_CLOCK_FFT:
            sysctl->clk_en_peri.fft_clk_en = en;
            break;
        case SYSCTL_CLOCK_SPI3:
            sysctl->clk_en_peri.spi3_clk_en = en;
            break;

        /*
         * These peripheral devices are under APB0
         * GPIO, UART1, UART2, UART3, SPI_SLAVE, I2S0, I2S1,
         * I2S2, I2C0, I2C1, I2C2, FPIOA, SHA256, TIMER0,
         * TIMER1, TIMER2
         */
        case SYSCTL_CLOCK_GPIO:
            sysctl->clk_en_peri.gpio_clk_en = en;
            break;
        case SYSCTL_CLOCK_SPI2:
            sysctl->clk_en_peri.spi2_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2S0:
            sysctl->clk_en_peri.i2s0_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2S1:
            sysctl->clk_en_peri.i2s1_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2S2:
            sysctl->clk_en_peri.i2s2_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2C0:
            sysctl->clk_en_peri.i2c0_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2C1:
            sysctl->clk_en_peri.i2c1_clk_en = en;
            break;
        case SYSCTL_CLOCK_I2C2:
            sysctl->clk_en_peri.i2c2_clk_en = en;
            break;
        case SYSCTL_CLOCK_UART1:
            sysctl->clk_en_peri.uart1_clk_en = en;
            break;
        case SYSCTL_CLOCK_UART2:
            sysctl->clk_en_peri.uart2_clk_en = en;
            break;
        case SYSCTL_CLOCK_UART3:
            sysctl->clk_en_peri.uart3_clk_en = en;
            break;
        case SYSCTL_CLOCK_FPIOA:
            sysctl->clk_en_peri.fpioa_clk_en = en;
            break;
        case SYSCTL_CLOCK_TIMER0:
            sysctl->clk_en_peri.timer0_clk_en = en;
            break;
        case SYSCTL_CLOCK_TIMER1:
            sysctl->clk_en_peri.timer1_clk_en = en;
            break;
        case SYSCTL_CLOCK_TIMER2:
            sysctl->clk_en_peri.timer2_clk_en = en;
            break;
        case SYSCTL_CLOCK_SHA:
            sysctl->clk_en_peri.sha_clk_en = en;
            break;

        /*
         * These peripheral devices are under APB1
         * WDT, AES, OTP, DVP, SYSCTL
         */
        case SYSCTL_CLOCK_AES:
            sysctl->clk_en_peri.aes_clk_en = en;
            break;
        case SYSCTL_CLOCK_WDT0:
            sysctl->clk_en_peri.wdt0_clk_en = en;
            break;
        case SYSCTL_CLOCK_WDT1:
            sysctl->clk_en_peri.wdt1_clk_en = en;
            break;
        case SYSCTL_CLOCK_OTP:
            sysctl->clk_en_peri.otp_clk_en = en;
            break;
        case SYSCTL_CLOCK_RTC:
            sysctl->clk_en_peri.rtc_clk_en = en;
            break;

        /*
         * These peripheral devices are under APB2
         * SPI0, SPI1
         */
        case SYSCTL_CLOCK_SPI0:
            sysctl->clk_en_peri.spi0_clk_en = en;
            break;
        case SYSCTL_CLOCK_SPI1:
            sysctl->clk_en_peri.spi1_clk_en = en;
            break;

        default:
            break;
    }

    return 0;
}

int sysctl_clock_enable(enum sysctl_clock_e clock)
{
    if (clock >= SYSCTL_CLOCK_MAX)
        return -1;
    sysctl_clock_bus_en(clock, 1);
    sysctl_clock_device_en(clock, 1);
    return 0;
}

int sysctl_clock_disable(enum sysctl_clock_e clock)
{
    if (clock >= SYSCTL_CLOCK_MAX)
        return -1;
    sysctl_clock_bus_en(clock, 0);
    sysctl_clock_device_en(clock, 0);
    return 0;
}

int sysctl_clock_set_threshold(enum sysctl_threshold_e which, int threshold)
{
    int result = 0;
    switch (which)
    {
        /*
         * These threshold is 2 bit width
         */
    case SYSCTL_THRESHOLD_ACLK:
        sysctl->clk_sel0.aclk_divider_sel = (uint8_t)threshold & 0x03;
        break;

        /*
         * These threshold is 3 bit width
         */
        case SYSCTL_THRESHOLD_APB0:
            sysctl->clk_sel0.apb0_clk_sel = (uint8_t)threshold & 0x07;
            break;
        case SYSCTL_THRESHOLD_APB1:
            sysctl->clk_sel0.apb1_clk_sel = (uint8_t)threshold & 0x07;
            break;
        case SYSCTL_THRESHOLD_APB2:
            sysctl->clk_sel0.apb2_clk_sel = (uint8_t)threshold & 0x07;
            break;

        /*
         * These threshold is 4 bit width
         */
        case SYSCTL_THRESHOLD_SRAM0:
            sysctl->clk_th0.sram0_gclk_threshold = (uint8_t)threshold & 0x0F;
            break;
        case SYSCTL_THRESHOLD_SRAM1:
            sysctl->clk_th0.sram1_gclk_threshold = (uint8_t)threshold & 0x0F;
            break;
        case SYSCTL_THRESHOLD_AI:
            sysctl->clk_th0.ai_gclk_threshold = (uint8_t)threshold & 0x0F;
            break;
        case SYSCTL_THRESHOLD_DVP:
            sysctl->clk_th0.dvp_gclk_threshold = (uint8_t)threshold & 0x0F;
            break;
        case SYSCTL_THRESHOLD_ROM:
            sysctl->clk_th0.rom_gclk_threshold = (uint8_t)threshold & 0x0F;
            break;

        /*
         * These threshold is 8 bit width
         */
        case SYSCTL_THRESHOLD_SPI0:
            sysctl->clk_th1.spi0_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_SPI1:
            sysctl->clk_th1.spi1_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_SPI2:
            sysctl->clk_th1.spi2_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_SPI3:
            sysctl->clk_th1.spi3_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER0:
            sysctl->clk_th2.timer0_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER1:
            sysctl->clk_th2.timer1_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER2:
            sysctl->clk_th2.timer2_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2S0_M:
            sysctl->clk_th4.i2s0_mclk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2S1_M:
            sysctl->clk_th4.i2s1_mclk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2S2_M:
            sysctl->clk_th5.i2s2_mclk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2C0:
            sysctl->clk_th5.i2c0_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2C1:
            sysctl->clk_th5.i2c1_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2C2:
            sysctl->clk_th5.i2c2_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_WDT0:
            sysctl->clk_th6.wdt0_clk_threshold = (uint8_t)threshold;
            break;
        case SYSCTL_THRESHOLD_WDT1:
            sysctl->clk_th6.wdt1_clk_threshold = (uint8_t)threshold;
            break;

        /*
         * These threshold is 16 bit width
         */
        case SYSCTL_THRESHOLD_I2S0:
            sysctl->clk_th3.i2s0_clk_threshold = (uint16_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2S1:
            sysctl->clk_th3.i2s1_clk_threshold = (uint16_t)threshold;
            break;
        case SYSCTL_THRESHOLD_I2S2:
            sysctl->clk_th4.i2s2_clk_threshold = (uint16_t)threshold;
            break;

    default:
        result = -1;
        break;
    }
    return result;
}

int sysctl_clock_get_threshold(enum sysctl_threshold_e which)
{
    int threshold = 0;

    switch (which)
    {
        /*
         * Select and get threshold value
         */
        case SYSCTL_THRESHOLD_ACLK:
            threshold = (int)sysctl->clk_sel0.aclk_divider_sel;
            break;
        case SYSCTL_THRESHOLD_APB0:
            threshold = (int)sysctl->clk_sel0.apb0_clk_sel;
            break;
        case SYSCTL_THRESHOLD_APB1:
            threshold = (int)sysctl->clk_sel0.apb1_clk_sel;
            break;
        case SYSCTL_THRESHOLD_APB2:
            threshold = (int)sysctl->clk_sel0.apb2_clk_sel;
            break;
        case SYSCTL_THRESHOLD_SRAM0:
            threshold = (int)sysctl->clk_th0.sram0_gclk_threshold;
            break;
        case SYSCTL_THRESHOLD_SRAM1:
            threshold = (int)sysctl->clk_th0.sram1_gclk_threshold;
            break;
        case SYSCTL_THRESHOLD_AI:
            threshold = (int)sysctl->clk_th0.ai_gclk_threshold;
            break;
        case SYSCTL_THRESHOLD_DVP:
            threshold = (int)sysctl->clk_th0.dvp_gclk_threshold;
            break;
        case SYSCTL_THRESHOLD_ROM:
            threshold = (int)sysctl->clk_th0.rom_gclk_threshold;
            break;
        case SYSCTL_THRESHOLD_SPI0:
            threshold = (int)sysctl->clk_th1.spi0_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_SPI1:
            threshold = (int)sysctl->clk_th1.spi1_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_SPI2:
            threshold = (int)sysctl->clk_th1.spi2_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_SPI3:
            threshold = (int)sysctl->clk_th1.spi3_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER0:
            threshold = (int)sysctl->clk_th2.timer0_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER1:
            threshold = (int)sysctl->clk_th2.timer1_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_TIMER2:
            threshold = (int)sysctl->clk_th2.timer2_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S0:
            threshold = (int)sysctl->clk_th3.i2s0_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S1:
            threshold = (int)sysctl->clk_th3.i2s1_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S2:
            threshold = (int)sysctl->clk_th4.i2s2_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S0_M:
            threshold = (int)sysctl->clk_th4.i2s0_mclk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S1_M:
            threshold = (int)sysctl->clk_th4.i2s1_mclk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2S2_M:
            threshold = (int)sysctl->clk_th5.i2s2_mclk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2C0:
            threshold = (int)sysctl->clk_th5.i2c0_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2C1:
            threshold = (int)sysctl->clk_th5.i2c1_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_I2C2:
            threshold = (int)sysctl->clk_th5.i2c2_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_WDT0:
            threshold = (int)sysctl->clk_th6.wdt0_clk_threshold;
            break;
        case SYSCTL_THRESHOLD_WDT1:
            threshold = (int)sysctl->clk_th6.wdt1_clk_threshold;
            break;

        default:
            break;
    }

    return threshold;
}

int sysctl_clock_set_clock_select(enum sysctl_clock_select_e which, int select)
{
    int result = 0;
    switch (which)
    {
        /*
         * These clock select is 1 bit width
         */
        case SYSCTL_CLOCK_SELECT_PLL0_BYPASS:
            sysctl->pll0.pll_bypass0 = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_PLL1_BYPASS:
            sysctl->pll1.pll_bypass1 = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_PLL3_BYPASS:
            sysctl->pll2.pll_bypass2 = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_ACLK:
            sysctl->clk_sel0.aclk_sel = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_SPI3:
            sysctl->clk_sel0.spi3_clk_sel = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER0:
            sysctl->clk_sel0.timer0_clk_sel = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER1:
            sysctl->clk_sel0.timer1_clk_sel = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER2:
            sysctl->clk_sel0.timer2_clk_sel = select & 0x01;
            break;
        case SYSCTL_CLOCK_SELECT_SPI3_SAMPLE:
            sysctl->clk_sel1.spi3_sample_clk_sel = select & 0x01;
            break;

        /*
         * These clock select is 2 bit width
         */
        case SYSCTL_CLOCK_SELECT_PLL2:
            sysctl->pll2.pll_ckin_sel2 = select & 0x03;
            break;

    default:
        result = -1;
        break;
    }

    return result;
}

int sysctl_clock_get_clock_select(enum sysctl_clock_select_e which)
{
    int clock_select = 0;

    switch (which)
    {
        /*
         * Select and get clock select value
         */
        case SYSCTL_CLOCK_SELECT_PLL0_BYPASS:
            clock_select = (int)sysctl->pll0.pll_bypass0;
            break;
        case SYSCTL_CLOCK_SELECT_PLL1_BYPASS:
            clock_select = (int)sysctl->pll1.pll_bypass1;
            break;
        case SYSCTL_CLOCK_SELECT_PLL3_BYPASS:
            clock_select = (int)sysctl->pll2.pll_bypass2;
            break;
        case SYSCTL_CLOCK_SELECT_PLL2:
            clock_select = (int)sysctl->pll2.pll_ckin_sel2;
            break;
        case SYSCTL_CLOCK_SELECT_ACLK:
            clock_select = (int)sysctl->clk_sel0.aclk_sel;
            break;
        case SYSCTL_CLOCK_SELECT_SPI3:
            clock_select = (int)sysctl->clk_sel0.spi3_clk_sel;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER0:
            clock_select = (int)sysctl->clk_sel0.timer0_clk_sel;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER1:
            clock_select = (int)sysctl->clk_sel0.timer1_clk_sel;
            break;
        case SYSCTL_CLOCK_SELECT_TIMER2:
            clock_select = (int)sysctl->clk_sel0.timer2_clk_sel;
            break;
        case SYSCTL_CLOCK_SELECT_SPI3_SAMPLE:
            clock_select = (int)sysctl->clk_sel1.spi3_sample_clk_sel;
            break;

        default:
            break;
    }

    return clock_select;
}

uint32_t sysctl_clock_source_get_freq(enum sysctl_clock_source_e input)
{
    uint32_t result;

    switch (input)
    {
        case SYSCTL_SOURCE_IN0:
            result = SYSCTRL_CLOCK_FREQ_IN0;
            break;
        case SYSCTL_SOURCE_PLL0:
            result = sysctl_pll_get_freq(SYSCTL_PLL0);
            break;
        case SYSCTL_SOURCE_PLL1:
            result = sysctl_pll_get_freq(SYSCTL_PLL1);
            break;
        case SYSCTL_SOURCE_PLL2:
            result = sysctl_pll_get_freq(SYSCTL_PLL2);
            break;
        case SYSCTL_SOURCE_ACLK:
            result = sysctl_clock_get_freq(SYSCTL_CLOCK_ACLK);
            break;

        default:
            result = 0;
            break;
    }
    return result;
}

int sysctl_pll_is_lock(enum sysctl_pll_e pll)
{
    /*
     * All bit enable means PLL lock
     *
     * struct pll_lock_t
     * {
     *         uint8_t overflow : 1;
     *         uint8_t rfslip : 1;
     *         uint8_t fbslip : 1;
     * };
     *
     */

    uint8_t lock = 0;

    if (pll >= SYSCTL_PLL_MAX)
        return 0;

    switch (pll)
    {
        case SYSCTL_PLL0:
            lock = sysctl->pll_lock.pll_lock0;
            break;

        case SYSCTL_PLL1:
            lock = sysctl->pll_lock.pll_lock1;
            break;

        case SYSCTL_PLL2:
            lock = sysctl->pll_lock.pll_lock2;
            break;

        default:
            break;
    }

    if (lock == 3)
        return 1;

    return 0;
}

int sysctl_pll_clear_slip(enum sysctl_pll_e pll)
{
    if (pll >= SYSCTL_PLL_MAX)
        return -1;

    switch (pll)
    {
        case SYSCTL_PLL0:
            sysctl->pll_lock.pll_slip_clear0 = 1;
            break;

        case SYSCTL_PLL1:
            sysctl->pll_lock.pll_slip_clear1 = 1;
            break;

        case SYSCTL_PLL2:
            sysctl->pll_lock.pll_slip_clear2 = 1;
            break;

        default:
            break;
    }

    return sysctl_pll_is_lock(pll) ? 0 : -1;
}

int sysctl_pll_enable(enum sysctl_pll_e pll)
{
    /*
     *       ---+
     * PWRDN    |
     *          +-------------------------------------------------------------
     *          ^
     *          |
     *          |
     *          t1
     *                 +------------------+
     * RESET           |                  |
     *       ----------+                  +-----------------------------------
     *                 ^                  ^                              ^
     *                 |<----- t_rst ---->|<---------- t_lock ---------->|
     *                 |                  |                              |
     *                 t2                 t3                             t4
     */

    if (pll >= SYSCTL_PLL_MAX)
        return -1;

    switch (pll)
    {
        case SYSCTL_PLL0:
            /* Do not bypass PLL */
            sysctl->pll0.pll_bypass0 = 0;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll0.pll_pwrd0 = 1;
            /*
             * Reset trigger of the PLL, connected RESET
             * 0 is free
             * 1 is reset
             */
            sysctl->pll0.pll_reset0 = 0;
            sysctl->pll0.pll_reset0 = 1;
            asm volatile ("nop");
            asm volatile ("nop");
            sysctl->pll0.pll_reset0 = 0;
            break;

        case SYSCTL_PLL1:
            /* Do not bypass PLL */
            sysctl->pll1.pll_bypass1 = 0;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll1.pll_pwrd1 = 1;
            /*
             * Reset trigger of the PLL, connected RESET
             * 0 is free
             * 1 is reset
             */
            sysctl->pll1.pll_reset1 = 0;
            sysctl->pll1.pll_reset1 = 1;
            asm volatile ("nop");
            asm volatile ("nop");
            sysctl->pll1.pll_reset1 = 0;
            break;

        case SYSCTL_PLL2:
            /* Do not bypass PLL */
            sysctl->pll2.pll_bypass2 = 0;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll2.pll_pwrd2 = 1;
            /*
             * Reset trigger of the PLL, connected RESET
             * 0 is free
             * 1 is reset
             */
            sysctl->pll2.pll_reset2 = 0;
            sysctl->pll2.pll_reset2 = 1;
            asm volatile ("nop");
            asm volatile ("nop");
            sysctl->pll2.pll_reset2 = 0;
            break;

        default:
            break;
    }

    return 0;
}

int sysctl_pll_disable(enum sysctl_pll_e pll)
{
    if (pll >= SYSCTL_PLL_MAX)
        return -1;

    switch (pll)
    {
        case SYSCTL_PLL0:
            /* Bypass PLL */
            sysctl->pll0.pll_bypass0 = 1;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll0.pll_pwrd0 = 0;
            break;

        case SYSCTL_PLL1:
            /* Bypass PLL */
            sysctl->pll1.pll_bypass1 = 1;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll1.pll_pwrd1 = 0;
            break;

        case SYSCTL_PLL2:
            /* Bypass PLL */
            sysctl->pll2.pll_bypass2 = 1;
            /*
             * Power on the PLL, negtive from PWRDN
             * 0 is power off
             * 1 is power on
             */
            sysctl->pll2.pll_pwrd2 = 0;
            break;

        default:
            break;
    }

    return 0;
}

uint32_t sysctl_pll_get_freq(enum sysctl_pll_e pll)
{
    uint32_t freq_in = 0, freq_out = 0;
    uint32_t nr = 0, nf = 0, od = 0;
    uint8_t select = 0;

    if (pll >= SYSCTL_PLL_MAX)
        return 0;

    switch (pll)
    {
        case SYSCTL_PLL0:
            freq_in = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            nr      = sysctl->pll0.clkr0 + 1;
            nf      = sysctl->pll0.clkf0 + 1;
            od      = sysctl->pll0.clkod0 + 1;
            break;

        case SYSCTL_PLL1:
            freq_in = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            nr      = sysctl->pll1.clkr1 + 1;
            nf      = sysctl->pll1.clkf1 + 1;
            od      = sysctl->pll1.clkod1 + 1;
            break;

        case SYSCTL_PLL2:
            /*
             * Get input frequency accroding select register
             */
            select = sysctl->pll2.pll_ckin_sel2;
            if (select < sizeof(get_source_pll2))
                freq_in = sysctl_clock_source_get_freq(get_source_pll2[select]);
            else
                return 0;

            nr      = sysctl->pll2.clkr2 + 1;
            nf      = sysctl->pll2.clkf2 + 1;
            od      = sysctl->pll2.clkod2 + 1;
            break;

        default:
            break;
    }

    /*
     * Get final PLL output frequency
     * FOUT = FIN / NR * NF / OD
     */
    freq_out = (double)freq_in / (double)nr * (double)nf / (double)od;
    return freq_out;
}

uint32_t sysctl_pll_set_freq(enum sysctl_pll_e pll, enum sysctl_clock_source_e source, uint32_t freq)
{
    uint32_t freq_in = 0;

    if (pll >= SYSCTL_PLL_MAX)
        return 0;

    if (source >= SYSCTL_SOURCE_MAX)
        return 0;

    switch (pll)
    {
        case SYSCTL_PLL0:
        case SYSCTL_PLL1:
            /*
             * Check input clock source
             */
            if (source != SYSCTL_SOURCE_IN0)
                return 0;
            freq_in = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            /*
             * Check input clock frequency
             */
            if (freq_in == 0)
                return 0;
            break;

        case SYSCTL_PLL2:
            /*
             * Check input clock source
             */
            if (source < sizeof(get_select_pll2))
                freq_in = sysctl_clock_source_get_freq(source);
            /*
             * Check input clock frequency
             */
            if (freq_in == 0)
                return 0;
            break;

        default:
            return 0;
    }

    /*
     * Begin calculate PLL registers' value
     */

    /* constants */
    const double vco_min = 3.5e+08;
    const double vco_max = 1.75e+09;
    const double ref_min = 1.36719e+07;
    const double ref_max = 1.75e+09;
    const int nr_min     = 1;
    const int nr_max     = 16;
    const int nf_min     = 1;
    const int nf_max     = 64;
    const int no_min     = 1;
    const int no_max     = 16;
    const int nb_min     = 1;
    const int nb_max     = 64;
    const int max_vco    = 1;
    const int ref_rng    = 1;

    /* variables */
    int nr     = 0;
    int nrx    = 0;
    int nf     = 0;
    int nfi    = 0;
    int no     = 0;
    int noe    = 0;
    int not    = 0;
    int nor    = 0;
    int nore   = 0;
    int nb     = 0;
    int first  = 0;
    int firstx = 0;
    int found  = 0;

    long long nfx = 0;
    double fin = 0, fout = 0, fvco = 0;
    double val = 0, nval = 0, err = 0, merr = 0, terr = 0;
    int x_nrx = 0, x_no = 0, x_nb = 0;
    long long x_nfx = 0;
    double x_fvco = 0, x_err = 0;

    fin   = freq_in;
    fout  = freq;
    val   = fout / fin;
    terr  = 0.5 / ((double)(nf_max / 2));
    first = firstx = 1;
    if (terr != -2)
    {
        first = 0;
        if (terr == 0)
            terr = 1e-16;
        merr = fabs(terr);
    }
    found = 0;
    for (nfi = val; nfi < nf_max; ++nfi)
    {
        nr = rint(((double)nfi) / val);
        if (nr == 0)
            continue;
        if ((ref_rng) && (nr < nr_min))
            continue;
        if (fin / ((double)nr) > ref_max)
            continue;
        nrx = nr;
        nf = nfx = nfi;
        nval = ((double)nfx) / ((double)nr);
        if (nf == 0)
            nf = 1;
        err = 1 - nval / val;

        if ((first) || (fabs(err) < merr * (1 + 1e-6)) || (fabs(err) < 1e-16))
        {
            not = floor(vco_max / fout);
            for (no = (not > no_max) ? no_max : not; no > no_min; --no)
            {
                if ((ref_rng) && ((nr / no) < nr_min))
                    continue;
                if ((nr % no) == 0)
                    break;
            }
            if ((nr % no) != 0)
                continue;
            nor  = ((not > no_max) ? no_max : not) / no;
            nore = nf_max / nf;
            if (nor > nore)
                nor = nore;
            noe  = ceil(vco_min / fout);
            if (!max_vco)
            {
                nore = (noe - 1) / no + 1;
                nor  = nore;
                not  = 0; /* force next if to fail */
            }
            if ((((no * nor) < (not >> 1)) || ((no * nor) < noe)) && ((no * nor) < (nf_max / nf)))
            {
                no = nf_max / nf;
                if (no > no_max)
                    no = no_max;
                if (no > not)
                    no = not;
                nfx *= no;
                nf *= no;
                if ((no > 1) && (!firstx))
                    continue;
                /* wait for larger nf in later iterations */
            }
            else
            {
                nrx /= no;
                nfx *= nor;
                nf *= nor;
                no *= nor;
                if (no > no_max)
                    continue;
                if ((nor > 1) && (!firstx))
                    continue;
                /* wait for larger nf in later iterations */
            }

            nb = nfx;
            if (nb < nb_min)
                nb = nb_min;
            if (nb > nb_max)
                continue;

            fvco = fin / ((double)nrx) * ((double)nfx);
            if (fvco < vco_min)
                continue;
            if (fvco > vco_max)
                continue;
            if (nf < nf_min)
                continue;
            if ((ref_rng) && (fin / ((double)nrx) < ref_min))
                continue;
            if ((ref_rng) && (nrx > nr_max))
                continue;
            if (!(((firstx) && (terr < 0)) || (fabs(err) < merr * (1 - 1e-6)) || ((max_vco) && (no > x_no))))
                continue;
            if ((!firstx) && (terr >= 0) && (nrx > x_nrx))
                continue;

            found  = 1;
            x_no   = no;
            x_nrx  = nrx;
            x_nfx  = nfx;
            x_nb   = nb;
            x_fvco = fvco;
            x_err  = err;
            first  = firstx = 0;
            merr   = fabs(err);
            if (terr != -1)
                continue;
        }
    }
    if (!found)
    {
        return 0;
    }

    nrx  = x_nrx;
    nfx  = x_nfx;
    no   = x_no;
    nb   = x_nb;
    fvco = x_fvco;
    err  = x_err;
    if ((terr != -2) && (fabs(err) >= terr * (1 - 1e-6)))
    {
        return 0;
    }

#ifdef CONFIG_PLL_DEBUG_ENABLE
    printf("NR = %d\n", nrx);
    printf("NF = %lld\n", nfx);
    printf("OD = %d\n", no);
    printf("NB = %d\n", nb);

    printf("\n");
    printf("Fin  = %g\n", fin);
    printf("Fvco = %g\n", fvco);
    printf("Fout = %g\n", fvco / no);
    printf("error = %+g\n", err);

    printf("\n");
    printf("CLKR[3:0] = %02x\n", nrx - 1);
    printf("CLKF[5:0] = %02x\n", (unsigned int)nfx - 1);
    printf("CLKOD[3:0] = %02x\n", no - 1);
    printf("BWADJ[5:0] = %02x\n", nb - 1);
#endif /* CONFIG_PLL_DEBUG_ENABLE */

    /*
     * Begin write PLL registers' value,
     * Using atomic write method.
     */
    struct sysctl_pll0_t pll0;
    struct sysctl_pll1_t pll1;
    struct sysctl_pll2_t pll2;

    switch (pll)
    {
        case SYSCTL_PLL0:
            /* Read register from bus */
            pll0 = sysctl->pll0;
            /* Set register temporary value */
            pll0.clkr0  = nrx - 1;
            pll0.clkf0  = nfx - 1;
            pll0.clkod0 = no - 1;
            pll0.bwadj0 = nb - 1;
            /* Write register back to bus */
            sysctl->pll0 = pll0;
            break;

        case SYSCTL_PLL1:
            /* Read register from bus */
            pll1 = sysctl->pll1;
            /* Set register temporary value */
            pll1.clkr1  = nrx - 1;
            pll1.clkf1  = nfx - 1;
            pll1.clkod1 = no - 1;
            pll1.bwadj1 = nb - 1;
            /* Write register back to bus */
            sysctl->pll1 = pll1;
            break;

        case SYSCTL_PLL2:
            /* Read register from bus */
            pll2 = sysctl->pll2;
            /* Set register temporary value */
            if (source < sizeof(get_select_pll2))
                pll2.pll_ckin_sel2 = get_select_pll2[source];

            pll2.clkr2  = nrx - 1;
            pll2.clkf2  = nfx - 1;
            pll2.clkod2 = no - 1;
            pll2.bwadj2 = nb - 1;
            /* Write register back to bus */
            sysctl->pll2 = pll2;
            break;

        default:
            return 0;
    }

    return sysctl_pll_get_freq(pll);
}

uint32_t sysctl_clock_get_freq(enum sysctl_clock_e clock)
{
    uint32_t source = 0;
    uint32_t result = 0;

    switch (clock)
    {
        /*
         * The clock IN0
         */
        case SYSCTL_CLOCK_IN0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            result = source;
            break;

        /*
         * These clock directly under PLL clock domain
         * They are using gated divider.
         */
        case SYSCTL_CLOCK_PLL0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source;
            break;
        case SYSCTL_CLOCK_PLL1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL1);
            result = source;
            break;
        case SYSCTL_CLOCK_PLL2:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL2);
            result = source;
            break;

        /*
         * These clock directly under ACLK clock domain
         */
        case SYSCTL_CLOCK_CPU:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_ACLK))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0) /
                        (2ULL << sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ACLK));
                    break;
                default:
                    break;
            }
            result = source;
            break;
        case SYSCTL_CLOCK_DMA:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_ACLK))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0) /
                        (2ULL << sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ACLK));
                    break;
                default:
                    break;
            }
            result = source;
            break;
        case SYSCTL_CLOCK_FFT:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_ACLK))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0) /
                        (2ULL << sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ACLK));
                    break;
                default:
                    break;
            }
            result = source;
            break;
        case SYSCTL_CLOCK_ACLK:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_ACLK))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0) /
                        (2ULL << sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ACLK));
                    break;
                default:
                    break;
            }
            result = source;
            break;
        case SYSCTL_CLOCK_HCLK:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_ACLK))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0) /
                        (2ULL << sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ACLK));
                    break;
                default:
                    break;
            }
            result = source;
            break;

        /*
         * These clock under ACLK clock domain.
         * They are using gated divider.
         */
        case SYSCTL_CLOCK_SRAM0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SRAM0) + 1);
            break;
        case SYSCTL_CLOCK_SRAM1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SRAM1) + 1);
            break;
        case SYSCTL_CLOCK_ROM:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_ROM) + 1);
            break;
        case SYSCTL_CLOCK_DVP:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_DVP) + 1);
            break;

        /*
         * These clock under ACLK clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_APB0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_APB0) + 1);
            break;
        case SYSCTL_CLOCK_APB1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_APB1) + 1);
            break;
        case SYSCTL_CLOCK_APB2:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_ACLK);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_APB2) + 1);
            break;

        /*
         * These clock under AI clock domain.
         * They are using gated divider.
         */
        case SYSCTL_CLOCK_AI:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL1);
            result = source / (sysctl_clock_get_threshold(SYSCTL_THRESHOLD_AI) + 1);
            break;

        /*
         * These clock under I2S clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_I2S0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL2);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2S0) + 1) * 2);
            break;
        case SYSCTL_CLOCK_I2S1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL2);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2S1) + 1) * 2);
            break;
        case SYSCTL_CLOCK_I2S2:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL2);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2S2) + 1) * 2);
            break;

        /*
         * These clock under WDT clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_WDT0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_WDT0) + 1) * 2);
            break;
        case SYSCTL_CLOCK_WDT1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_WDT1) + 1) * 2);
            break;

        /*
         * These clock under PLL0 clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_SPI0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SPI0) + 1) * 2);
            break;
        case SYSCTL_CLOCK_SPI1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SPI1) + 1) * 2);
            break;
        case SYSCTL_CLOCK_SPI2:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SPI2) + 1) * 2);
            break;
        case SYSCTL_CLOCK_I2C0:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2C0) + 1) * 2);
            break;
        case SYSCTL_CLOCK_I2C1:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2C1) + 1) * 2);
            break;
        case SYSCTL_CLOCK_I2C2:
            source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_I2C2) + 1) * 2);
            break;

        /*
         * These clock under PLL0_SEL clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_SPI3:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_SPI3))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
                    break;
                default:
                    break;
            }

            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_SPI3) + 1) * 2);
            break;
        case SYSCTL_CLOCK_TIMER0:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_TIMER0))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
                    break;
                default:
                    break;
            }

            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_TIMER0) + 1) * 2);
            break;
        case SYSCTL_CLOCK_TIMER1:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_TIMER1))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
                    break;
                default:
                    break;
            }

            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_TIMER1) + 1) * 2);
            break;
        case SYSCTL_CLOCK_TIMER2:
            switch (sysctl_clock_get_clock_select(SYSCTL_CLOCK_SELECT_TIMER2))
            {
                case 0:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
                    break;
                case 1:
                    source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_PLL0);
                    break;
                default:
                    break;
            }

            result = source / ((sysctl_clock_get_threshold(SYSCTL_THRESHOLD_TIMER2) + 1) * 2);
            break;

        /*
         * These clock under MISC clock domain.
         * They are using even divider.
         */

        /*
         * These clock under APB0 clock domain.
         * They are using even divider.
         */
        case SYSCTL_CLOCK_GPIO:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;
        case SYSCTL_CLOCK_UART1:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;
        case SYSCTL_CLOCK_UART2:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;
        case SYSCTL_CLOCK_UART3:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;
        case SYSCTL_CLOCK_FPIOA:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;
        case SYSCTL_CLOCK_SHA:
            source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB0);
            result = source;
            break;

        /*
         * These clock under APB1 clock domain.
         * They are using even divider.
         */
    case SYSCTL_CLOCK_AES:
        source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB1);
        result = source;
        break;
    case SYSCTL_CLOCK_OTP:
        source = sysctl_clock_get_freq(SYSCTL_CLOCK_APB1);
        result = source;
        break;
    case SYSCTL_CLOCK_RTC:
        source = sysctl_clock_source_get_freq(SYSCTL_SOURCE_IN0);
        result = source;
        break;

        /*
         * These clock under APB2 clock domain.
         * They are using even divider.
         */
        /*
         * Do nothing.
         */
        default:
            break;
    }
    return result;
}

int sysctl_dma_select(enum sysctl_dma_channel_e channel, enum sysctl_dma_select_e select)
{
    struct sysctl_dma_sel0_t dma_sel0;
    struct sysctl_dma_sel1_t dma_sel1;

    /* Read register from bus */
    dma_sel0 = sysctl->dma_sel0;
    dma_sel1 = sysctl->dma_sel1;
    switch (channel)
    {
        case SYSCTL_DMA_CHANNEL_0:
            dma_sel0.dma_sel0 = select;
            break;

        case SYSCTL_DMA_CHANNEL_1:
            dma_sel0.dma_sel1 = select;
            break;

        case SYSCTL_DMA_CHANNEL_2:
            dma_sel0.dma_sel2 = select;
            break;

        case SYSCTL_DMA_CHANNEL_3:
            dma_sel0.dma_sel3 = select;
            break;

        case SYSCTL_DMA_CHANNEL_4:
            dma_sel0.dma_sel4 = select;
            break;

        case SYSCTL_DMA_CHANNEL_5:
            dma_sel1.dma_sel5 = select;
            break;

        default:
            return -1;
    }

    /* Write register back to bus */
    sysctl->dma_sel0 = dma_sel0;
    sysctl->dma_sel1 = dma_sel1;

    return 0;
}

uint32_t sysctl_pll_fast_enable_pll(void)
{
    /*
     * Begin write PLL registers' value,
     * Using atomic write method.
     */
    struct sysctl_pll0_t pll0;
    struct sysctl_pll1_t pll1;
    struct sysctl_pll2_t pll2;

    /* Read register from bus */
    pll0 = sysctl->pll0;
    pll1 = sysctl->pll1;
    pll2 = sysctl->pll2;

    /* PLL VCO MAX freq: 1.8GHz */

    /* PLL0: 26M reference clk get 793M output clock */
    pll0.clkr0  = 0;
    pll0.clkf0  = 60;
    pll0.clkod0 = 1;
    pll0.bwadj0 = 60;

    /* PLL1: 26M reference clk get 390M output clock */
    pll1.clkr1  = 0;
    pll1.clkf1  = 59;
    pll1.clkod1 = 3;
    pll1.bwadj1 = 59;

    /* PLL2: 26M reference clk get 390M output clock */
    pll2.clkr2  = 0;
    pll2.clkf2  = 59;
    pll2.clkod2 = 3;
    pll2.bwadj2 = 59;

    /* Write register to bus */
    sysctl->pll0 = pll0;
    sysctl->pll1 = pll1;
    sysctl->pll2 = pll2;

    sysctl_pll_enable(SYSCTL_PLL0);
    sysctl_pll_enable(SYSCTL_PLL1);
    sysctl_pll_enable(SYSCTL_PLL2);

    while (sysctl_pll_is_lock(SYSCTL_PLL0) == 0)
        sysctl_pll_clear_slip(SYSCTL_PLL0);
    while (sysctl_pll_is_lock(SYSCTL_PLL1) == 0)
        sysctl_pll_clear_slip(SYSCTL_PLL1);
    while (sysctl_pll_is_lock(SYSCTL_PLL2) == 0)
        sysctl_pll_clear_slip(SYSCTL_PLL2);

    sysctl_clock_enable(SYSCTL_CLOCK_PLL0);
    sysctl_clock_enable(SYSCTL_CLOCK_PLL1);
    sysctl_clock_enable(SYSCTL_CLOCK_PLL2);

    /* Set ACLK to PLL0 */
    sysctl_clock_set_clock_select(SYSCTL_CLOCK_SELECT_ACLK, SYSCTL_SOURCE_PLL0);

    return 0;
}

uint32_t sysctl_spi0_dvp_data_set(uint8_t en)
{
    sysctl->misc.spi_dvp_data_enable = en;
    return 0;
}

uint32_t sysctl_power_mode_sel(uint8_t power_bank, io_power_mode_t io_power_mode)
{
    if(io_power_mode)
        *((uint32_t *)(&sysctl->power_sel)) |= (1 << power_bank);
    else
        *((uint32_t *)(&sysctl->power_sel)) &= ~(1 << power_bank);
	
    return 0;
}

uint32_t sysctl_cpu_set_freq(uint32_t freq)
{
    if(freq == 0)
        return 0;

    return sysctl_pll_set_freq(SYSCTL_PLL0, SYSCTL_SOURCE_IN0, (sysctl->clk_sel0.aclk_divider_sel + 1) * 2 * freq);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
