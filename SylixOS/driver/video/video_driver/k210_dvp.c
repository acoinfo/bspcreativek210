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
** 文   件   名: k210_dvp.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 23 日
**
** 描        述: 数字摄像头接口(DVP)驱动
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
  定义
*********************************************************************************************************/
#define __DVP_CONTROLLER_NAME_FMT       "/dev/dvp"
#define __DVP_CONTROLLER_SEM_FMT        "dvp_sem"
#define __DVP_CONTROLLER_CHANNEL_NR     (2)
#define __DVP_LCD_X_MAX                 (320)
#define __DVP_LCD_Y_MAX                 (240)
#define __DVP_LCD_FRAME_SIZE            (2 * __DVP_LCD_X_MAX * __DVP_LCD_Y_MAX)
/*********************************************************************************************************
  DVP 类型定义
*********************************************************************************************************/
typedef struct {
    enum sysctl_clock_e         DVP_clock;                              /*  设备时钟类型配置            */
    BOOL                        DVP_bIsInit;                            /*  设备是否初始化              */
    addr_t                      DVP_ulPhyAddrBase;                      /*  物理地址基地址              */
    ULONG                       DVP_ulIrqVector;                        /*  DVP 中断向量                */

    LW_VIDEO_DEVICE             DVP_videoEntry;                         /* 视频类字符设备实体           */
    LW_VIDEO_CARD_SUBDEV        DVP_vdieoSubDev;                        /* 摄像头ISP对象                */

    UINT                        DVP_uiBufQueueNr;                       /* 摄像头缓冲队列大小           */

    /* Other user member of video device. */
    UINT32                      DVP_uiCaptureMode;                      /*  用于记录video_cap_ctl.flag  */
    UINT                        DVP_checkData;                          /*  用于测试框架的标志位        */

} __K210_DVP_INSTANCE,  *__PK210_DVP_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_DVP_INSTANCE   _G_k210DvpInstance = {
        .DVP_clock          = SYSCTL_CLOCK_DVP,
        .DVP_bIsInit         = LW_FALSE,
        .DVP_ulPhyAddrBase  = DVP_BASE_ADDR,
        .DVP_ulIrqVector    = KENDRYTE_PLIC_VECTOR(IRQN_DVP_INTERRUPT),
        .DVP_checkData      = 0xdeadbeef,
};
/*********************************************************************************************************
 函数接口实现
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
     * 初始化队列结构
     */
    pDvpInstance->DVP_uiBufQueueNr = pCtrl->queue ? pCtrl->queue : 2;
    API_VideoBufQueueInit(&pVideoChannel->VCHAN_videoBuffer, __DVP_LCD_FRAME_SIZE, pCtrl->queue);

    /*
     * 设置图像接收模式，RGB 或YUV; 参数 format 参考 video_format_t
     */
    if (pCtrl->format == VIDEO_PIXEL_FORMAT_RGB_565) {
        dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    } else {
        printk(KERN_ERR "__k210DvpChannelConfig(): unknow image format!\n");
        return  (PX_ERROR);
    }

    /*
     * 设置 DVP 图像采集尺寸
     */
    dvp_set_image_size(pCtrl->xsize, pCtrl->ysize);

    return  (ERROR_NONE);
}

static INT __k210DvpPrepareBuf (PLW_VIDEO_DEVICE pVideoDevice, video_buf_ctl *pCtrl)
{
    addr_t                memAddrBase;
    PLW_VIDEO_CHANNEL     pVideoChannel  =  &pVideoDevice->VIDEO_channelArray[pCtrl->channel];

    /*
     * 为视频队列中的BUF分配内存  注意: 这里的3和测试程序中的queue对应，
     * 也可以使用更方便的接口 API_VideoBufMemPrep。
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
     * 设置输出模式使能或禁用
     */
    pVideoChannel->VCHAN_flags |= pCtrl->on;
    if (pVideoChannel->VCHAN_uiIndex == 0) {
        dvp_set_output_enable(DVP_OUTPUT_DISPLAY, pCtrl->on);
    } else {
        dvp_set_output_enable(DVP_OUTPUT_AI, pCtrl->on);
    }

    /*
     * 配置 DVP 中断类型
     */
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, pCtrl->on);

    API_VideoChanStreamSet(pVideoChannel, pCtrl->on);           //该函数接口暂时只是设置标志，没有控制硬件

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
         * TODO: 设置下一个用于存放视频帧的地址
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
     * 初始化DVP, reg_len = 8 (sccb 寄存器长度)
     */
    dvp_init(16);

    /*
     * 使能突发传输模式
     */
    dvp_enable_burst();

    /*
     * 配置 DVP 中断类型: 禁止所有中断
     */
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /*
     * 注册中断
     */
    API_InterVectorSetPriority(pDvpInstance->DVP_ulIrqVector, BSP_CFG_GPIO_INT_PRIO);
    API_InterVectorConnect(pDvpInstance->DVP_ulIrqVector, (PINT_SVR_ROUTINE)__k210DvpOnIrqIsr,
                           (PVOID)pDvpInstance, "dvp_isr");
    API_InterVectorEnable(pDvpInstance->DVP_ulIrqVector);

    /*
     * 通过 sccb 总线初始化 ov5640
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
** 函数名称: k210DvpDrv
** 功能描述: 安装 DVP 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210DvpDrv (VOID)
{
    INT                 iRet = ERROR_NONE;
    PLW_VIDEO_DEVICE     pVideoDevice;
    PLW_VIDEO_CHANNEL   pVideoChannel;

    pVideoDevice = &_G_k210DvpInstance.DVP_videoEntry;

    API_VideoDeviceInit(&_G_k210DvpInstance.DVP_videoEntry, __DVP_CONTROLLER_CHANNEL_NR);

    API_VideoDevImageAttrSet(&_G_k210DvpInstance.DVP_videoEntry, __DVP_LCD_X_MAX, __DVP_LCD_Y_MAX);

    API_VideoDevCapAttrSet(&_G_k210DvpInstance.DVP_videoEntry,    /* 初始化及设置相关硬件特性参数 */
                                 VIDEO_CAP_FORMAT_RGB_888 | VIDEO_CAP_FORMAT_RGB_565,
                                 VIDEO_CAP_EFFECT_BRIGHTNESS | VIDEO_CAP_EFFECT_CONTRAST);

    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[0]);
    strncpy(pVideoChannel->VCHAN_cDescription, "display_channel", strlen("display_channel"));
    pVideoChannel = &(pVideoDevice->VIDEO_channelArray[1]);
    strncpy(pVideoChannel->VCHAN_cDescription, "ai_caculate_channel", strlen("ai_caculate_channel"));

                                                                        /* 安装 video device 驱动       */
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
** 函数名称: k210DvpDevAdd
** 功能描述: 创建 DVP 设备
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
                                                                        /* 绑定 subdev 到 video device  */
    API_VideoDeviceSubDevInstall(&_G_k210DvpInstance.DVP_videoEntry, pSubDevice);

    _G_k210DvpInstance.DVP_videoEntry.VIDEO_pVideoDevPri =  &_G_k210DvpInstance;
                                                                        /* 安装 video device 设备       */
    iRet = API_VideoDeviceCreate(&_G_k210DvpInstance.DVP_videoEntry, "/dev/video0");

    return  (iRet);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
