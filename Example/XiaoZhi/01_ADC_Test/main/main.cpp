#include <stdio.h>
#include "user_app.h"
#include "driver/gpio.h"
#include "esp_log.h"


extern "C" void app_main(void)
{
    ESP_LOGI("main","adc-example run");
    user_app_init();
}
