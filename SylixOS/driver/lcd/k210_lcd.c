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
** 文   件   名: k210_lcd.c
**
** 创   建   人: Yu.KangZhi (余康志)
**
** 文件创建日期: 2018 年 10 月 29 日
**
** 描        述: K210 处理器 LCD 驱动头文件
*********************************************************************************************************/
#define  __SYLIXOS_KERNEL
#define  __SYLIXOS_STDIO
#include "SylixOS.h"
#include "config.h"
#include <linux/compat.h>
#include "driver/common.h"
#include "driver/fix_arch_def.h"
#include "driver/clock/k210_clock.h"

#include "driver/lcd/k210_lcd.h"
#include "driver/lcd/lcd.h"
#include "driver/lcd/nt35310.h"

#include "KendryteWare/include/fpioa.h"
/*********************************************************************************************************
  宏定义
*********************************************************************************************************/
#define LCD_BITS_PER_PIXEL      (16)
#define LCD_BYTES_PER_PIXEL     (2)
#define LCD_FB_SIZE             (LCD_X_MAX * LCD_Y_MAX * LCD_BYTES_PER_PIXEL)
#define LCD_FB_ALIGN            (128)
#define LCD_RED_MASK            (0xF800)
#define LCD_GREEN_MASK          (0x03E0)
#define LCD_BLUE_MASK           (0x001F)
#define BURST_SIZE              (0x7F)
/*********************************************************************************************************
  LCD 控制器类型定义
*********************************************************************************************************/
typedef struct {
    LW_GM_DEVICE     LCDC_gmDev;                                        /*  图形设备                    */
    CPCHAR           LCDC_pcName;                                       /*  设备名                      */
    UINT             LCDD_iLcdMode;                                     /*  设备Mode                    */
    BOOL             LCDC_bIsInit;                                      /*  是否已经初始化              */
    PVOID            LCDC_pvFrameBuffer;
} __K210_LCD_CONTROLER, *__PK210_LCD_CONTROLER;
/*********************************************************************************************************
  显示信息
*********************************************************************************************************/
static  __K210_LCD_CONTROLER    _G_k210LcdControler;
static  LW_GM_FILEOPERATIONS    _G_k210LcdGmFileOper;
/*********************************************************************************************************
** 函数名称: __k210LcdHwInit
** 功能描述: 初始化 LCD 硬件
** 输　入  : pLcdControler         LCD 控制器
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdHwInit (__PK210_LCD_CONTROLER  pLcdControler)
{
    /*
     * Init SPI IO map and function settings
     */
    fpioa_set_function(8, FUNC_GPIOHS0 + DCX_GPIONUM);
    fpioa_set_function(6, FUNC_SPI0_SS3);
    fpioa_set_function(7, FUNC_SPI0_SCLK);

    /*
     * Lcd hardware initialization
     */
    lcd_init();

    return (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210LcdInit
** 功能描述: 初始化 LCD
** 输　入  : pLcdControler         LCD 控制器
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdInit (__PK210_LCD_CONTROLER  pLcdControler)
{
    if (!pLcdControler->LCDC_bIsInit) {

        pLcdControler->LCDC_pvFrameBuffer = __SHEAP_ALLOC_ALIGN(LCD_FB_SIZE, LCD_FB_ALIGN);
        if (pLcdControler->LCDC_pvFrameBuffer == LW_NULL) {
            printk("__k210LcdInit: failed to alloc framebuffer\r\n");
            return  (PX_ERROR);
        }

        lib_memset(pLcdControler->LCDC_pvFrameBuffer, 0x00, LCD_FB_SIZE);

        __k210LcdHwInit(pLcdControler);

        pLcdControler->LCDC_bIsInit = LW_TRUE;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210LcdOpen
** 功能描述: 打开 FB
** 输　入  : pgmdev    fb 设备
**           iFlag     打开设备标志
**           iMode     打开设备模式
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdOpen (PLW_GM_DEVICE  pgmdev, INT  iFlag, INT  iMode)
{
    __PK210_LCD_CONTROLER  pControler = &_G_k210LcdControler;

    if (__k210LcdInit(pControler) != ERROR_NONE) {
        printk(KERN_ERR "__k210LcdOpen(): failed to init!\n");
        return  (PX_ERROR);
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210LcdClose
** 功能描述: FB 关闭函数
** 输　入  : pgmdev    fb 设备
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdClose (PLW_GM_DEVICE  pgmdev)
{
    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210LcdGetVarInfo
** 功能描述: 获得 FB 信息
** 输　入  : pgmdev    fb 设备
**           pgmsi     fb 屏幕信息
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdGetVarInfo (PLW_GM_DEVICE  pgmdev, PLW_GM_VARINFO  pgmvi)
{
    if (pgmvi) {
        pgmvi->GMVI_ulXRes              = LCD_X_MAX;
        pgmvi->GMVI_ulYRes              = LCD_Y_MAX;

        pgmvi->GMVI_ulXResVirtual       = LCD_X_MAX;
        pgmvi->GMVI_ulYResVirtual       = LCD_Y_MAX;

        pgmvi->GMVI_ulXOffset           = 0;
        pgmvi->GMVI_ulYOffset           = 0;

        pgmvi->GMVI_ulBitsPerPixel      = LCD_BITS_PER_PIXEL;
        pgmvi->GMVI_ulBytesPerPixel     = LCD_BYTES_PER_PIXEL;

        pgmvi->GMVI_ulGrayscale         = (1 << LCD_BITS_PER_PIXEL);

        /*
         *  RGB565 RED 0xF800, GREEN 0x07E0, BLUE 0x001F
         */
        pgmvi->GMVI_ulRedMask           = LCD_RED_MASK;
        pgmvi->GMVI_ulGreenMask         = LCD_GREEN_MASK;
        pgmvi->GMVI_ulBlueMask          = LCD_BLUE_MASK;
        pgmvi->GMVI_ulTransMask         = 0;

        pgmvi->GMVI_bHardwareAccelerate = LW_FALSE;
        pgmvi->GMVI_ulMode              = LW_GM_SET_MODE;
        pgmvi->GMVI_ulStatus            = 0;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: __k210LcdGetScrInfo
** 功能描述: 获得 LCD 屏幕信息
** 输　入  : pgmdev    fb 设备
**           pgmsi     fb 屏幕信息
** 输　出  : 错误号
** 全局变量:
** 调用模块:
*********************************************************************************************************/
static INT  __k210LcdGetScrInfo (PLW_GM_DEVICE  pgmdev, PLW_GM_SCRINFO  pgmsi)
{
    __PK210_LCD_CONTROLER   pControler = &_G_k210LcdControler;

    if (pgmsi) {
        pgmsi->GMSI_pcName           = (PCHAR)pControler->LCDC_pcName;
        pgmsi->GMSI_ulId             = 0;
        pgmsi->GMSI_stMemSize        = LCD_X_MAX * LCD_Y_MAX * LCD_BYTES_PER_PIXEL;
        pgmsi->GMSI_stMemSizePerLine = LCD_X_MAX * LCD_BYTES_PER_PIXEL;
        pgmsi->GMSI_pcMem            = (caddr_t)pControler->LCDC_pvFrameBuffer;
    }

    return  (ERROR_NONE);
}
/*********************************************************************************************************
** 函数名称: k210FbDevCreate
** 功能描述: 创建 FrameBuffer 设备
** 输　入  : cpcName          设备名
** 输　出  : ERROR_CODE
** 全局变量:
** 调用模块:
*********************************************************************************************************/
INT  k210FbDevCreate (CPCHAR  cpcName)
{
    __PK210_LCD_CONTROLER     pControler  =  &_G_k210LcdControler;
    PLW_GM_FILEOPERATIONS     pGmFileOper =  &_G_k210LcdGmFileOper;

    if (cpcName == LW_NULL) {
        return  (PX_ERROR);
    }

    /*
     *  仅支持 framebuffer 模式.
     */
    pGmFileOper->GMFO_pfuncOpen         = __k210LcdOpen;
    pGmFileOper->GMFO_pfuncClose        = __k210LcdClose;
    pGmFileOper->GMFO_pfuncGetVarInfo   = (INT (*)(LONG, PLW_GM_VARINFO))__k210LcdGetVarInfo;
    pGmFileOper->GMFO_pfuncGetScrInfo   = (INT (*)(LONG, PLW_GM_SCRINFO))__k210LcdGetScrInfo;

    pControler->LCDC_gmDev.GMDEV_gmfileop   = pGmFileOper;
    pControler->LCDC_gmDev.GMDEV_ulMapFlags = LW_VMM_FLAG_DMA | LW_VMM_FLAG_BUFFERABLE;
    pControler->LCDC_pcName                 = cpcName;
    pControler->LCDD_iLcdMode               = LCD_BITS_PER_PIXEL;

    return  (gmemDevAdd(pControler->LCDC_pcName, &pControler->LCDC_gmDev));
}
/*********************************************************************************************************
  END
*********************************************************************************************************/
