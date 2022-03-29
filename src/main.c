/* TODO ---------------
  * ESP-NOW P2P
  * ADC config
  * convert raw ADC to lumens???
  * WiFi RPi communication
-----------------------*/

/* NOTES --------------
  * 10k resistor (DS18B20 & photoresistor)
  * PR on pin 34 (ADC1)
  * DS on pin 4
-----------------------*/

#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "esp_system.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

#include "driver/gpio.h"

#include "driver/adc.h"
#include "esp_adc_cal.h"

#include "wifi.h"
#include "firmware.h"
//#include "esp-now-lib.h"

static esp_adc_cal_characteristics_t *adc_chars;
static const adc_channel_t channel = ADC_CHANNEL_6;     //GPIO34 if ADC1, GPIO14 if ADC2
static const adc_bits_width_t width = ADC_WIDTH_BIT_10;
static const adc_atten_t atten = ADC_ATTEN_DB_11;       // ADC_ATTEN_DB_2_5, ADC_ATTEN_DB_6, ADC_ATTEN_DB_11, ADC_ATTEN_MAX
static const adc_unit_t unit = ADC_UNIT_1;

// Shared data variables
int light_intensity = 0;
float temperature = 0.0f;

int raw_to_lumens(int raw){
    uint32_t vout = esp_adc_cal_raw_to_voltage(raw, adc_chars);

    float RLDR = (R * (VIN - vout))/vout;

    int lumens = 500/(RLDR/1000);

    return lumens;
}

static void check_efuse(void)
{
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK) {
        printf("eFuse Two Point: Supported\n");
    } else {
        printf("eFuse Two Point: NOT supported\n");
    }
    //Check Vref is burned into eFuse
    if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF) == ESP_OK) {
        printf("eFuse Vref: Supported\n");
    } else {
        printf("eFuse Vref: NOT supported\n");
    }
}

static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
        printf("Characterized using Two Point Value\n");
    } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
        printf("Characterized using eFuse Vref\n");
    } else {
        printf("Characterized using Default Vref\n");
    }
}

void light_intensity_task(void *pvParameter)
{
    while (1) {
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

        light_intensity = raw_to_lumens(adc_reading); // TODO transform to lumens
        //light_intensity = adc_reading;
        
        //Convert adc_reading to voltage in mV
        uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);

        printf("Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}

void onewire_task(void *pvParameter)
{
    // Stable readings require a brief period before communication
    vTaskDelay(2000.0 / portTICK_PERIOD_MS);

    // Create a 1-Wire bus, using the RMT timeslot driver
    OneWireBus * owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0,
                             RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    // Find all connected devices
    ESP_LOGI("one-wire", "Find devices:\n");
    OneWireBus_ROMCode device_rom_codes[MAX_DEVICES] = {0};
    int num_devices = 0;
    OneWireBus_SearchState search_state = {0};
    bool found = false;
    owb_search_first(owb, &search_state, &found);
    while (found)
    {
        char rom_code_s[17];
        owb_string_from_rom_code(search_state.rom_code, rom_code_s,
                                 sizeof(rom_code_s));
        printf("  %d : %s\n", num_devices, rom_code_s);
        device_rom_codes[num_devices] = search_state.rom_code;
        ++num_devices;
        owb_search_next(owb, &search_state, &found);
    }
    ESP_LOGI("one-wire", "Found %d device%s", num_devices, 
                         num_devices == 1 ? "" : "s");



    // Create DS18B20 devices on the 1-Wire bus
    DS18B20_Info * devices[MAX_DEVICES] = {0};
    for (int i = 0; i < num_devices; ++i)
    {
        DS18B20_Info * ds18b20_info = ds18b20_malloc();  
        // heap allocation
        devices[i] = ds18b20_info;

        if (num_devices == 1)
        {
            ESP_LOGI("one-wire", "Single device optimisations enabled");
            ds18b20_init_solo(ds18b20_info, owb);          
            // only one device on bus
        }
        else
        {
            ds18b20_init(ds18b20_info, owb, device_rom_codes[i]); 
            // associate with bus and device
        }
        ds18b20_use_crc(ds18b20_info, true);           
        // enable CRC check on all reads
        ds18b20_set_resolution(ds18b20_info, DS18B20_RESOLUTION);
    }

    // Read temperatures from all sensors sequentially
    while (1)
    {
        //ESP_LOGI("one-wire", "Temperature readings (degrees C):");
        ds18b20_convert_all(owb);

        // In this application all devices use the same resolution,
        // so use the first device to determine the delay
        ds18b20_wait_for_conversion(devices[0]);
        
        for (int i = 0; i < num_devices; ++i)
        {
            ds18b20_read_temp(devices[i], &temperature);
        }
        vTaskDelay(5000 / portTICK_PERIOD_MS); 
    }
}

void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());

    // wifi_init_sta(); /* uncomment this */
    
    // check ADC
    check_efuse();

    //Configure ADC
    if (unit == ADC_UNIT_1) {
        adc1_config_width(width);
        adc1_config_channel_atten(channel, atten);
    } else {
        adc2_config_channel_atten((adc2_channel_t)channel, atten);
    }

    //Characterize ADC
    adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, width, DEFAULT_VREF, adc_chars);
    print_char_val_type(val_type);

    xTaskCreate(&onewire_task, "one_wire_task", 2048, NULL, 5, NULL);
    xTaskCreate(&light_intensity_task, "light_intensity_task", 2048, NULL, 5, NULL);

    while(1){
        ESP_LOGI("light-temperature", " %d lm-%.3f degrees", light_intensity, temperature);
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}