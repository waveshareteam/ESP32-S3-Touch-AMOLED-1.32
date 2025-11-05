#ifndef USER_CONFIG_H
#define USER_CONFIG_H

//spi & i2c handle
#define LCD_HOST SPI2_HOST
#define TOUCH_HOST I2C_NUM_0

// touch I2C port
#define ESP32_SCL_NUM (GPIO_NUM_48)
#define ESP32_SDA_NUM (GPIO_NUM_47)

//  DISP

#define LCD_H_RES              466 
#define LCD_V_RES              466 
#define LVGL_BUF_HEIGHT        (30)

#define LCD_CS_PIN          10
#define LCD_PCLK_PIN        11
#define LCD_DATA0_PIN       12
#define LCD_DATA1_PIN       13
#define LCD_DATA2_PIN       14
#define LCD_DATA3_PIN       15
#define LCD_RST_PIN         8
#define BK_LIGHT_PIN        (-1)
#define LCD_TE_PIN          9


#define TOUCH_ADDR            0x15
#define TOUCH_RST_PIN         7
#define TOUCH_INT_PIN         6


#define LVGL_TICK_PERIOD_MS    2
#define LVGL_TASK_MAX_DELAY_MS 500
#define LVGL_TASK_MIN_DELAY_MS 5
#define LVGL_TASK_STACK_SIZE   (4 * 1024)
#define LVGL_TASK_PRIORITY     2


#define USER_DISP_ROT_90    1
#define USER_DISP_ROT_NONO  0
#define Rotated USER_DISP_ROT_NONO   //软件实现旋转


//sys power io

#define SYS_POWER_IO_PIN 18


#define TearingPrevention 0        //打开防撕裂

#endif