#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "configuration.h"

void adc_init(void);

void print_char_val_type(esp_adc_cal_value_t val_type);
void check_efuse(void);

int light_intensity_task();

int raw_to_lumens(int raw);

void onewire_task();

float get_temperature(DS18B20_Info *dev);

#endif
