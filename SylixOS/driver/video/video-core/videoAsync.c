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
** 文   件   名: videoAsync.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: videoDevice 和 videoSubDev 异步匹配层
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "video_debug.h"

#include "videoAsync.h"
#include "videoSubDev.h"
#include "videoDevice.h"
/*********************************************************************************************************
  全局变量
*********************************************************************************************************/
static LW_OBJECT_HANDLE     _G_hListLock         = LW_OBJECT_HANDLE_INVALID;
static LW_LIST_LINE_HEADER  _G_asyncNotifierList = LW_NULL;
static LW_LIST_LINE_HEADER  _G_asyncDeviceList   = LW_NULL;
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define container_of(ptr, type, member) ({ \
        const typeof( ((type *)0)->member ) *__mptr = (ptr); \
        (type *)( (char *)__mptr - offsetof(type,member) );})
/*********************************************************************************************************
  宏操作
*********************************************************************************************************/
#define __VIDEO_LIST_LOCK()      API_SemaphoreBPend(_G_hListLock, LW_OPTION_WAIT_INFINITE)
#define __VIDEO_LIST_UNLOCK()    API_SemaphoreBPost(_G_hListLock)

#define ASYNC_MATCH_SUCESS      (1)
#define ASYNC_MATCH_FAILED      (0)

static INT __tryToMatchNotifier(PLW_ASYNC_NOTIFIER  pNotifier, PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    INT                         index;
    UINT                        uiSubDevNr;
    PLW_MATCH_INFO              pSubDevInfo;

    if (!pNotifier || !pSubDevice) {
        printk("__tryToMatchNotifier(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    uiSubDevNr   = pNotifier->NOTIFY_uiSubDevNumbers;
    pSubDevInfo  = pNotifier->NOTIFY_pMatchInfoArray;

    for (index = 0; index < uiSubDevNr; ++index) {
        if (lib_strcmp(pSubDevice->SUBDEV_bindCardName, pSubDevInfo->pMatchName) == 0) {
            return  (ASYNC_MATCH_SUCESS);
        }
        pSubDevInfo++;
    }

    return  (ASYNC_MATCH_FAILED);
}

INT API_VideoCardProberInstall (PLW_VIDEO_DEVICE  pVideoDevice, PLW_ASYNC_NOTIFIER  pNotifier)
{
    INT                     iResult = ASYNC_MATCH_FAILED;
    PLW_LIST_LINE           pNode;
    PLW_VIDEO_CARD_SUBDEV   pSubDevice = LW_NULL;

    if (!pVideoDevice || !pNotifier || !pNotifier->NOTIFY_pMatchInfoArray) {
        printk("API_VideoCardProbeInstall(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pNotifier->NOTIFY_hListSignal == LW_OBJECT_HANDLE_INVALID) {
        printk("API_VideoCardProbeInstall(): NOTIFY_hListSignal not init.\n");
        return  (PX_ERROR);
    }

    pNotifier->NOTIFY_pVideoDevice = pVideoDevice;

    /*
     * 遍历 _G_asyncDeviceList 去寻找要匹配的 SubDevice
     */
    for (pNode = _G_asyncDeviceList; pNode; pNode = pNode->LINE_plistNext) {
        pSubDevice = container_of(pNode, LW_VIDEO_CARD_SUBDEV, SUBDEV_lineNode);

        iResult = __tryToMatchNotifier(pNotifier, pSubDevice);
        if (iResult == ASYNC_MATCH_SUCESS) {                            /*          是否匹配成功        */

            API_SpinLock(&pVideoDevice->VIDEO_slSubDevLock);
            _List_Line_Add_Ahead(&pSubDevice->SUBDEV_lineNode, &pVideoDevice->VIDEO_subDevList);
            API_SpinUnlock(&pVideoDevice->VIDEO_slSubDevLock);
                                                                        /* 执行 subdev 被绑定的回调函数 */
            if (pSubDevice->SUBDEV_pInternalOps && pSubDevice->SUBDEV_pInternalOps->bind) {
                pSubDevice->SUBDEV_pInternalOps->bind(pSubDevice);
            }

            pNotifier->NOTIFY_uiMatchedDevNr++;                         /* 检查该Notifier是否匹配到所有 */
            if (pNotifier->NOTIFY_uiMatchedDevNr == pNotifier->NOTIFY_uiSubDevNumbers) {
                if (pNotifier->NOTIFY_pfuncComplete) {                  /* 执行匹配完成后的回调函数     */
                    pNotifier->NOTIFY_pfuncComplete(pNotifier);
                }

                break;                                                  /* 若所有的都匹配到了就退出循环 */
            }

        }
    }
                                                                        /* 若还有部分为匹配到则进入链表 */
    if (pNotifier->NOTIFY_uiMatchedDevNr < pNotifier->NOTIFY_uiSubDevNumbers) {
        __VIDEO_LIST_LOCK();
        _List_Line_Add_Ahead(&pNotifier->NOTIFY_lineNode, &_G_asyncNotifierList);
        __VIDEO_LIST_UNLOCK();
    }

    return  (ERROR_NONE);
}

INT API_VideoCardSubDevInstall (PLW_VIDEO_CARD_SUBDEV  pSubDevice)
{
    INT                 iResult = ASYNC_MATCH_FAILED;
    PLW_LIST_LINE       pNode;
    PLW_ASYNC_NOTIFIER  pNotifier;
    PLW_VIDEO_DEVICE    pVideoDevice = LW_NULL;

    if (!pSubDevice) {
        printk("API_VideoCardSubDevInstall(): invalid parameter.\n");
        return  (PX_ERROR);
    }

    /*
     * 遍历 _G_asyncNotifierList 去寻找要匹配的 Notifier
     */
    for (pNode = _G_asyncNotifierList; pNode; pNode = pNode->LINE_plistNext) {
        pNotifier = container_of(pNode, LW_ASYNC_NOTIFIER, NOTIFY_lineNode);

        iResult = __tryToMatchNotifier(pNotifier, pSubDevice);
        if (iResult == ASYNC_MATCH_SUCESS) {                            /*          是否匹配成功        */

            pVideoDevice = pNotifier->NOTIFY_pVideoDevice;

            API_SpinLock(&pVideoDevice->VIDEO_slSubDevLock);
            _List_Line_Add_Ahead(&pSubDevice->SUBDEV_lineNode, &pVideoDevice->VIDEO_subDevList);
            API_SpinUnlock(&pVideoDevice->VIDEO_slSubDevLock);
                                                                        /* 执行 subdev 被绑定的回调函数 */
            if (pSubDevice->SUBDEV_pInternalOps && pSubDevice->SUBDEV_pInternalOps->bind) {
                pSubDevice->SUBDEV_pInternalOps->bind(pSubDevice);
            }

            pNotifier->NOTIFY_uiMatchedDevNr++;                         /* 检查该Notifier是否匹配到所有 */
            if (pNotifier->NOTIFY_uiMatchedDevNr == pNotifier->NOTIFY_uiSubDevNumbers) {
                if (pNotifier->NOTIFY_pfuncComplete) {                  /*  执行匹配完成后的回调函数    */
                    pNotifier->NOTIFY_pfuncComplete(pNotifier);
                }
            }

            break;
        }
    }

    if (iResult != ASYNC_MATCH_SUCESS) {
        __VIDEO_LIST_LOCK();
        _List_Line_Add_Ahead(&pSubDevice->SUBDEV_lineNode, &_G_asyncDeviceList);
        __VIDEO_LIST_UNLOCK();
    }

    return  (ERROR_NONE);
}

INT API_AsyncCompleteFunSet(PLW_ASYNC_NOTIFIER  pNotifier, PLW_ASYNC_COMPLETE_FUNC pFunComplete)
{
    if (!pNotifier || !pFunComplete) {
        printk("API_AsyncCompleteFunSet(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    pNotifier->NOTIFY_pfuncComplete = pFunComplete;

    return  (ERROR_NONE);
}

INT API_AsyncNotifierInit (PLW_ASYNC_NOTIFIER  pNotifier)
{
    if (!pNotifier) {
        printk("API_AsyncCompleteFunSet(): invalid pNotifier.\n");
        return  (PX_ERROR);
    }

    pNotifier->NOTIFY_uiMatchedDevNr  = 0;
    pNotifier->NOTIFY_uiSubDevNumbers = 0;
    pNotifier->NOTIFY_pMatchInfoArray = LW_NULL;

    if (pNotifier->NOTIFY_hListSignal == LW_OBJECT_HANDLE_INVALID) {
        pNotifier->NOTIFY_hListSignal = API_SemaphoreBCreate("notifier_sem",
                                            LW_FALSE,
                                            LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    }

    if (pNotifier->NOTIFY_hListSignal) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}

INT API_AsyncSystemLibInit (VOID)
{
    /*
     * Using this interface to init video async matching system.
     */

    if (_G_hListLock == LW_OBJECT_HANDLE_INVALID) {                     /*  初始化用户互斥访问List的锁  */
        _G_hListLock = API_SemaphoreBCreate("video_list_lock",
                                            LW_TRUE,
                                            LW_OPTION_WAIT_FIFO | LW_OPTION_OBJECT_GLOBAL,
                                            LW_NULL);
    }

    if (_G_hListLock) {
        return  (ERROR_NONE);
    } else {
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
