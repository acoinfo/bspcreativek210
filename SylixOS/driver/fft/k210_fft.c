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
** ��   ��   ��: k210_fft.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 22 ��
**
** ��        ��: FFT ���ٸ���Ҷ�任������ ����ͷ�ļ�
*********************************************************************************************************/
#define __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "config.h"
#include "SylixOS.h"
#include <string.h>
#include "k210_fft.h"
#include "driver/common.h"
#include "driver/clock/k210_clock.h"

#include "KendryteWare/include/fft.h"
#include "KendryteWare/include/dmac.h"
#include "KendryteWare/include/plic.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __FFT_CHANNEL_NAME_FMT    "/dev/fft"
/*********************************************************************************************************
  FFT ���Ͷ���
*********************************************************************************************************/
typedef struct {
    LW_DEV_HDR                  FFT_devHdr;
    LW_LIST_LINE_HEADER         FFT_fdNodeHeader;
    time_t                      FFT_time;
    ULONG                       FFT_ulIrqVector;                        /*  FFT �ж�����                */

    dmac_channel_number_t       FFT_DmaSendChannel;
    dmac_channel_number_t       FFT_DmaReceiveChannel;
    uintptr_t                   FFT_ulPhyBaseAddr;
    spinlock_t                  FFT_mutex;
    BOOL                        FFT_bIsInit;
} __K210_FFT_INSTANCE,  *__PK210_FFT_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_FFT_INSTANCE   _G_k210FftINSTANCE = {
    .FFT_ulPhyBaseAddr      =  FFT_BASE_ADDR,
    .FFT_DmaSendChannel     =  DMAC_CHANNEL4,
    .FFT_DmaReceiveChannel  =  DMAC_CHANNEL5,
    .FFT_bIsInit            =  LW_FALSE,
    .FFT_ulIrqVector        =  KENDRYTE_PLIC_VECTOR(IRQN_FFT_INTERRUPT),
};

static INT   _G_iK210FftDrvNum  =  PX_ERROR;
/*********************************************************************************************************
** ��������: __k210FftOpen
** ��������: �� Fft �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __k210FftOpen (PLW_DEV_HDR    pDev,
                            PCHAR          pcName,
                            INT            iFlags,
                            INT            iMode)
{
    __PK210_FFT_INSTANCE    pFftChannel = container_of(pDev, __K210_FFT_INSTANCE, FFT_devHdr);
    PLW_FD_NODE             pFdNode;
    BOOL                    bIsNew;
    UINT                    uiRet = 0;

    volatile fft_t 	*fft = (volatile fft_t*)pFftChannel->FFT_ulPhyBaseAddr;

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pFftChannel->FFT_fdNodeHeader,
                                   (dev_t)pFftChannel, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210FftOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pFftChannel->FFT_devHdr) == 1) {

            LW_SPIN_INIT(&pFftChannel->FFT_mutex);                      /*       ��ʼ��������           */

            /*
             * Hardware initialize
             */

            uiRet = sysctl_clock_enable(SYSCTL_CLOCK_FFT);
            if (uiRet != ERROR_NONE) {
                printk(KERN_ERR "__k210FftOpen(): failed to sysctl_clock_enable!\n");
                goto  __error_handle;
            }

            sysctl_reset(SYSCTL_RESET_FFT);
            fft->intr_clear.fft_done_clear = 1;
            fft->intr_mask.fft_done_mask   = 0;

            pFftChannel->FFT_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pFdNode) {
        API_IosFdNodeDec(&pFftChannel->FFT_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pFftChannel->FFT_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210FftClose
** ��������: �ر� Fft �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210FftClose (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_FFT_INSTANCE  pFftChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                     __K210_FFT_INSTANCE, FFT_devHdr);
    PLW_FD_NODE           pFdNode     = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {

        API_IosFdNodeDec(&pFftChannel->FFT_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pFftChannel->FFT_devHdr) == 0) {

            /*
             * TODO: release system resource
             */
            pFftChannel->FFT_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210FftLstat
** ��������: ��� Fft �豸״̬
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           pStat                 stat �ṹָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210FftLstat (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            struct stat    *pStat)
{
    __PK210_FFT_INSTANCE  pFftChannel = container_of(pDev, __K210_FFT_INSTANCE, FFT_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pFftChannel;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pFftChannel->FFT_time;
        pStat->st_mtime   = pFftChannel->FFT_time;
        pStat->st_ctime   = pFftChannel->FFT_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210FftIoctl
** ��������: ���� Fft �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210FftIoctl (PLW_FD_ENTRY    pFdEntry,
                            INT             iCmd,
                            LONG            lArg)
{
    __PK210_FFT_INSTANCE     pFftChannel = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                        __K210_FFT_INSTANCE, FFT_devHdr);
    struct stat             *pStat;
    fft_transaction_t       *pTransaction;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pFftChannel;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pFftChannel->FFT_time;
            pStat->st_mtime   = pFftChannel->FFT_time;
            pStat->st_ctime   = pFftChannel->FFT_time;
            return  (ERROR_NONE);
        }
        return  (PX_ERROR);

    case FIOSETFL:
        if ((int)lArg & O_NONBLOCK) {
            pFdEntry->FDENTRY_iFlag |= O_NONBLOCK;
        } else {
            pFdEntry->FDENTRY_iFlag &= ~O_NONBLOCK;
        }
        return  (ERROR_NONE);

    case START_FFT_TRANSACTION:
        /*
         * FFT_DmaSendChannel       ��������ʹ�õ�DMA ͨ����                  ����
         *
         * FFT_DmaReceiveChannel    ��������ʹ�õ�DMA ͨ����                  ����
         *
         * direction                FFT ���任������任                      ����
         * input                    ������������У���ʽΪRIRI.., ʵ��
         *
         *                          ���鲿�ľ��ȶ�Ϊ16bit��                   ����
         *
         * point_num                ����������ݵ�����ֻ��Ϊ512/256/128/64    ����
         * output                   �����������ʽΪRIRI.., ʵ����
         *
         *                          �鲿�ľ��ȶ�Ϊ16bit��                     ���
         */
        pTransaction = (fft_transaction_t *)lArg;

        API_SpinLock(&pFftChannel->FFT_mutex);

        fft_complex_uint16_dma(pFftChannel->FFT_DmaSendChannel,
                               pFftChannel->FFT_DmaReceiveChannel,
                               pTransaction->direction,
                               pTransaction->input,
                               pTransaction->point_num,
                               pTransaction->output);

        API_SpinUnlock(&pFftChannel->FFT_mutex);

        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: k210FFTDrv
** ��������: ��װ FFT ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210FFTDrv (VOID)
{
    struct file_operations  fileOper;

    if (_G_iK210FftDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210FftOpen;
    fileOper.fo_open   = __k210FftOpen;
    fileOper.fo_close  = __k210FftClose;
    fileOper.fo_lstat  = __k210FftLstat;
    fileOper.fo_ioctl  = __k210FftIoctl;

    _G_iK210FftDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210FftDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210FftDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210FftDrvNum, "k210 fft driver.");

    return  ((_G_iK210FftDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: k210FFTDevAdd
** ��������: ���� FFT �豸
** �䡡��  : uiChannel         ͨ����
**           pPinMux           �ܽŸ���
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210FFTDevAdd(VOID)
{
    __PK210_FFT_INSTANCE  pPwmChannel;

    pPwmChannel = (__PK210_FFT_INSTANCE)&_G_k210FftINSTANCE;

    pPwmChannel->FFT_time = time(LW_NULL);

    if (API_IosDevAddEx(&pPwmChannel->FFT_devHdr,
                        __FFT_CHANNEL_NAME_FMT,
                        _G_iK210FftDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210PwmDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
