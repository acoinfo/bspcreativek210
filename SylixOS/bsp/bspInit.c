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
** ��   ��   ��: bspInit.c
**
** ��   ��   ��: Jiao.JinXing (������)
**
** �ļ���������: 2018 �� 03 �� 20 ��
**
** ��        ��: BSP �û� C �������
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "config.h"                                                     /*  �������� & ���������       */
/*********************************************************************************************************
  ����ϵͳ���
*********************************************************************************************************/
#include "SylixOS.h"                                                    /*  ����ϵͳ                    */
#include "stdlib.h"                                                     /*  for system() function       */
#include "gdbmodule.h"                                                  /*  GDB ������                  */
#include "gdbserver.h"                                                  /*  GDB ������                  */
#include "sys/compiler.h"                                               /*  ���������                  */
/*********************************************************************************************************
  BSP �� �������� (������� BSP �� ���������ͷ�ļ�)
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
  �ڴ��ʼ��ӳ���
*********************************************************************************************************/
/*********************************************************************************************************
  ���߳��������̶߳�ջ (t_boot ���Դ�һ��, startup.sh �п����кܶ����Ķ�ջ�Ĳ���)
*********************************************************************************************************/
#define  __LW_THREAD_BOOT_STK_SIZE      (16 * LW_CFG_KB_SIZE)
#define  __LW_THREAD_MAIN_STK_SIZE      (16 * LW_CFG_KB_SIZE)
/*********************************************************************************************************
  ���߳�����
*********************************************************************************************************/
VOID  t_main(VOID);
/*********************************************************************************************************
** ��������: bspSetupCpuFreq
** ��������: ��ʼ�� CPU ʱ��
** �䡡��  : frequency     CPU����Ƶ��
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: bspSetupClocks
** ��������: ��ʼ��ʱ��ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: targetInit
** ��������: ��ʼ��Ŀ���·��ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: halModeInit
** ��������: ��ʼ��Ŀ��ϵͳ���е�ģʽ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halModeInit (VOID)
{
    /*
     * TODO: ������Ĵ������, �����鲻������
     */
}
/*********************************************************************************************************
** ��������: halTimeInit
** ��������: ��ʼ��Ŀ��ϵͳʱ��ϵͳ (ϵͳĬ��ʱ��Ϊ: ��8��)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_RTC_EN > 0

static VOID  halTimeInit (VOID)
{
    PLW_RTC_FUNCS   pRtcFuncs = k210RtcGetFuncs();

    rtcDrv();
    rtcDevCreate(pRtcFuncs);                                            /*  ����Ӳ�� RTC �豸           */
    rtcToSys();                                                         /*  �� RTC ʱ��ͬ����ϵͳʱ��   */
}

#endif                                                                  /*  LW_CFG_RTC_EN > 0           */
/*********************************************************************************************************
** ��������: halIdleInit
** ��������: ��ʼ��Ŀ��ϵͳ����ʱ����ҵ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halIdleInit (VOID)
{
    API_SystemHookAdd(archWaitForInterrupt,
                      LW_OPTION_THREAD_IDLE_HOOK);                      /*  ����ʱ��ͣ CPU              */
}
/*********************************************************************************************************
** ��������: halCacheInit
** ��������: Ŀ��ϵͳ CPU ���ٻ����ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CACHE_EN > 0

static VOID  halCacheInit (VOID)
{
    API_CacheLibInit(CACHE_COPYBACK, CACHE_COPYBACK, RISCV_MACHINE_GENERAL);
    API_CacheEnable(INSTRUCTION_CACHE);
    API_CacheEnable(DATA_CACHE);                                        /*  ʹ�� CACHE                  */
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halFpuInit
** ��������: Ŀ��ϵͳ FPU �������㵥Ԫ��ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_CPU_FPU_EN > 0

static VOID  halFpuInit (VOID)
{
    API_KernelFpuInit(RISCV_MACHINE_GENERAL, RISCV_FPU_DP);
}

#endif                                                                  /*  LW_CFG_CACHE_EN > 0         */
/*********************************************************************************************************
** ��������: halPmInit
** ��������: ��ʼ��Ŀ��ϵͳ��Դ����ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POWERM_EN > 0

static VOID  halPmInit (VOID)
{
    /*
     * TODO: ������Ĵ������, �ο���������:
     */
#if 0                                                                   /*  �ο����뿪ʼ                */
    PLW_PMA_FUNCS  pmafuncs = pmGetFuncs();

    pmAdapterCreate("inner_pm", 21, pmafuncs);
#endif                                                                  /*  �ο��������                */
}

#endif                                                                  /*  LW_CFG_POWERM_EN > 0        */
/*********************************************************************************************************
** ��������: halBusInit
** ��������: ��ʼ��Ŀ��ϵͳ����ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halBusInit (VOID)
{
    INT              i;
    ULONG            ulMaxBytes;
    PLW_DMA_FUNCS    pDmaFuncs;
    PLW_I2C_FUNCS    pi2cfuncHandle;
    PLW_SPI_FUNCS    pspifuncHandle;

    for (i = 0; i < DMA_CHANNEL_NR; i++) {                              /*  ��װ 64 ��ͨ�� DMA ͨ��     */
        pDmaFuncs = dmaGetFuncs(LW_DMA_CHANNEL0 + i, &ulMaxBytes);
        if (pDmaFuncs) {
            API_DmaDrvInstall((UINT)i, pDmaFuncs, (size_t)ulMaxBytes);  /*  ��װ DMA ����������         */
        }
    }

    API_I2cLibInit();                                                   /*  ��ʼ�� i2c ��ϵͳ           */
    pi2cfuncHandle = i2cBusFuncs(0);
    if (pi2cfuncHandle) {
        API_I2cAdapterCreate("/bus/i2c/0", pi2cfuncHandle, 10, 1);      /*  ���� i2c0 ����������        */
    }
    pi2cfuncHandle = i2cBusFuncs(1);
    if (pi2cfuncHandle) {
        API_I2cAdapterCreate("/bus/i2c/1", pi2cfuncHandle, 10, 1);      /*  ���� i2c1 ����������        */
    }

    API_SpiLibInit();                                                   /*  ��ʼ�� spi �����           */
    pspifuncHandle = spiBusDrv(0);                                      /*  �������� TF ��              */
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/0", pspifuncHandle);             /*  ���� spi0 ����������        */
    }
    pspifuncHandle = spiBusDrv(1);
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/1", pspifuncHandle);             /*  ���� spi1 ����������        */
    }
    pspifuncHandle = spiBusDrv(3);
    if (pspifuncHandle) {
        API_SpiAdapterCreate("/bus/spi/3", pspifuncHandle);             /*  ���� spi3 ����������        */
    }

}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDrvInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬��������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
    fpioa_set_function(32, FUNC_GPIOHS7);                               /*  ע��: �˴�����ΪS7��Ӧ���  */
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
    spisdiDrvInstall(1, 3, SPI_SDI_PIN_NONE, SPI_SDI_PIN_NONE);         /*  ע��: �˴� sdcard ʹ�� spi1 */
#endif

#endif

}

#endif                                                                  /*  LW_CFG_DEVICE_EN > 0        */
/*********************************************************************************************************
** ��������: halDevInit
** ��������: ��ʼ��Ŀ��ϵͳ��̬�豸���
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_DEVICE_EN > 0

static VOID  halDevInit (VOID)
{
    SIO_CHAN    *psio;
    SIO_CHAN    *psio0;

    /*
     *  �������ļ�ϵͳʱ, ���Զ����� dev, mnt, var Ŀ¼.
     */
    rootFsDevCreate();                                                  /*  �������ļ�ϵͳ              */
    procFsDevCreate();                                                  /*  ���� proc �ļ�ϵͳ          */
#if LW_CFG_SHM_DEVICE_EN > 0
    shmDevCreate();                                                     /*  ���������ڴ��豸            */
#endif
    randDevCreate();                                                    /*  ����������ļ�              */

    psio0 = sioChanCreate(0);                                           /*  �������� 0 ͨ��             */
    ttyDevCreate("/dev/ttyS0", psio0, 30, 50);                          /*  add    tty   device         */

    psio  = uartChanCreate(0);                                          /*  �������� 0 ͨ��             */
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
** ��������: halLogInit
** ��������: ��ʼ��Ŀ��ϵͳ��־ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_LOG_LIB_EN > 0

static VOID  halLogInit (VOID)
{
    fd_set      fdLog;

    FD_ZERO(&fdLog);
    FD_SET(STD_OUT, &fdLog);
    API_LogFdSet(STD_OUT + 1, &fdLog);                                  /*  ��ʼ����־                  */
}

#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */
/*********************************************************************************************************
** ��������: halStdFileInit
** ��������: ��ʼ��Ŀ��ϵͳ��׼�ļ�ϵͳ
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: halShellInit
** ��������: ��ʼ��Ŀ��ϵͳ shell ����, (getopt ʹ��ǰһ��Ҫ��ʼ�� shell ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_SHELL_EN > 0

static VOID  halShellInit (VOID)
{
    API_TShellInit();

    /*
     *  ��ʼ�� appl �м�� shell �ӿ�
     */
#ifndef __SYLIXOS_LITE
    zlibShellInit();
#endif
    viShellInit();

    /*
     *  ��ʼ�� GDB ������
     */
#if LW_CFG_GDB_EN > 0
    gdbInit();
    gdbModuleInit();
#endif                                                                  /*  LW_CFG_GDB_EN > 0           */
}

#endif                                                                  /*  LW_CFG_SHELL_EN > 0         */
/*********************************************************************************************************
** ��������: halVmmInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ڴ�������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_VMM_EN > 0

static VOID  halVmmInit (VOID)
{
    /*
     * TODO: ������Ĵ���
     */
}

#endif                                                                  /*  LW_CFG_VMM_EN > 0           */
/*********************************************************************************************************
** ��������: halNetInit
** ��������: ���������ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_NET_EN > 0

static VOID  halNetInit (VOID)
{
    API_NetInit();                                                      /*  ��ʼ������ϵͳ              */
    API_NetSnmpInit();

    /*
     *  ��ʼ�����總�ӹ���
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
** ��������: halNetifAttch
** ��������: ����ӿ�����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  halNetifAttch (VOID)
{
    /*
     * DM9051 ��������ע��, �������Ĭ�����ã������뿴netif.c�е����ò����꣩
     */
    if (enetInit(DM9051_AUTO, LW_NULL, 0) != ERROR_NONE) {
        printk("[halNetifAttch]: network interface init FILAED!\n");
    }
}

#endif                                                                  /*  LW_CFG_NET_EN > 0           */
/*********************************************************************************************************
** ��������: halMonitorInit
** ��������: �ں˼�����ϴ���ʼ��
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MONITOR_EN > 0

static VOID  halMonitorInit (VOID)
{
    /*
     *  ���������ﴴ���ں˼�����ϴ�ͨ��, Ҳ����ʹ�� shell �������.
     */
}

#endif                                                                  /*  LW_CFG_MONITOR_EN > 0       */
/*********************************************************************************************************
** ��������: halPosixInit
** ��������: ��ʼ�� posix ��ϵͳ (���ϵͳ֧�� proc �ļ�ϵͳ, �������� proc �ļ�ϵͳ��װ֮��!)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_POSIX_EN > 0

static VOID  halPosixInit (VOID)
{
    API_PosixInit();
}

#endif                                                                  /*  LW_CFG_POSIX_EN > 0         */
/*********************************************************************************************************
** ��������: halSymbolInit
** ��������: ��ʼ��Ŀ��ϵͳ���ű���, (Ϊ module loader �ṩ����)
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: halLoaderInit
** ��������: ��ʼ��Ŀ��ϵͳ�����ģ��װ����
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
#if LW_CFG_MODULELOADER_EN > 0

static VOID  halLoaderInit (VOID)
{
    API_LoaderInit();
}

#endif                                                                  /*  LW_CFG_SYMBOL_EN > 0        */
/*********************************************************************************************************
** ��������: halBootThread
** ��������: ������״̬�µĳ�ʼ����������
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static PVOID  halBootThread (PVOID  pvBootArg)
{
    LW_CLASS_THREADATTR     threadattr = API_ThreadAttrGetDefault();    /*  ʹ��Ĭ������                */

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
    console_loglevel = default_message_loglevel;                        /*  ���� printk ��ӡ��Ϣ�ȼ�    */
#endif                                                                  /*  LW_CFG_LOG_LIB_EN > 0       */

    /*
     *  ��Ϊ yaffs ���������ʱ, ��Ҫ stdout ��ӡ��Ϣ, ����� halDevInit() �б�����, ����û�д���
     *  ��׼�ļ�, ���Ի��ӡ���������Ϣ, ���Խ��˺�����������!
     *  ���δ��ʼ����׼�ļ�����ʾ������Ϣ
     */

#if KENDRYTE_FLASH_ENABLE
    /*
     * SPI-norFlash Driver Register
     */
    spiFlashDevCreate("/bus/spi/3", 0);                                 /* ���� norflash �豸           */

    /*
     * Skip front 4MB memory just like kendryte sdk did.
     */
    spiFlashDrvInstall(LW_NULL, 0x400000);                              /* ע�� SPI-norflash ����       */
#endif


#if LW_CFG_DEVICE_EN > 0                                                /*  map rootfs                  */
    rootFsMap(LW_ROOTFS_MAP_LOAD_VAR | LW_ROOTFS_MAP_SYNC_TZ | LW_ROOTFS_MAP_SET_TIME);
#endif

    /*
     *  �����ʼ��һ����� shell ��ʼ��֮��, ��Ϊ��ʼ���������ʱ, ���Զ�ע�� shell ����.
     */
#if LW_CFG_NET_EN > 0
    halNetInit();
    halNetifAttch();                                                    /*  wlan ������Ҫ���ع̼�       */
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
    tshellStartup();                                                    /*  ִ�������ű�                */
#endif

    API_ThreadAttrSetStackSize(&threadattr, __LW_THREAD_MAIN_STK_SIZE); /*  ���� main �̵߳Ķ�ջ��С    */
    API_ThreadCreate("t_main",
                     (PTHREAD_START_ROUTINE)t_main,
                     &threadattr,
                     LW_NULL);                                          /*  Create "t_main()" thread    */

    return  (LW_NULL);
}
/*********************************************************************************************************
** ��������: usrStartup
** ��������: ��ʼ��Ӧ��������, ��������ϵͳ�ĵ�һ������.
** �䡡��  : NONE
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  usrStartup (VOID)
{
    LW_CLASS_THREADATTR     threadattr;

    /*
     *  ע��, ��Ҫ�޸ĸó�ʼ��˳�� (�����ȳ�ʼ�� vmm ������ȷ�ĳ�ʼ�� cache,
     *                              ������Ҫ������Դ��������ʼ��)
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
** ��������: bspInit
** ��������: C ���
** �䡡��  : NONE
** �䡡��  : 0
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT bspInit (ULONG  ulMHartId, addr_t  ulDtb)
{
    /*
     *  ϵͳ�ں˶���ϵͳ��
     */
    extern UCHAR  __heap_start, __heap_end;

    targetInit();                                                       /*  ��ʼ��ϵͳʱ�Ӽ���������    */

    halModeInit();                                                      /*  ��ʼ��Ӳ��                  */

    /*
     *  ����ĵ��Զ˿����������ϵͳ��, ������Ӧ�ò������ڲ���ϵͳ������.
     *  ��ϵͳ���ִ���ʱ, ����˿��Ե���Ϊ�ؼ�. (��Ŀ��������ͨ�����ùص�)
     */
    uarths_init();

    /*
     *  ����ʹ�� bsp ������������, ��� bootloader ֧��, ��ʹ�� bootloader ����.
     *  Ϊ�˼�����ǰ����Ŀ, ���� kfpu=yes �����ں���(�����ж�)ʹ�� FPU.
     */
    API_KernelStartParam("ncpus=1 kdlog=no kderror=yes kfpu=no heapchk=yes "
                         "rfsmap=/boot:/yaffs2/n0,/:/yaffs2/flash");
                                                                        /*  ����ϵͳ������������        */
    API_KernelStart(usrStartup,
                    (PVOID)&__heap_start,
                    (size_t)&__heap_end - (size_t)&__heap_start,
                    LW_NULL, 0);                                        /*  �����ں�                    */

    return  (ERROR_NONE);                                               /*  ����ִ�е�����              */
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
