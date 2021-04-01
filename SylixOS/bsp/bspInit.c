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
** 文   件   名: bspInit.c
**
** 创   建   人: Jiao.JinXing (焦进星)
**
** 文件创建日期: 2018 年 03 月 20 日
**
** 描        述: BSP 用户 C 程序入口
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "config.h"                                                     /*  工程配置 & 处理器相关       */
/*********************************************************************************************************
  操作系统相关
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  操作系统                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbmodule.h"                                                  /*  GDB 调试器                  */
#include "gdbserver.h"                                                  /*  GDB 调试器                  */
#include "sys/compiler.h"                                               /*  编译器相关                  */
/*********************************************************************************************************
  BSP 及 驱动程序 (包含你的 BSP 及 驱动程序的头文件)
*********************************************************************************************************/
#include "driver/clock/k210_clock.h"
#include "driver/sio/sio.h"
#include "driver/uart/k210_uart.h"
#include "driver/dmac/k210_dma.h"
#include "driver/gpio/k210_gpio.h"
#include "driver/gpiohs/k210_gpiohs.h"

#include "driver/aes/k210_aes.h"
#include "driver/fft/k210_fft.h"
#include "driver/i2c/k210_i2c.h"
#include "driver/i2s/k210_i2s.h"
#include "driver/rtc/k210_rtc.h"
#include "driver/watchdog/k210_watchdog.h"
#include "driver/sha256/k210_sha256.h"
#include "driver/pwm/k210_pwm.h"
#include "driver/lcd/k210_lcd.h"

#include "driver/spi/k210_spi.h"
#include "driver/spi_sdi/spi_sdi.h"
#include "driver/spi_sdi/sdcard.h"
#include "driver/spiflash/k210_spiflash.h"

#include "driver/video/video_driver/k210_dvp.h"
#include "driver/video/video_dummy/dummySubDev.h"
#include "driver/video/video_dummy/dummyVideoDev.h"
#include "driver/video/video-core/videoAsync.h"

#include "driver/kpu/k210_kpu.h"

#include "driver/netif/netif.h"

#include "KendryteWare/include/fpioa.h"
#include "KendryteWare/include/uarths.h"
#include "KendryteWare/include/gpio_common.h"
#include "KendryteWare/include/gpiohs.h"
/*********************************************************************************************************
  内存初始化映射表
*********************************************************************************************************/
/*********************************************************************************************************
  主线程与启动线程堆栈 (t_boot 可以大一点, startup.sh 中可能有很多消耗堆栈的操作)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  主线程声明
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** 函数名称: bspSetupCpuFreq
** 功能描述: 初始化 CPU 时钟
** 输　入  : frequency     CPU工作频率
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static UINT32  bspSetupCpuFreq (uint32_t  frequency)
{
    UINT32  uiResult = 0;

    sysctl_clock_set_clock_select(SYSCTL_CLOCK_SELECT_ACLK, SYSCTL_SOURCE_IN0);

    sysctl_pll_disable(SYSCTL_PLL0);
    sysctl_pll_enable(SYSCTL_PLL0);

    uiResult = sysctl_pll_set_freq(SYSCTL_PLL0, SYSCTL_SOURCE_IN0, frequency * 2);

    while (sysctl_pll_is_lock(SYSCTL_PLL0) == 0) {
        sysctl_pll_clear_slip(SYSCTL_PLL0);
    }

    sysctl_clock_enable(SYSCTL_PLL0);
    sysctl_clock_set_clock_select(SYSCTL_CLOCK_SELECT_ACLK, SYSCTL_SOURCE_PLL0);

    return  (uiResult);
}
/*********************************************************************************************************
** 函数名称: bspSetupClocks
** 功能描述: 初始化时钟系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  bspSetupClocks (VOID)
{
    bspSetupCpuFreq(configCPU_CLOCK_HZ);

    sysctl_pll_enable(SYSCTL_PLL1);
    sysctl_pll_set_freq(SYSCTL_PLL1, SYSCTL_SOURCE_IN0, PLL1_OUTPUT_FREQ);

    while (sysctl_pll_is_lock(SYSCTL_PLL1) == 0) {
        sysctl_pll_clear_slip(SYSCTL_PLL1);
    }
    sysctl_clock_enable(SYSCTL_CLOCK_PLL1);

    sysctl_pll_enable(SYSCTL_PLL2);
    sysctl_pll_set_freq(SYSCTL_PLL2, SYSCTL_SOURCE_IN0, PLL2_OUTPUT_FREQ);

    while (sysctl_pll_is_lock(SYSCTL_PLL2) == 0) {
        sysctl_pll_clear_slip(SYSCTL_PLL2);
    }
    sysctl_clock_enable(SYSCTL_CLOCK_PLL2);
}
/*********************************************************************************************************
** 函数名称: targetInit
** 功能描述: 初始化目标电路板系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  targetInit (VOID)
{
    /*
     * Any IO of FPIOA have 256 functions, it is a IO-function matrix. All IO have
     * default reset function, after reset, re-configure IO function is required.
     */
    fpioa_init();                                                           /*  Init FPIOA              */

    bspSetupClocks();                                                       /*  Setup clocks            */
}
/*********************************************************************************************************
** 函数名称: halModeInit
** 功能描述: 初始化目标系统运行的模式
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 但建议不作处理
     */
}
/*********************************************************************************************************
** 函数名称: halTimeInit
** 功能描述: 初始化目标系统时间系统 (系统默认时区为: 东8区)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    PLW_RTC_FUNCS   pRtcFuncs = k210RtcGetFuncs();

    rtcDrv();
    rtcDevCreate(pRtcFuncs);                                            /*  创建硬件 RTC 设备           */
    rtcToSys();                                                         /*  将 RTC 时间同步到系统时间   */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** 函数名称: halIdleInit
** 功能描述: 初始化目标系统空闲时间作业
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{
    API_SystemHookAdd(archWaitForInterrupt,
                      LW_OPTION_THREAD_IDLE_HOOK);                      /*  空闲时暂停 CPU              */
}
/*********************************************************************************************************
** 函数名称: halCacheInit
** 功能描述: 目标系统 CPU 高速缓冲初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, RISCV_MACHINE_GENERAL);
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  使能 CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halFpuInit
** 功能描述: 目标系统 FPU 浮点运算单元初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(RISCV_MACHINE_GENERAL, RISCV_FPU_DP);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halPmInit
** 功能描述: 初始化目标系统电源管理系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
    /*
     * TODO: 加入你的处理代码, 参考代码如下:
     */
#if 0                                                                   /*  参考代码开始                */
    PLW_PMA_FUNCS  pmafuncs = pmGetFuncs();

    pmAdapterCreate("inner_pm", 21, pmafuncs);
#endif                                                                  /*  参考代码结束                */
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** 函数名称: halBusInit
** 功能描述: 初始化目标系统总线系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
    INT              i;
    ULONG            ulMaxBytes;
    PLW_DMA_FUNCS    pDmaFuncs;
    PLW_I2C_FUNCS    pi2cfuncHandle;
    PLW_SPI_FUNCS    pspifuncHandle;

    for (i = 0; i < DMA_CHANNEL_NR; i++) {                              /*  安装 64 个通用 DMA 通道     */
        pDmaFuncs = dmaGetFuncs(LW_DMA_CHANNEL0 + i, &ulMaxBytes);
        if (pDmaFuncs) {
            API_DmaDrvInstall((UINT)i, pDmaFuncs, (size_t)ulMaxBytes);  /*  安装 DMA 控制器驱动         */
        }
    }

    API_I2cLibInit();                                                   /*  初始化 i2c 子系统           */
    pi2cfuncHandle = i2cBusFuncs(0);
    if (pi2cfuncHandle) {
        API_I2cAdapterCreate("/bus/i2c/0", pi2cfuncHandle, 10, 1);      /*  创建 i2c0 总线适配器        */
    }
    pi2cfuncHandle = i2cBusFuncs(1);
    if (pi2cfuncHandle) {
        API_I2cAdapterCreate("/bus/i2c/1", pi2cfuncHandle, 10, 1);      /*  创建 i2c1 总线适配器        */
    }

    API_SpiLibInit();                                                   /*  初始化 spi 组件库           */
    pspifuncHandle = spiBusDrv(0);                                      /*  用于驱动 TF 卡              */
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/0", pspifuncHandle);             /*  创建 spi0 总线适配器        */
    }
    pspifuncHandle = spiBusDrv(1);
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/1", pspifuncHandle);             /*  创建 spi1 总线适配器        */
    }
    pspifuncHandle = spiBusDrv(3);
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/3", pspifuncHandle);             /*  创建 spi3 总线适配器        */
    }

}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDrvInit
** 功能描述: 初始化目标系统静态驱动程序
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDrvInit (VOID)
{
    /*
     *  standard device driver (rootfs and procfs need install first.)
     */
    rootFsDrv();                                                        /*  ROOT   device driver        */
    procFsDrv();                                                        /*  proc   device driver        */
#if LW_CFG_SHM_DEVICE_EN > 0
    shmDrv();                                                           /*  shm    device driver        */
#endif                                                                  /*  LW_CFG_SHM_DEVICE_EN        */
    randDrv();                                                          /*  random device driver        */
    ptyDrv();                                                           /*  pty    device driver        */
    ttyDrv();                                                           /*  tty    device driver        */
#if LW_CFG_MEMDEV_EN > 0
    memDrv();                                                           /*  mem    device driver        */
#endif                                                                  /*  (LW_CFG_MEMDEV_EN > 0)      */
    pipeDrv();                                                          /*  pipe   device driver        */
    spipeDrv();                                                         /*  spipe  device driver        */
#if LW_CFG_TPSFS_EN > 0
    tpsFsDrv();                                                         /*  TPS FS device driver        */
#endif                                                                  /*  LW_CFG_TPSFS_EN > 0         */
#if LW_CFG_FATFS_EN > 0
    fatFsDrv();                                                         /*  FAT FS device driver        */
#endif                                                                  /*  (LW_CFG_FATFS_EN > 0)       */
    ramFsDrv();                                                         /*  RAM FS device driver        */
#if LW_CFG_ROMFS_EN > 0
    romFsDrv();                                                         /*  ROM FS device driver        */
#endif                                                                  /*  LW_CFG_ROMFS_EN > 0         */
#if LW_CFG_NFS_EN > 0
    nfsDrv();                                                           /*  nfs    device driver        */
#endif                                                                  /*  (LW_CFG_NFS_EN > 0)         */
#if LW_CFG_YAFFS_EN > 0
    yaffsDrv();                                                         /*  yaffs  device driver        */
#endif                                                                  /*  (LW_CFG_YAFFS_EN > 0)       */
#if LW_CFG_CAN_EN > 0
    canDrv();                                                           /*  CAN    device driver        */
#endif                                                                  /*  (LW_CFG_CAN_EN > 0)         */

    k210AesDrv();

    k210GpioDrv();

#if KENDRYTE_VIDEO_ENABLE
    k210DvpDrv();
#endif

    k210FFTDrv();

    k210GpiohsDrv();

    k210I2sDrv();

    k210WatchDogDrv();

    k210Sha256Drv();

    k210PwmDrv();
	
#if KENDRYTE_INSTALL_KPU
    k210KpuDrv();
#endif
	
#if KENDRYTE_SDCARD_ENABLE

    /*
     * Initialize IO Mapping
     */
    fpioa_set_function(29, FUNC_SPI1_SCLK);
    fpioa_set_function(30, FUNC_SPI1_D0);
    fpioa_set_function(31, FUNC_SPI1_D1);
    fpioa_set_function(32, FUNC_GPIOHS7);                               /*  注意: 此处必须为S7对应裸机  */
    fpioa_set_function(24, FUNC_SPI1_SS3);

    /*
     * Initialize SD_SPI
     */
    gpiohs_set_drive_mode(7, GPIO_DM_OUTPUT);
    spi_set_clk_rate(SPI_DEVICE_1, 200000);                             /*  set clk rate                */

#if (LW_CFG_SDCARD_EN > 0)
    /*
     * SD chip select high
     */
    SD_CS_HIGH();

    SPI_PLATFORM_OPS  spiplatops;
    spiplatops.SPIPO_pfuncChipSelect = (PVOID)k210SdiChipSelect;
    spisdiLibInit(&spiplatops);
    spisdiDrvInstall(1, 3, SPI_SDI_PIN_NONE, SPI_SDI_PIN_NONE);         /*  注意: 此处 sdcard 使用 spi1 */
#endif

#endif

}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halDevInit
** 功能描述: 初始化目标系统静态设备组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    SIO_CHAN    *psio;
    SIO_CHAN    *psio0;

    /*
     *  创建根文件系统时, 将自动创建 dev, mnt, var 目录.
     */
    rootFsDevCreate();                                                  /*  创建根文件系统              */
    procFsDevCreate();                                                  /*  创建 proc 文件系统          */
#if LW_CFG_SHM_DEVICE_EN > 0
    shmDevCreate();                                                     /*  创建共享内存设备            */
#endif
    randDevCreate();                                                    /*  创建随机数文件              */

    psio0 = sioChanCreate(0);                                           /*  创建串口 0 通道             */
    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */

    psio  = uartChanCreate(0);                                          /*  创建串口 0 通道             */
    ttyDevCreate("/dev/ttyS1", psio, 30, 50);                           /*  add    tty   device         */

    k210AesDevAdd();

#if KENDRYTE_VIDEO_ENABLE
    k210DvpDevAdd();
#endif

    k210FFTDevAdd();

    k210FbDevCreate("/dev/fb0");                                        /*  create lcd device           */

    k210I2sDevAdd(0);
    k210I2sDevAdd(1);
    k210I2sDevAdd(2);

    k210WatchDogDevAdd(0);
    k210WatchDogDevAdd(1);

    k210Sha256DevAdd();

    k210PwmDevAdd(0);
    k210PwmDevAdd(1);
    k210PwmDevAdd(2);
	
#if KENDRYTE_INSTALL_KPU
    k210KpuDevAdd();
#endif

#if LW_CFG_YAFFS_EN > 0
    yaffsDevCreate("/yaffs2");                                          /* create yaffs device(only fs) */
#endif
}

#endif                                                                  /* LW_CFG_DEVICE_EN > 0         */
/*********************************************************************************************************
** 函数名称: halLogInit
** 功能描述: 初始化目标系统日志系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  初始化日志                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** 函数名称: halStdFileInit
** 功能描述: 初始化目标系统标准文件系统
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halStdFileInit (VOID)
{
    INT     iFd = open("/dev/ttyS0", O_RDWR, 0);

    if (iFd >= 0) {
        ioctl(iFd, FIOBAUDRATE,   SIO_BAUD_115200);
        ioctl(iFd, FIOSETOPTIONS, (OPT_TERMINAL & (~OPT_7_BIT)));       /*  system terminal 8 bit mode  */

        ioGlobalStdSet(STD_IN,  iFd);
        ioGlobalStdSet(STD_OUT, iFd);
        ioGlobalStdSet(STD_ERR, iFd);
    }
}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** 函数名称: halShellInit
** 功能描述: 初始化目标系统 shell 环境, (getopt 使用前一定要初始化 shell 环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  初始化 appl 中间件 shell 接口
     */
#ifndef __SYLIXOS_LITE
    zlibShellInit();
#endif
    viShellInit();

    /*
     *  初始化 GDB 调试器
     */
#if LW_CFG_GDB_EN > 0
    gdbInit();
    gdbModuleInit();
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** 函数名称: halVmmInit
** 功能描述: 初始化目标系统虚拟内存管理组件
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    /*
     * TODO: 加入你的代码
     */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** 函数名称: halNetInit
** 功能描述: 网络组件初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  初始化网络系统              */
    API_NetSnmpInit();

    /*
     *  初始化网络附加工具
     */
#if LW_CFG_NET_PING_EN > 0
    API_INetPingInit();
    API_INetPing6Init();
#endif                                                                  /*  LW_CFG_NET_PING_EN > 0      */

#if LW_CFG_NET_NETBIOS_EN > 0
    API_INetNetBiosInit();
    API_INetNetBiosNameSet("sylixos");
#endif                                                                  /*  LW_CFG_NET_NETBIOS_EN > 0   */

#if LW_CFG_NET_TFTP_EN > 0
    API_INetTftpServerInit("/tmp");
#endif                                                                  /*  LW_CFG_NET_TFTP_EN > 0      */

#if LW_CFG_NET_FTPD_EN > 0
    API_INetFtpServerInit("/");
#endif                                                                  /*  LW_CFG_NET_FTP_EN > 0       */

#if LW_CFG_NET_TELNET_EN > 0
    API_INetTelnetInit(LW_NULL);
#endif                                                                  /*  LW_CFG_NET_TELNET_EN > 0    */

#if LW_CFG_NET_NAT_EN > 0
    API_INetNatInit();
#endif                                                                  /*  LW_CFG_NET_NAT_EN > 0       */

#if LW_CFG_NET_NPF_EN > 0
    API_INetNpfInit();
#endif                                                                  /*  LW_CFG_NET_NPF_EN > 0       */

#if LW_CFG_NET_QOS_EN > 0
    API_INetQosInit();
#endif                                                                  /*  LW_CFG_NET_QOS_EN > 0       */
}
/*********************************************************************************************************
** 函数名称: halNetifAttch
** 功能描述: 网络接口连接
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    /*
     * DM9051 网卡驱动注册, 这里采用默认配置（具体请看netif.c中的配置参数宏）
     */
    if (enetInit(DM9051_AUTO, LW_NULL, 0) != ERROR_NONE) {
        printk("[halNetifAttch]: network interface init FILAED!\n");
    }
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** 函数名称: halMonitorInit
** 功能描述: 内核监控器上传初始化
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  可以再这里创建内核监控器上传通道, 也可以使用 shell 命令操作.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** 函数名称: halPosixInit
** 功能描述: 初始化 posix 子系统 (如果系统支持 proc 文件系统, 则必须放在 proc 文件系统安装之后!)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** 函数名称: halSymbolInit
** 功能描述: 初始化目标系统符号表环境, (为 module loader 提供环境)
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_SYMBOL_EN > 0

static VOID  halSymbolInit (VOID)
{
#ifdef __GNUC__
    void *__aeabi_read_tp();
#endif                                                                  /*  __GNUC__                    */

    API_SymbolInit();

#ifdef __GNUC__
    symbolAddAll();

    /*
     *  GCC will emit calls to this routine under -mtp=soft.
     */
    API_SymbolAdd("__aeabi_read_tp", (caddr_t)__aeabi_read_tp, LW_SYMBOL_FLAG_XEN);
#endif                                                                  /*  __GNUC__                    */
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halLoaderInit
** 功能描述: 初始化目标系统程序或模块装载器
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** 函数名称: halBootThread
** 功能描述: 多任务状态下的初始化启动任务
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threadattr = API_ThreadAttrGetDefault();    /*  使用默认属性                */

    (VOID)pvBootArg;

#if LW_CFG_SHELL_EN > 0
    halShellInit();
#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */

#if LW_CFG_POWERM_EN > 0
    halPmInit();
#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */

#if LW_CFG_DEVICE_EN > 0
    halBusInit();
    halDrvInit();
    halDevInit();
    halStdFileInit();
#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */

#if LW_CFG_LOG_LIB_EN > 0
    halLogInit();
    console_loglevel = default_message_loglevel;                        /*  设置 printk 打印信息等级    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    /*
     *  因为 yaffs 挂载物理卷时, 需要 stdout 打印信息, 如果在 halDevInit() 中被调用, 由于没有创建
     *  标准文件, 所以会打印警告错误信息, 所以将此函数放在这里!
     *  如果未初始化标准文件会提示错误信息
     */

#if KENDRYTE_FLASH_ENABLE
    /*
     * SPI-norFlash Driver Register
     */
    spiFlashDevCreate("/bus/spi/3", 0);                                 /* 创建 norflash 设备           */

    /*
     * Skip front 4MB memory just like kendryte sdk did.
     */
    spiFlashDrvInstall(LW_NULL, 0x400000);                              /* 注册 SPI-norflash 驱动       */
#endif


#if LW_CFG_DEVICE_EN > 0                                                /*  map rootfs                  */
    rootFsMap(LW_ROOTFS_MAP_LOAD_VAR | LW_ROOTFS_MAP_SYNC_TZ | LW_ROOTFS_MAP_SET_TIME);
#endif

    /*
     *  网络初始化一般放在 shell 初始化之后, 因为初始化网络组件时, 会自动注册 shell 命令.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan 网卡需要下载固件       */
#endif                                                                  /*  LW_CFG_NET_EN > 0           */

#if LW_CFG_POSIX_EN > 0
    halPosixInit();
#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */

#if LW_CFG_SYMBOL_EN > 0
    halSymbolInit();
#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */

#if LW_CFG_MODULELOADER_EN > 0
    halLoaderInit();
#endif                                                                  /*  LW_CFG_MODULELOADER_EN > 0  */

#if LW_CFG_MONITOR_EN > 0
    halMonitorInit();
#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */

#if LW_CFG_SHELL_EN > 0
    tshellStartup();                                                    /*  执行启动脚本                */
#endif

    API_ThreadAttrSetStackSize(&threadattr, __LW_THREAD_MAIN_STK_SIZE); /*  设置 main 线程的堆栈大小    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threadattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: usrStartup
** 功能描述: 初始化应用相关组件, 创建操作系统的第一个任务.
** 输　入  : NONE
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threadattr;

    /*
     *  注意, 不要修改该初始化顺序 (必须先初始化 vmm 才能正确的初始化 cache,
     *                              网络需要其他资源必须最后初始化)
     */
    halIdleInit();
#if LW_CFG_CPU_FPU_EN > 0
    halFpuInit();
#endif                                                                  /*  LW_CFG_CPU_FPU_EN > 0       */

#if LW_CFG_RTC_EN > 0
    halTimeInit();
#endif                                                                  /*  LW_CFG_RTC_EN > 0           */

#if LW_CFG_VMM_EN > 0
    halVmmInit();
#endif                                                                  /*  LW_CFG_VMM_EN > 0           */

#if LW_CFG_CACHE_EN > 0
    halCacheInit();
#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */

    API_ThreadAttrBuild(&threadattr,
                        __LW_THREAD_BOOT_STK_SIZE,
                        LW_PRIO_CRITICAL,
                        LW_OPTION_THREAD_STK_CHK,
                        LW_NULL);
    API_ThreadCreate("t_boot",
                     (PTHREAD_START_ROUTINE)halBootThread,
                     &threadattr,
                     LW_NULL);                                          /*  Create boot thread          */
}
/*********************************************************************************************************
** 函数名称: bspInit
** 功能描述: C 入口
** 输　入  : NONE
** 输　出  : 0
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT bspInit (ULONG  ulMHartId, addr_t  ulDtb)
{
    /*
     *  系统内核堆与系统堆
     */
    extern UCHAR  __heap_start, __heap_end;

    targetInit();                                                       /*  初始化系统时钟及引脚配置    */

    halModeInit();                                                      /*  初始化硬件                  */

    /*
     *  这里的调试端口是脱离操作系统的, 所以他应该不依赖于操作系统而存在.
     *  当系统出现错误时, 这个端口显得尤为关键. (项目成熟后可以通过配置关掉)
     */
    uarths_init();

    /*
     *  这里使用 bsp 设置启动参数, 如果 bootloader 支持, 可使用 bootloader 设置.
     *  为了兼容以前的项目, 这里 kfpu=yes 允许内核中(包括中断)使用 FPU.
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes "
                         "rfsmap=/boot:/yaffs2/n0,/:/yaffs2/flash");
                                                                        /*  操作系统启动参数设置        */
    API_KernelStart(usrStartup,
                    (PVOID)&__heap_start,
                    (size_t)&__heap_end - (size_t)&__heap_start,
                    LW_NULL, 0);                                        /*  启动内核                    */

    return  (ERROR_NONE);                                               /*  不会执行到这里              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
