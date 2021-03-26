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
** ��   ��   ��: k210_sha256.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 23 ��
**
** ��        ��: ��ȫɢ���㷨������(SHA256)����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "k210_sha256.h"
#include "driver/clock/k210_clock.h"
#include "driver/common.h"
#include "driver/fix_arch_def.h"

#include "KendryteWare/include/sha256.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __SHA256_NAME_FMT       "/dev/sha256"
#define __SHA256_SEM_FMT        "sha256_sem"
/*********************************************************************************************************
  ��ȫɢ���� SHA256 ���Ͷ���
*********************************************************************************************************/
typedef struct {

    addr_t                      SHA256_ulPhyAddrBase;                   /* �����ַ����ַ               */
    LW_OBJECT_HANDLE            SHA256_hCaculateSem;
    BOOL                        SHA256_bIsInit;

    LW_DEV_HDR                  SHA256_devHdr;
    LW_LIST_LINE_HEADER         SHA256_fdNodeHeader;
    time_t                      SHA256_time;

    sha256_context_t            SHA256_context;
    spinlock_t                  SHA256_mutexLock;
    size_t                      SHA256_stPerTransSize;                  /* һ��sha256������������ݴ�С */
    size_t                      SHA256_stTransInputCount;               /* ��Ƭ����ʱ��¼���������ݴ�С */

} __K210_SHA256_INSTANCE, *__PK210_SHA256_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_SHA256_INSTANCE   _G_k210Sha256Instance = {
        .SHA256_ulPhyAddrBase       = SHA256_BASE_ADDR,
        .SHA256_bIsInit             = LW_FALSE,
        .SHA256_stTransInputCount   = 0,
};

static INT _G_iK210Sha256DrvNum  = PX_ERROR;                            /*          �豸��              */
/*********************************************************************************************************
** ��������: __k210Sha256Open
** ��������: �� SHA256 �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __k210Sha256Open (PLW_DEV_HDR     pDev,
                               PCHAR           pcName,
                               INT             iFlags,
                               INT             iMode)
{
    PLW_FD_NODE                pFdNode;
    BOOL                       bIsNew;
    __PK210_SHA256_INSTANCE    pSha256Instance = container_of(pDev, __K210_SHA256_INSTANCE, SHA256_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pSha256Instance->SHA256_fdNodeHeader,
                                   (dev_t)pSha256Instance, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210Sha256Open(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pSha256Instance->SHA256_devHdr) == 1) {
            pSha256Instance->SHA256_hCaculateSem = API_SemaphoreBCreate(__SHA256_SEM_FMT, LW_FALSE,
                                                                        LW_OPTION_OBJECT_GLOBAL |
                                                                        LW_OPTION_WAIT_FIFO,
                                                                        LW_NULL);
            if (pSha256Instance->SHA256_hCaculateSem == LW_OBJECT_HANDLE_INVALID) {
                printk(KERN_ERR "__sifiveSpiInit(): failed to create spi_signal!\n");
                goto  __error_handle;
            }

            if (API_SpinInit(&pSha256Instance->SHA256_mutexLock) != ERROR_NONE) {
                printk(KERN_ERR "__sifiveSpiInit(): failed to create spi_signal!\n");
                goto  __error_handle;
            }

            /*
             * Hardware initialize
             */
            sysctl_clock_enable(SYSCTL_CLOCK_SHA);
            sysctl_reset(SYSCTL_RESET_SHA);

            pSha256Instance->SHA256_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pSha256Instance->SHA256_hCaculateSem) {
        API_SemaphoreBDelete(&pSha256Instance->SHA256_hCaculateSem);
    }

    if (pFdNode) {
        API_IosFdNodeDec(&pSha256Instance->SHA256_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pSha256Instance->SHA256_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210Sha256Close
** ��������: �ر� SHA256 �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210Sha256Close (PLW_FD_ENTRY   pFdEntry)
{
    __PK210_SHA256_INSTANCE  pSha256Instance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_SHA256_INSTANCE, SHA256_devHdr);
    PLW_FD_NODE              pFdNode         = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pSha256Instance->SHA256_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pSha256Instance->SHA256_devHdr) == 0) {

            /*
             * Release system source
             */
            if (pSha256Instance->SHA256_hCaculateSem) {
                API_SemaphoreBDelete(&pSha256Instance->SHA256_hCaculateSem);
            }

            pSha256Instance->SHA256_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210Sha256Lstat
** ��������: ��� SHA256 �豸״̬
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           pStat                 stat �ṹָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Sha256Lstat (PLW_DEV_HDR      pDev,
                               PCHAR            pcName,
                               struct stat     *pStat)
{
    __PK210_SHA256_INSTANCE  pSha256Instance = container_of(pDev, __K210_SHA256_INSTANCE, SHA256_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pSha256Instance;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pSha256Instance->SHA256_time;
        pStat->st_mtime   = pSha256Instance->SHA256_time;
        pStat->st_ctime   = pSha256Instance->SHA256_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210Sha256Ioctl
** ��������: ���� SHA256 �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210Sha256Ioctl (PLW_FD_ENTRY      pFdEntry,
                               INT               iCmd,
                               LONG              lArg)
{
    __PK210_SHA256_INSTANCE    pSha256Instance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                          __K210_SHA256_INSTANCE, SHA256_devHdr);
    struct stat               *pStat;
    sha256_msg_t              *pMsg;

    switch (iCmd) {

    case FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pSha256Instance;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pSha256Instance->SHA256_time;
            pStat->st_mtime   = pSha256Instance->SHA256_time;
            pStat->st_ctime   = pSha256Instance->SHA256_time;
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

        /*
         * TODO ���û�ȡƵ�ʡ�ռ�ñȡ���������ʵ��
         */
    case IOCTL_SHA256_HARD_CACULATE:

        pMsg  =  (sha256_msg_t *)lArg;

        if ((pMsg->input != LW_NULL && pMsg->output != LW_NULL && pMsg->input_len > 0) != LW_TRUE) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        API_SpinLock(&pSha256Instance->SHA256_mutexLock);
        sha256_hard_calculate(pMsg->input, pMsg->input_len, pMsg->output);
        API_SpinUnlock(&pSha256Instance->SHA256_mutexLock);
        break;

    case IOCTL_SHA256_CONTEXT_INIT:

        if (lArg <= 0) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        pSha256Instance->SHA256_stPerTransSize    = (size_t)lArg;
        pSha256Instance->SHA256_stTransInputCount = 0;
        API_SpinLock(&pSha256Instance->SHA256_mutexLock);

        lib_memset(&pSha256Instance->SHA256_context, 0, sizeof(pSha256Instance->SHA256_context));

        sha256_init(&pSha256Instance->SHA256_context, pSha256Instance->SHA256_stPerTransSize);
        API_SpinUnlock(&pSha256Instance->SHA256_mutexLock);
        return  (ERROR_NONE);

    case IOCTL_SHA256_SUBMIT_INPUT:

        pMsg  =  (sha256_msg_t *)lArg;

        if ((pMsg->input_len > 0 && pMsg->input != LW_NULL) != LW_TRUE) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if (pMsg->input_len > (pSha256Instance->SHA256_stPerTransSize -
                              pSha256Instance->SHA256_stTransInputCount)) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        API_SpinLock(&pSha256Instance->SHA256_mutexLock);
        pSha256Instance->SHA256_stTransInputCount += pMsg->input_len;
        sha256_update(&pSha256Instance->SHA256_context, pMsg->input, pMsg->input_len);
        API_SpinUnlock(&pSha256Instance->SHA256_mutexLock);
        return  (ERROR_NONE);

    case IOCTL_SHA256_OBTAIN_OUTPUT:

        pMsg  =  (sha256_msg_t *)lArg;

        if ((pMsg->output != LW_NULL && pMsg->input_len >= 32) != LW_TRUE) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        if (pSha256Instance->SHA256_stPerTransSize != pSha256Instance->SHA256_stTransInputCount) {
            _ErrorHandle(EINVAL);
            return  (PX_ERROR);
        }

        API_SpinLock(&pSha256Instance->SHA256_mutexLock);
        sha256_final(&pSha256Instance->SHA256_context, (UINT8 *)pMsg->output);
        API_SpinUnlock(&pSha256Instance->SHA256_mutexLock);
        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** ��������: k210Sha256Drv
** ��������: ��װ SHA256 ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210Sha256Drv (VOID)
{
    struct file_operations    fileOper;

    if (_G_iK210Sha256DrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210Sha256Open;
    fileOper.fo_open   = __k210Sha256Open;
    fileOper.fo_close  = __k210Sha256Close;
    fileOper.fo_lstat  = __k210Sha256Lstat;
    fileOper.fo_ioctl  = __k210Sha256Ioctl;

    _G_iK210Sha256DrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210Sha256DrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210Sha256DrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210Sha256DrvNum, "k210 sha256 driver.");

    return  ((_G_iK210Sha256DrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: k210Sha256DevAdd
** ��������: ���� SHA256 �豸
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210Sha256DevAdd (VOID)
{
    __PK210_SHA256_INSTANCE    pSha256Instance;

    pSha256Instance = (__PK210_SHA256_INSTANCE)&_G_k210Sha256Instance;

    pSha256Instance->SHA256_time = time(LW_NULL);

    if (API_IosDevAddEx(&pSha256Instance->SHA256_devHdr,
                        __SHA256_NAME_FMT,
                        _G_iK210Sha256DrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210Sha256DevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
