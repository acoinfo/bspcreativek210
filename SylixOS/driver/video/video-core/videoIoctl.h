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
** 文   件   名: videoIoctl.h
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: video device 文件操作框架层接口头文件
*********************************************************************************************************/

#ifndef __VIDEOIOCTL_H
#define __VIDEOIOCTL_H

/*********************************************************************************************************
   超前声明
*********************************************************************************************************/
struct lw_video_device;
/*********************************************************************************************************

*********************************************************************************************************/
typedef struct lw_video_ioctl_ops {

    /* VIDIOC_DEVDESC       :   获得设备描述信息 */
    /* VIDIOC_CHANDESC      :   获得指定通道的信息 */
    /* VIDIOC_FORMATDESC    :   获得指定通道支持的格式信息 */
    INT               (*vidioc_g_dev_desc)(struct lw_video_device* pVideoDevice, video_dev_desc *pVideoDevDesc);
    INT               (*vidioc_g_chan_desc)(struct lw_video_device* pVideoDevice, video_channel_desc *pVideoChanDesc);
    INT               (*vidioc_g_format_desc)(struct lw_video_device* pVideoDevice, video_format_desc *pVideoFormatDesc);

    /* VIDIOC_GCHANCTL      :   获取通道控制字 */
    /* VIDIOC_SCHANCTL      :   设置通道控制字 */
    INT               (*vidioc_g_chan_config)(struct lw_video_device* pVideoDevice, video_channel_ctl *pVideoChanCtl);
    INT               (*vidioc_s_chan_config)(struct lw_video_device* pVideoDevice, video_channel_ctl *pVideoChanCtl);   //===> 必须实现

    /* VIDIOC_MAPCAL        :   读取内存需求情况 */
    /* VIDIOC_MAPPREPAIR    :   准备映射指定通道内存 */
    INT               (*vidioc_g_vbuf_info)(struct lw_video_device* pVideoDevice, video_buf_cal *pVideoBufInfo);
    INT               (*vidioc_prepare_vbuf)(struct lw_video_device* pVideoDevice, video_buf_ctl *pVideoBufConfig);      //===> 必须实现

    /* VIDIOC_CAPSTAT        :  查询当前捕获信息 */
    INT               (*vidioc_querycap_stat)(struct lw_video_device* pVideoDevice, video_cap_stat *pVideoCapStat);

    /* VIDIOC_GCAPCTL        :  获得当前视频捕获状态 */
    /* VIDIOC_SCAPCTL        :  设置视频捕获状态 1:ON 0:OFF */
    INT               (*vidioc_g_cap_ctl)(struct lw_video_device* pVideoDevice, video_cap_ctl *pVideoCaptureCtl);
    INT               (*vidioc_s_cap_ctl)(struct lw_video_device* pVideoDevice, video_cap_ctl *pVideoCaptureCtl);        //===> 必须实现

    /* VIDIOC_GEFFCTL        :  获得图像特效信息 */
    /* VIDIOC_SEFFCTL        :  设置设置图像特效 */
    INT               (*vidioc_g_image_effect)(struct lw_video_device* pVideoDevice, video_effect_info *pVideoImageEffect);
    INT               (*vidioc_s_image_effect)(struct lw_video_device* pVideoDevice, video_effect_ctl *pVideoEffectConfig);
} LW_VIDEO_IOCTL_OPS, *PLW_VIDEO_IOCTL_OPS;
/*********************************************************************************************************

*********************************************************************************************************/
INT  API_VideoIoctlManager(PLW_FD_ENTRY  pFdEntry, INT  iCmd, LONG  lArg);

#endif                                                                  /*       __VIDEOIOCTL_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
