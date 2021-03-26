/*********************************************************************************************************
**
**                                    �й������Դ��֯
**
**                                   Ƕ��ʽʵʱ����ϵͳ
**
**                                SylixOS(TM)  LW : long wing
**
**                               Copyright All Rights Reserved
**
**--------------�ļ���Ϣ--------------------------------------------------------------------------------
**
** ��   ��   ��: k210_i2c_test.c
**
** ��   ��   ��: Yu.KangZhi (�࿵־)
**
** �ļ���������: 2018 �� 11 �� 5 ��
**
** ��        ��: K210 ������ I2C �������Գ���
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#include "pthread.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/i2c/k210_i2c.h"
#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
 ���� k210_i2c-test.c �еĽӿ�
*********************************************************************************************************/
extern void i2c_master_init(void);
extern uint8_t i2c_write_reg(uint8_t reg, uint8_t *data_buf, size_t length);
extern uint8_t i2c_read_reg(uint8_t reg, uint8_t *data_buf, size_t length);
/*********************************************************************************************************
** ��������: pthread_test
** ��������: I2C �������� pthread �߳�
** �䡡��  : pvArg         �̲߳���
** �䡡��  : �̷߳���ֵ
** ȫ�ֱ���:
** ����ģ��:
*********************************************************************************************************/
static VOID  *pthread_test (VOID  *pvArg)
{
    INT     uiResult = 0;
    INT     iError   = 0;
    UINT8   index, data_buf[I2C_SLAVE_MAX_ADDR];

    PLW_I2C_DEVICE      pI2cDevice;
    i2c_slave_config_t  slave_config;

    /*
     * �ò��Գ���ʹ�� GPIO ģ�� i2c-master �������ݣ�����i2c����Ϊ���豸���������ݣ�
     * ���д������ݺͶ�����������ͬ����  i2c slave test ok ! �ڲ���ʱ��Ҫ������
     * �Ű��߽� IO_30 �� IO_32 �������� IO_31 �� IO_33 ������
     */

    printf(" \n test i2c slave mode \n");

    /*
     * ���� I2C ����ӳ��
     */
    fpioa_set_function(30, FUNC_I2C0_SCLK);                             /*  I2C0 �ӻ�ģʽ�µ� SCLK ��   */
    fpioa_set_function(31, FUNC_I2C0_SDA);                              /*  I2C0 �ӻ�ģʽ�µ� SDA ��    */
    fpioa_set_function(32, FUNC_GPIO6);                                 /*  GPIO6 ģ�� SCLK ��          */
    fpioa_set_function(33, FUNC_GPIO7);                                 /*  GPIO7 ģ�� SDA ��           */

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
** ��������: i2cTestStart
** ��������: I2C ����
** �䡡��  : NONE
** �䡡��  : ERROR_CODE
** ȫ�ֱ���:
** ����ģ��:
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
