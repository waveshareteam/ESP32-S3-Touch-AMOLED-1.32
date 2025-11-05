#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "user_audio.h"
#include "esp_log.h"
#include "esp_err.h"

#include "esp_gmf_init.h"

extern const uint8_t music_pcm_start[] asm("_binary_canon_pcm_start");
extern const uint8_t music_pcm_end[]   asm("_binary_canon_pcm_end");

void I2sAudioCodec::I2sAudio_MusicTask(void *arg) {
	I2sAudioCodec *codec = (I2sAudioCodec *)arg;
	codec->I2sAudio_SetSpeakerVol(80);
	for(;;) {
		size_t bytes_write = 0;
  	  	size_t bytes_sizt = music_pcm_end - music_pcm_start;
  	  	uint8_t *data_ptr = (uint8_t *)music_pcm_start;
		codec->I2sAudio_SetCodecInfo("es8311",1,24000,2,16);
		do
		{
			codec->I2sAudio_PlayWrite(data_ptr, 256);
  	  	  	data_ptr += 256;
  	  	  	bytes_write += 256;
		} while (bytes_write < bytes_sizt);
	}
}

void I2sAudioCodec::I2sAudio_EchoTask(void *arg) {
	I2sAudioCodec *codec = (I2sAudioCodec *)arg;
	codec->I2sAudio_SetSpeakerVol(80);
	codec->I2sAudio_SetMicGain(25);
	uint8_t *data_ptr = (uint8_t *)heap_caps_malloc(1024 * sizeof(uint8_t), MALLOC_CAP_SPIRAM);
	codec->I2sAudio_SetCodecInfo("es8311 & es7210",1,24000,2,16);
	for(;;)
  	{
  	  	if(ESP_CODEC_DEV_OK == codec->I2sAudio_EchoRead(data_ptr, 1024))
  	  	{
  	  	  	codec->I2sAudio_PlayWrite(data_ptr, 1024);
  	  	}
  	}
}

I2sAudioCodec::I2sAudioCodec(const char *strName)
{
	set_codec_board_type(strName);
  	codec_init_cfg_t codec_cfg = {};
		codec_cfg.in_mode = CODEC_I2S_MODE_STD;
		codec_cfg.out_mode = CODEC_I2S_MODE_STD;
		codec_cfg.in_use_tdm = false;
		codec_cfg.reuse_dev = false;
  	ESP_ERROR_CHECK(init_codec(&codec_cfg));
  	playback = get_playback_handle();
  	record = get_record_handle();
}

I2sAudioCodec::~I2sAudioCodec()
{

}

void I2sAudioCodec::I2sAudio_SetCodecInfo(const char *strName,int open_en,int sample_rate,int channel,int bits_per_sample) {
	esp_codec_dev_sample_info_t fs = {};
    	fs.sample_rate = sample_rate;
    	fs.channel = channel;
    	fs.bits_per_sample = bits_per_sample;
	if(open_en) {
		if(!strcmp(strName,"es8311")) {
			esp_codec_dev_open(playback, &fs);
		} else if(!strcmp(strName,"es7210")) {
			esp_codec_dev_open(record, &fs);
		} else {
			esp_codec_dev_open(playback, &fs);
  			esp_codec_dev_open(record, &fs);  
		}
	}
}

void I2sAudioCodec::I2sAudio_SetSpeakerVol(int vol) {
	esp_codec_dev_set_out_vol(playback, vol);
}

void I2sAudioCodec::I2sAudio_SetMicGain(float db_value) {
	esp_codec_dev_set_in_gain(record, db_value);
}

void I2sAudioCodec::I2sAudio_CloseSpeaker(void) {
	esp_codec_dev_close(playback);
}

void I2sAudioCodec::I2sAudio_CloseMic(void) {
	esp_codec_dev_close(record);
}

int I2sAudioCodec::I2sAudio_PlayWrite(void *ptr,int ptr_len) {
	return esp_codec_dev_write(playback, ptr, ptr_len);
}

int I2sAudioCodec::I2sAudio_EchoRead(void *ptr,int ptr_len) {
	return esp_codec_dev_read(record, ptr, ptr_len);
}

void I2sAudioCodec::I2sAudio_CreateMusicTask(void) {
	xTaskCreate(I2sAudio_MusicTask, "I2sAudio_MusicTask", 4 * 1024, (void *)this, 2, NULL);
}

void I2sAudioCodec::I2sAudio_CreateEchoTask(void) {
	xTaskCreate(I2sAudio_EchoTask, "I2sAudio_EchoTask", 4 * 1024, (void *)this, 2, NULL);
}

int I2sAudioCodec::I2sAudio_GetPcmSize() {
	return music_pcm_end - music_pcm_start;
}

uint8_t* I2sAudioCodec::I2sAudio_GetPcmBuffer() {
	return (uint8_t *)music_pcm_start;
}













/*
Board: AMOLED_1_43
i2c: {sda: 18, scl: 8}
i2s: {bclk: 21, ws: 22, dout: 23, din: 20, mclk: 19}
out: {codec: ES8311, pa: -1, use_mclk: 1, pa_gain:6}
in: {codec: ES7210}
"S3_LCD_2_33"
*/