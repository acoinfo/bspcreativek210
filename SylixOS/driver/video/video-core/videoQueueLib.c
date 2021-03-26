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
** 文   件   名: videoQueueLib.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 28 日
**
** 描        述: 队列库.
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "video_debug.h"

#include "videoQueueLib.h"
/*********************************************************************************************************
** 函数名称: API_QueueInit
** 功能描述: 初始化队列
** 输　入  : pQueue            队列
**           uiSize            队列大小
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
VOID  API_QueueInit (PLW_QUEUE  pQueue, UINT  uiSize)
{
    pQueue->QUEUE_uiFront = 0;
    pQueue->QUEUE_uiRear  = 0;
    pQueue->QUEUE_uiSize  = uiSize;
    pQueue->QUEUE_uiCount = 0;
}
/*********************************************************************************************************
** 函数名称: API_QueueIn
** 功能描述: 入队列
** 输　入  : pQueue            队列
**           pInPos            入队点
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueIn (PLW_QUEUE  pQueue, UINT  *pInPos)
{
    if (API_QueueIsFull(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount++;
        *pInPos = pQueue->QUEUE_uiRear;
        pQueue->QUEUE_uiRear = (pQueue->QUEUE_uiRear + 1) % pQueue->QUEUE_uiSize;
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: API_QueueUnIn
** 功能描述: 取消入队列
** 输　入  : pQueue            队列
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueUnIn (PLW_QUEUE  pQueue)
{
    if (API_QueueIsEmpty(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount--;
        pQueue->QUEUE_uiRear--;
        if (pQueue->QUEUE_uiRear >= pQueue->QUEUE_uiSize) {
            pQueue->QUEUE_uiRear =  pQueue->QUEUE_uiSize - 1;
        }
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: API_QueueOut
** 功能描述: 出队列
** 输　入  : pQueue            队列
**           pOutPos           出队点
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueOut (PLW_QUEUE  pQueue, UINT  *pOutPos)
{
    if (API_QueueIsEmpty(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount--;
        *pOutPos = pQueue->QUEUE_uiFront;
        pQueue->QUEUE_uiFront = (pQueue->QUEUE_uiFront + 1) % pQueue->QUEUE_uiSize;
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: API_QueueUnOut
** 功能描述: 取消出队列
** 输　入  : pQueue            队列
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueUnOut (PLW_QUEUE  pQueue)
{
    if (API_QueueIsFull(pQueue)) {
        return  (LW_FALSE);
    } else {
        pQueue->QUEUE_uiCount++;
        pQueue->QUEUE_uiFront--;
        if (pQueue->QUEUE_uiFront >= pQueue->QUEUE_uiSize) {
            pQueue->QUEUE_uiFront =  pQueue->QUEUE_uiSize - 1;
        }
        return  (LW_TRUE);
    }
}
/*********************************************************************************************************
** 函数名称: API_QueueIsFull
** 功能描述: 判断队列是否满
** 输　入  : pQueue            队列
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueIsFull (PLW_QUEUE  pQueue)
{
    if (pQueue->QUEUE_uiCount >= pQueue->QUEUE_uiSize)
        return  (LW_TRUE);
    else
        return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_QueueIsEmpty
** 功能描述: 判断队列是否空
** 输　入  : pQueue            队列
** 输　出  : TRUE OR FALSE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
BOOL  API_QueueIsEmpty (PLW_QUEUE  pQueue)
{
    if (pQueue->QUEUE_uiCount == 0)
        return  (LW_TRUE);
    else
        return  (LW_FALSE);
}
/*********************************************************************************************************
** 函数名称: API_QueueCount
** 功能描述: 获得队列元素个数
** 输　入  : pQueue            队列
** 输　出  : 元素个数
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT  API_QueueCount (PLW_QUEUE  pQueue)
{
    return  (pQueue->QUEUE_uiCount);
}
/*********************************************************************************************************
** 函数名称: API_QueueFreeSpace
** 功能描述: 获得队列空闲空间大小
** 输　入  : pQueue            队列
** 输　出  : 空闲空间大小
** 全局变量:
** 调用模块:
*********************************************************************************************************/
UINT  API_QueueFreeSpace (PLW_QUEUE  pQueue)
{
    return  (pQueue->QUEUE_uiSize - pQueue->QUEUE_uiCount);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
