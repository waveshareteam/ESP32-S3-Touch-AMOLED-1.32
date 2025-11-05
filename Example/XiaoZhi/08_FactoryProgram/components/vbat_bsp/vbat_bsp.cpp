#include <stdio.h>
#include "vbat_bsp.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static adc_cali_handle_t cali_handle;
static adc_oneshot_unit_handle_t adc1_handle;

BatteryMonitor::BatteryMonitor(int adc_channel,int vbat_enpin,int vbat_keypin,int batt_pin) : adc_channel_(adc_channel) ,
vbat_enpin_(vbat_enpin) ,
vbat_keypin_(vbat_keypin), 
batt_pin_(batt_pin) {
	adc_cali_curve_fitting_config_t cali_config = {};
    	cali_config.unit_id = ADC_UNIT_1;
    	cali_config.atten = ADC_ATTEN_DB_12;
    	cali_config.bitwidth = ADC_BITWIDTH_12;
  	ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_config, &cali_handle));

	adc_oneshot_unit_init_cfg_t init_config1 = {};
    	init_config1.unit_id = ADC_UNIT_1;
  	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
  	adc_oneshot_chan_cfg_t config = {};
    	config.bitwidth = ADC_BITWIDTH_12;            
    	config.atten = ADC_ATTEN_DB_12;
  	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, (adc_channel_t)adc_channel_, &config));

	gpio_config_t gpio_conf = {};
	if(batt_pin_ != GPIO_NUM_NC) {
  		gpio_conf.intr_type = GPIO_INTR_DISABLE;
  		gpio_conf.mode = GPIO_MODE_INPUT;
  		gpio_conf.pin_bit_mask = (0x1ULL<<batt_pin_);
  		gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  		gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  		ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
	}

	if(vbat_enpin_ != GPIO_NUM_NC) {
  		gpio_conf.intr_type = GPIO_INTR_DISABLE;
  		gpio_conf.mode = GPIO_MODE_OUTPUT;
  		gpio_conf.pin_bit_mask = (0x1ULL<<vbat_enpin_);
  		gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  		gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  		ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
	}

	if(vbat_keypin_ != GPIO_NUM_NC) {
  		gpio_conf.intr_type = GPIO_INTR_DISABLE;
  		gpio_conf.mode = GPIO_MODE_INPUT;
  		gpio_conf.pin_bit_mask = (0x1ULL<<vbat_keypin_);
  		gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  		gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  		ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
	}
}

BatteryMonitor::~BatteryMonitor() {

}

float BatteryMonitor::Get_BatteryVoltage() {
	int value;
  	int tage = 0;
    float vol = 0;
  	esp_err_t err;
  	err = adc_oneshot_read(adc1_handle,(adc_channel_t)adc_channel_,&value);
  	if(err == ESP_OK) {
    	adc_cali_raw_to_voltage(cali_handle,value,&tage);
    	vol = 0.001 * tage * 2;
	}
	return vol;
}

int BatteryMonitor::Get_isBatteryCharg() {
	if(batt_pin_ != GPIO_NUM_NC) {
		return (gpio_get_level((gpio_num_t)batt_pin_) ? 0 : 1);
	}
	return -1;
}

void BatteryMonitor::Set_VbatPowerON() {
	gpio_set_level((gpio_num_t)vbat_enpin_,1);
}

void BatteryMonitor::Set_VbatPowerOFF() {
	gpio_set_level((gpio_num_t)vbat_enpin_,0);
}

int BatteryMonitor::Get_isVbatPowerState() {
	if(vbat_keypin_ != GPIO_NUM_NC) {
		return gpio_get_level((gpio_num_t)vbat_keypin_);
	}
	return 1;
}