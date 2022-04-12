/* NOTES --------------
  * 10k resistor (DS18B20 & photoresistor)
  * PR on pin 34 (ADC1)
  * DS on pin 4
-----------------------*/

#include "sensors.h"

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "driver/gpio.h"

#include "configuration.h"

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_10;
static const adc_atten_t atten = ADC_ATTEN_DB_11;       // ADC_ATTEN_DB_2_5, ADC_ATTEN_DB_6, ADC_ATTEN_DB_11, ADC_ATTEN_MAX
static const adc_unit_t unit = ADC_UNIT_1;

int raw_to_lumens(int raw)
{
    uint32_t vout = esp_adc_cal_raw_to_voltage(raw, adc_chars);

    float RLDR = (R * (VIN - vout))/vout;

    int lumens = 500/(RLDR/1000);

    return lumens;
}

int get_light_intensity(void)
{
    int light_intensity = 0;
    uint32_t adc_reading = 0;
    
    //Multisampling
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
        if (unit == ADC_UNIT_1) {
            adc_reading += adc1_get_raw((adc1_channel_t)channel);
        } else {
            int raw;
            adc2_get_raw((adc2_channel_t)channel, width, &raw);
            adc_reading += raw;
        }
    }
    adc_reading /= NO_OF_SAMPLES;

    light_intensity = raw_to_lumens(adc_reading);
    
    return light_intensity;
}

void adc_init(void)
{
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);

    adc1_config_width(width);
    adc1_config_channel_atten(channel, atten);
}

float get_temperature(DS18B20_Info * dev){
    ds18b20_convert(dev);

    ds18b20_wait_for_conversion(dev);
    
    float temp = 0;
    ds18b20_read_temp(dev, &temp);
    return temp;
}