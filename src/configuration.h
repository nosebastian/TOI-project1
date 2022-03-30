#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

#define GPIO_DS18B20_0      4
#define MAX_DEVICES         8
#define DS18B20_RESOLUTION  DS18B20_RESOLUTION_12_BIT
#define SAMPLE_PERIOD       1000   // milliseconds
#define GPIO_PHOTORESISTOR  2

// ADC vars and consts
#define DEFAULT_VREF    1100        // Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          // Multisampling
#define R               10000       // Resistor in Ohms
#define VIN             3300        // Input voltage in mV

#define CONFIG_ESPNOW_CHANNEL 0
#define CONFIG_ESPNOW_LMK "lmk1234567890123"
#define CONFIG_ESPNOW_PMK "pmk1234567890123"
#define CONFIG_ESPNOW_SEND_COUNT 100
#define CONFIG_ESPNOW_SEND_DELAY 1000
#define CONFIG_ESPNOW_SEND_LEN 200
#define CONFIG_ESPNOW_WIFI_MODE

#define CONFIG_ESP_MAXIMUM_RETRY 100
#define CONFIG_ESP_WIFI_SSID "rpiIotGateway"
#define CONFIG_ESP_WIFI_PASS "abcdefgh"
/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define CONFIG_ESP_WIFI_CONNECTED_BIT BIT0
#define CONFIG_ESP_WIFI_FAIL_BIT      BIT1


#define CONFIG_TAG "esp_now_toi"
#define CONFIG_HOTSPOT_IS_1 // or CONFIG_HOTSPOT_IS_2
//#define CONFIG_IS_GATEWAY

#define CONFIG_BROKER_URL "mqtt://10.10.0.1"

#endif
