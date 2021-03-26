#define  __SYLIXOS_KERNEL                                               /*   因为使用到了内核接口       */
#include "unistd.h"
#include "stdlib.h"
#include "driver/i2c/k210_i2c.h"
#include "KendryteWare/include/gpio.h"
#include "KendryteWare/include/gpio_common.h"

#define SLAVE_ADDRESS       (0x32)
#define DELAY_TIME          (10)
#define VIRT_MASTER_SCL     (6)     //FUNC_GPIO6
#define VIRT_MASTER_SD      (7)     //FUNC_GPIO7

void i2c_master_init(void)
{
    gpio_set_drive_mode(6, GPIO_DM_OUTPUT);
    gpio_set_drive_mode(7, GPIO_DM_OUTPUT);
    gpio_set_pin(6, GPIO_PV_HIGH);
    gpio_set_pin(7, GPIO_PV_HIGH);
}

void i2c_start(void)
{
    gpio_set_drive_mode(7, GPIO_DM_OUTPUT);
    gpio_set_pin(7, GPIO_PV_HIGH);
    gpio_set_pin(6, GPIO_PV_HIGH);
    usleep(DELAY_TIME);
    gpio_set_pin(7, GPIO_PV_LOW);
    usleep(DELAY_TIME);
    gpio_set_pin(6, GPIO_PV_LOW);
}

void i2c_stop(void)
{
    gpio_set_drive_mode(7, GPIO_DM_OUTPUT);
    gpio_set_pin(7, GPIO_PV_LOW);
    gpio_set_pin(6, GPIO_PV_HIGH);
    usleep(DELAY_TIME);
    gpio_set_pin(7, GPIO_PV_HIGH);
}

uint8_t i2c_send_byte(uint8_t data)
{
    uint8_t index;

    gpio_set_drive_mode(7, GPIO_DM_OUTPUT);
    for (index = 0; index < 8; index++)
    {
        if (data & 0x80)
            gpio_set_pin(7, GPIO_PV_HIGH);
        else
            gpio_set_pin(7, GPIO_PV_LOW);
        usleep(DELAY_TIME);
        gpio_set_pin(6, GPIO_PV_HIGH);
        usleep(DELAY_TIME);
        gpio_set_pin(6, GPIO_PV_LOW);
        data <<= 1;
    }
    gpio_set_drive_mode(7, GPIO_DM_INPUT);
    usleep(DELAY_TIME);
    gpio_set_pin(6, GPIO_PV_HIGH);
    data = gpio_get_pin(7);
    usleep(DELAY_TIME);
    gpio_set_pin(6, GPIO_PV_LOW);

    return data;
}

uint8_t i2c_receive_byte(uint8_t ack)
{
    uint8_t index, data = 0;

    gpio_set_drive_mode(7, GPIO_DM_INPUT);
    for (index = 0; index < 8; index++)
    {
        usleep(DELAY_TIME);
        gpio_set_pin(6, GPIO_PV_HIGH);
        data <<= 1;
        if (gpio_get_pin(7))
            data++;
        usleep(DELAY_TIME);
        gpio_set_pin(6, GPIO_PV_LOW);
    }
    gpio_set_drive_mode(7, GPIO_DM_OUTPUT);
    if (ack)
        gpio_set_pin(7, GPIO_PV_HIGH);
    else
        gpio_set_pin(7, GPIO_PV_LOW);
    usleep(DELAY_TIME);
    gpio_set_pin(6, GPIO_PV_HIGH);
    usleep(DELAY_TIME);
    gpio_set_pin(6, GPIO_PV_LOW);

    return data;
}

uint8_t i2c_write_reg(uint8_t reg, uint8_t *data_buf, size_t length)
{
    i2c_start();
    i2c_send_byte(SLAVE_ADDRESS << 1);
    i2c_send_byte(reg);
    while (length--)
        i2c_send_byte(*data_buf++);
    i2c_stop();

    return 0;
}

uint8_t i2c_read_reg(uint8_t reg, uint8_t *data_buf, size_t length)
{
    i2c_start();
    i2c_send_byte(SLAVE_ADDRESS << 1);
    i2c_send_byte(reg);
    i2c_start();
    i2c_send_byte((SLAVE_ADDRESS << 1) + 1);
    while (length > 1L)
    {
        *data_buf++ = i2c_receive_byte(0);
        length--;
    }
    *data_buf++ = i2c_receive_byte(1);
    i2c_stop();

    return 0;
}

