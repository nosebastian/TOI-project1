#ifndef __DEVICES_H__
#define __DEVICES_H__

#include <stdint.h>
#include <stdio.h>
#include "esp_log.h"

#include "configuration.h"

#define MAC_LENGHT 6

typedef struct {
    uint8_t number;                      // dev rel. number
    uint8_t mac_addr[MAC_LENGHT]; // mac addr
    char name[CONFIG_NAME_LENGHT];       // short string to mqtt
} device_t;

int init_device(device_t *dev);

int set_device(device_t *dev, const uint8_t mac_addr[MAC_LENGHT], const uint8_t num);

int check_in_devices(device_t devices[CONFIG_MAX_DEVICES], const uint8_t *mac_addr);
#endif