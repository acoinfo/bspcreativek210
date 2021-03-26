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
** 文   件   名: videoFileHandle.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: VIDEO DEVICE 文件操作管理层
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "driver/common.h"
#include "video_debug.h"

#include "videoFileHandle.h"
#include "videoDevice.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
LONG  API_VideoFileOpen (PLW_DEV_HDR    pDev,
                         PCHAR          pcName,
                         INT            iFlags,
                         INT            iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    PLW_VIDEO_DEVICE           pVideoDevice = (PLW_VIDEO_DEVICE)pDev;
    PLW_VIDEO_DEVICE_DRV       pVideoDevDrv = pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_FILE_HANDLES     pFileHandle  = pVideoDevDrv->DEVICEDRV_pvideoFileOps;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pVideoDevice->VIDEO_fdNodeHeader,
                                   (dev_t)pVideoDevice, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__v4l2Open(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pVideoDevice->VIDEO_devHdr) == 1) {

            /*
             * TODO: 执行 Video 控制器的初始化和所有sub_device的初始化
             */
            if (pFileHandle->video_fh_open) {
                pFileHandle->video_fh_open(pVideoDevice);
            } else {
                printk("API_VideoFileOpen(): video_fh_open is invalid.\n");
                goto  __error_handle;
            }

        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pVideoDevice->VIDEO_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pVideoDevice->VIDEO_devHdr);

    return  (PX_ERROR);
}

INT API_VideoFileClose (PLW_FD_ENTRY   pFdEntry)
{
    PLW_FD_NODE           pFdNode      = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;
    PLW_VIDEO_DEVICE      pVideoDevice = (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;

    PLW_VIDEO_DEVICE_DRV       pVideoDevDrv = pVideoDevice->VIDEO_pVideoDriver;
    PLW_VIDEO_FILE_HANDLES     pFileHandle  = pVideoDevDrv->DEVICEDRV_pvideoFileOps;


    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pVideoDevice->VIDEO_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pVideoDevice->VIDEO_devHdr) == 0) {

            /*
             * TODO: 重置 Video 控制器和所有 sub_device 设备
             */
            if (pFileHandle->video_fh_close) {
                pFileHandle->video_fh_close(pVideoDevice);
            } else {
                printk("API_VideoFileClose(): video_fh_close is invalid.\n");
            }

        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}

INT  API_VideoFileLstat (PLW_DEV_HDR           pDev,
                         PCHAR                 pcName,
                         struct stat          *pStat)
{
    PLW_VIDEO_DEVICE        pVideoDevice = (PLW_VIDEO_DEVICE)pDev;

    if (pStat) {
        pStat->st_dev     = (dev_t)pVideoDevice;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pVideoDevice->VIDEO_time;
        pStat->st_mtime   = pVideoDevice->VIDEO_time;
        pStat->st_ctime   = pVideoDevice->VIDEO_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}

ssize_t  API_VideoFileRead (PLW_FD_ENTRY      pFdEntry,
                            PCHAR             pcBuffer,
                            size_t            stMaxBytes)
{
    ULONG             timeout;
    size_t            msglen       = 0;
    PLW_VIDEO_DEVICE  pVideoDevice = (PLW_VIDEO_DEVICE)pFdEntry->FDENTRY_pdevhdrHdr;

    if (!pFdEntry || !pcBuffer) {
        errno = EINVAL;
        return  (PX_ERROR);
    }

    if (!stMaxBytes) {
        return  (0);
    }

    if (pFdEntry->FDENTRY_iFlag & O_NONBLOCK) {
        timeout = 0;
    } else {
        timeout = LW_OPTION_WAIT_INFINITE;
    }

    if (API_MsgQueueReceive(pVideoDevice->VIDEO_hlMsgQueue, pcBuffer, stMaxBytes, &msglen, timeout)) {
        return  (PX_ERROR);
    }

    return  ((ssize_t)msglen);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
