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
** ��   ��   ��: k210_dma_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 25 ��
**
** ��        ��: K210 ������ DMA �������Գ���
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"
#include "driver/dmac/k210_dma.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __DMA_TEST_DMA_CH_NUM             (2)                           /*  DMA ����ͨ����              */
#define __DMA_TEST_CHANNEL_PRIO           (0)                           /*  DMA ����ͨ�������ȼ�        */
#define __DMA_TEST_SYNC_DMA_BUF_SIZE      (1024)                        /*  DMA �������ݴ�С            */
#define __DMA_TEST_BUF_ALIGN              (8)                           /*  DMA ����������              */
/*********************************************************************************************************
** ��������: __dmaTestDone
** ��������: DMA ������ɻص�����
** �䡡��  : uiChannel     DMA ͨ����
**           pvArg         ����
** �䡡��  : NONE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  __dmaTestDone (UINT  uiChannel, PVOID  pvArg)
{
    API_SemaphoreBPost((LW_HANDLE)pvArg);
}
/*********************************************************************************************************
** ��������: __dmaTest
** ��������: DMA ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __dmaTest (INT  iLen)
{
    INT                  i;
    INT                  uiError;
    LW_DMA_TRANSACTION   dmaTransAction;

    LW_HANDLE            hSignal;
    UCHAR               *pucDestBuffer = __SHEAP_ALLOC_ALIGN(iLen, __DMA_TEST_BUF_ALIGN);
    UCHAR               *pucSrcBuffer  = __SHEAP_ALLOC_ALIGN(iLen, __DMA_TEST_BUF_ALIGN);

    if (pucSrcBuffer == LW_NULL || pucDestBuffer == LW_NULL) {
        _PrintFormat("pucSrcBuffer:%p,  pucDestBuffer:%p\n", pucSrcBuffer, pucDestBuffer);
        return  (PX_ERROR);
    }

    hSignal = API_SemaphoreBCreate("dma_signal",
                                   LW_FALSE,
                                   LW_OPTION_OBJECT_GLOBAL, LW_NULL);
    if (hSignal == LW_OBJECT_HANDLE_INVALID) {
        _PrintFormat("__dmaTest(): failed to create spi_signal!\n");
        return  (PX_ERROR);
    }

    lib_memset(&dmaTransAction, 0, sizeof(dmaTransAction));
    lib_memset(pucDestBuffer,   0, iLen);

    for (i = 0; i < iLen; i++) {                                        /*  initialize pucSrcBuffer     */
        pucSrcBuffer[i] = i;
    }

    dmaTransAction.DMAT_pucSrcAddress  = pucSrcBuffer;
    dmaTransAction.DMAT_iSrcAddrCtl    = LW_DMA_ADDR_INC;

    dmaTransAction.DMAT_pucDestAddress = pucDestBuffer;
    dmaTransAction.DMAT_iDestAddrCtl   = LW_DMA_ADDR_INC;

    dmaTransAction.DMAT_iTransMode     = DMA_MAKE_TARNS_MODE(DMAC_MSIZE_1,
                                                             DMAC_TRANS_WIDTH_8,
                                                             __DMA_TEST_CHANNEL_PRIO);

    dmaTransAction.DMAT_ulOption       = DMA_MAKE_OPTION(1);
    dmaTransAction.DMAT_stDataBytes    = iLen;

    dmaTransAction.DMAT_pfuncCallback  = __dmaTestDone;
    dmaTransAction.DMAT_pvArg          = (PVOID *)hSignal;

    uiError = API_DmaJobAdd(__DMA_TEST_DMA_CH_NUM, &dmaTransAction);
    if (uiError == PX_ERROR) {
        _PrintFormat("==> [ERROR]: API_DmaJobAdd() \r\n");
    }

    API_SemaphoreBPend(hSignal, LW_OPTION_WAIT_INFINITE);

    for (i = 0; i < iLen; i++) {                                        /*  whether transmit success    */
        if (pucDestBuffer[i] != pucSrcBuffer[i]) {
            break;
        }
    }

    API_SemaphoreBDelete(&hSignal);
    __SHEAP_FREE(pucDestBuffer);
    __SHEAP_FREE(pucSrcBuffer);

    return  (i);
}
/*********************************************************************************************************
** ��������: pthread_test
** ��������: DMA ���� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT  i;
    INT  iTestCount = 5;

    while (1) {

        i = __dmaTest(__DMA_TEST_SYNC_DMA_BUF_SIZE);

        if (i == __DMA_TEST_SYNC_DMA_BUF_SIZE) {
            printf("DMA async mode test success!\n");
        } else {
            printf("DMA async mode test failed! i=%d\n", i);
        }

        sleep(2);

        if (--iTestCount == 0) {
            break;
        }
    }

    printf("DMA test finished.\n");

    return  (NULL);
}
/*********************************************************************************************************
** ��������: dmaTestStart
** ��������: DMA ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  dmaTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
