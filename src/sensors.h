#ifndef __SENSORS_H__
#define __SENSORS_H__

#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "configuration.h"

void adc_init(void);

int get_light_intensity(void);

int raw_to_lumens(int raw);

float get_temperature(DS18B20_Info *dev);

#endif
