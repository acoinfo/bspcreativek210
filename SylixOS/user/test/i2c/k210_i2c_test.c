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
** 文   件   名: k210_i2c_test.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 11 月 5 日
**
** 描        述: K210 处理器 I2C 驱动测试程序
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/i2c/k210_i2c.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
 声明 k210_i2c-test.c 中的接口
*********************************************************************************************************/
extern void i2c_master_init(void);
extern uint8_t i2c_write_reg(uint8_t reg, uint8_t *data_buf, size_t length);
extern uint8_t i2c_read_reg(uint8_t reg, uint8_t *data_buf, size_t length);
/*********************************************************************************************************
** 函数名称: pthread_test
** 功能描述: I2C 驱动测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static VOID  *pthread_test (VOID  *pvArg)
{
    INT     uiResult = 0;
    INT     iError   = 0;
    UINT8   index, data_buf[I2C_SLAVE_MAX_ADDR];

    PLW_I2C_DEVICE      pI2cDevice;
    i2c_slave_config_t  slave_config;

    /*
     * 该测试程序使用 GPIO 模拟 i2c-master 发送数据，并将i2c配置为从设备来接收数据，
     * 如果写入的数据和读出的数据相同，则  i2c slave test ok ! 在测试时需要用两根
     * 杜邦线将 IO_30 和 IO_32 相连，将 IO_31 和 IO_33 相连。
     */

    printf(" \n test i2c slave mode \n");

    /*
     * 配置 I2C 引脚映射
     */
    fpioa_set_function(30, FUNC_I2C0_SCLK);                             /*  I2C0 从机模式下的 SCLK 线   */
    fpioa_set_function(31, FUNC_I2C0_SDA);                              /*  I2C0 从机模式下的 SDA 线    */
    fpioa_set_function(32, FUNC_GPIO6);                                 /*  GPIO6 模拟 SCLK 线          */
    fpioa_set_function(33, FUNC_GPIO7);                                 /*  GPIO7 模拟 SDA 线           */

    /*
     * i2c_slave_init();
     */
    pI2cDevice = API_I2cDeviceCreate("/bus/i2c/0","/dev/i2c_0", 0x32, 0);
    if (pI2cDevice == LW_NULL) {
        printk(KERN_ERR "__At24cxxOpen(): failed to create i2c device\n");
        return  (LW_NULL);
    }

    /*
     * configure i2c controller as slaver
     */
    slave_config.slave_address = 0x32;
    slave_config.address_width = 7;
    iError = API_I2cDeviceCtl(pI2cDevice, CONFIG_I2C_AS_SLAVE, (LONG)&slave_config);
    if (iError == PX_ERROR) {
        printf("API_I2cDeviceCtl: failed to config i2c as slave. \r\n");
    }

    /*
     * GPIO-I2C master init
     */
    i2c_master_init();

    printf("write data\n");
    for (index = 0; index < I2C_SLAVE_MAX_ADDR; index++) {
        data_buf[index] = index + 30;
    }
    i2c_write_reg(0, &data_buf[0], 7);
    i2c_write_reg(7, &data_buf[7], 7);

    printf("read data\n");
    for (index = 0; index < I2C_SLAVE_MAX_ADDR; index++) {
        data_buf[index] = 0;
    }
    i2c_read_reg(0, &data_buf[0], 7);
    i2c_read_reg(7, &data_buf[7], 7);

    for (index = 0; index < I2C_SLAVE_MAX_ADDR; index++) {
        if (data_buf[index] != index + 30) {
            printf("write: %x , read: %x\n", index, data_buf[index]);
            uiResult++;
        }
    }

    if(uiResult) {
        printf("i2c slave test error\n");
        printf("TEST_FAIL !\n");
        return NULL;
    }

    printf("i2c slave test ok\n");
    printf("TEST_PASS !\n");

    API_I2cDeviceDelete(pI2cDevice);

    while(1);

    return  (NULL);
}
/*********************************************************************************************************
** 函数名称: i2cTestStart
** 功能描述: I2C 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  i2cSlaveTestStart (VOID)
{
    pthread_t   tid_key;
    INT         iError;

    iError  = pthread_create(&tid_key, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
