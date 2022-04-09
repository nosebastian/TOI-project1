#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#include "owb.h"
#include "owb_rmt.h"
#include "ds18b20.h"

// OneWire Bus
#define GPIO_DS18B20_0      4
#define MAX_DEVICES         8
#define DS18B20_RESOLUTION  DS18B20_RESOLUTION_12_BIT
#define SAMPLE_PERIOD       1000   // milliseconds

// Photoresistor and ADC
#define GPIO_PHOTORESISTOR  2
#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Multisampling
#define R               10000       // Resistor in Ohms
#define VIN             3300        // Input voltage in mV

// ESP-NOW
#define CONFIG_ESPNOW_CHANNEL 0
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_SEND_COUNT 100
#define CONFIG_ESPNOW_SEND_DELAY 1000
#define CONFIG_ESPNOW_SEND_LEN 200
#define CONFIG_ESPNOW_WIFI_MODE

// Wifi
#define CONFIG_ESP_MAXIMUM_RETRY 100
#define CONFIG_ESP_WIFI_SSID "rpiIotGateway"
#define CONFIG_ESP_WIFI_PASS "abcdefgh"

#define CONFIG_ESP_WIFI_CONNECTED_BIT BIT0
#define CONFIG_ESP_WIFI_FAIL_BIT      BIT1

//Others
#define CONFIG_TAG "esp-now-toi"
#define ESP_NOW_TAG "esp-now"
#define MQTT_TAG "mqtt"
#define ESPNOW_ERROR_TAG "esp-now-err"
#define WIFI_TAG "wi-fi"
#define WIFI_CONNECT_TAG "wifi-connect"

// Comment this on second and more ESPs
//#define CONFIG_IS_GATEWAY

#define CONFIG_BROKER_URL "mqtt://10.10.0.1"

#endif
