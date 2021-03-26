/*********************************************************************************************************
**
**                                    中国软件开源组织
**
**                                   嵌入式实时操作系统
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------文件信息--------------------------------------------------------------------------------
**
** 文   件   名: k210_sha256_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 2 日
**
** 描        述: K210 处理器 SHA256 驱动测试程序
*********************************************************************************************************/
#define  __SYLIXOS_STDIO
#define  __SYLIXOS_KERNEL
#include "SylixOS.h"
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/sha256/k210_sha256.h"
#include "KendryteWare/include/sha256.h"
/*********************************************************************************************************
 全局变量定义
*********************************************************************************************************/
uint8_t hash[SHA256_HASH_LEN];

uint8_t compare1[] = {0xba, 0x78, 0x16, 0xbf, 0x8f, 0x01, 0xcf, 0xea, 0x41, 0x41, 0x40, 0xde, 0x5d, 0xae, 0x22, 0x23,
                      0xb0, 0x03, 0x61, 0xa3, 0x96, 0x17, 0x7a, 0x9c, 0xb4, 0x10, 0xff, 0x61, 0xf2, 0x00, 0x15, 0xad};

uint8_t compare2[] = {0x58, 0xbe, 0xb6, 0xbb, 0x9b, 0x80, 0xb2, 0x12, 0xc3, 0xdb, 0xc1, 0xc1, 0x02, 0x0c, 0x69, 0x6f,
                      0xbf, 0xa3, 0xaa, 0xd8, 0xe8, 0xa4, 0xef, 0x4d, 0x38, 0x5e, 0x9b, 0x07, 0x32, 0xfc, 0x5d, 0x98};

uint8_t compare3[] = {0x6e, 0x65, 0xda, 0xd1, 0x7a, 0xa2, 0x3e, 0x72, 0x79, 0x8d, 0x50, 0x33, 0xa1, 0xae, 0xe5, 0x9e,
                      0xe3, 0x35, 0x2d, 0x3c, 0x49, 0x6c, 0x18, 0xfb, 0x71, 0xe3, 0xa5, 0x37, 0x22, 0x11, 0xfc, 0x6c};

uint8_t compare4[] = {0xcd, 0xc7, 0x6e, 0x5c, 0x99, 0x14, 0xfb, 0x92, 0x81, 0xa1, 0xc7, 0xe2, 0x84, 0xd7, 0x3e, 0x67,
                      0xf1, 0x80, 0x9a, 0x48, 0xa4, 0x97, 0x20, 0x0e, 0x04, 0x6d, 0x39, 0xcc, 0xc7, 0x11, 0x2c, 0xd0};

uint8_t data_buf[1000*1000];
/*********************************************************************************************************
** 函数名称: pthread_test2
** 功能描述: SHA256 驱动测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test2 (VOID  *pvArg)
{
    INT           iFd;
    uint32_t      i;
    sha256_msg_t  myMsg;
    uint8_t       total_check_tag = 0;

    iFd = open("/dev/sha256", O_RDWR, 0666);
    if (iFd < 0) {
        printf("failed to open /dev/sha256\n");
        return  (LW_NULL);
    }
    printf("\n");

    sha256_hard_calculate((uint8_t *)"abc", 3, hash);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare1[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    printf("------total_check_tag: %d --------------\n", total_check_tag);

    myMsg.input     = (uint8_t *)"abc";
    myMsg.input_len = 3;
    myMsg.output    = hash;
    ioctl(iFd, IOCTL_SHA256_HARD_CACULATE, &myMsg);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare1[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    printf("------total_check_tag: %d --------------\n", total_check_tag);

    memset(data_buf, 'a', sizeof(data_buf));

    sha256_hard_calculate(data_buf, sizeof(data_buf), hash);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare4[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    printf("------total_check_tag: %d --------------\n", total_check_tag);

    myMsg.input     = data_buf;
    myMsg.input_len = sizeof(data_buf);
    myMsg.output    = hash;
    ioctl(iFd, IOCTL_SHA256_HARD_CACULATE, &myMsg);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare4[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    printf("------total_check_tag: %d --------------\n", total_check_tag);

    memset(data_buf, 'a', sizeof(data_buf));

    sha256_context_t context;
    sha256_init(&context, sizeof(data_buf));
    sha256_update(&context, data_buf, 1111);
    sha256_update(&context, data_buf + 1111, sizeof(data_buf) - 1111);
    sha256_final(&context, hash);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare4[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    printf("------total_check_tag: %d --------------\n", total_check_tag);

    myMsg.trans_len  = sizeof(data_buf);
    ioctl(iFd, IOCTL_SHA256_CONTEXT_INIT, sizeof(data_buf));
    myMsg.input      = data_buf;
    myMsg.input_len  = 1111;
    ioctl(iFd, IOCTL_SHA256_SUBMIT_INPUT, &myMsg);
    myMsg.input      = data_buf + 1111;
    myMsg.input_len  = sizeof(data_buf) - 1111;
    ioctl(iFd, IOCTL_SHA256_SUBMIT_INPUT, &myMsg);
    myMsg.output     = hash;
    myMsg.output_len = sizeof(data_buf);
    ioctl(iFd, IOCTL_SHA256_OBTAIN_OUTPUT, &myMsg);
    for (i = 0; i < SHA256_HASH_LEN;)
    {
        if (hash[i] != compare4[i])
            total_check_tag = 1;
        printf("%02x", hash[i++]);
        if (!(i % 4))
            printf(" ");
    }
    printf("\n");

    if (total_check_tag == 1)
        printf("\n===> SHA256_TEST _TEST_FAIL \n");
    else
        printf("\n===> SHA256_TEST _TEST_PASS \n");

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: sha256TestStart
** 功能描述: SHA256 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  sha256TestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test2, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/


