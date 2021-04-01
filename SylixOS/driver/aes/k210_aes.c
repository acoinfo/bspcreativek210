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
** ��   ��   ��: k210_aes.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 10 �� 23 ��
**
** ��        ��: �߼����ܼ�����(AES)����
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <string.h>
#include "driver/fix_arch_def.h"
#include "driver/clock/k210_clock.h"
#include "driver/aes/k210_aes.h"

#include "KendryteWare/include/common.h"
#include "KendryteWare/include/aes.h"
/*********************************************************************************************************
  ����
*********************************************************************************************************/
#define __AES_INSTANCE_NAME_FMT     "/dev/aes"
#define __AES_INSTANCE_SEM_FMT      "aes_sem"
/*********************************************************************************************************
  ioctl �û����� aes_work_t �Ϸ��Լ���
*********************************************************************************************************/
#define __AES_PARAM_CHECK(aes_work, iRet)                               \
do {                                                                    \
        if (aes_work->encrypt_mode == AES_ECB) {                        \
            if (pTransaction->input_key  == LW_NULL) iRet = PX_ERROR;   \
        } else if (aes_work->encrypt_mode == AES_CBC) {                 \
            if (pTransaction->input_key  == LW_NULL) iRet = PX_ERROR;   \
            if (pTransaction->cbs_gcm_iv == LW_NULL) iRet = PX_ERROR;   \
        } else if (aes_work->encrypt_mode == AES_GCM) {                 \
            if (pTransaction->input_key  == LW_NULL) iRet = PX_ERROR;   \
            if (pTransaction->gcm_tag    == LW_NULL) iRet = PX_ERROR;   \
            if (pTransaction->cbs_gcm_iv == LW_NULL) iRet = PX_ERROR;   \
        }                                                               \
        if (pTransaction->input_data == LW_NULL) iRet = PX_ERROR;       \
        if (pTransaction->input_len <= 0) iRet = PX_ERROR;              \
        if (pTransaction->output_data == LW_NULL) iRet = PX_ERROR;      \
} while(0);
/*********************************************************************************************************
  AES �����ӿڻ�����ã����ƺ�
*********************************************************************************************************/
#define __AES_RAW_API_LOCK(objHandle)      API_SemaphoreBPend(objHandle, LW_OPTION_WAIT_INFINITE)
#define __AES_RAW_API_UNLOCK(objHandle)    API_SemaphoreBPost(objHandle)
/*********************************************************************************************************
  ��ȫɢ���� SHA256 ���Ͷ���
*********************************************************************************************************/
typedef struct {
    addr_t                  AES_ulPhyAddrBase;                          /*      �����ַ����ַ          */
    LW_OBJECT_HANDLE        AES_hMutexSem;
    BOOL                    AES_bIsInit;
    LW_DEV_HDR              AES_devHdr;
    LW_LIST_LINE_HEADER     AES_fdNodeHeader;
    time_t                  AES_time;
} __K210_AES_INSTANCE,  *__PK210_AES_INSTANCE;
/*********************************************************************************************************
  ȫ�ֱ�������
*********************************************************************************************************/
static __K210_AES_INSTANCE   _G_k210AesInstance = {                     /*       AES�豸ʵ��            */
        .AES_ulPhyAddrBase  =  AES_BASE_ADDR,
        .AES_bIsInit        =  LW_FALSE,
};

static INT  _G_iK210AesDrvNum  =  PX_ERROR;                             /*       �豸������             */
/*********************************************************************************************************
** ��������: __k210AesOpen
** ��������: �� AES �豸
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           iFlags                ��־
**           iMode                 ģʽ
** �䡡��  : �ļ��ڵ�
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static LONG  __k210AesOpen (PLW_DEV_HDR    pDev,
                            PCHAR          pcName,
                            INT            iFlags,
                            INT            iMode)
{
    PLW_FD_NODE            pFdNode;
    BOOL                   bIsNew;
    __PK210_AES_INSTANCE   pAesInstance = container_of(pDev, __K210_AES_INSTANCE, AES_devHdr);

    if (pcName == LW_NULL) {
        _ErrorHandle(ERROR_IO_NO_DEVICE_NAME_IN_PATH);
        return  (PX_ERROR);

    } else {
        pFdNode = API_IosFdNodeAdd(&pAesInstance->AES_fdNodeHeader,
                                   (dev_t)pAesInstance, 0,
                                   iFlags, iMode, 0, 0, 0, LW_NULL, &bIsNew);
        if (pFdNode == LW_NULL) {
            printk(KERN_ERR "__k210AesOpen(): failed to add fd node!\n");
            return  (PX_ERROR);
        }

        if (LW_DEV_INC_USE_COUNT(&pAesInstance->AES_devHdr) == 1) {

            pAesInstance->AES_hMutexSem = API_SemaphoreBCreate(__AES_INSTANCE_SEM_FMT, LW_TRUE,
                                               LW_OPTION_OBJECT_GLOBAL | LW_OPTION_WAIT_FIFO, LW_NULL);
            if (pAesInstance->AES_hMutexSem == LW_OBJECT_HANDLE_INVALID) {
                printk(KERN_ERR "__k210AesOpen(): failed to create spi_signal!\n");
                goto  __error_handle;
            }

            /*
             * Hardware initialize
             */
            sysctl_clock_enable(SYSCTL_CLOCK_AES);
            sysctl_reset(SYSCTL_RESET_AES);

            pAesInstance->AES_bIsInit = LW_TRUE;
        }

        return  ((LONG)pFdNode);
    }

__error_handle:

    if (pAesInstance->AES_hMutexSem) {
        API_SemaphoreBDelete(&pAesInstance->AES_hMutexSem);
    }

    if (pFdNode) {
        API_IosFdNodeDec(&pAesInstance->AES_fdNodeHeader, pFdNode, LW_NULL);
        pFdNode = LW_NULL;
    }

    LW_DEV_DEC_USE_COUNT(&pAesInstance->AES_devHdr);

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210AesClose
** ��������: �ر� AES �豸
** �䡡��  : pFdEntry              �ļ��ṹ
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT __k210AesClose (PLW_FD_ENTRY  pFdEntry)
{
    PLW_FD_NODE           pFdNode      = (PLW_FD_NODE)pFdEntry->FDENTRY_pfdnode;
    __PK210_AES_INSTANCE  pAesInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                      __K210_AES_INSTANCE, AES_devHdr);

    if (pFdEntry && pFdNode) {
        API_IosFdNodeDec(&pAesInstance->AES_fdNodeHeader, pFdNode, LW_NULL);

        if (LW_DEV_DEC_USE_COUNT(&pAesInstance->AES_devHdr) == 0) {

            /*
             * Release system source
             */
            if (pAesInstance->AES_hMutexSem) {
                API_SemaphoreBDelete(&pAesInstance->AES_hMutexSem);
            }

            pAesInstance->AES_bIsInit = LW_FALSE;
        }

        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210AesLstat
** ��������: ��� AES �豸״̬
** �䡡��  : pDev                  �豸
**           pcName                �豸����
**           pStat                 stat �ṹָ��
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210AesLstat (PLW_DEV_HDR     pDev,
                            PCHAR           pcName,
                            struct stat    *pStat)
{
    __PK210_AES_INSTANCE  pAesInstance = container_of(pDev, __K210_AES_INSTANCE, AES_devHdr);

    if (pStat) {
        pStat->st_dev     = (dev_t)pAesInstance;
        pStat->st_ino     = (ino_t)0;
        pStat->st_mode    = (S_IFCHR | S_IRUSR | S_IRGRP | S_IROTH);
        pStat->st_nlink   = 1;
        pStat->st_uid     = 0;
        pStat->st_gid     = 0;
        pStat->st_rdev    = 0;
        pStat->st_size    = 0;
        pStat->st_blksize = 0;
        pStat->st_blocks  = 0;
        pStat->st_atime   = pAesInstance->AES_time;
        pStat->st_mtime   = pAesInstance->AES_time;
        pStat->st_ctime   = pAesInstance->AES_time;
        return  (ERROR_NONE);
    }

    return  (PX_ERROR);
}
/*********************************************************************************************************
** ��������: __k210AesIoctl
** ��������: ���� AES �豸
** �䡡��  : pFdEntry              �ļ��ṹ
**           iCmd                  ����
**           lArg                  ����
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static INT  __k210AesIoctl (PLW_FD_ENTRY    pFdEntry,
                            INT             iCmd,
                            LONG            lArg)
{
    struct stat            *pStat;
    INT                     iRet         = ERROR_NONE;
    aes_work_t             *pTransaction = (aes_work_t *)lArg;
    __PK210_AES_INSTANCE    pAesInstance = container_of(pFdEntry->FDENTRY_pdevhdrHdr,
                                                        __K210_AES_INSTANCE, AES_devHdr);
    cbc_context_t           context;                                    /*      CBC ��/����������       */
    gcm_context_t           gcm_context;                                /*      GCM ��/����������       */

    switch (iCmd) {

    case  FIOFSTATGET:
        pStat = (struct stat *)lArg;
        if (pStat) {
            pStat->st_dev     = (dev_t)pAesInstance;
            pStat->st_ino     = (ino_t)0;
            pStat->st_mode    = (S_IRUSR | S_IRGRP | S_IROTH);
            pStat->st_nlink   = 1;
            pStat->st_uid     = 0;
            pStat->st_gid     = 0;
            pStat->st_rdev    = 0;
            pStat->st_size    = 0;
            pStat->st_blksize = 0;
            pStat->st_blocks  = 0;
            pStat->st_atime   = pAesInstance->AES_time;
            pStat->st_mtime   = pAesInstance->AES_time;
            pStat->st_ctime   = pAesInstance->AES_time;
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

    case  AES_ECB128_HARD_DECRYPT:
        /*
         * ECB �ӽ��ܲ���:
         *
         * input_key        �ӽ�����Կ
         * input_data       ��������(����/����)
         * input_len        �������ݳ���
         * output_data      �������(����/����)
         */
        __AES_PARAM_CHECK(pTransaction, iRet);                          /* ���ü����������Ĳ���     */
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB128_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);                /* �����API�Ļ������,�������� */
        aes_ecb128_hard_decrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);              /* �����API�Ļ������,�������� */

        return  (ERROR_NONE);

    case  AES_ECB128_HARD_ENCRYPT:

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB128_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_ecb128_hard_encrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_ECB192_HARD_DECRYPT:

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB192_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_ecb192_hard_decrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_ECB192_HARD_ENCRYPT:

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB192_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_ecb192_hard_encrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_ECB256_HARD_DECRYPT:

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB256_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_ecb256_hard_decrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_ECB256_HARD_ENCRYPT:

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB256_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_ecb256_hard_encrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC128_HARD_DECRYPT:
        /*
         * CBC �ӽ��ܲ���:
         *
         * iv               ��ʼ������������128bit
         * input_key        ������ܻ������Կ�Ļ�����
         * input_data       ��������(����/����)
         * input_len        �������ݳ���
         * output_data      �������(����/����)
         */
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC128_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc128_hard_decrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC128_HARD_ENCRYPT:
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC128_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc128_hard_encrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC192_HARD_DECRYPT:
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC192_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc192_hard_decrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC192_HARD_ENCRYPT:
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC192_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc192_hard_encrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC256_HARD_DECRYPT:
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC256_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc256_hard_decrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_CBC256_HARD_ENCRYPT:
        context.input_key = pTransaction->input_key;
        context.iv        = pTransaction->cbs_gcm_iv;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_CBC256_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_cbc256_hard_encrypt(&context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM128_HARD_DECRYPT:
        /*
         * GCM �ӽ��ܲ���:
         *
         * iv               ��ʼ������������96bit
         * input_key        ������ܻ������Կ�Ļ�����
         * input_data       ��������(����/����)
         * input_len        �������ݳ���
         * output_data      �������(����/����)
         * gcm_aad          The buffer holding the Additional authenticated data or NULL.
         * gcm_aad_len      The length of the Additional authenticated data or 0L.
         */
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM128_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm128_hard_decrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM128_HARD_ENCRYPT:
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM128_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm128_hard_encrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM192_HARD_DECRYPT:
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM192_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm192_hard_decrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM192_HARD_ENCRYPT:
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM192_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm192_hard_encrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM256_HARD_DECRYPT:
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM256_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm256_hard_decrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    case  AES_GCM256_HARD_ENCRYPT:
        gcm_context.input_key   = pTransaction->input_key;
        gcm_context.iv          = pTransaction->cbs_gcm_iv;
        gcm_context.gcm_aad     = pTransaction->gcm_aad;
        gcm_context.gcm_aad_len = pTransaction->gcm_aad_len;

        __AES_PARAM_CHECK(pTransaction, iRet);
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_GCM256_HARD_ENCRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);
        aes_gcm256_hard_encrypt(&gcm_context,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data,
                                pTransaction->gcm_tag);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);

        return  (ERROR_NONE);

    default:
        _ErrorHandle(ENOSYS);
        return  (PX_ERROR);
    }
}
/*********************************************************************************************************
** ��������: k210AesDrv
** ��������: ��װ AES ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210AesDrv (VOID)
{
    struct file_operations  fileOper;

    if (_G_iK210AesDrvNum >= 0) {
        return  (ERROR_NONE);
    }

    lib_memset(&fileOper, 0, sizeof(struct file_operations));

    fileOper.owner     = THIS_MODULE;
    fileOper.fo_create = __k210AesOpen;
    fileOper.fo_open   = __k210AesOpen;
    fileOper.fo_close  = __k210AesClose;
    fileOper.fo_lstat  = __k210AesLstat;
    fileOper.fo_ioctl  = __k210AesIoctl;

    _G_iK210AesDrvNum = iosDrvInstallEx2(&fileOper, LW_DRV_TYPE_NEW_1);

    DRIVER_LICENSE(_G_iK210AesDrvNum,     "Dual BSD/GPL->Ver 1.0");
    DRIVER_AUTHOR(_G_iK210AesDrvNum,      "Yu.KangZhi");
    DRIVER_DESCRIPTION(_G_iK210AesDrvNum, "k210 aes driver.");

    return  ((_G_iK210AesDrvNum > 0) ? (ERROR_NONE) : (PX_ERROR));
}
/*********************************************************************************************************
** ��������: k210AesDrv
** ��������: ���� AES �豸
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
INT  k210AesDevAdd (VOID)
{
    __PK210_AES_INSTANCE    pAesInstance;

    pAesInstance = (__PK210_AES_INSTANCE)&_G_k210AesInstance;

    pAesInstance->AES_time = time(LW_NULL);

    if (API_IosDevAddEx(&pAesInstance->AES_devHdr,
                        __AES_INSTANCE_NAME_FMT,
                        _G_iK210AesDrvNum,
                        DT_CHR) != ERROR_NONE) {
        printk(KERN_ERR "k210AesDevAdd(): can not add device : %s.\n", strerror(errno));
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
