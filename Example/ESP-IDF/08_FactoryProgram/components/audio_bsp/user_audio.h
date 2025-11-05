#ifndef USER_AUDIO_BSP_H
#define USER_AUDIO_BSP_H

#include "codec_board.h"
#include "codec_init.h"

class I2sAudioCodec
{
private:
    esp_codec_dev_handle_t playback = NULL;
    esp_codec_dev_handle_t record = NULL;

    static void I2sAudio_MusicTask(void *arg);
    static void I2sAudio_EchoTask(void *arg);
public:
    I2sAudioCodec(const char *strName);
    ~I2sAudioCodec();

    void I2sAudio_SetSpeakerVol(int vol);
    void I2sAudio_SetMicGain(float db_value);

    void I2sAudio_SetCodecInfo(const char *strName,int open_en,int sample_rate,int channel,int bits_per_sample);
    void I2sAudio_CloseSpeaker(void);
    void I2sAudio_CloseMic(void);
    int I2sAudio_PlayWrite(void *ptr,int ptr_len);
    int I2sAudio_EchoRead(void *ptr,int ptr_len);

    void I2sAudio_CreateMusicTask(void);
    void I2sAudio_CreateEchoTask(void);

    int I2sAudio_GetPcmSize(void);
    uint8_t* I2sAudio_GetPcmBuffer();
};

#endif // !MY_ADF_BSP_H
