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
** ��   ��   ��: spi_sdi.c
**
** ��   ��   ��: Zeng.Bo(����)
**
** �ļ���������: 2016 �� 01 �� 20 ��
**
** ��        ��: ʹ��SPI���ߵ�SD����׼����Դ�ļ�
**               SylixOS��SD�����, һ����������һ��SD��һһ��Ӧ.
                 ���, ��һ��SPI�Ϲҽ���n��SD�豸, �������n��SDM������HOST.
** BUG:
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#define __SYLIXOS_IO
#include "SylixOS.h"
#include "driver/spi_sdi/spi_sdi.h"
/*********************************************************************************************************
  �궨��
*********************************************************************************************************/
#define SDI_CHAN_NUM                8                                   /*  ����ʹ�õ�ͨ���������      */
/*********************************************************************************************************
  ���� & �������
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
  ǰ������
*********************************************************************************************************/
struct __sdi_channel;
struct __sdi_sdm_host;
typedef struct __sdi_channel     __SDI_CHANNEL;
typedef struct __sdi_sdm_host    __SDI_SDM_HOST;
/*********************************************************************************************************
  GPIO ����
*********************************************************************************************************/
typedef struct {
    UINT                    SDIGPIO_uiCdPin;
    UINT                    SDIGPIO_uiWpPin;
} __SDI_GPIO;
/*********************************************************************************************************
  ע�ᵽ SDM ģ������ݽṹ
*********************************************************************************************************/
struct __sdi_sdm_host {
    SD_HOST                 SDISDMH_sdhost;
    __SDI_CHANNEL          *SDISDMH_psdichannel;
    SD_CALLBACK             SDISDMH_callbackChkDev;
    PVOID                   SDISDMH_pvCallBackArg;
    PVOID                   SDISDMH_pvDevAttached;
};
/*********************************************************************************************************
  ͨ�� ����
*********************************************************************************************************/
typedef struct __sdi_channel {
    __SDI_SDM_HOST          SDICH_sdmhost;
    CHAR                   *SDICH_cpcHostName;
    UINT                    SDICH_uiMasterID;
    UINT                    SDICH_uiDeviceID;                           /*  ��Ӧ�� SPI ���豸���       */
    UINT                    SDICH_iCardSta;                             /*  ��ǰ�豸����/�γ�״̬       */
    __SDI_GPIO              SDICH_sdigpio;
    PVOID                   SDICH_pvSdmHost;                            /*  ��Ӧ�� SDM HOST ����        */
} __SDI_CHANNEL;

#define __TO_SDMHOST(p)            ((__SDI_SDM_HOST *)p)
#define __TO_SDHOST(p)             ((SD_HOST *)p)
#define __TO_CHANNEL(p)            ((__SDI_CHANNEL *)p)
/*********************************************************************************************************
  ȫ�ֱ���
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
  ǰ������
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
** ��������: spisdiLibInit
** ��������: ��ʼ�� SPI SDI �����
** ��    ��: pspiplatformops  SPI ƽ̨����������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: spisdiDrvInstall
** ��������: ��װͨ������
** ��    ��: uiMasterID     SPI �����������(����0,��Ӧ"/bus/spi/0")
**           uiDeviceID     ��ǰ SPI �������ϵ� �豸���
**           uiCdPin        ��������ű��(SPI_SDI_PIN_NONE��ʾû�д�����, ���ʾ��������)
**           uiWpPin        д��������(SPI_SDI_PIN_NONE��ʾ�޴�����, ��Ĭ�ϲ�д����)
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
     * ʹ��SD��������չ�ӿ�����SD��ǰ�� 50M �Ŀռ䡣
     */
    //API_SdmHostExtOptSet(psdichannel->SDICH_pvSdmHost, SDHOST_EXTOPT_RESERVE_SECTOR_SET, 50 * 1024 * 1024 / 512);

    __sdiHotPlugInit(psdichannel);

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiChannelDataInit
** ��������: ͨ�����ݳ�ʼ��
** ��    ��: uiMasterID     SPI �����������(����0,��Ӧ"/bus/spi/0")
**           uiDeviceID     ��ǰ SPI �������ϵ� �豸���
**           uiCdPin        ��������ű��(SPI_SDI_PIN_NONE��ʾû�д�����, ���ʾ��������)
**           uiWpPin        д��������(SPI_SDI_PIN_NONE��ʾ�޴�����, ��Ĭ�ϲ�д����)
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiPortInit
** ��������: ͨ���˿ڳ�ʼ��
** ��    ��: psdichannel      ͨ������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiHotPlugInit
** ��������: �Ȳ��֧�ֳ�ʼ��
** ��    ��: psdichannel      ͨ������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdiHotPlugInit (__SDI_CHANNEL  *psdichannel)
{
    hotplugPollAdd((VOIDFUNCPTR)__sdiCdScan, (PVOID)psdichannel);
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: __sdiCdScan
** ��������: �豸״̬ɨ��
** ��    ��: psdichannel      ͨ������
** ��    ��: NONE
** ȫ�ֱ���:
** ����ģ��:
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

    if (iStaLast ^ iStaCurr) {                                          /*  ����״̬�仯                */
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

        psdichannel->SDICH_iCardSta = iStaCurr;                         /*  ����״̬                    */
    }
}
/*********************************************************************************************************
** ��������: __sdiCardStaGet
** ��������: �鿴�豸״̬
** ��    ��: psdichannel      ͨ������
** ��    ��: 0: ���γ�  ��0: ������
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __sdiCardStaGet (__SDI_CHANNEL  *psdichannel)
{
    UINT32  uiStatus;

    if (psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin != SPI_SDI_PIN_NONE) {
        uiStatus = API_GpioGetValue(psdichannel->SDICH_sdigpio.SDIGPIO_uiCdPin);
    } else {
        uiStatus = 0;                                                   /*  ��ʾ���ò���                */
    }

    return  (uiStatus == 0);                                            /*  ����ʱΪ�͵�ƽ              */
}
/*********************************************************************************************************
** ��������: __sdiCardStaGet
** ��������: �鿴�豸д״̬
** ��    ��: psdichannel      ͨ������
** ��    ��: 0: δд����  1: д����
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiSdmHostRegister
** ��������: �� SDM ��ע�� HOST ��Ϣ
** ��    ��: psdichannel     ͨ������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiDevStaCheck
** ��������: �豸״̬���
** ��    ��: psdichannel      ͨ������
**           iDevSta          �豸״̬
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiSdmCallBackInstall
** ��������: ��װ�ص�����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           iCallbackType    �ص���������
**           callback         �ص�����ָ��
**           pvCallbackArg    �ص���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiSdmCallBackUnInstall
** ��������: ע����װ�Ļص�����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
**           iCallbackType    �ص���������
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
** ��������: __sdiSdmIsCardWp
** ��������: �жϸ� HOST �϶�Ӧ�Ŀ��Ƿ�д����
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: LW_TRUE: ��д����    LW_FALSE: ��δд����
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static BOOL  __sdiSdmIsCardWp (SD_HOST  *psdhost)
{
    INT iIsWp;

    iIsWp = __sdiCardWpGet(__TO_CHANNEL(psdhost));
    return  ((BOOL)iIsWp);
}
/*********************************************************************************************************
** ��������: __sdiSdmSpicsEn
** ��������: Ƭѡʹ��
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID   __sdiSdmSpicsEn (SD_HOST *psdhost)
{
    __SDI_CHANNEL *psdichannel;

    psdichannel = __TO_CHANNEL(psdhost);
    __SDI_HOST_CS(psdichannel, LW_TRUE);
}
/*********************************************************************************************************
** ��������: __sdiSdmSpicsDis
** ��������: Ƭѡ��ֹ
** ��    ��: psdhost          SDM ��涨�� HOST ��Ϣ�ṹ
** ��    ��: ERROR CODE
** ȫ�ֱ���:
** ����ģ��:
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
