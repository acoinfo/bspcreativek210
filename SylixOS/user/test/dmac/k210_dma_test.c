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
** 文   件   名: k210_dma_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 25 日
**
** 描        述: K210 处理器 DMA 驱动测试程序
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "pthread.h"
#include "stdio.h"
#include "stdlib.h"
#include "driver/dmac/k210_dma.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define __DMA_TEST_DMA_CH_NUM             (2)                           /*  DMA 测试通道号              */
#define __DMA_TEST_CHANNEL_PRIO           (0)                           /*  DMA 测试通道的优先级        */
#define __DMA_TEST_SYNC_DMA_BUF_SIZE      (1024)                        /*  DMA 传输数据大小            */
#define __DMA_TEST_BUF_ALIGN              (8)                           /*  DMA 缓冲区对齐              */
/*********************************************************************************************************
** 函数名称: __dmaTestDone
** 功能描述: DMA 测试完成回调函数
** 输　入  : uiChannel     DMA 通道号
**           pvArg         参数
** 输　出  : NONE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  __dmaTestDone (UINT  uiChannel, PVOID  pvArg)
{
    API_SemaphoreBPost((LW_HANDLE)pvArg);
}
/*********************************************************************************************************
** 函数名称: __dmaTest
** 功能描述: DMA 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: pthread_test
** 功能描述: DMA 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
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
** 函数名称: dmaTestStart
** 功能描述: DMA 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
