#include <stdio.h>
#include "i2c_bsp.h"
#include "lcd_touch_bsp.h"
#include "driver/gpio.h"
#include "user_config.h"
#include "esp_log.h"
#include "user_app.h"
#include "button_bsp.H"
#include "gui_guider.h"
#include "user_audio.h"
#include "lvgl_port_bsp.h"

I2cMasterBus i2c_dev(ESP32_SCL_NUM,ESP32_SDA_NUM,0);
LcdTouchPanel touch_dev(i2c_dev,TOUCH_ADDR);
I2sAudioCodec *audio_dev = NULL;

static int is_MusicFlag = 0;

static lv_ui src_ui;

LcdTouchPanel Custom_GetLcdTouchPanel(void) {
	return touch_dev;
}

void Custom_ButtonBootLoopTask(void *arg) {
	bool is_cont3Flag = 0;
	for(;;) {
		EventBits_t even = xEventGroupWaitBits(boot_groups,set_bit_all,pdTRUE,pdFALSE,pdMS_TO_TICKS(200));
    	if(get_bit_button(even,3)) {
			if(!is_MusicFlag) {
				is_MusicFlag = 1;
				lv_label_set_text(src_ui.screen_label_1, "Music Mode");
			}
		} else {
			if(is_MusicFlag) {
				is_MusicFlag = 0;
				lv_label_set_text(src_ui.screen_label_1, "Echo Mode");
			}
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

void user_app_init(void) {
	user_button_init();
	audio_dev = new I2sAudioCodec("S3_AMOLED_1_32");
}

void user_ui_init(void) {
	setup_ui(&src_ui);
	lv_label_set_text(src_ui.screen_label_1, "Echo Mode");
	xTaskCreatePinnedToCore(Custom_ButtonBootLoopTask, "Custom_ButtonBootLoopTask", 4 * 1024, NULL, 2, NULL,0);
	xTaskCreatePinnedToCore(Custom_AudioLoopTask, "Custom_AudioLoopTask", 4 * 1024, NULL, 2, NULL,1);
}

