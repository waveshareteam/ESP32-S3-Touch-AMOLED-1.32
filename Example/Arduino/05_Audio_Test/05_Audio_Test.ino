#include "user_config.h"
#include "esp_err.h"
#include "src/ui_src/generated/gui_guider.h"
#include "lvgl_port.h"
#include "src/audio_bsp/user_audio.h"

I2sAudioCodec *audio_dev = NULL;

static lv_ui src_ui;

void Custom_AudioLoopTask(void *arg) {
	uint8_t *rec_data_ptr = (uint8_t *)heap_caps_malloc(512 * sizeof(uint8_t), MALLOC_CAP_DEFAULT);
	audio_dev->I2sAudio_SetMicGain(20);
	audio_dev->I2sAudio_SetSpeakerVol(90);
	audio_dev->I2sAudio_SetCodecInfo("mic&spk",1,24000,2,16);
	for(;;) {
		if(ESP_CODEC_DEV_OK == audio_dev->I2sAudio_EchoRead(rec_data_ptr, 512)) {
      audio_dev->I2sAudio_PlayWrite(rec_data_ptr, 512);
		}
	}
}

void user_ui_init(void) {
	setup_ui(&src_ui);
	lv_label_set_text(src_ui.screen_label_1, "Echo Mode");
	xTaskCreatePinnedToCore(Custom_AudioLoopTask, "Custom_AudioLoopTask", 4 * 1024, NULL, 2, NULL,1);
}

void setup() {
	audio_dev = new I2sAudioCodec("S3_AMOLED_1_32");
  
  Lvgl_PortInit();
  if (lvgl_lock(0)) {
      user_ui_init();
      lvgl_unlock();
  }
}

void loop() {

}
