#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "lvgl_port.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "src/sh8601/esp_lcd_sh8601.h"
#include "esp_timer.h"
#include "lvgl.h"

#include "user_config.h"
#include "src/i2c_bsp/i2c_bsp.h"
#include "lcd_touch.h"

I2cMasterBus i2c_dev(ESP32_SCL_NUM,ESP32_SDA_NUM,0);
LcdTouchPanel touch_dev(i2c_dev,TOUCH_ADDR);

#define LCD_BIT_PER_PIXEL (16)

static esp_lcd_panel_handle_t    panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle    = NULL;
static SemaphoreHandle_t         lvgl_mux     = NULL;
static SemaphoreHandle_t flush_done_semaphore = NULL; 
#define BYTES_PER_PIXEL (LV_COLOR_FORMAT_GET_SIZE(LV_COLOR_FORMAT_RGB565))
#define BUFF_SIZE (LCD_H_RES * LVGL_BUF_HEIGHT * BYTES_PER_PIXEL)

static const sh8601_lcd_init_cmd_t lcd_init_cmds[] = {
    {0xFE, (uint8_t[]) {0x00}, 1, 0},
    {0xC4, (uint8_t[]) {0x80}, 1, 0},
    {0x3A, (uint8_t[]) {0x55}, 1, 0},
    {0x35, (uint8_t[]) {0x00}, 1, 0},
    {0x53, (uint8_t[]) {0x20}, 1, 0},
    {0x51, (uint8_t[]) {0xFF}, 1, 0},
    {0x63, (uint8_t[]) {0xFF}, 1, 0},
    {0x2A, (uint8_t[]) {0x00, 0x06, 0x01, 0xD7}, 4, 0},
    {0x2B, (uint8_t[]) {0x00, 0x00, 0x01, 0xD1}, 4, 0},

    {0x11, (uint8_t[]) {0x00}, 0, 100},
    {0x29, (uint8_t[]) {0x00}, 0, 0},
};

void Lcd_SetRotation(uint8_t brig) {
    uint32_t lcd_cmd = 0x36;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;
    uint8_t param = brig;
    esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &param, 1);
}

static bool Lcd_OnColorTransDone(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    BaseType_t high_task_awoken = pdFALSE;
    xSemaphoreGiveFromISR(flush_done_semaphore, &high_task_awoken);
    return high_task_awoken == pdTRUE;
}

static void Lvgl_LcdPanelIinit(int LcdHOST) {
    spi_bus_config_t buscfg = {};
    buscfg.sclk_io_num      = LCD_PCLK_PIN;
    buscfg.data0_io_num     = LCD_DATA0_PIN;
    buscfg.data1_io_num     = LCD_DATA1_PIN;
    buscfg.data2_io_num     = LCD_DATA2_PIN;
    buscfg.data3_io_num     = LCD_DATA3_PIN;
    buscfg.max_transfer_sz  = LCD_H_RES * LCD_V_RES * LCD_BIT_PER_PIXEL / 8;
    ESP_ERROR_CHECK(spi_bus_initialize((spi_host_device_t) LcdHOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_spi_config_t io_config = {};
    io_config.cs_gpio_num                   = LCD_CS_PIN;
    io_config.dc_gpio_num                   = -1;
    io_config.spi_mode                      = 0;
    io_config.pclk_hz                       = 40 * 1000 * 1000;
    io_config.trans_queue_depth             = 10;
    io_config.on_color_trans_done           = Lcd_OnColorTransDone;
    //io_config.user_ctx                      = &disp_drv;
    io_config.lcd_cmd_bits                  = 32;
    io_config.lcd_param_bits                = 8;
    io_config.flags.quad_mode               = true;
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) LcdHOST, &io_config, &io_handle));

    sh8601_vendor_config_t vendor_config   = {};
    vendor_config.init_cmds                = lcd_init_cmds;
    vendor_config.init_cmds_size           = sizeof(lcd_init_cmds) / sizeof(lcd_init_cmds[0]);
    vendor_config.flags.use_qspi_interface = 1;

    esp_lcd_panel_dev_config_t panel_config = {};
    panel_config.reset_gpio_num             = LCD_RST_PIN;
    panel_config.rgb_ele_order              = LCD_RGB_ELEMENT_ORDER_RGB;
    panel_config.bits_per_pixel             = LCD_BIT_PER_PIXEL;
    panel_config.vendor_config              = &vendor_config;

    ESP_ERROR_CHECK(esp_lcd_new_panel_sh8601(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    Lcd_SetRotation(0xC0);
}

static void lvgl_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * color_p) {
    esp_lcd_panel_handle_t panel = (esp_lcd_panel_handle_t)lv_display_get_user_data(disp);
    lv_draw_sw_rgb565_swap(color_p, lv_area_get_width(area) * lv_area_get_height(area));

    const int              offsetx1 = area->x1 + 0x06;
    const int              offsetx2 = area->x2 + 0x06;
    const int              offsety1 = area->y1;
    const int              offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_p);
    //lv_disp_flush_ready(disp);
}

void lvgl_rounder_cb(lv_event_t *e) {
    lv_area_t *area = (lv_area_t *)lv_event_get_param(e);

    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;

    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

static void lvgl_touch_cb(lv_indev_t * indev, lv_indev_data_t *indevData) {
    uint16_t      x        = 0x00;
    uint16_t      y        = 0x00;
    if (touch_dev.GetCoords(&x, &y)) {
        indevData->point.x = (LCD_H_RES - x);
        indevData->point.y = (LCD_H_RES - y);
        if (indevData->point.x > LCD_H_RES)
            indevData->point.x = LCD_H_RES;
        if (indevData->point.y > LCD_V_RES)
            indevData->point.y = LCD_V_RES;
        indevData->state = LV_INDEV_STATE_PRESSED;
    } else {
        indevData->state = LV_INDEV_STATE_RELEASED;
    }
}

static void increase_lvgl_tick(void *arg) {
    lv_tick_inc(LVGL_TICK_PERIOD_MS);
}

bool lvgl_lock(int timeout_ms) {
    assert(lvgl_mux && "bsp_display_start must be called first");

    const TickType_t timeout_ticks = (timeout_ms == 0) ? portMAX_DELAY : pdMS_TO_TICKS(timeout_ms);
    return xSemaphoreTake(lvgl_mux, timeout_ticks) == pdTRUE;
}

void lvgl_unlock(void) {
    assert(lvgl_mux && "bsp_display_start must be called first");
    xSemaphoreGive(lvgl_mux);
}

static void lvgl_port_task(void *arg) {
    uint32_t task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
    while (1) {
        if (lvgl_lock(-1)) {
            task_delay_ms = lv_timer_handler();
            lvgl_unlock();
        }
        if (task_delay_ms > LVGL_TASK_MAX_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MAX_DELAY_MS;
        } else if (task_delay_ms < LVGL_TASK_MIN_DELAY_MS) {
            task_delay_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        vTaskDelay(pdMS_TO_TICKS(task_delay_ms));
    }
}

void Lcd_SetBacklight(uint8_t brig) {
    uint32_t lcd_cmd = 0x51;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;
    uint8_t param = brig;
    esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &param, 1);
}

static void lvgl_wait_cb(lv_display_t * disp) 
{
    xSemaphoreTake(flush_done_semaphore, portMAX_DELAY);
}

void Lvgl_PortInit(void) {
    flush_done_semaphore = xSemaphoreCreateBinary();
    assert(flush_done_semaphore);
    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);

    Lvgl_LcdPanelIinit(LCD_HOST);

    lv_init();
    lv_display_t * disp = lv_display_create(LCD_H_RES, LCD_V_RES); 
    lv_display_set_flush_cb(disp, lvgl_flush_cb);                  
    lv_display_set_flush_wait_cb(disp,lvgl_wait_cb);

    uint8_t *buffer1 = NULL;
    uint8_t *buffer2 = NULL;
    buffer1 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_DMA);
    buffer2 = (uint8_t *)heap_caps_malloc(BUFF_SIZE, MALLOC_CAP_DMA);
    assert(buffer1);
    assert(buffer2);
    lv_display_set_buffers(disp, buffer1, buffer2, BUFF_SIZE, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_user_data(disp, panel_handle);
    lv_display_add_event_cb(disp,lvgl_rounder_cb,LV_EVENT_INVALIDATE_AREA,NULL);
    
    lv_indev_t *touch_indev = NULL;
    touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, lvgl_touch_cb);


    esp_timer_create_args_t lvgl_tick_timer_args = {};
    lvgl_tick_timer_args.callback                = &increase_lvgl_tick;
    lvgl_tick_timer_args.name                    = "lvgl_tick";
    esp_timer_handle_t lvgl_tick_timer           = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
}


