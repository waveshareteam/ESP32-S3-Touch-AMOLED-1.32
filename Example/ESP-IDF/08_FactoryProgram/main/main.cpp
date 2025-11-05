#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "lvgl_port_bsp.h"
#include "user_app.h"
#include "user_config.h"
#include <stdio.h>

#include "lv_demos.h"
#include "lvgl.h"

static const char *TAG = "esp32_1_32";

extern "C" void app_main(void) {
    user_app_init();

    Lvgl_PortInit();

    if (lvgl_lock(0)) {
        user_ui_init();
        lvgl_unlock();
    }
}
