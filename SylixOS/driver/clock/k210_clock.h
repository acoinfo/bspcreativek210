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
** 文   件   名: k210_clock.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 18 日
**
** 描        述: 系统时钟控制器驱动头文件
*********************************************************************************************************/
#ifndef __K210_CLOCK_H
#define __K210_CLOCK_H
/*********************************************************************************************************
 * @brief      System controller register
 *
 * @note       System controller register table
 *
 * | Offset    | Name           | Description                         |
 * |-----------|----------------|-------------------------------------|
 * | 0x00      | git_id         | Git short commit id                 |
 * | 0x04      | clk_freq       | System clock base frequency         |
 * | 0x08      | pll0           | PLL0 controller                     |
 * | 0x0c      | pll1           | PLL1 controller                     |
 * | 0x10      | pll2           | PLL2 controller                     |
 * | 0x14      | resv5          | Reserved                            |
 * | 0x18      | pll_lock       | PLL lock tester                     |
 * | 0x1c      | rom_error      | AXI ROM detector                    |
 * | 0x20      | clk_sel0       | Clock select controller0            |
 * | 0x24      | clk_sel1       | Clock select controller1            |
 * | 0x28      | clk_en_cent    | Central clock enable                |
 * | 0x2c      | clk_en_peri    | Peripheral clock enable             |
 * | 0x30      | soft_reset     | Soft reset ctrl                     |
 * | 0x34      | peri_reset     | Peripheral reset controller         |
 * | 0x38      | clk_th0        | Clock threshold controller 0        |
 * | 0x3c      | clk_th1        | Clock threshold controller 1        |
 * | 0x40      | clk_th2        | Clock threshold controller 2        |
 * | 0x44      | clk_th3        | Clock threshold controller 3        |
 * | 0x48      | clk_th4        | Clock threshold controller 4        |
 * | 0x4c      | clk_th5        | Clock threshold controller 5        |
 * | 0x50      | clk_th6        | Clock threshold controller 6        |
 * | 0x54      | misc           | Miscellaneous controller            |
 * | 0x58      | peri           | Peripheral controller               |
 * | 0x5c      | spi_sleep      | SPI sleep controller                |
 * | 0x60      | reset_status   | Reset source status                 |
 * | 0x64      | dma_sel0       | DMA handshake selector              |
 * | 0x68      | dma_sel1       | DMA handshake selector              |
 * | 0x6c      | power_sel      | IO Power Mode Select controller     |
 * | 0x70      | resv28         | Reserved                            |
 * | 0x74      | resv29         | Reserved                            |
 * | 0x78      | resv30         | Reserved                            |
 * | 0x7c      | resv31         | Reserved                            |
 *
*********************************************************************************************************/

typedef enum sysctl_pll_e
{
    SYSCTL_PLL0,
    SYSCTL_PLL1,
    SYSCTL_PLL2,
    SYSCTL_PLL_MAX
} sysctl_pll_t;

typedef enum sysctl_clock_source_e
{
    SYSCTL_SOURCE_IN0,
    SYSCTL_SOURCE_PLL0,
    SYSCTL_SOURCE_PLL1,
    SYSCTL_SOURCE_PLL2,
    SYSCTL_SOURCE_ACLK,
    SYSCTL_SOURCE_MAX
} sysctl_clock_source_t;

typedef enum sysctl_dma_channel_e
{
    SYSCTL_DMA_CHANNEL_0,
    SYSCTL_DMA_CHANNEL_1,
    SYSCTL_DMA_CHANNEL_2,
    SYSCTL_DMA_CHANNEL_3,
    SYSCTL_DMA_CHANNEL_4,
    SYSCTL_DMA_CHANNEL_5,
    SYSCTL_DMA_CHANNEL_MAX
} sysctl_dma_channel_t;

typedef enum sysctl_dma_select_e
{
    SYSCTL_DMA_SELECT_SSI0_RX_REQ,
    SYSCTL_DMA_SELECT_SSI0_TX_REQ,
    SYSCTL_DMA_SELECT_SSI1_RX_REQ,
    SYSCTL_DMA_SELECT_SSI1_TX_REQ,
    SYSCTL_DMA_SELECT_SSI2_RX_REQ,
    SYSCTL_DMA_SELECT_SSI2_TX_REQ,
    SYSCTL_DMA_SELECT_SSI3_RX_REQ,
    SYSCTL_DMA_SELECT_SSI3_TX_REQ,
    SYSCTL_DMA_SELECT_I2C0_RX_REQ,
    SYSCTL_DMA_SELECT_I2C0_TX_REQ,
    SYSCTL_DMA_SELECT_I2C1_RX_REQ,
    SYSCTL_DMA_SELECT_I2C1_TX_REQ,
    SYSCTL_DMA_SELECT_I2C2_RX_REQ,
    SYSCTL_DMA_SELECT_I2C2_TX_REQ,
    SYSCTL_DMA_SELECT_UART1_RX_REQ,
    SYSCTL_DMA_SELECT_UART1_TX_REQ,
    SYSCTL_DMA_SELECT_UART2_RX_REQ,
    SYSCTL_DMA_SELECT_UART2_TX_REQ,
    SYSCTL_DMA_SELECT_UART3_RX_REQ,
    SYSCTL_DMA_SELECT_UART3_TX_REQ,
    SYSCTL_DMA_SELECT_AES_REQ,
    SYSCTL_DMA_SELECT_SHA_RX_REQ,
    SYSCTL_DMA_SELECT_AI_RX_REQ,
    SYSCTL_DMA_SELECT_FFT_RX_REQ,
    SYSCTL_DMA_SELECT_FFT_TX_REQ,
    SYSCTL_DMA_SELECT_I2S0_TX_REQ,
    SYSCTL_DMA_SELECT_I2S0_RX_REQ,
    SYSCTL_DMA_SELECT_I2S1_TX_REQ,
    SYSCTL_DMA_SELECT_I2S1_RX_REQ,
    SYSCTL_DMA_SELECT_I2S2_TX_REQ,
    SYSCTL_DMA_SELECT_I2S2_RX_REQ,
    SYSCTL_DMA_SELECT_I2S0_BF_DIR_REQ,
    SYSCTL_DMA_SELECT_I2S0_BF_VOICE_REQ,
    SYSCTL_DMA_SELECT_MAX
} sysctl_dma_select_t;

/**
 * @brief      System controller clock id
 */
typedef enum sysctl_clock_e
{
    SYSCTL_CLOCK_PLL0,
    SYSCTL_CLOCK_PLL1,
    SYSCTL_CLOCK_PLL2,
    SYSCTL_CLOCK_CPU,
    SYSCTL_CLOCK_SRAM0,
    SYSCTL_CLOCK_SRAM1,
    SYSCTL_CLOCK_APB0,
    SYSCTL_CLOCK_APB1,
    SYSCTL_CLOCK_APB2,
    SYSCTL_CLOCK_ROM,
    SYSCTL_CLOCK_DMA,
    SYSCTL_CLOCK_AI,
    SYSCTL_CLOCK_DVP,
    SYSCTL_CLOCK_FFT,
    SYSCTL_CLOCK_GPIO,
    SYSCTL_CLOCK_SPI0,
    SYSCTL_CLOCK_SPI1,
    SYSCTL_CLOCK_SPI2,
    SYSCTL_CLOCK_SPI3,
    SYSCTL_CLOCK_I2S0,
    SYSCTL_CLOCK_I2S1,
    SYSCTL_CLOCK_I2S2,
    SYSCTL_CLOCK_I2C0,
    SYSCTL_CLOCK_I2C1,
    SYSCTL_CLOCK_I2C2,
    SYSCTL_CLOCK_UART1,
    SYSCTL_CLOCK_UART2,
    SYSCTL_CLOCK_UART3,
    SYSCTL_CLOCK_AES,
    SYSCTL_CLOCK_FPIOA,
    SYSCTL_CLOCK_TIMER0,
    SYSCTL_CLOCK_TIMER1,
    SYSCTL_CLOCK_TIMER2,
    SYSCTL_CLOCK_WDT0,
    SYSCTL_CLOCK_WDT1,
    SYSCTL_CLOCK_SHA,
    SYSCTL_CLOCK_OTP,
    SYSCTL_CLOCK_RTC,
    SYSCTL_CLOCK_ACLK = 40,
    SYSCTL_CLOCK_HCLK,
    SYSCTL_CLOCK_IN0,
    SYSCTL_CLOCK_MAX
} sysctl_clock_t;

/**
 * @brief      System controller clock select id
 */
typedef enum sysctl_clock_select_e
{
    SYSCTL_CLOCK_SELECT_PLL0_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL1_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL3_BYPASS,
    SYSCTL_CLOCK_SELECT_PLL2,
    SYSCTL_CLOCK_SELECT_ACLK,
    SYSCTL_CLOCK_SELECT_SPI3,
    SYSCTL_CLOCK_SELECT_TIMER0,
    SYSCTL_CLOCK_SELECT_TIMER1,
    SYSCTL_CLOCK_SELECT_TIMER2,
    SYSCTL_CLOCK_SELECT_SPI3_SAMPLE,
    SYSCTL_CLOCK_SELECT_MAX = 11
} sysctl_clock_select_t;

/**
 * @brief      System controller clock threshold id
 */
typedef enum sysctl_threshold_e
{
    SYSCTL_THRESHOLD_ACLK,
    SYSCTL_THRESHOLD_APB0,
    SYSCTL_THRESHOLD_APB1,
    SYSCTL_THRESHOLD_APB2,
    SYSCTL_THRESHOLD_SRAM0,
    SYSCTL_THRESHOLD_SRAM1,
    SYSCTL_THRESHOLD_AI,
    SYSCTL_THRESHOLD_DVP,
    SYSCTL_THRESHOLD_ROM,
    SYSCTL_THRESHOLD_SPI0,
    SYSCTL_THRESHOLD_SPI1,
    SYSCTL_THRESHOLD_SPI2,
    SYSCTL_THRESHOLD_SPI3,
    SYSCTL_THRESHOLD_TIMER0,
    SYSCTL_THRESHOLD_TIMER1,
    SYSCTL_THRESHOLD_TIMER2,
    SYSCTL_THRESHOLD_I2S0,
    SYSCTL_THRESHOLD_I2S1,
    SYSCTL_THRESHOLD_I2S2,
    SYSCTL_THRESHOLD_I2S0_M,
    SYSCTL_THRESHOLD_I2S1_M,
    SYSCTL_THRESHOLD_I2S2_M,
    SYSCTL_THRESHOLD_I2C0,
    SYSCTL_THRESHOLD_I2C1,
    SYSCTL_THRESHOLD_I2C2,
    SYSCTL_THRESHOLD_WDT0,
    SYSCTL_THRESHOLD_WDT1,
    SYSCTL_THRESHOLD_MAX = 28
} sysctl_threshold_t;

/**
 * @brief      System controller reset control id
 */
typedef enum sysctl_reset_e
{
    SYSCTL_RESET_SOC,
    SYSCTL_RESET_ROM,
    SYSCTL_RESET_DMA,
    SYSCTL_RESET_AI,
    SYSCTL_RESET_DVP,
    SYSCTL_RESET_FFT,
    SYSCTL_RESET_GPIO,
    SYSCTL_RESET_SPI0,
    SYSCTL_RESET_SPI1,
    SYSCTL_RESET_SPI2,
    SYSCTL_RESET_SPI3,
    SYSCTL_RESET_I2S0,
    SYSCTL_RESET_I2S1,
    SYSCTL_RESET_I2S2,
    SYSCTL_RESET_I2C0,
    SYSCTL_RESET_I2C1,
    SYSCTL_RESET_I2C2,
    SYSCTL_RESET_UART1,
    SYSCTL_RESET_UART2,
    SYSCTL_RESET_UART3,
    SYSCTL_RESET_AES,
    SYSCTL_RESET_FPIOA,
    SYSCTL_RESET_TIMER0,
    SYSCTL_RESET_TIMER1,
    SYSCTL_RESET_TIMER2,
    SYSCTL_RESET_WDT0,
    SYSCTL_RESET_WDT1,
    SYSCTL_RESET_SHA,
    SYSCTL_RESET_RTC,
    SYSCTL_RESET_MAX = 31
} sysctl_reset_t;

typedef enum _sysctl_power_bank
{
    SYSCTL_POWER_BANK0,
    SYSCTL_POWER_BANK1,
    SYSCTL_POWER_BANK2,
    SYSCTL_POWER_BANK3,
    SYSCTL_POWER_BANK4,
    SYSCTL_POWER_BANK5,
    SYSCTL_POWER_BANK6,
    SYSCTL_POWER_BANK7,
    SYSCTL_POWER_BANK_MAX,
} sysctl_power_bank_t;

typedef enum _io_power_mode
{
    POWER_V33,
    POWER_V18
} io_power_mode_t;
/*********************************************************************************************************
  SYSCTL 控制器API (参考自 kendryte-standalone-sdk , 保留原本结构体命名风格)
*********************************************************************************************************/

/**
 * @brief       Enable clock for peripheral
 *
 * @param[in]   clock       The clock to be enable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_enable(enum sysctl_clock_e clock);

/**
 * @brief       Enable clock for peripheral
 *
 * @param[in]   clock       The clock to be disable
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_disable(enum sysctl_clock_e clock);

/**
 * @brief       Sysctl clock set threshold
 *
 * @param[in]   which           Which threshold to set
 * @param[in]   threshold       The threshold value
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_set_threshold(enum sysctl_threshold_e which, int threshold);

/**
 * @brief       Sysctl clock get threshold
 *
 * @param[in]   which       Which threshold to get
 *
 * @return      The threshold value
 *     - Other  Value of threshold
 *     - -1     Fail
 */
int sysctl_clock_get_threshold(enum sysctl_threshold_e which);

/**
 * @brief       Sysctl clock set clock select
 *
 * @param[in]   which       Which clock select to set
 * @param[in]   select      The clock select value
 *
 * @return      result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_clock_set_clock_select(enum sysctl_clock_select_e which, int select);

/**
 * @brief       Sysctl clock get clock select
 *
 * @param[in]   which  Which clock select to get
 *
 * @return      The clock select value
 *     - Other  Value of clock select
 *     - -1     Fail
 */
int sysctl_clock_get_clock_select(enum sysctl_clock_select_e which);

/**
 * @brief       Get clock source frequency
 *
 * @param[in]   input       The input clock source
 *
 * @return      The frequency of clock source
 */
uint32_t sysctl_clock_source_get_freq(enum sysctl_clock_source_e input);

/**
 * @brief       Get PLL frequency
 *
 * @param[in]   pll     The PLL id
 *
 * @return      The frequency of PLL
 */
uint32_t sysctl_pll_get_freq(enum sysctl_pll_e pll);

/**
 * @brief       Set PLL frequency and input clock
 *
 * @param[in]   pll     The PLL id
 * @param[in]   sel     The selected PLL input clock
 * @param[in]   freq    The frequency
 *
 * @return      The frequency of PLL
 */
uint32_t sysctl_pll_set_freq(enum sysctl_pll_e pll, enum sysctl_clock_source_e source, uint32_t freq);

/**
 * @brief       Get base clock frequency by clock id
 *
 * @param[in]   clock       The clock id
 *
 * @return      The clock frequency
 */
uint32_t sysctl_clock_get_freq(enum sysctl_clock_e clock);

/**
 * @brief       Reset device by reset controller
 *
 * @param[in]   reset       The reset signal
 */
void sysctl_reset(enum sysctl_reset_e reset);

/**
 * @brief       Get git commit id
 *
 * @return      The 4 bytes git commit id
 */
uint32_t sysctl_get_git_id(void);

/**
 * @brief       Get base clock frequency, default is 26MHz
 *
 * @return      The base clock frequency
 */
uint32_t sysctl_get_freq(void);

/**
 * @brief       Get pll lock status
 *
 * @param[in]   pll     The pll id
 *
 * @return      The lock status
 *     - 1      Pll is lock
 *     - 0      Pll have lost lock
 */
int sysctl_pll_is_lock(enum sysctl_pll_e pll);

/**
 * @brief       Clear pll lock status
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_clear_slip(enum sysctl_pll_e pll);

/**
 * @brief       Enable the PLL and power on with reset
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_enable(enum sysctl_pll_e pll);

/**
 * @brief       Disable the PLL and power off
 *
 * @param[in]   pll     The pll id
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_pll_disable(enum sysctl_pll_e pll);

/**
 * @brief       Select DMA channel handshake peripheral signal
 *
 * @param[in]   channel     The DMA channel
 * @param[in]   select      The peripheral select
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
int sysctl_dma_select(enum sysctl_dma_channel_e channel, enum sysctl_dma_select_e select);

/**
 * @brief       Fast set all PLL and CPU clock
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
uint32_t sysctl_pll_fast_enable_pll(void);

/**
 * @brief       Set SPI0_D0-D7 DVP_D0-D7 as spi and dvp data pin
 *
 * @param[in]   en     Enable or not
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
uint32_t sysctl_spi0_dvp_data_set(uint8_t en);

/**
 * @brief       Set io power mode
 *
 * @param[in]   power_bank          IO power bank
 * @param[in]   io_power_mode       Set power mode 3.3v or 1.8v
 *
 * @return      Result
 *     - 0      Success
 *     - Other  Fail
 */
uint32_t sysctl_power_mode_sel(uint8_t power_bank, io_power_mode_t io_power_mode);

/**
 * @brief       Set cpu clock frequency
 *
 * @param[in]   freq          cpu clock frequency
 *
 * @return      Result
 *     - 0      Fail
 */
uint32_t sysctl_cpu_set_freq(uint32_t freq);

#endif /* __K210_CLOCK_H */
/*********************************************************************************************************
  END
*********************************************************************************************************/
