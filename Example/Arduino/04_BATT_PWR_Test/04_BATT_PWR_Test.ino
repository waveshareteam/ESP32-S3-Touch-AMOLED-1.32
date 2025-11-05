#include "user_config.h"
#include "esp_err.h"
#include "src/button_bsp/button_bsp.h"
#include "src/ui_src/generated/gui_guider.h"
#include "src/vbat_bsp/vbat_bsp.h"
#include "lvgl_port.h"

BatteryMonitor batt_dev(ADC_CHANNEL_3,18,17);

static lv_ui src_ui;


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

void user_ui_init(void) {
	setup_ui(&src_ui);
	lv_label_set_text(src_ui.screen_label_1, "Power ON");
	while(!batt_dev.Get_isVbatPowerState()) {
		vTaskDelay(pdMS_TO_TICKS(100));
	}
	user_button_init();
	xTaskCreatePinnedToCore(Custom_ButtonPWRLoopTask, "Custom_ButtonPWRLoopTask", 4 * 1024, NULL, 2, NULL,0);
}

void setup() {
  batt_dev.Set_VbatPowerON();
  
  Lvgl_PortInit();
  if (lvgl_lock(0)) {
      user_ui_init();
      lvgl_unlock();
  }
}

void loop() {

}
