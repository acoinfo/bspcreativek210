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
** 文   件   名: dummySubDev.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 12 月 7 日
**
** 描        述: 高级加密加速器(ISP)驱动
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>

#include "dummySubDev.h"
#include "driver/video/video-core/video_fw.h"
/*********************************************************************************************************
  类型定义
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  ISP_devHdr;                             /* 对于特殊的子设备可以单独注册 */
    LW_LIST_LINE_HEADER         ISP_fdNodeHeader;                       /* 成一个字符设备提供特殊操作   */
    time_t                      ISP_time;

    LW_VIDEO_CARD_SUBDEV        ISP_subDev;                             /* Video Card Sub Device 抽象   */

    /*
     * TODO: to add your device or driver info.
     */

} __K210_ISP_INSTANCE, *__PK210_ISP_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_ISP_INSTANCE   _G_k210IspInstance = {
        /*
         * TODO: to init your device hw info.
         */
};
static INT  _G_iK210IspDrvNum    =  PX_ERROR;
/*********************************************************************************************************
** 函数名称: __dummyIspOpen
** 功能描述: 打开 ISP 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static LONG  __dummyIspOpen (PLW_DEV_HDR           pDev,
                            PCHAR                 pcName,
                            INT                   iFlags,
                            INT                   iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    __PK210_ISP_INSTANCE       pIspInstance = container_of(pDev, __K210_ISP_INSTANCE, ISP_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pIspInstance->ISP_fdNodeHeader,
                                   (dev_t)pIspInstance, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__dummyIspOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pIspInstance->ISP_devHdr) == 1) {

            /*
             * TODO:  __dummyIspOpen
             */
            bspDebugMsg("__dummyIspOpen CALLED.\r\n");
        }

        return  ((LONG)pFdNode);
    }

    __error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pIspInstance->ISP_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pIspInstance->ISP_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __dummyIspClose
** 功能描述: 关闭 ISP 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT __dummyIspClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_ISP_INSTANCE  pIspInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                      __K210_ISP_INSTANCE,
                                                      ISP_devHdr);
    PLW_FD_NODE           pFdNode      = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pIspInstance->ISP_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pIspInstance->ISP_devHdr) == 0) {

            /*
             * TODO:  __dummyIspClose
             */
            bspDebugMsg("__dummyIspClose CALLED.\r\n");
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __dummyIspLstat
** 功能描述: 获得 ISP 设备状态
** 输　入  : pDev                  设备
**           pcName                设备名字
**           pStat                 stat 结构指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dummyIspLstat (PLW_DEV_HDR           pDev,
                            PCHAR                 pcName,
                            struct stat          *pStat)
{
    __PK210_ISP_INSTANCE  pIspInstance = container_of(pDev, __K210_ISP_INSTANCE, ISP_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pIspInstance;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pIspInstance->ISP_time;
        pStat->st_mtime   = pIspInstance->ISP_time;
        pStat->st_ctime   = pIspInstance->ISP_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** 函数名称: __dummyIspIoctl
** 功能描述: 控制 ISP 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __dummyIspIoctl (PLW_FD_ENTRY      pFdEntry,
                            INT               iCmd,
                            LONG              lArg)
{
    struct stat               *pStat;
    __PK210_ISP_INSTANCE       pIspInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_ISP_INSTANCE, ISP_devHdr);


    switch (iCmd) {

    case  FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pIspInstance;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pIspInstance->ISP_time;
            pStat->st_mtime   = pIspInstance->ISP_time;
            pStat->st_ctime   = pIspInstance->ISP_time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case  FIOSETFL:
        if ((int)lArg & O_NONBLOCK) {
            pFdEntry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pFdEntry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    /*
     * TODO: __dummyIspIoctl
     */

    case  DUMMY_SUBDEV_SPEC_FUNC1:
        /*
         * TODO: to realize DUMMY_SUBDEV_SPEC_FUNC1
         */
        bspDebugMsg("DUMMY_SUBDEV_SPEC_FUNC1 CALLED.\r\n");
        return  (ERROR_NONE);

    case  DUMMY_SUBDEV_SPEC_FUNC2:
        /*
         * TODO: to realize DUMMY_SUBDEV_SPEC_FUNC2
         */
        bspDebugMsg("DUMMY_SUBDEV_SPEC_FUNC2 CALLED.\r\n");
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** 函数名称: k210IspDrv
** 功能描述: 安装 ISP 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  dummyVideoIspDrv(VOID)
{
    struct file_operations  fileOper;

    bspDebugMsg("==> dummyVideoIspDrv\r\n");

    if (_G_iK210IspDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __dummyIspOpen;
    fileOper.fo_open   = __dummyIspOpen;
    fileOper.fo_close  = __dummyIspClose;
    fileOper.fo_lstat  = __dummyIspLstat;
    fileOper.fo_ioctl  = __dummyIspIoctl;

    _G_iK210IspDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210IspDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210IspDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210IspDrvNum, "k210 video dummy driver.");

    return  ((_G_iK210IspDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
 LW_SUBDEV_INTERNAL_OPS
*********************************************************************************************************/
INT __dummyI2cIspBind (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    printk("__dummyI2cIspBind(): do something when subdev ISP bind to video device.\n");

    return  (ERROR_NONE);
}

INT __dummyI2cIspUnbind (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    printk("__dummyI2cIspUnbind(): do something when subdev ISP unbind to video device.\n");

    return  (ERROR_NONE);
}

static LW_SUBDEV_INTERNAL_OPS __dummyI2cSubDevOps = {
       .bind     =  __dummyI2cIspBind,                                  /*  bind和unbind函数必须被实现  */
       .unbind   =  __dummyI2cIspUnbind,
       .init     =  LW_NULL,                                            /*  init和release函数可选择实现 */
       .release  =  LW_NULL,
};

static LW_SUBDEV_COMMON_OPS  __dummySubDevCommonOps = {
       .cameraIspOps  =  LW_NULL,
};
/*********************************************************************************************************
** 函数名称: k210IspDrv
** 功能描述: 创建 ISP 设备
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  dummyVideoIspDevAdd(VOID)
{
    __PK210_ISP_INSTANCE    pIspInstance;

    bspDebugMsg("==> dummyVideoIspDevAdd\r\n");

    pIspInstance = (__PK210_ISP_INSTANCE)&_G_k210IspInstance;

    /* To prepare our video sub device here. */

    PLW_VIDEO_CARD_SUBDEV   pSubDevice  = &pIspInstance->ISP_subDev;

    API_VideoCardSubDevInit(pSubDevice,
                            "/dev/video_dummy",
                            SUBDEV_BUS_TYPE_I2C,
                            "/dev/i2c",
                            0X00);

    API_VideoCardSubDevOpsSet(pSubDevice, &__dummySubDevCommonOps, &__dummyI2cSubDevOps);

    API_VideoSubDevPriDataSet(pSubDevice, pIspInstance);                /* 用于保存私有数据             */

    API_VideoCardSubDevInstall(pSubDevice);                             /* 注册 sub device 用于异步匹配 */

    /* Video sub device preparing finished here. */

    pIspInstance->ISP_time = time(LW_NULL);

    if (API_IosDevAddEx(&pIspInstance->ISP_devHdr, "/dev/isp", _G_iK210IspDrvNum, DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210IspDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
