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
** 文   件   名: videoSubDev.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 视频卡子设备头文件
*********************************************************************************************************/

#ifndef __VIDEOSUBDEV_H
#define __VIDEOSUBDEV_H

/*********************************************************************************************************
   超前声明
*********************************************************************************************************/
struct lw_video_card_subdev;
struct lw_video_device;
struct lw_video_event;
/*********************************************************************************************************

*********************************************************************************************************/

typedef enum lw_card_client_bus_type{
    SUBDEV_BUS_TYPE_SCCB  =  0,
    SUBDEV_BUS_TYPE_I2C   =  1,
    SUBDEV_BUS_TYPE_SPI   =  2,
    SUBDEV_BUS_TYPE_USB   =  3,
    SUBDEV_BUS_TYPE_MAX
} LW_CARD_CLIENT_BUS_TYPE;

struct subdev_camera_isp_ops {                                                                 //该接口根据Linux中OV5640驱动的部分接口定义，参考而来 ov5640_ctrl_ops
    INT        (*set_ctrl_gain)(struct lw_video_card_subdev*  pSubDevice, INT auto_gain);      //设置视频采集模式为自动获取，还是手动触发
    INT        (*set_ctrl_exposure)(struct lw_video_card_subdev*  pSubDevice, INT exp);        //设置摄像头的曝光控制
    INT        (*set_ctrl_white_balance)(struct lw_video_card_subdev*  pSubDevice, INT awb);   //设置摄像头的白平衡
    INT        (*set_ctrl_hue)(struct lw_video_card_subdev*  pSubDevice, int value);           //设置摄像头的色调
    INT        (*set_ctrl_contrast)(struct lw_video_card_subdev*  pSubDevice, int value);      //设置摄像头的对比度
    INT        (*set_ctrl_saturation)(struct lw_video_card_subdev*  pSubDevice, int value);    //设置摄像头采集图像的饱和度
};

typedef struct lw_subdev_common_ops {
    const struct subdev_camera_isp_ops   *cameraIspOps;                 /*  摄像头中 Image sensor processor 一些通用功能的接口 */
    const uint32_t                       *otherIC[8];                   /*  预留给其它类型 IC 的通用工程接口 */
} LW_SUBDEV_COMMON_OPS,  *PLW_SUBDEV_COMMON_OPS;

typedef struct lw_subdev_internal_ops {
    INT         (*bind)(struct lw_video_card_subdev*  pSubDevice);             /*  当与video_card匹配时被调用  */
    INT         (*unbind)(struct lw_video_card_subdev*  pSubDevice);           /*  当从video_card匹配链卸载时被调用  */

    INT         (*init)(struct lw_video_card_subdev*  pSubDevice);             /*  用于初始化该设备的工作模式  */
    INT         (*release)(struct lw_video_card_subdev*  pSubDevice);          /*  用于释放为该设备所申请的资源  */
} LW_SUBDEV_INTERNAL_OPS,  *PLW_SUBDEV_INTERNAL_OPS;
/*********************************************************************************************************

*********************************************************************************************************/
typedef struct lw_video_card_subdev {
    LW_LIST_LINE                    SUBDEV_lineNode;                    /*  用于链接到异步匹配链和所匹配到的 video_card_devcie 上 */
    CHAR                            SUBDEV_bindCardName[64];            /*  保存所要匹配的视频卡的名字  */
    struct lw_video_device*         SUBDEV_pCardDevice;                 /*  指向所匹配到的视频卡结构体  */

    LW_CARD_CLIENT_BUS_TYPE         SUBDEV_clientBusType;               /*  控制 Coder, Decoder, ISP 所使用用的总线类型 */
    CHAR                            SUBDEV_clientBusName[64];           /*  总线的设备文件路径，如: /dev/i2c */
    PVOID                           SUBDEV_pBusClient;                  /*  如: i2c_device, spi_device 等 */
    INT32                           SUBDEV_iClientAddr;                 /*  从机的地址 */

    spinlock_t                      SUBDEV_lock;                        /*  主要用于管理 ops 的互斥访问 */
    PLW_SUBDEV_COMMON_OPS           SUBDEV_pCommonOps;                  /*  视屏设备的一些通用功能实现接口，根据设备类型和具体的支持情况选择实现 */
    PLW_SUBDEV_INTERNAL_OPS         SUBDEV_pInternalOps;                /*  被框架所使用的一些通用接口 */

    PVOID                           SUBDEV_pSubDevPri;                  /*  私有数据指针，一般用于指向包含该结构体的子类 */

} LW_VIDEO_CARD_SUBDEV, *PLW_VIDEO_CARD_SUBDEV;
/*********************************************************************************************************

*********************************************************************************************************/
INT    API_VideoCardSubDevInit(PLW_VIDEO_CARD_SUBDEV       pSubDevice,
                               PCHAR                       pBindCardName,
                               LW_CARD_CLIENT_BUS_TYPE     busType,
                               PCHAR                       pBusAdapterName,
                               INT32                       iClientAddr);

INT    API_VideoCardSubDevOpsSet(PLW_VIDEO_CARD_SUBDEV     pSubDevice,
                                 PLW_SUBDEV_COMMON_OPS     pCommonOps,
                                 PLW_SUBDEV_INTERNAL_OPS   pInternalOps);

VOID   API_VideoSubDevPriDataGet(PLW_VIDEO_CARD_SUBDEV  pSubDevice, PVOID  *pSubDevPriData);

VOID   API_VideoSubDevPriDataSet(PLW_VIDEO_CARD_SUBDEV  pSubDevice, PVOID  pSubDevPriData);

VOID   API_VideoSubDevEntryGet(PLW_LIST_LINE  pSubDevNode, PLW_VIDEO_CARD_SUBDEV  *pSubDev);

INT    API_VideoSubDevEventAdd(struct lw_video_device*  pVideoDevice, struct lw_video_event*  pVideoEvent);

INT    API_VideoSubDevNotifyEvent(PLW_VIDEO_CARD_SUBDEV  pSubDevice, struct lw_video_event*  pVideoEvent);

#endif                                                                  /*      __VIDEOSUBDEV_H         */
/*********************************************************************************************************
  END
*********************************************************************************************************/
