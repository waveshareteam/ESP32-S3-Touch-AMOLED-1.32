#include "user_config.h"
#include "esp_err.h"
#include "lvgl_port.h"
#include "lvgl.h"
#include "demos/lv_demos.h"


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

void setup() {

  Lvgl_PortInit();
  if (lvgl_lock(0)) {
      user_ui_init();
      lvgl_unlock();
  }
}

void loop() {

}
