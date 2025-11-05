#ifndef ADC_BSP_H
#define ADC_BSP_H

#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"

class BatteryMonitor
{
private:
    int adc_channel_;
    int vbat_enpin_ = GPIO_NUM_NC;
    int vbat_keypin_ = GPIO_NUM_NC;
    int batt_pin_ = GPIO_NUM_NC;
    int value;
public:
    BatteryMonitor(int adc_channel,int vbat_enpin = GPIO_NUM_NC,int vbat_keypin = GPIO_NUM_NC,int batt_pin = GPIO_NUM_NC);
    ~BatteryMonitor();

    float Get_BatteryVoltage();
    int Get_BatteryValue();
    int Get_isBatteryCharg();

    void Set_VbatPowerON();
    void Set_VbatPowerOFF();

    int Get_isVbatPowerState();
};

#endif