#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "lcd_touch_bsp.h"
#include "esp_log.h"
#include "i2c_bsp.h"

LcdTouchPanel::LcdTouchPanel(I2cMasterBus& i2cbus,int dev_addr,int touch_rst_pin,int touch_int_pin) : i2cbus_(i2cbus) {
    i2c_master_bus_handle_t BusHandle = i2cbus_.Get_I2cBusHandle();
    i2c_device_config_t     dev_cfg   = {};
    dev_cfg.dev_addr_length           = I2C_ADDR_BIT_LEN_7;
    dev_cfg.scl_speed_hz              = 300000;
    dev_cfg.device_address            = dev_addr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(BusHandle, &dev_cfg, &touch_dev_handle));

    if(touch_rst_pin != GPIO_NUM_NC) {
        gpio_config_t gpio_conf = {};
        gpio_conf.intr_type     = GPIO_INTR_DISABLE;
        gpio_conf.mode          = GPIO_MODE_OUTPUT;
        gpio_conf.pin_bit_mask  = (0x1ULL<<touch_rst_pin);
        gpio_conf.pull_down_en  = GPIO_PULLDOWN_DISABLE;
        gpio_conf.pull_up_en    = GPIO_PULLUP_ENABLE;

        ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
        touch_rst_pin_ = touch_rst_pin;
    }
}

LcdTouchPanel::~LcdTouchPanel() {
}

void LcdTouchPanel::ResetTouch() {
    if(touch_rst_pin_ != GPIO_NUM_NC) {
        gpio_set_level((gpio_num_t)touch_rst_pin_,1);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level((gpio_num_t)touch_rst_pin_,0);
        vTaskDelay(pdMS_TO_TICKS(200));
        gpio_set_level((gpio_num_t)touch_rst_pin_,1);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

uint8_t LcdTouchPanel::GetCoords(uint16_t *x, uint16_t *y) {
    uint8_t GestureNum[2] = {0, 0};
    uint8_t Event         = 0x01;
    uint8_t Gpos[4]       = {0};
    i2cbus_.i2c_read_buff(touch_dev_handle, 0x02, GestureNum, 2);
    Event = GestureNum[1] >> 6;
    if (GestureNum[0] && (Event != 0x01)) {
        i2cbus_.i2c_read_buff(touch_dev_handle, 0x03, Gpos, 4);
        *x = (((uint16_t) Gpos[0] & 0x0f) << 8 | Gpos[1]);
        *y = (((uint16_t) Gpos[2] & 0x0f) << 8 | Gpos[3]);
        return 1;
    }
    return 0;
}




