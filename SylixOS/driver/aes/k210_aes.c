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
** 文   件   名: k210_aes.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 23 日
**
** 描        述: 高级加密加速器(AES)驱动
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
  定义
*********************************************************************************************************/
#define __AES_INSTANCE_NAME_FMT     "/dev/aes"
#define __AES_INSTANCE_SEM_FMT      "aes_sem"
/*********************************************************************************************************
  ioctl 用户数据 aes_work_t 合法性检查宏
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
  AES 裸机库接口互斥调用，控制宏
*********************************************************************************************************/
#define __AES_RAW_API_LOCK(objHandle)      API_SemaphoreBPend(objHandle, LW_OPTION_WAIT_INFINITE)
#define __AES_RAW_API_UNLOCK(objHandle)    API_SemaphoreBPost(objHandle)
/*********************************************************************************************************
  安全散列器 SHA256 类型定义
*********************************************************************************************************/
typedef struct {
    addr_t                  AES_ulPhyAddrBase;                          /*      物理地址基地址          */
    LW_OBJECT_HANDLE        AES_hMutexSem;
    BOOL                    AES_bIsInit;
    LW_DEV_HDR              AES_devHdr;
    LW_LIST_LINE_HEADER     AES_fdNodeHeader;
    time_t                  AES_time;
} __K210_AES_INSTANCE,  *__PK210_AES_INSTANCE;
/*********************************************************************************************************
  全局变量定义
*********************************************************************************************************/
static __K210_AES_INSTANCE   _G_k210AesInstance = {                     /*       AES设备实例            */
        .AES_ulPhyAddrBase  =  AES_BASE_ADDR,
        .AES_bIsInit        =  LW_FALSE,
};

static INT  _G_iK210AesDrvNum  =  PX_ERROR;                             /*       设备驱动号             */
/*********************************************************************************************************
** 函数名称: __k210AesOpen
** 功能描述: 打开 AES 设备
** 输　入  : pDev                  设备
**           pcName                设备名字
**           iFlags                标志
**           iMode                 模式
** 输　出  : 文件节点
** 全局变量:
** 调用模块:
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
** 函数名称: __k210AesClose
** 功能描述: 关闭 AES 设备
** 输　入  : pFdEntry              文件结构
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __k210AesLstat
** 功能描述: 获得 AES 设备状态
** 输　入  : pDev                  设备
**           pcName                设备名字
**           pStat                 stat 结构指针
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: __k210AesIoctl
** 功能描述: 控制 AES 设备
** 输　入  : pFdEntry              文件结构
**           iCmd                  命令
**           lArg                  参数
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
    cbc_context_t           context;                                    /*      CBC 加/解密上下文       */
    gcm_context_t           gcm_context;                                /*      GCM 加/解密上下文       */

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
         * ECB 加解密参数:
         *
         * input_key        加解密秘钥
         * input_data       输入数据(明文/密文)
         * input_len        输入数据长度
         * output_data      输出数据(密文/明文)
         */
        __AES_PARAM_CHECK(pTransaction, iRet);                          /* 检查该加密事务必须的参数     */
        if (iRet != ERROR_NONE) {
            printk(KERN_ERR "AES_ECB128_HARD_DECRYPT invalid transaction.\n");
            return  (iRet);
        }

        __AES_RAW_API_LOCK(pAesInstance->AES_hMutexSem);                /* 裸机库API的互斥访问,加锁操作 */
        aes_ecb128_hard_decrypt(pTransaction->input_key,
                                pTransaction->input_data,
                                pTransaction->input_len,
                                pTransaction->output_data);
        __AES_RAW_API_UNLOCK(pAesInstance->AES_hMutexSem);              /* 裸机库API的互斥访问,解锁操作 */

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
         * CBC 加解密参数:
         *
         * iv               初始化向量，必须128bit
         * input_key        保存加密或解密密钥的缓冲区
         * input_data       输入数据(明文/密文)
         * input_len        输入数据长度
         * output_data      输出数据(密文/明文)
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
         * GCM 加解密参数:
         *
         * iv               初始化向量，必须96bit
         * input_key        保存加密或解密密钥的缓冲区
         * input_data       输入数据(明文/密文)
         * input_len        输入数据长度
         * output_data      输出数据(密文/明文)
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
** 函数名称: k210AesDrv
** 功能描述: 安装 AES 驱动
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
** 函数名称: k210AesDrv
** 功能描述: 创建 AES 设备
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
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
