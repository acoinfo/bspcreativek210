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
** 文   件   名: k210_aes_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 25 日
**
** 描        述: K210 处理器 AES 驱动测试程序
*********************************************************************************************************/
#include "pthread.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"

#include "driver/aes/k210_aes.h"
/*********************************************************************************************************
 全局变量定义
*********************************************************************************************************/
uint8_t aes_key[]  = {0xfe, 0xff, 0xe9, 0x92, 0x86, 0x65, 0x73, 0x1c, 0x00, 0x6a, 0x8f, 0x94, 0x67, 0x30, 0x83, 0x08};
size_t  key_len    = AES_128;

uint8_t aes_iv[]   = {0xca, 0xfe, 0xba, 0xbe, 0xfa, 0xce, 0xdb, 0xad, 0xde, 0xca, 0xf8, 0x88, 0x67, 0x30, 0x83, 0x08};
uint8_t iv_len     = 16;
uint8_t iv_gcm_len = 12;

uint8_t aes_aad[]  = {0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef, 0xfe, 0xed, 0xfa, 0xce, 0xde, 0xad, 0xbe, 0xef,
                     0xab, 0xad, 0xda, 0xd2};
uint8_t aad_len    = 20;

#define AES_TEST_DATA_LEN       (1024 + 4UL)
#define AES_TEST_PADDING_LEN    ((AES_TEST_DATA_LEN + 15) / 16 *16)

uint8_t aes_hard_in_data[AES_TEST_PADDING_LEN];
uint8_t aes_hard_out_data[AES_TEST_PADDING_LEN];
/*********************************************************************************************************
** 函数名称: pthread_test
** 功能描述: AES 驱动测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT             iFd;
    aes_work_t      aes_cbc_work;

    iFd = open("/dev/aes", O_RDWR, 0666);
    if (iFd < 0) {
        printf("failed to open /dev/aes\n");
        return  (LW_NULL);
    }

    for (int index = 0; index < (AES_TEST_DATA_LEN < 256 ? AES_TEST_DATA_LEN : 256); index++) {
        aes_hard_in_data[index] = 'a' + (index % 26);
    }

    aes_cbc_work.encrypt_mode = AES_CBC;                                /*      初始化加密/解密事务     */
    aes_cbc_work.cbs_gcm_iv   = aes_iv;
    aes_cbc_work.input_key    = aes_key;
    aes_cbc_work.input_data   = aes_hard_in_data;
    aes_cbc_work.output_data  = aes_hard_out_data;
    aes_cbc_work.input_len    = AES_TEST_PADDING_LEN;

    ioctl(iFd, AES_CBC128_HARD_ENCRYPT, &aes_cbc_work);

    printf("\ninput_string: \n");                                       /*      构造明文数据            */
    for (int index = 0; index < (AES_TEST_DATA_LEN < 256 ? AES_TEST_DATA_LEN : 256); index++) {
        printf("0x%x ", aes_hard_in_data[index]);
    }
    printf("\n");

    printf("\nstr_encrypted: \n");                                      /*      输出加密后的数据        */
    for (int index = 0; index < (AES_TEST_DATA_LEN < 256 ? AES_TEST_DATA_LEN : 256); index++) {
        printf("0x%x ", aes_hard_out_data[index]);
    }
    printf("\n");

    memcpy(aes_hard_in_data, aes_hard_out_data, AES_TEST_PADDING_LEN);
                                                                        /*      解密加密后的数据        */
    ioctl(iFd, AES_CBC128_HARD_DECRYPT, &aes_cbc_work);

    printf("\ndecrypt_out: \n");
    for (int index = 0; index < (AES_TEST_DATA_LEN < 256 ? AES_TEST_DATA_LEN : 256); index++) {
        printf("0x%x ", aes_hard_out_data[index]);                      /*      输出解密得到的明文      */
    }
    printf("\n");

    close(iFd);

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: aesTestStart
** 功能描述: AES 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  aesTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
