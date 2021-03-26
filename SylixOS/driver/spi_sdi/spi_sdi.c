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
** 文   件   名: spi_sdi.c
**
** 创   建   人: Zeng.Bo(曾波)
**
** 文件创建日期: 2016 年 01 月 20 日
**
** 描        述: 使用SPI总线的SD卡标准驱动源文件
**               SylixOS的SD设计中, 一个控制器与一个SD卡一一对应.
                 因此, 若一个SPI上挂接了n个SD设备, 则会抽象出n个SDM层管理的HOST.
** BUG:
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#define __SYLIXOS_IO
#include "SylixOS.h"
#include "driver/spi_sdi/spi_sdi.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define SDI_CHAN_NUM                8                                   /*  配置使用的通道最大数量      */
/*********************************************************************************************************
  工具 & 调试相关
*********************************************************************************************************/
#define __SDI_EN_INFO               0

#define __SDI_DEBUG(fmt, arg...)    printk("[SPI-SDI] %s() " fmt, __func__, ##arg)

#if __SDI_EN_INFO > 0
#define __SDI_INFO(fmt, arg...)     printk("[SPI-SDI] " fmt, ##arg)
#else
#define __SDI_INFO(fmt, arg...)
#endif

#ifndef __OFFSET_OF
#define __OFFSET_OF(t, m)           ((ULONG)((CHAR *)&((t *)0)->m - (CHAR *)(t *)0))
#endif
#ifndef __CONTAINER_OF
#define __CONTAINER_OF(p, t, m)     ((t *)((CHAR *)p - __OFFSET_OF(t, m)))
#endif
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
struct __sdi_channel;
struct __sdi_sdm_host;
typedef struct __sdi_channel     __SDI_CHANNEL;
typedef struct __sdi_sdm_host    __SDI_SDM_HOST;
/*********************************************************************************************************
  GPIO 描述
*********************************************************************************************************/
typedef struct {
    UINT                    SDIGPIO_uiCdPin;
    UINT                    SDIGPIO_uiWpPin;
} __SDI_GPIO;
/*********************************************************************************************************
  注册到 SDM 模块的数据结构
*********************************************************************************************************/
struct __sdi_sdm_host {
    SD_HOST                 SDISDMH_sdhost;
    __SDI_CHANNEL          *SDISDMH_psdichannel;
    SD_CALLBACK             SDISDMH_callbackChkDev;
    PVOID                   SDISDMH_pvCallBackArg;
    PVOID                   SDISDMH_pvDevAttached;
};
/*********************************************************************************************************
  通道 描述
*********************************************************************************************************/
typedef struct __sdi_channel {
    __SDI_SDM_HOST          SDICH_sdmhost;
    CHAR                   *SDICH_cpcHostName;
    UINT                    SDICH_uiMasterID;
    UINT                    SDICH_uiDeviceID;                           /*  对应的 SPI 从设备标号       */
    UINT                    SDICH_iCardSta;                             /*  当前设备插入/拔出状态       */
    __SDI_GPIO              SDICH_sdigpio;
    PVOID                   SDICH_pvSdmHost;                            /*  对应的 SDM HOST 对象        */
} __SDI_CHANNEL;

#define __TO_SDMHOST(p)            ((__SDI_SDM_HOST *)p)
#define __TO_SDHOST(p)             ((SD_HOST *)p)
#define __TO_CHANNEL(p)            ((__SDI_CHANNEL *)p)
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static __SDI_CHANNEL        _G_sdichannelTbl[SDI_CHAN_NUM];
static SPI_PLATFORM_OPS     _G_spiplatops;

#define __SDI_CHIP_SELECT(m, d, b) _G_spiplatops.SPIPO_pfuncChipSelect(m, d, b)
#define __SDI_HOST_CS(h, b)        __SDI_CHIP_SELECT((h)->SDICH_uiMasterID, (h)->SDICH_uiDeviceID, b)

static CHAR                *_G_pcSpiAdapterName[SDI_CHAN_NUM] = {
        "/bus/spi/0", "/bus/spi/1", "/bus/spi/2", "/bus/spi/3",
        "/bus/spi/4", "/bus/spi/5", "/bus/spi/6", "/bus/spi/7",
};
/*********************************************************************************************************
  前置声明
*********************************************************************************************************/
static INT    __sdiChannelDataInit(UINT   uiMasterID,
                                   UINT   uiDeviceID,
                                   UINT   uiCdPin,
                                   UINT   uiWpPin);

static INT    __sdiPortInit(__SDI_CHANNEL       *psdichannel);
static INT    __sdiHotPlugInit(__SDI_CHANNEL    *psdichannel);
static VOID   __sdiCdScan(__SDI_CHANNEL         *psdichannel);
static INT    __sdiCardStaGet(__SDI_CHANNEL     *psdichannel);
static INT    __sdiCardWpGet(__SDI_CHANNEL      *psdichannel);
static INT    __sdiDevStaCheck(__SDI_CHANNEL    *psdichannel, INT iDevSta);

static INT    __sdiSdmHostRegister(__SDI_CHANNEL   *psdichannel);
static INT    __sdiSdmCallBackInstall(SD_HOST      *psdhost,
                                      INT           iCallbackType,
                                      SD_CALLBACK   callback,
                                      PVOID         pvCallbackArg);
static INT    __sdiSdmCallBackUnInstall(SD_HOST    *psdhost,
                                        INT         iCallbackType);

static BOOL   __sdiSdmIsCardWp(SD_HOST   *psdhost);
static VOID   __sdiSdmSpicsEn(SD_HOST    *psdhost);
static VOID   __sdiSdmSpicsDis(SD_HOST   *psdhost);
/*********************************************************************************************************
** 函数名称: spisdiLibInit
** 功能描述: 初始化 SPI SDI 组件库
** 输    入: pspiplatformops  SPI 平台操作函数集
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT   spisdiLibInit (SPI_PLATFORM_OPS *pspiplatformops)
{
    if (!pspiplatformops || !pspiplatformops->SPIPO_pfuncChipSelect) {
        return  (PX_ERROR);
    }

    _G_spiplatops = *pspiplatformops;

    API_SdmLibInit();
    API_SdMemDrvInstall();

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: spisdiDrvInstall
** 功能描述: 安装通道驱动
** 输    入: uiMasterID     SPI 主控制器编号(如编号0,对应"/bus/spi/0")
**           uiDeviceID     当前 SPI 控制器上的 设备编号
**           uiCdPin        卡检测引脚编号(SPI_SDI_PIN_NONE表示没有此引脚, 则表示永久链接)
**           uiWpPin        写保护引脚(SPI_SDI_PIN_NONE表示无此引脚, 则默认不写保护)
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT   spisdiDrvInstall (UINT   uiMasterID,
                        UINT   uiDeviceID,
                        UINT   uiCdPin,
                        UINT   uiWpPin)
{
    __SDI_CHANNEL  *psdichannel;
    INT             iRet;

    iRet = __sdiChannelDataInit(uiMasterID, uiDeviceID, uiCdPin, uiWpPin);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    psdichannel = &_G_sdichannelTbl[uiMasterID];

    iRet = __sdiPortInit(psdichannel);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    iRet = __sdiSdmHostRegister(psdichannel);
    if (iRet != ERROR_NONE) {
        return  (PX_ERROR);
    }

    /*
     * 使用SD卡驱动扩展接口跳过SD中前面 50M 的空间。
     */
    //API_SdmHostExtOptSet(psdichannel->SDICH_pvSdmHost, SDHOST_EXTOPT_RESERVE_SECTOR_SET, 50 * 1024 * 1024 / 512);

    __sdiHotPlugInit(psdichannel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiChannelDataInit
** 功能描述: 通道数据初始化
** 输    入: uiMasterID     SPI 主控制器编号(如编号0,对应"/bus/spi/0")
**           uiDeviceID     当前 SPI 控制器上的 设备编号
**           uiCdPin        卡检测引脚编号(SPI_SDI_PIN_NONE表示没有此引脚, 则表示永久链接)
**           uiWpPin        写保护引脚(SPI_SDI_PIN_NONE表示无此引脚, 则默认不写保护)
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdiChannelDataInit (UINT   uiMasterID,
                                 UINT   uiDeviceID,
                                 UINT   uiCdPin,
                                 UINT   uiWpPin)
{
    __SDI_CHANNEL       *psdichannel;
    __SDI_GPIO          *psdigpio;

    if (uiMasterID >= SDI_CHAN_NUM) {
        __SDI_DEBUG("master id is out of range(%d/%d)\r\n", uiMasterID, SDI_CHAN_NUM);
        return  (PX_ERROR);
    }

    psdichannel = &_G_sdichannelTbl[uiMasterID];
    psdigpio    = &psdichannel->SDICH_sdigpio;

    psdichannel->SDICH_uiMasterID  = uiMasterID;
    psdichannel->SDICH_uiDeviceID  = uiDeviceID;
    psdichannel->SDICH_iCardSta    = 0;
    psdichannel->SDICH_cpcHostName = _G_pcSpiAdapterName[uiMasterID];

    psdigpio->SDIGPIO_uiCdPin = uiCdPin;
    psdigpio->SDIGPIO_uiWpPin = uiWpPin;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiPortInit
** 功能描述: 通道端口初始化
** 输    入: psdichannel      通道对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdiPortInit (__SDI_CHANNEL  *psdichannel)
{
    if (psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin != SPI_SDI_PIN_NONE) {
        API_GpioRequestOne(psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin,
                           LW_GPIOF_IN,
                           LW_NULL);
    }

    if (psdichannel->SDICH_sdigpio.SDIGPIO_uiWpPin != SPI_SDI_PIN_NONE) {
        API_GpioRequestOne(psdichannel->SDICH_sdigpio.SDIGPIO_uiWpPin,
                           LW_GPIOF_IN,
                           LW_NULL);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiHotPlugInit
** 功能描述: 热插拔支持初始化
** 输    入: psdichannel      通道对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdiHotPlugInit (__SDI_CHANNEL  *psdichannel)
{
    hotplugPollAdd((VOIDFUNCPTR)__sdiCdScan, (PVOID)psdichannel);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiCdScan
** 功能描述: 设备状态扫描
** 输    入: psdichannel      通道对象
** 输    出: NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID __sdiCdScan (__SDI_CHANNEL  *psdichannel)
{
    INT  iStaLast;
    INT  iStaCurr;

    if (!psdichannel) {
        return;
    }

    iStaLast = psdichannel->SDICH_iCardSta;
    iStaCurr = __sdiCardStaGet(psdichannel);

    if (iStaLast ^ iStaCurr) {                                          /*  插入状态变化                */
        if (iStaCurr) {
            __SDI_INFO("sdcard insert to channel%d dev%d.\r\n",
                       psdichannel->SDICH_uiMasterID, psdichannel->SDICH_uiDeviceID);
            API_SdmEventNotify(psdichannel->SDICH_pvSdmHost,
                               SDM_EVENT_DEV_INSERT);
        } else {
            __SDI_INFO("sdcard removed from channel%d dev%d.\r\n",
                       psdichannel->SDICH_uiMasterID, psdichannel->SDICH_uiDeviceID);
            __sdiDevStaCheck(psdichannel, SDHOST_DEVSTA_UNEXIST);
            API_SdmEventNotify(psdichannel->SDICH_pvSdmHost,
                               SDM_EVENT_DEV_REMOVE);
        }

        psdichannel->SDICH_iCardSta = iStaCurr;                         /*  保存状态                    */
    }
}
/*********************************************************************************************************
** 函数名称: __sdiCardStaGet
** 功能描述: 查看设备状态
** 输    入: psdichannel      通道对象
** 输    出: 0: 卡拔出  非0: 卡插入
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __sdiCardStaGet (__SDI_CHANNEL  *psdichannel)
{
    UINT32  uiStatus;

    if (psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin != SPI_SDI_PIN_NONE) {
        uiStatus = API_GpioGetValue(psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin);
    } else {
        uiStatus = 0;                                                   /*  表示永久插入                */
    }

    return  (uiStatus == 0);                                            /*  插入时为低电平              */
}
/*********************************************************************************************************
** 函数名称: __sdiCardStaGet
** 功能描述: 查看设备写状态
** 输    入: psdichannel      通道对象
** 输    出: 0: 未写保护  1: 写保护
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiCardWpGet (__SDI_CHANNEL  *psdichannel)
{
    UINT32  uiStatus;

    if (psdichannel->SDICH_sdigpio.SDIGPIO_uiWpPin != SPI_SDI_PIN_NONE) {
        uiStatus = API_GpioGetValue(psdichannel->SDICH_sdigpio.SDIGPIO_uiWpPin);
    } else {
        uiStatus = 0;
    }

    __SDI_INFO("sdi channel%d dev%dcard insert with %s mode.\r\n",
               psdichannel->SDICH_uiMasterID, psdichannel->SDICH_uiDeviceID,
               uiStatus ? "read-only" : "read-write");

    return  (uiStatus);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmHostRegister
** 功能描述: 向 SDM 层注册 HOST 信息
** 输    入: psdichannel     通道对象
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiSdmHostRegister (__SDI_CHANNEL  *psdichannel)
{
    __SDI_SDM_HOST *psdmhost;
    SD_HOST        *psdhost;
    PVOID           pvSdmHost;

    psdhost  = __TO_SDHOST(psdichannel);
    psdmhost = __TO_SDMHOST(psdichannel);

    psdhost->SDHOST_cpcName                = psdichannel->SDICH_cpcHostName;
    psdhost->SDHOST_iCapbility             = 0;
    psdhost->SDHOST_iType                  = SDHOST_TYPE_SPI;
    psdhost->SDHOST_pfuncCallbackInstall   = __sdiSdmCallBackInstall;
    psdhost->SDHOST_pfuncCallbackUnInstall = __sdiSdmCallBackUnInstall;
    psdhost->SDHOST_pfuncIsCardWp          = __sdiSdmIsCardWp;
    psdhost->SDHOST_pfuncSpicsEn           = __sdiSdmSpicsEn;
    psdhost->SDHOST_pfuncSpicsDis          = __sdiSdmSpicsDis;
    psdhost->SDHOST_pfuncSdioIntEn         = LW_NULL;
    psdhost->SDHOST_pfuncDevAttach         = LW_NULL;
    psdhost->SDHOST_pfuncDevDetach         = LW_NULL;

    pvSdmHost = API_SdmHostRegister(psdhost);
    if (!pvSdmHost) {
        __SDI_DEBUG("can't register into sdm modules.\r\n");
        return  (PX_ERROR);
    }

    psdmhost->SDISDMH_psdichannel = psdichannel;
    psdichannel->SDICH_pvSdmHost  = pvSdmHost;

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiDevStaCheck
** 功能描述: 设备状态监测
** 输    入: psdichannel      通道对象
**           iDevSta          设备状态
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiDevStaCheck (__SDI_CHANNEL  *psdichannel, INT iDevSta)
{
    __SDI_SDM_HOST  *psdmhost;

    psdmhost = __TO_SDMHOST(psdichannel);

    if (psdmhost->SDISDMH_callbackChkDev) {
        psdmhost->SDISDMH_callbackChkDev(psdmhost->SDISDMH_pvCallBackArg, iDevSta);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmCallBackInstall
** 功能描述: 安装回调函数
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           iCallbackType    回调函数类型
**           callback         回调函数指针
**           pvCallbackArg    回调函数参数
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiSdmCallBackInstall (SD_HOST      *psdhost,
                                     INT           iCallbackType,
                                     SD_CALLBACK   callback,
                                     PVOID         pvCallbackArg)
{
    __SDI_SDM_HOST  *psdmhost;

    psdmhost = __TO_SDMHOST(psdhost);
    if (!psdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdmhost->SDISDMH_callbackChkDev = callback;
        psdmhost->SDISDMH_pvCallBackArg  = pvCallbackArg;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmCallBackUnInstall
** 功能描述: 注销安装的回调函数
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
**           iCallbackType    回调函数类型
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __sdiSdmCallBackUnInstall (SD_HOST    *psdhost,
                                       INT         iCallbackType)
{
    __SDI_SDM_HOST  *psdmhost;

    psdmhost = __TO_SDMHOST(psdhost);
    if (!psdmhost) {
        return  (PX_ERROR);
    }

    if (iCallbackType == SDHOST_CALLBACK_CHECK_DEV) {
        psdmhost->SDISDMH_callbackChkDev = LW_NULL;
        psdmhost->SDISDMH_pvCallBackArg  = LW_NULL;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmIsCardWp
** 功能描述: 判断该 HOST 上对应的卡是否写保护
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
** 输    出: LW_TRUE: 卡写保护    LW_FALSE: 卡未写保护
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static BOOL  __sdiSdmIsCardWp (SD_HOST  *psdhost)
{
    INT iIsWp;

    iIsWp = __sdiCardWpGet(__TO_CHANNEL(psdhost));
    return  ((BOOL)iIsWp);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmSpicsEn
** 功能描述: 片选使能
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID   __sdiSdmSpicsEn (SD_HOST *psdhost)
{
    __SDI_CHANNEL *psdichannel;

    psdichannel = __TO_CHANNEL(psdhost);
    __SDI_HOST_CS(psdichannel, LW_TRUE);
}
/*********************************************************************************************************
** 函数名称: __sdiSdmSpicsDis
** 功能描述: 片选禁止
** 输    入: psdhost          SDM 层规定的 HOST 信息结构
** 输    出: ERROR CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID   __sdiSdmSpicsDis (SD_HOST *psdhost)
{
    __SDI_CHANNEL *psdichannel;

    psdichannel = __TO_CHANNEL(psdhost);
    __SDI_HOST_CS(psdichannel, LW_FALSE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
