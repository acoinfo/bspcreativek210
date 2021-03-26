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
** ��   ��   ��: k210_dvp.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 23 ��
**
** ��        ��: ��������ͷ�ӿ�(DVP)����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "k210_dvp.h"
#include "driver/common.h"
#include "driver/clock/k210_clock.h"
#include "driver/fix_arch_def.h"
#include "KendryteWare/include/dvp.h"
#include "KendryteWare/include/plic.h"
#include "KendryteWare/include/fpioa.h"

#include "k210_sccb.h"
#include "driver/video/video-core/video_fw.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __DVP_CONTROLLER_NAME_FMT       "/dev/dvp"
#define __DVP_CONTROLLER_SEM_FMT        "dvp_sem"
#define __DVP_CONTROLLER_CHANNEL_NR     (2)
#define __DVP_LCD_X_MAX                 (320)
#define __DVP_LCD_Y_MAX                 (240)
#define __DVP_LCD_FRAME_SIZE            (2 * __DVP_LCD_X_MAX * __DVP_LCD_Y_MAX)
/*********************************************************************************************************
  DVP ���Ͷ���
*********************************************************************************************************/
typedef struct {
    enum sysctl_clock_e         DVP_clock;                              /*  �豸ʱ����������            */
    BOOL                        DVP_bIsInit;                            /*  �豸�Ƿ��ʼ��              */
    addr_t                      DVP_ulPhyAddrBase;                      /*  �����ַ����ַ              */
    ULONG                       DVP_ulIrqVector;                        /*  DVP �ж�����                */

    LW_VIDEO_DEVICE             DVP_videoEntry;                         /* ��Ƶ���ַ��豸ʵ��           */
    LW_VIDEO_CARD_SUBDEV        DVP_vdieoSubDev;                        /* ����ͷISP����                */

    UINT                        DVP_uiBufQueueNr;                       /* ����ͷ������д�С           */

    /* Other user member of video device. */
    UINT32                      DVP_uiCaptureMode;                      /*  ���ڼ�¼video_cap_ctl.flag  */
    UINT                        DVP_checkData;                          /*  ���ڲ��Կ�ܵı�־λ        */

} __K210_DVP_INSTANCE,  *__PK210_DVP_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_DVP_INSTANCE   _G_k210DvpInstance = {
        .DVP_clock          = SYSCTL_CLOCK_DVP,
        .DVP_bIsInit         = LW_FALSE,
        .DVP_ulPhyAddrBase  = DVP_BASE_ADDR,
        .DVP_ulIrqVector    = KENDRYTE_PLIC_VECTOR(IRQN_DVP_INTERRUPT),
        .DVP_checkData      = 0xdeadbeef,
};
/*********************************************************************************************************
 �����ӿ�ʵ��
*********************************************************************************************************/
static INT  __k210DvpChannelConfig (PLW_VIDEO_DEVICE pVideoDevice, video_channel_ctl  *pCtrl)
{
    __PK210_DVP_INSTANCE  pDvpInstance  =  (__PK210_DVP_INSTANCE)pVideoDevice->VIDEO_pVideoDevPri;
    PLW_VIDEO_CHANNEL     pVideoChannel =  &pVideoDevice->VIDEO_channelArray[pCtrl->channel];

    if (!pVideoChannel) {
        printk(KERN_ERR "__k210DvpChannelConfig(): invalid pVideoChannel.\n");
        return  (PX_ERROR);
    }

    /*
     * ��ʼ�����нṹ
     */
    pDvpInstance->DVP_uiBufQueueNr = pCtrl->queue ? pCtrl->queue : 2;
    API_VideoBufQueueInit(&pVideoChannel->VCHAN_videoBuffer, __DVP_LCD_FRAME_SIZE, pCtrl->queue);

    /*
     * ����ͼ�����ģʽ��RGB ��YUV; ���� format �ο� video_format_t
     */
    if (pCtrl->format == VIDEO_PIXEL_FORMAT_RGB_565) {
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    } else {
        printk(KERN_ERR "__k210DvpChannelConfig(): unknow image format!\n");
        return  (PX_ERROR);
    }

    /*
     * ���� DVP ͼ��ɼ��ߴ�
     */
    dvp_set_image_size(pCtrl->xsize, pCtrl->ysize);

    return  (ERROR_NONE);
}

static INT __k210DvpPrepareBuf (PLW_VIDEO_DEVICE pVideoDevice, video_buf_ctl *pCtrl)
{
    addr_t                memAddrBase;
    PLW_VIDEO_CHANNEL     pVideoChannel  =  &pVideoDevice->VIDEO_channelArray[pCtrl->channel];

    /*
     * Ϊ��Ƶ�����е�BUF�����ڴ�  ע��: �����3�Ͳ��Գ����е�queue��Ӧ��
     * Ҳ����ʹ�ø�����Ľӿ� API_VideoBufMemPrep��
     */
    API_VideoBufMemAlloc(&pVideoChannel->VCHAN_videoBuffer, 0, &memAddrBase, __DVP_LCD_FRAME_SIZE * 3);
    pCtrl->mem = (PVOID)memAddrBase;

    dvp_set_display_addr(memAddrBase);

    return  (ERROR_NONE);
}

static INT  __k210DvpVideoCapCtrl (PLW_VIDEO_DEVICE pVideoDevice, video_cap_ctl  *pCtrl)
{
    __PK210_DVP_INSTANCE  pDvpInstance  =  (__PK210_DVP_INSTANCE)pVideoDevice->VIDEO_pVideoDevPri;
    PLW_VIDEO_CHANNEL     pVideoChannel =  &pVideoDevice->VIDEO_channelArray[pCtrl->channel];

    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);

    pDvpInstance->DVP_uiCaptureMode = pCtrl->flags;

    /*
     * �������ģʽʹ�ܻ����
     */
    pVideoChannel->VCHAN_flags |= pCtrl->on;
    if (pVideoChannel->VCHAN_uiIndex == 0) {
        dvp_set_output_enable(DVP_OUTPUT_DISPLAY, pCtrl->on);
    } else {
        dvp_set_output_enable(DVP_OUTPUT_AI, pCtrl->on);
    }

    /*
     * ���� DVP �ж�����
     */
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, pCtrl->on);

    API_VideoChanStreamSet(pVideoChannel, pCtrl->on);           //�ú����ӿ���ʱֻ�����ñ�־��û�п���Ӳ��

    return  (ERROR_NONE);
}
/*********************************************************************************************************
 LW_VIDEO_IOCTL_OPS
*********************************************************************************************************/
static struct lw_video_ioctl_ops __k210DvpIoctls = {
        .vidioc_g_dev_desc      = LW_NULL,
        .vidioc_g_chan_desc     = LW_NULL,
        .vidioc_g_format_desc   = LW_NULL,

        .vidioc_s_chan_config   = __k210DvpChannelConfig,
        .vidioc_prepare_vbuf    = __k210DvpPrepareBuf,
        .vidioc_s_cap_ctl       = __k210DvpVideoCapCtrl,
};
/*********************************************************************************************************
 LW_VIDEO_FILE_HANDLES
*********************************************************************************************************/
static irqreturn_t  __k210DvpOnIrqIsr (PVOID pvArg, ULONG ulVector)
{
    addr_t                   frameAddr;
    UINT                     planeID;
    __PK210_DVP_INSTANCE     pDvpChannel   = (__PK210_DVP_INSTANCE)pvArg;
    PLW_VIDEO_DEVICE         pVideoDevice  = &pDvpChannel->DVP_videoEntry;
    PLW_VIDEO_CHANNEL        pVideoChannel =  &pVideoDevice->VIDEO_channelArray[0];

    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH)) {

        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);

        /*
         * TODO: ������һ�����ڴ����Ƶ֡�ĵ�ַ
         */
        API_VideoBufNextPlaneGet(&pVideoChannel->VCHAN_videoBuffer, &frameAddr, &planeID);
        dvp_set_display_addr(frameAddr);

        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);

    } else {

        dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return  (LW_IRQ_HANDLED);
}

static VOID  __k210DvpOpen (PLW_VIDEO_DEVICE   pVideoDevice)
{
    __PK210_DVP_INSTANCE  pDvpInstance  =  (__PK210_DVP_INSTANCE)pVideoDevice->VIDEO_pVideoDevPri;

    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /*
     * ��ʼ��DVP, reg_len = 8 (sccb �Ĵ�������)
     */
    dvp_init(16);

    /*
     * ʹ��ͻ������ģʽ
     */
    dvp_enable_burst();

    /*
     * ���� DVP �ж�����: ��ֹ�����ж�
     */
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /*
     * ע���ж�
     */
    API_InterVectorSetPriority(pDvpInstance->DVP_ulIrqVector, BSP_CFG_GPIO_INT_PRIO);
    API_InterVectorConnect(pDvpInstance->DVP_ulIrqVector, (PINT_SVR_ROUTINE)__k210DvpOnIrqIsr,
                           (PVOID)pDvpInstance, "dvp_isr");
    API_InterVectorEnable(pDvpInstance->DVP_ulIrqVector);

    /*
     * ͨ�� sccb ���߳�ʼ�� ov5640
     */
    ov5640_init();
}

static VOID  __k210DvpClose (PLW_VIDEO_DEVICE   pVideoDevice)
{
    __PK210_DVP_INSTANCE  pDvpInstance  =  (__PK210_DVP_INSTANCE)pVideoDevice->VIDEO_pVideoDevPri;

    dvp_disable_burst();
    dvp_disable_auto();
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);

    API_InterVectorDisable(pDvpInstance->DVP_ulIrqVector);
}

static struct lw_video_file_handles __k210FileOps = {
       .video_fh_open   =  __k210DvpOpen,
       .video_fh_close  =  __k210DvpClose,
};
/*********************************************************************************************************
 Video Device Driver
*********************************************************************************************************/
static LW_VIDEO_DEVICE_DRV _G_VideoDeviceDriver = {
        .DEVICEDRV_pvideoIoctls  = &__k210DvpIoctls,
        .DEVICEDRV_pvideoFileOps = &__k210FileOps,
};
/*********************************************************************************************************
** ��������: k210DvpDrv
** ��������: ��װ DVP ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210DvpDrv (VOID)
{
    INT                 iRet = ERROR_NONE;
    PLW_VIDEO_DEVICE     pVideoDevice;
    PLW_VIDEO_CHANNEL   pVideoChannel;

    pVideoDevice = &_G_k210DvpInstance.DVP_videoEntry;

    API_VideoDeviceInit(&_G_k210DvpInstance.DVP_videoEntry, __DVP_CONTROLLER_CHANNEL_NR);

    API_VideoDevImageAttrSet(&_G_k210DvpInstance.DVP_videoEntry, __DVP_LCD_X_MAX, __DVP_LCD_Y_MAX);

    API_VideoDevCapAttrSet(&_G_k210DvpInstance.DVP_videoEntry,    /* ��ʼ�����������Ӳ�����Բ��� */
                                 VIDEO_CAP_FORMAT_RGB_888 | VIDEO_CAP_FORMAT_RGB_565,
                                 VIDEO_CAP_EFFECT_BRIGHTNESS | VIDEO_CAP_EFFECT_CONTRAST);

    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[0]);
    strncpy(pVideoChannel->VCHAN_cDescription, "display_channel", strlen("display_channel"));
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[1]);
    strncpy(pVideoChannel->VCHAN_cDescription, "ai_caculate_channel", strlen("ai_caculate_channel"));

                                                                        /* ��װ video device ����       */
    iRet = API_VideoDeviceDrvInstall(&_G_k210DvpInstance.DVP_videoEntry, &_G_VideoDeviceDriver);

    return  (iRet);
}
/*********************************************************************************************************
 LW_SUBDEV_INTERNAL_OPS
*********************************************************************************************************/
INT __k210SccbIspBind (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    printk("__k210SccbIspBind(): do something when subdev ISP bind to video device.\n");

    return  (ERROR_NONE);
}

INT __k210SccbIspUnbind (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    printk("__k210SccbIspUnbind(): do something when subdev ISP unbind to video device.\n");

    return  (ERROR_NONE);
}

INT __k210SccbIspInit (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    printk("__k210SccbIspInit(): do something to init the ISP.\n");

    return  (ERROR_NONE);
}

static LW_SUBDEV_INTERNAL_OPS __k210SccbSubDevOps = {
       .bind     =  __k210SccbIspBind,
       .unbind   =  __k210SccbIspUnbind,
};
/*********************************************************************************************************
** ��������: k210DvpDevAdd
** ��������: ���� DVP �豸
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210DvpDevAdd (VOID)
{
    INT  iRet = ERROR_NONE;

    PLW_VIDEO_CARD_SUBDEV   pSubDevice  = &_G_k210DvpInstance.DVP_vdieoSubDev;

    API_VideoCardSubDevInit(pSubDevice,
                            "/dev/video0",
                            SUBDEV_BUS_TYPE_SCCB,
                            "/dev/sccb0",
                            0X78);                                      /* OV5640_ADDR  0X78            */

    pSubDevice->SUBDEV_pCommonOps    =  LW_NULL;
    pSubDevice->SUBDEV_pInternalOps  =  &__k210SccbSubDevOps;
                                                                        /* �� subdev �� video device  */
    API_VideoDeviceSubDevInstall(&_G_k210DvpInstance.DVP_videoEntry, pSubDevice);

    _G_k210DvpInstance.DVP_videoEntry.VIDEO_pVideoDevPri =  &_G_k210DvpInstance;
                                                                        /* ��װ video device �豸       */
    iRet = API_VideoDeviceCreate(&_G_k210DvpInstance.DVP_videoEntry, "/dev/video0");

    return  (iRet);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
