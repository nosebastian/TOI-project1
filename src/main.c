#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <assert.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_now.h"
#include "esp_crc.h"

#include "configuration.h"
#include "devices.h"
#include "mqtt.h"
#include "sensors.h"

#ifdef CONFIG_IS_GATEWAY
uint8_t actual_dev = CONFIG_DEFAULT_MIN_ID;
device_t devices[CONFIG_MAX_DEVICES];

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

static wifi_config_t wifi_config = {
    .sta = {
        .ssid = CONFIG_ESP_WIFI_SSID,
        .password = CONFIG_ESP_WIFI_PASS,
        .threshold.authmode = WIFI_AUTH_WPA2_PSK,

        .pmf_cfg = {
            .capable = true,
            .required = false
        },
    },
};

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(WIFI_CONNECT_TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, CONFIG_ESP_WIFI_FAIL_BIT);
        }
        ESP_LOGI(WIFI_CONNECT_TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(WIFI_CONNECT_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, CONFIG_ESP_WIFI_CONNECTED_BIT);
    }
}
#endif

static void espnow_send_cb(const uint8_t *mac_addr, esp_now_send_status_t status)
{
    uint8_t primary;
    wifi_second_chan_t second;

    esp_wifi_get_channel(&primary, &second);
    ESP_LOGI(ESP_NOW_TAG, "Sending data on chanel p: %hhi s: %hhi", primary, second);
}

static void espnow_recv_cb(const uint8_t *mac_addr, const uint8_t *data, int len)
{
#ifdef CONFIG_IS_GATEWAY
    bool exist = false;
    int idx = check_in_devices(devices, mac_addr);
    if (idx == -1){
        ESP_LOGI("ESP-NOW-DEVICES", "New device detected");
        if((actual_dev-2) >= CONFIG_MAX_DEVICES){
            ESP_LOGI("ESP-NOW-DEVICES", "Can't add new dev /esp%d, index: %d, slots: %d", actual_dev, (actual_dev-2), CONFIG_MAX_DEVICES);
            exist = false;
        } else {
            idx = actual_dev-2;
            set_device(&devices[idx], mac_addr, actual_dev);
            actual_dev++;
            exist = true;
        }
    } else {
        exist = true;
    }

    if(exist){
        measurement_t *recieved_data = (measurement_t *)data;
        ESP_LOGI(ESP_NOW_TAG, "Received [%d B]: light: %d, temp: %f From: "MACSTR, len, recieved_data->light, recieved_data->temp, MAC2STR(mac_addr));

        mqtt_publish_temp_light(devices[idx].name, *recieved_data);
    }
#endif
}

void add_peer(const uint8_t mac_addr[ESP_NOW_ETH_ALEN])
{
    esp_now_peer_info_t *peer = malloc(sizeof(esp_now_peer_info_t));
    memset(peer, 0, sizeof(esp_now_peer_info_t));
    peer->channel = CONFIG_ESPNOW_CHANNEL;
    peer->ifidx = ESP_IF_WIFI_STA;
    peer->encrypt = false;
    memcpy(peer->peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    ESP_ERROR_CHECK( esp_now_add_peer(peer) );
    free(peer);
}

void wifi_init()
{
    ESP_ERROR_CHECK(esp_netif_init());
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
#ifdef CONFIG_IS_GATEWAY
    s_wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
#endif
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
#ifdef CONFIG_IS_GATEWAY
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_APSTA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
#endif
    ESP_ERROR_CHECK( esp_wifi_start());
}

void espnow_init()
{
    ESP_ERROR_CHECK( esp_now_init() );
    ESP_ERROR_CHECK( esp_now_register_send_cb(espnow_send_cb) );
    ESP_ERROR_CHECK( esp_now_register_recv_cb(espnow_recv_cb) );
    //ESP_ERROR_CHECK( esp_now_set_pmk((uint8_t *)CONFIG_ESPNOW_PMK) );
}

void app_main() 
{
    ESP_ERROR_CHECK(nvs_flash_init());

#ifndef DUMMY_DEVICE
    // OWB init =============================
    vTaskDelay(2000.0 / portTICK_PERIOD_MS);

    // Create a 1-Wire bus, using the RMT timeslot driver
    OneWireBus * owb;
    owb_rmt_driver_info rmt_driver_info;
    owb = owb_rmt_initialize(&rmt_driver_info, GPIO_DS18B20_0,
                            RMT_CHANNEL_1, RMT_CHANNEL_0);
    owb_use_crc(owb, true);  // enable CRC check for ROM code

    DS18B20_Info * dev = ds18b20_malloc();
    ds18b20_init_solo(dev, owb);
    ds18b20_use_crc(dev, true);
    ds18b20_set_resolution(dev, DS18B20_RESOLUTION);
    // end of OWB init ======================

    adc_init();
#endif

    wifi_init();
    espnow_init();

#ifdef CONFIG_IS_GATEWAY
    mqtt_init();

    //set_device(&devices[0], slave_mac, CONFIG_DEFAULT_MIN_ID);
    //ESP_LOGI("ESP-NOW-DEVICES", "Added device %d, "MACSTR", %s", devices[0].number, MAC2STR(devices[0].mac_addr), devices[0].name);
#else
    add_peer(master_mac);
#endif

    measurement_t measurement;
    for(;;)
    {
#ifndef DUMMY_DEVICE
        measurement.light = get_light_intensity();
        measurement.temp = get_temperature(dev);
#else
        measurement.light = 28;
        measurement.temp = 25.5;
#endif

#ifdef CONFIG_IS_GATEWAY
        mqtt_publish_temp_light("/esp/1", measurement);
#else
        ESP_LOGI(ESP_NOW_TAG,"Sending...temp: %f, light: %d", measurement.temp, measurement.light);
        esp_err_t result = esp_now_send(master_mac, (uint8_t *)(&measurement), sizeof(measurement_t));
        if (result == ESP_OK) {
            printf("Sent with success\n");
        } else {
            printf("Error sending the data\n");
        }
#endif
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}