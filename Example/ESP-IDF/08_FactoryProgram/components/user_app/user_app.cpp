#include <stdio.h>
#include "i2c_bsp.h"
#include "lcd_touch_bsp.h"
#include "driver/gpio.h"
#include "user_config.h"
#include "esp_log.h"
#include "vbat_bsp.h"
#include "user_app.h"
#include "button_bsp.H"
#include "gui_guider.h"
#include "scan_bsp.h"
#include "user_audio.h"
#include "lvgl_port_bsp.h"

I2cMasterBus i2c_dev(ESP32_SCL_NUM,ESP32_SDA_NUM,0);
LcdTouchPanel touch_dev(i2c_dev,TOUCH_ADDR);
BatteryMonitor batt_dev(ADC_CHANNEL_3,18,17);
ScanWirelessBsp scan_dev;
I2sAudioCodec *audio_dev = NULL;

static SemaphoreHandle_t ColorLoopSemap;
static lv_ui src_ui;
static bool is_MusicFlag = 0;

LcdTouchPanel Custom_GetLcdTouchPanel(void) {
	return touch_dev;
}

static void Custom_SetCurrentlyCont(uint8_t val) {
	if(val == 1) {
		lv_obj_add_flag(src_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
  		lv_obj_clear_flag(src_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
	} else if(val == 2) {
		lv_obj_add_flag(src_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
  		lv_obj_clear_flag(src_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
	} else if(val == 3) {
		lv_obj_add_flag(src_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
  		lv_obj_clear_flag(src_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
	} else if(val == 4) {
		lv_obj_add_flag(src_ui.screen_cont_2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_1, LV_OBJ_FLAG_HIDDEN);
  		lv_obj_clear_flag(src_ui.screen_cont_4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(src_ui.screen_cont_3, LV_OBJ_FLAG_HIDDEN);
	}
}

void Custom_ColorLoopTask(void *arg) {
	Custom_SetCurrentlyCont(1);
  	lv_obj_clear_flag(src_ui.screen_img_1,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(src_ui.screen_img_2, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(src_ui.screen_img_3, LV_OBJ_FLAG_HIDDEN);
  	vTaskDelay(pdMS_TO_TICKS(1500));
  	lv_obj_clear_flag(src_ui.screen_img_2,LV_OBJ_FLAG_HIDDEN); 
  	lv_obj_add_flag(src_ui.screen_img_3, LV_OBJ_FLAG_HIDDEN);
  	lv_obj_add_flag(src_ui.screen_img_1, LV_OBJ_FLAG_HIDDEN);
	vTaskDelay(pdMS_TO_TICKS(1500));
	lv_obj_clear_flag(src_ui.screen_img_3,LV_OBJ_FLAG_HIDDEN); 
	lv_obj_add_flag(src_ui.screen_img_2, LV_OBJ_FLAG_HIDDEN);
	lv_obj_add_flag(src_ui.screen_img_1, LV_OBJ_FLAG_HIDDEN);
	vTaskDelay(pdMS_TO_TICKS(1500));
	Custom_SetCurrentlyCont(2);
	xSemaphoreGive(ColorLoopSemap);
	vTaskDelete(NULL);
}

void Custom_UiSetupLoopTask(void *arg) {
	int Timer = 5;
	int Adc_Timer = 0;
	char StrSend[50] = {""};
	int is_SetupFlag = 1;
	xSemaphoreTake(ColorLoopSemap,portMAX_DELAY);
	for(;;) {
		if(Timer - Adc_Timer == 5) {
			Adc_Timer = Timer;
			float vol = batt_dev.Get_BatteryVoltage();
			snprintf(StrSend,50,"Voltage : %.2fV",vol);
      		lv_label_set_text(src_ui.screen_label_2, StrSend);
		}
		if(is_SetupFlag) {
			is_SetupFlag = 0;
			scan_dev.Scan_WifiSetup();
			vTaskDelay(pdMS_TO_TICKS(3500));
			uint16_t Wifi_Value = scan_dev.Scan_GetWifiValue();
			snprintf(StrSend,50,"Scan_WiFi : %d",Wifi_Value);
    		lv_label_set_text(src_ui.screen_label_3, StrSend);
			scan_dev.Scan_WifiDelete();
			lv_label_set_text(src_ui.screen_label_4, "Scan_BLE : pass");
			vTaskDelay(pdMS_TO_TICKS(500));
		}
		vTaskDelay(pdMS_TO_TICKS(200));
		Timer++;
	} 
}

void Custom_ButtonBootLoopTask(void *arg) {
	bool is_cont3Flag = 0;
	for(;;) {
		EventBits_t even = xEventGroupWaitBits(boot_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(2 * 1000));
    	if(get_bit_button(even,0)) {
			if(is_cont3Flag == 0) {
				Custom_SetCurrentlyCont(3);
				is_cont3Flag = 1;
			} else {
				is_cont3Flag = 0;
				Custom_SetCurrentlyCont(2);
			}
		} else if(get_bit_button(even,3)) {
			is_MusicFlag = 1;
		} else {
			is_MusicFlag = 0;
		}
	}
}

void Custom_ButtonPWRLoopTask(void *arg) {
	bool is_cont4Flag = 0;
	for(;;) {
		EventBits_t even = xEventGroupWaitBits(pwr_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(2 * 1000));
    	if(get_bit_button(even,0)) {
			if(is_cont4Flag == 0) {
				Custom_SetCurrentlyCont(4);
				is_cont4Flag = 1;
			} else {
				is_cont4Flag = 0;
				Custom_SetCurrentlyCont(2);
			}
		} else if(get_bit_button(even,2)) {
			batt_dev.Set_VbatPowerOFF();
		}
	}
}

void Custom_AudioLoopTask(void *arg) {
	uint8_t *rec_data_ptr = (uint8_t *)heap_caps_malloc(512 * sizeof(uint8_t), MALLOC_CAP_DEFAULT);
	audio_dev->I2sAudio_SetMicGain(20);
	audio_dev->I2sAudio_SetSpeakerVol(90);
	audio_dev->I2sAudio_SetCodecInfo("mic&spk",1,24000,2,16);
	int bytes_sizt = audio_dev->I2sAudio_GetPcmSize();
	for(;;) {
		if(is_MusicFlag) {
			size_t bytes_write = 0;
			uint8_t *data_ptr = audio_dev->I2sAudio_GetPcmBuffer();
			while ((bytes_write < bytes_sizt) && (is_MusicFlag)) {
      		  audio_dev->I2sAudio_PlayWrite(data_ptr, 256);
      		  data_ptr += 256;
      		  bytes_write += 256;
      		}
		} else {
			while (!is_MusicFlag) {
				if(ESP_CODEC_DEV_OK == audio_dev->I2sAudio_EchoRead(rec_data_ptr, 512)) {
        		  	audio_dev->I2sAudio_PlayWrite(rec_data_ptr, 512);
        		}
			}
		}
		vTaskDelay(pdMS_TO_TICKS(100));
	}
}

void Custom_TouchLoopTest(uint16_t x,uint16_t y) {
  	char str[12] = {""};
  	snprintf(str,11,"(%hd,%hd)",x,y);
  	lv_label_set_text(src_ui.screen_label_10, str);
}

void user_app_init(void) {
	batt_dev.Set_VbatPowerON();
	while (batt_dev.Get_isVbatPowerState() == 0) {
		vTaskDelay(pdMS_TO_TICKS(50));
	}
	scan_dev.ScanWirelessBsp_Init();
	user_button_init();
	audio_dev = new I2sAudioCodec("S3_AMOLED_1_32");
}

static void Custom_LvglLoopEvent(lv_event_t *e) {
	lv_event_code_t code = lv_event_get_code(e);
	lv_obj_t * module = e->current_target;
	switch (code) {
		case LV_EVENT_CLICKED: {
    	  	if(module == src_ui.screen_slider_1)
    	  	{
    	  	  	uint8_t value = lv_slider_get_value(module);
    	  	  	Lcd_SetBacklight(value);
    	  	}
    	  	break;
    	}
    	default:
    	  	break;
	}
}

void user_ui_init(void) {
	ColorLoopSemap = xSemaphoreCreateBinary();
	setup_ui(&src_ui);
	xTaskCreatePinnedToCore(Custom_ColorLoopTask, "Custom_ColorLoopTask", 3 * 1024, NULL, 2, NULL,0);
	xTaskCreatePinnedToCore(Custom_UiSetupLoopTask, "Custom_UiSetupLoopTask", 4 * 1024, NULL, 2, NULL,0);
	xTaskCreatePinnedToCore(Custom_ButtonBootLoopTask, "Custom_ButtonBootLoopTask", 4 * 1024, NULL, 2, NULL,0);
	xTaskCreatePinnedToCore(Custom_ButtonPWRLoopTask, "Custom_ButtonPWRLoopTask", 4 * 1024, NULL, 2, NULL,0);
	xTaskCreatePinnedToCore(Custom_AudioLoopTask, "Custom_AudioLoopTask", 4 * 1024, NULL, 2, NULL,1);
	lv_obj_add_event_cb(src_ui.screen_slider_1, Custom_LvglLoopEvent, LV_EVENT_ALL, NULL);
}

