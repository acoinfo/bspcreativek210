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
** 文   件   名: ai_test_main.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 12 月 7 日
**
** 描        述: KPU AI 测试程序
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include "pthread.h"
#include "unistd.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "unistd.h"
#include "math.h"
#include "driver/clock/k210_clock.h"
#include "driver/fix_arch_def.h"
#include "driver/lcd/lcd.h"
#include "driver/lcd/nt35310.h"
#include "driver/video/video_driver/k210_sccb.h"

#include "fpioa.h"
#include "plic.h"
#include "dvp.h"
#include "cnn.h"
#include "region_layer.h"
#include "k210_kpu.h"
/*********************************************************************************************************
  定义
*********************************************************************************************************/
#define PLL0_OUTPUT_FREQ 1000000000UL
#define PLL1_OUTPUT_FREQ 300000000UL
#define PLL2_OUTPUT_FREQ 45158400UL
/*********************************************************************************************************
  全局变量 (注意: 这里为了和后续官方更新进行对照，没有使用 SylixOS 应用编程规范)
*********************************************************************************************************/
cnn_task_t task;                                                        /*  AI 计算任务                 */
uint8_t g_ai_done_flag;                                                 /*  AI 计算任务是否完成         */

uint32_t g_lcd_gram0[38400] __attribute__((aligned(64)));               /*  摄像头图像缓冲 BUFFER1      */
uint32_t g_lcd_gram1[38400] __attribute__((aligned(64)));               /*  摄像头图像缓冲 BUFFER2      */

volatile uint8_t g_dvp_finish_flag = 0;                                 /*  摄像头图像捕捉完成信号      */
volatile uint8_t g_ram_mux = 0;                                         /*  摄像头图像缓冲BINGPONG标志  */

uint8_t g_ai_buf[320 * 240 *3] __attribute__((aligned(128)));           /*  AI 人脸识别图像缓冲区       */
uint64_t image_dst[(10*7*125+7)/8] __attribute__((aligned(128)));       /*  AI 人脸识别输出缓冲区       */
/*********************************************************************************************************
  AI 识别标签分类 (注意: 这里为了和后续官方更新进行对照，没有使用 SylixOS 应用编程规范)
*********************************************************************************************************/
#if (CLASS_NUMBER > 1)
typedef struct
{
    char *str;
    uint16_t color;
    uint16_t height;
    uint16_t width;
    uint32_t *ptr;
} class_lable_t;

class_lable_t class_lable[CLASS_NUMBER] =
{
    {"aeroplane", GREEN},
    {"bicycle", GREEN},
    {"bird", GREEN},
    {"boat", GREEN},
    {"bottle", 0xF81F},
    {"bus", GREEN},
    {"car", GREEN},
    {"cat", GREEN},
    {"chair", 0xFD20},
    {"cow", GREEN},
    {"diningtable", GREEN},
    {"dog", GREEN},
    {"horse", GREEN},
    {"motorbike", GREEN},
    {"person", 0xF800},
    {"pottedplant", GREEN},
    {"sheep", GREEN},
    {"sofa", GREEN},
    {"train", GREEN},
    {"tvmonitor", 0xF9B6}
};

static uint32_t lable_string_draw_ram[115 * 16 * 8 / 2];                /*  标签字符串缓冲              */

#endif
/*********************************************************************************************************
  相关函数接口定义 (注意: 这里为了和后续官方更新进行对照，没有使用 SylixOS 应用编程规范)
*********************************************************************************************************/
static int ai_done(void *ctx)
{
    g_ai_done_flag = 1;
    return 0;
}

static irqreturn_t on_irq_dvp(PVOID ctx, ULONG ulVector)
{
    if (dvp_get_interrupt(DVP_STS_FRAME_FINISH))
    {
        /* switch gram */
        dvp_set_display_addr(g_ram_mux ? (uint32_t)g_lcd_gram0 : (uint32_t)g_lcd_gram1);

        dvp_clear_interrupt(DVP_STS_FRAME_FINISH);
        g_dvp_finish_flag = 1;
    } else {
        if(g_dvp_finish_flag == 0)
            dvp_start_convert();
        dvp_clear_interrupt(DVP_STS_FRAME_START);
    }

    return (LW_IRQ_HANDLED);
}

static void io_mux_init(void)
{
    /* Init DVP IO map and function settings */
    fpioa_set_function(11, FUNC_CMOS_RST);
    fpioa_set_function(13, FUNC_CMOS_PWDN);
    fpioa_set_function(14, FUNC_CMOS_XCLK);
    fpioa_set_function(12, FUNC_CMOS_VSYNC);
    fpioa_set_function(17, FUNC_CMOS_HREF);
    fpioa_set_function(15, FUNC_CMOS_PCLK);
    fpioa_set_function(10, FUNC_SCCB_SCLK);
    fpioa_set_function(9, FUNC_SCCB_SDA);

    /* Init SPI IO map and function settings */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    sysctl_spi0_dvp_data_set(1);
}

static void io_set_power(void)
{
    /* Set dvp and spi pin to 1.8V */
    sysctl_power_mode_sel(SYSCTL_POWER_BANK1, POWER_V18);
    sysctl_power_mode_sel(SYSCTL_POWER_BANK2, POWER_V18);
}

static void lable_init(void)
{
#if (CLASS_NUMBER > 1)
    uint8_t index;

    class_lable[0].height = 16;
    class_lable[0].width = 8 * strlen(class_lable[0].str);
    class_lable[0].ptr = lable_string_draw_ram;
    lcd_ram_draw_string(class_lable[0].str, class_lable[0].ptr, BLACK, class_lable[0].color);
    for (index = 1; index < CLASS_NUMBER; index++) {
        class_lable[index].height = 16;
        class_lable[index].width = 8 * strlen(class_lable[index].str);
        class_lable[index].ptr = class_lable[index - 1].ptr + class_lable[index - 1].height * class_lable[index - 1].width / 2;
        lcd_ram_draw_string(class_lable[index].str, class_lable[index].ptr, BLACK, class_lable[index].color);
    }
#endif
}

static void drawboxes(uint32_t x1, uint32_t y1, uint32_t x2, uint32_t y2, uint32_t class, float prob)
{
    if (x1 >= 320)
        x1 = 319;
    if (x2 >= 320)
        x2 = 319;
    if (y1 >= 240)
        y1 = 239;
    if (y2 >= 240)
        y2 = 239;

#if (CLASS_NUMBER > 1)
    lcd_draw_rectangle(x1, y1, x2, y2, 2, class_lable[class].color);
    lcd_draw_picture(x1 + 1, y1 + 1, class_lable[class].width, class_lable[class].height, class_lable[class].ptr);
#else
    lcd_draw_rectangle(x1, y1, x2, y2, 2, RED);
#endif
}

static void ai_lcd_init(void)
{
    /* LCD init */
    printf("LCD init\n");
    lcd_init();
    lcd_set_direction(DIR_YX_RLUD);
    lcd_clear(BLACK);
    lcd_draw_string(136, 70, "DEMO 1", WHITE);
    lcd_draw_string(104, 150, "face detection", WHITE);
}

static void ai_dvp_init(void)
{
    /* DVP init */
    printf("DVP init\n");
    dvp_init(16);
    dvp_enable_burst();
    dvp_set_output_enable(0, 1);
    dvp_set_output_enable(1, 1);
    dvp_set_image_format(DVP_CFG_RGB_FORMAT);
    dvp_set_image_size(320, 240);
    ov5640_init();

    dvp_set_ai_addr((uint32_t)g_ai_buf, (uint32_t)(g_ai_buf + 320 * 240), (uint32_t)(g_ai_buf + 320 * 240 * 2));
    dvp_set_display_addr((uint32_t)g_lcd_gram0);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 0);
    dvp_disable_auto();

    /* DVP interrupt config */
    printf("DVP interrupt config\n");
    API_InterVectorSetPriority(KENDRYTE_PLIC_VECTOR(IRQN_DVP_INTERRUPT), 1);
    API_InterVectorConnect(KENDRYTE_PLIC_VECTOR(IRQN_DVP_INTERRUPT), (PINT_SVR_ROUTINE)on_irq_dvp,
                           NULL, "dvp_isr");
    API_InterVectorEnable(KENDRYTE_PLIC_VECTOR(IRQN_DVP_INTERRUPT));
}

static void ai_system_start(void)
{
    /* system start */
    printf("system start\n");
    g_ram_mux = 0;
    g_dvp_finish_flag = 0;
    dvp_clear_interrupt(DVP_STS_FRAME_START | DVP_STS_FRAME_FINISH);
    dvp_config_interrupt(DVP_CFG_START_INT_ENABLE | DVP_CFG_FINISH_INT_ENABLE, 1);
}
/*********************************************************************************************************
** 函数名称: pthread_test_led
** 功能描述: LED 测试 pthread 线程
** 输　入  : pvArg         线程参数
** 输　出  : 线程返回值
** 全局变量:
** 调用模块:
** 注意事项: 该测试程序仅仅用于测试字符设备KPU，其它部分均使用的裸机接口。
**           (注意: 这里为了和后续官方更新进行对照，没有使用 SylixOS 应用编程规范)
*********************************************************************************************************/
static VOID  *pthread_test(VOID  *pvArg)
{
    INT                iKpuFd;
    kpu_transaction_t  kpuTransaction;

    sysctl_clock_enable(SYSCTL_CLOCK_AI);                               /*  使能 KPU 模块               */
    sysctl_clock_enable(SYSCTL_CLOCK_AI);

    io_mux_init();                                                      /*  IO 映射和电压配置           */
    io_set_power();

    lable_init();                                                       /*  分类标签初始化              */

    iKpuFd = open("/dev/k210_kpu", O_RDWR, 0666);                       /*  打开 KPU 字符设备           */
    if (iKpuFd < 0) {
        printf("failed to open /dev/k210_kpu\n");
        return  (LW_NULL);
    }

    ai_lcd_init();                                                      /*  用于测试的LCD屏初始化       */

    ai_dvp_init();                                                      /*  用于测试的DVP摄像头初始化   */

    ai_system_start();                                                  /*  使能摄像头捕捉要识别的图像  */

    while (1)
    {
        while (g_dvp_finish_flag == 0)                                  /*  等待摄像头捕捉一帧图像      */
            ;

        kpuTransaction.task = &task;
        ioctl(iKpuFd, IOCTRL_KPU_TASK_INIT, &kpuTransaction);           /*  初始化 AI 计算模型(.pb)     */

        kpuTransaction.dma_ch = DMAC_CHANNEL5;                          /*  初始化 AI 计算事务          */
        kpuTransaction.src    = g_ai_buf;
        kpuTransaction.dst    = image_dst;
        kpuTransaction.arg    = &g_ai_done_flag;
        kpuTransaction.ai_done_callback = ai_done;
        ioctl(iKpuFd, IOCTRL_KPU_CALCULATE_RUN, &kpuTransaction);       /*  开始运行 AI 计算模型        */

        while(!g_ai_done_flag);                                         /*  AI 计算任务完成             */
        g_ai_done_flag = 0;

        region_layer_cal((uint8_t *)image_dst);                         /*  根据AI输出结果计算识别区域  */

        g_ram_mux ^= 0x01;                                              /*  显示摄像头采集到的图像帧    */
        lcd_draw_picture(0, 0, 320, 240, g_ram_mux ? g_lcd_gram0 : g_lcd_gram1);
        g_dvp_finish_flag = 0;

        region_layer_draw_boxes(drawboxes);                             /*  将识别到的区域用方框框起来  */
    }

    close(iKpuFd);

    return  (LW_NULL);
}
/*********************************************************************************************************
** 函数名称: aiTestStart
** 功能描述: KPU 测试
** 输　入  : NONE
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  aiTestStart (VOID)
{
    pthread_t   tid;
    INT         iError;

    iError  = pthread_create(&tid, NULL, pthread_test, NULL);

    return  (iError == 0 ? ERROR_NONE : PX_ERROR);
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
