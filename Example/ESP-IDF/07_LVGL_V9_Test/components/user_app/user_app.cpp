#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "i2c_bsp.h"
#include "lcd_touch_bsp.h"
#include "driver/gpio.h"
#include "user_config.h"
#include "esp_log.h"
#include "user_app.h"
#include "lvgl_port_bsp.h"
#include "lv_demos.h"

I2cMasterBus i2c_dev(ESP32_SCL_NUM,ESP32_SDA_NUM,0);
LcdTouchPanel touch_dev(i2c_dev,TOUCH_ADDR);

LcdTouchPanel Custom_GetLcdTouchPanel(void) {
	return touch_dev;
}

void user_app_init(void) {

}

void Custom_BacklightLoopTask(void *arg) {
	for(;;) {
		vTaskDelay(pdMS_TO_TICKS(1500));
		Lcd_SetBacklight(255);
		vTaskDelay(pdMS_TO_TICKS(1500));
		Lcd_SetBacklight(200);
		vTaskDelay(pdMS_TO_TICKS(1500));
		Lcd_SetBacklight(150);
		vTaskDelay(pdMS_TO_TICKS(1500));
		Lcd_SetBacklight(100);
		vTaskDelay(pdMS_TO_TICKS(1500));
		Lcd_SetBacklight(0);
	}
}

void user_ui_init(void) {
	lv_demo_widgets();        /* A widgets example */
#if BacklightTestEN
	xTaskCreatePinnedToCore(Custom_BacklightLoopTask, "Custom_BacklightLoopTask", 4 * 1024, NULL, 2, NULL,1);
#endif
}

