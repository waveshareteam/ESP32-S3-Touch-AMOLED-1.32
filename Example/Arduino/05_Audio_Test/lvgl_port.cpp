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

static lv_disp_draw_buf_t        disp_buf;
static lv_disp_drv_t             disp_drv;
static esp_lcd_panel_handle_t    panel_handle = NULL;
static esp_lcd_panel_io_handle_t io_handle    = NULL;
static SemaphoreHandle_t         lvgl_mux     = NULL;

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

static bool Lcd_OnColorTransDone(esp_lcd_panel_io_handle_t panel_io, esp_lcd_panel_io_event_data_t *edata, void *user_ctx) {
    lv_disp_drv_t *disp_driver = (lv_disp_drv_t *) user_ctx;
    lv_disp_flush_ready(disp_driver);
    return false;
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
    io_config.user_ctx                      = &disp_drv;
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
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map) {
    esp_lcd_panel_handle_t panel    = (esp_lcd_panel_handle_t) drv->user_data;
    const int              offsetx1 = area->x1 + 0x06;
    const int              offsetx2 = area->x2 + 0x06;
    const int              offsety1 = area->y1;
    const int              offsety2 = area->y2;

    esp_lcd_panel_draw_bitmap(panel, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
}

void lvgl_rounder_cb(struct _lv_disp_drv_t *disp_drv, lv_area_t *area) {
    uint16_t x1 = area->x1;
    uint16_t x2 = area->x2;

    uint16_t y1 = area->y1;
    uint16_t y2 = area->y2;

    area->x1 = (x1 >> 1) << 1;
    area->y1 = (y1 >> 1) << 1;

    area->x2 = ((x2 >> 1) << 1) + 1;
    area->y2 = ((y2 >> 1) << 1) + 1;
}

static void lvgl_touch_cb(lv_indev_drv_t *drv, lv_indev_data_t *data) {
    uint16_t      x        = 0x00;
    uint16_t      y        = 0x00;
    if (touch_dev.GetCoords(&x, &y)) {
        data->point.x = (LCD_H_RES - x);
        data->point.y = (LCD_H_RES - y);
        if (data->point.x > LCD_H_RES)
            data->point.x = LCD_H_RES;
        if (data->point.y > LCD_V_RES)
            data->point.y = LCD_V_RES;
        data->state = LV_INDEV_STATE_PRESSED;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
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

void Lcd_SetRotation(uint8_t brig) {
    uint32_t lcd_cmd = 0x36;
    lcd_cmd &= 0xff;
    lcd_cmd <<= 8;
    lcd_cmd |= 0x02 << 24;
    uint8_t param = brig;
    esp_lcd_panel_io_tx_param(io_handle, lcd_cmd, &param, 1);
}

void Lvgl_PortInit(void) {
    lvgl_mux = xSemaphoreCreateMutex();
    assert(lvgl_mux);

    Lvgl_LcdPanelIinit(LCD_HOST);

    lv_init();
    lv_color_t *buffer1 = (lv_color_t *) heap_caps_malloc(LCD_H_RES * LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buffer1);
    lv_color_t *buffer2 = (lv_color_t *) heap_caps_malloc(LCD_H_RES * LVGL_BUF_HEIGHT * sizeof(lv_color_t), MALLOC_CAP_DMA);
    assert(buffer2);

    lv_disp_draw_buf_init(&disp_buf, buffer1, buffer2, LCD_H_RES * LVGL_BUF_HEIGHT);

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res    = LCD_H_RES;
    disp_drv.ver_res    = LCD_V_RES;
    disp_drv.flush_cb   = lvgl_flush_cb;
    disp_drv.rounder_cb = lvgl_rounder_cb;
    disp_drv.draw_buf   = &disp_buf;
    disp_drv.user_data  = panel_handle;
    lv_disp_t *disp     = lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_drv.disp    = disp;
    indev_drv.read_cb = lvgl_touch_cb;
    lv_indev_drv_register(&indev_drv);

    esp_timer_create_args_t lvgl_tick_timer_args = {};
    lvgl_tick_timer_args.callback                = &increase_lvgl_tick;
    lvgl_tick_timer_args.name                    = "lvgl_tick";
    esp_timer_handle_t lvgl_tick_timer           = NULL;

    ESP_ERROR_CHECK(esp_timer_create(&lvgl_tick_timer_args, &lvgl_tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(lvgl_tick_timer, LVGL_TICK_PERIOD_MS * 1000));

    xTaskCreate(lvgl_port_task, "LVGL", LVGL_TASK_STACK_SIZE, NULL, LVGL_TASK_PRIORITY, NULL);
    Lcd_SetRotation(0xC0);
}


