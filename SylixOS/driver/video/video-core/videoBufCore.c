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
** ��   ��   ��: videoBufCore.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 28 ��
**
** ��        ��: video buffer library �����������ṩ������й���ӿ�
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include <string.h>
#include "video.h"
#include "video_debug.h"

#include "videoBufCore.h"
#include "videoQueueLib.h"
#include "videoChannel.h"
#include "videoDevice.h"
/*********************************************************************************************************
  �����ӿ�ʵ��
*********************************************************************************************************/

INT  API_VideoBufQueueInit (PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiFrameSize, UINT  uiPlaneNum)
{
    UINT  pos;

    if (!pVideoBuf || (uiPlaneNum > VIDEO_BUFFER_MAX_PLANE_NR || uiPlaneNum <= 0)) {
        printk("API_VideoBufQueueInit(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (!pVideoBuf->VBUF_bIsInit) {

        pVideoBuf->VBUF_uiPlaneNum   = uiPlaneNum;
        pVideoBuf->VBUF_uiFrameSize  = uiFrameSize;
        pVideoBuf->VBUF_uiCapPlaneId = 0;
        pVideoBuf->VBUF_ulMemAddr    = 0;

        API_QueueInit(&pVideoBuf->VBUF_planesQueue, uiPlaneNum);

        if (API_SpinInit(&pVideoBuf->VBUF_slBufLock) != ERROR_NONE) {
            printk("API_VideoBufQueueInit(): init spin lock failed.\n");
            return  (PX_ERROR);
        }

        pVideoBuf->VBUF_uiRecvFrameCount = 0;

        pVideoBuf->VBUF_planesArray = (PLW_VBUF_PLANE)__SHEAP_ALLOC((sizeof(LW_VBUF_PLANE) * uiPlaneNum));
        if (pVideoBuf->VBUF_planesArray == LW_NULL) {
            printk("API_VideoBufQueueInit(): alloc plane array failed.\n");
            return  (PX_ERROR);
        }

        for (INT index = 0; index < uiPlaneNum; ++index) {
            pVideoBuf->VBUF_planesArray[index].PLANE_state   = VB_BUF_STATE_IDLE;
            pVideoBuf->VBUF_planesArray[index].PLANE_uiIndex = index;

            API_QueueIn(&pVideoBuf->VBUF_planesQueue, &pos);         /* ����plane��������ж�����    */
            //video_debug("IN: pos = %d\r\n", pos);
        }
        API_QueueOut(&pVideoBuf->VBUF_planesQueue, &pos);            /* ��0ȡ����ʼ�ɼ�ʱ����1�ſ�ʼ */
        //video_debug("OUT: pos = %d\r\n", pos);

        pVideoBuf->VBUF_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);
}

INT  API_VideoBufMemPrep (PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiMemType, addr_t *pMemAddr)
{
    UINT  uiMemSize = 0;

    if (!pVideoBuf) {
        printk("API_VideoBufMemPrep(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    uiMemSize = pVideoBuf->VBUF_uiPlaneNum * pVideoBuf->VBUF_uiFrameSize;

    if (uiMemSize <= 0) {
        printk("API_VideoBufMemPrep(): invalid video buf_queue may not been init.\n");
        return  (PX_ERROR);
    }

    return  (API_VideoBufMemAlloc(pVideoBuf, uiMemType, pMemAddr, uiMemSize));
}

INT  API_VideoBufMemAlloc (PLW_VIDEO_BUFFER  pVideoBuf, UINT  uiMemType, addr_t *pMemAddr, UINT  uiMemSize)
{
    addr_t  ulMemAddr = 0;

    if (!pVideoBuf) {
        printk("API_VideoBufMemAlloc(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoBuf->VBUF_bIsInit) {

        if (!(uiMemType & VB_USERPTR)) {                                /* ���û������ڴ棬������������ */
            ulMemAddr = (addr_t)__SHEAP_ALLOC_ALIGN(uiMemSize, VIDOE_BUFFER_ALING_SIZE);
            *pMemAddr = ulMemAddr;
            video_debug("ulMemAddr = 0x%x, uiMemSize = %d\r\n", ulMemAddr, uiMemSize);
        }

        API_SpinLock(&pVideoBuf->VBUF_slBufLock);
        pVideoBuf->VBUF_uiMemType = uiMemType;
        pVideoBuf->VBUF_uiMemSize = uiMemSize;
        pVideoBuf->VBUF_ulMemAddr = ulMemAddr;
        API_SpinUnlock(&pVideoBuf->VBUF_slBufLock);
        video_debug("uiMemSize = %d, MemAddr = 0x%x\r\n", uiMemSize, *pMemAddr);

    } else {
        printk("API_VideoBufMemAlloc(): video buffer queue not init ahead.\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}

INT  API_VideoBufMemFree (PLW_VIDEO_BUFFER  pVideoBuf)
{
    if (!pVideoBuf) {
        printk("API_VideoBufMemAlloc(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoBuf->VBUF_ulMemAddr) {
        __SHEAP_FREE((PVOID)pVideoBuf->VBUF_ulMemAddr);
        pVideoBuf->VBUF_ulMemAddr = 0;
    }

    return  (ERROR_NONE);
}

INT  API_VideoBufQueueRelease (PLW_VIDEO_BUFFER  pVideoBuf)
{
    if (!pVideoBuf) {
        printk("API_VideoBufQueueInit(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    API_QueueFreeSpace(&pVideoBuf->VBUF_planesQueue);

    if (pVideoBuf->VBUF_planesArray) {
        __SHEAP_FREE(pVideoBuf->VBUF_planesArray);
    }

    pVideoBuf->VBUF_bIsInit = LW_FALSE;

    return  (ERROR_NONE);
}

INT  API_VideoBufNextPlaneGet (PLW_VIDEO_BUFFER  pVideoBuf, addr_t *pMemAddr , UINT *pPlaneID)
{
    UINT             uiImagePlaneId = 0;
    PLW_VIDEO_DEVICE pVideoDevice   = pVideoBuf->VBUF_pVideoChannel->VCHAN_pVideoDevice;

    struct video_frame_msg  knotify;

    if (!pVideoBuf) {
        printk("API_VideoBufNextPlaneGet(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoBuf->VBUF_bIsInit) {
                                                                        /* ����ȡ��һ�������һ��       */
        API_SpinLock(&pVideoBuf->VBUF_slBufLock);
        uiImagePlaneId = pVideoBuf->VBUF_uiCapPlaneId;
        pVideoBuf->VBUF_planesArray[uiImagePlaneId].PLANE_state = VB_BUF_STATE_READY;
        API_QueueOut(&pVideoBuf->VBUF_planesQueue, &pVideoBuf->VBUF_uiCapPlaneId);
                                                                        /* ������ø��ǵķ�ʽ�����IDLE */
        uiImagePlaneId = pVideoBuf->VBUF_uiCapPlaneId;
        pVideoBuf->VBUF_planesArray[uiImagePlaneId].PLANE_state = VB_BUF_STATE_PREPARING;
        *pPlaneID = pVideoBuf->VBUF_uiCapPlaneId;
        *pMemAddr = pVideoBuf->VBUF_ulMemAddr + (uiImagePlaneId * pVideoBuf->VBUF_uiFrameSize);
                                                                        /* ���һ��                     */
        API_QueueIn(&pVideoBuf->VBUF_planesQueue, &uiImagePlaneId);

        API_SpinUnlock(&pVideoBuf->VBUF_slBufLock);

                                                                        /* Ĭ����һ֡�Ѳɼ���ɷ��ͻ��� */
        knotify.channel  = pVideoBuf->VBUF_pVideoChannel->VCHAN_uiIndex;
        knotify.plane_id = (uiImagePlaneId == 0) ? (pVideoBuf->VBUF_uiPlaneNum  - 1) : --uiImagePlaneId;
        API_MsgQueueSend2(pVideoDevice->VIDEO_hlMsgQueue,
                          (void *)&knotify,
                          (u_long)sizeof(knotify),
                          LW_OPTION_WAIT_INFINITE);
        SEL_WAKE_UP_ALL(&pVideoDevice->VIDEO_waitList, SELREAD);
    }

    return  (ERROR_NONE);
}

INT  API_VideoBufImageFrameGet (PLW_VIDEO_BUFFER  pVideoBuf, addr_t *pMemAddr , UINT *pPlaneID)
{
    UINT    uiImagePlaneId = 0;

    if (!pVideoBuf || !pMemAddr || !pPlaneID) {
        printk("API_VideoBufImageFrameGet(): invalid parameters.\n");
        return  (PX_ERROR);
    }

    if (pVideoBuf->VBUF_bIsInit) {
        uiImagePlaneId = pVideoBuf->VBUF_uiCapPlaneId;
        if (uiImagePlaneId == 0) {                                      /*  �ҵ���ǰ�ɼ�֡��ǰһ֡      */
            uiImagePlaneId = pVideoBuf->VBUF_uiPlaneNum - 1;
        } else {
            --uiImagePlaneId;
        }
                                                                        /*  ������Ҫ����֡���ڴ��ַ    */
        *pPlaneID = uiImagePlaneId;
        *pMemAddr = pVideoBuf->VBUF_ulMemAddr + (uiImagePlaneId * pVideoBuf->VBUF_uiFrameSize);

        if (pVideoBuf->VBUF_planesArray[uiImagePlaneId].PLANE_state != VB_BUF_STATE_READY) {
            return  (PX_ERROR);
        }
                                                                        /*  ���Ϊ���б�ʾ��֡�ѱ��ȡ  */
        pVideoBuf->VBUF_planesArray[uiImagePlaneId].PLANE_state = VB_BUF_STATE_IDLE;
    }

    return  (ERROR_NONE);
}

/*********************************************************************************************************
  END
*********************************************************************************************************/
