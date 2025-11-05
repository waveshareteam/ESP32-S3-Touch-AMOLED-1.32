#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "user_config.h"
#include "esp_log.h"
#include "vbat_bsp.h"
#include "user_app.h"


BatteryMonitor batt_dev(ADC_CHANNEL_3,18,17);


void Custom_VbatLoopTask(void *arg) {
	for(;;) {
		float vol = batt_dev.Get_BatteryVoltage();
		ESP_LOGI("adc-example","value:%d,vol:%.2f",batt_dev.Get_BatteryValue(),vol);
		vTaskDelay(pdMS_TO_TICKS(1000));
	}
}

void user_app_init(void) {
	xTaskCreatePinnedToCore(Custom_VbatLoopTask, "Custom_VbatLoopTask", 3 * 1024, NULL, 2, NULL,0);
}
