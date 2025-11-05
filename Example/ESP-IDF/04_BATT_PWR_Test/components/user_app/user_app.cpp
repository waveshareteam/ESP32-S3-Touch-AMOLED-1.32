#include <stdio.h>
#include "i2c_bsp.h"
#include "lcd_touch_bsp.h"
#include "driver/gpio.h"
#include "user_config.h"
#include "esp_log.h"
#include "user_app.h"
#include "button_bsp.H"
#include "gui_guider.h"
#include "lvgl_port_bsp.h"
#include "vbat_bsp.h"

I2cMasterBus i2c_dev(ESP32_SCL_NUM,ESP32_SDA_NUM,0);
LcdTouchPanel touch_dev(i2c_dev,TOUCH_ADDR);
BatteryMonitor batt_dev(ADC_CHANNEL_3,18,17);

static lv_ui src_ui;

LcdTouchPanel Custom_GetLcdTouchPanel(void) {
	return touch_dev;
}

void Custom_ButtonPWRLoopTask(void *arg) {
	for(;;) {
		EventBits_t even = xEventGroupWaitBits(pwr_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(2 * 1000));
    	if(get_bit_button(even,2)) {
			lv_label_set_text(src_ui.screen_label_1, "Power OFF");
			vTaskDelay(pdMS_TO_TICKS(200));
			batt_dev.Set_VbatPowerOFF();
		}
	}
}

void user_app_init(void) {
	batt_dev.Set_VbatPowerON();
}

void user_ui_init(void) {
	setup_ui(&src_ui);
	lv_label_set_text(src_ui.screen_label_1, "Power ON");
	while(!batt_dev.Get_isVbatPowerState()) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	user_button_init();
	xTaskCreatePinnedToCore(Custom_ButtonPWRLoopTask, "Custom_ButtonPWRLoopTask", 4 * 1024, NULL, 2, NULL,0);
}

