#include "devices.h"

int init_device(device_t *dev)
{
    if (dev == NULL){
        return -1;
    }
    dev->number = 255;
    for (uint8_t i = 0; i < MAC_LENGHT; i++)
    {
        dev->mac_addr[i] = 0;
    }
    for (uint8_t i = 0; i < CONFIG_NAME_LENGHT; i++)
    {
        dev->name[i] = '\0';
    }

    return 0;
}

int set_device(device_t *dev, const uint8_t mac_addr[MAC_LENGHT], const uint8_t num)
{
    ESP_LOGI("ESP-NOW-DEVICES", "Initializing dev %d", num);
    init_device(dev);

    sprintf(dev->name, "/esp/%d", num);
    for (uint8_t i = 0; i < MAC_LENGHT; i++)
    {
        dev->mac_addr[i] = mac_addr[i];
    }
    dev->number = num;

    ESP_LOGI("ESP-NOW-DEVICES", "Setting(adding) %d device to name: %s and MAC: "MACSTR, dev->number, dev->name, MAC2STR(dev->mac_addr));

    return 0;
}

bool check_mac(const uint8_t *mac_addr_1, const uint8_t *mac_addr_2)
{
    for (uint8_t i = 0; i < MAC_LENGHT; i++)
    {
        if (mac_addr_1[i] != mac_addr_2[i]){
            return false;
        }
    }
    return true;
}

// returns index of dev, or -1
int check_in_devices(device_t devices[CONFIG_MAX_DEVICES], const uint8_t *mac_addr)
{
    for (uint8_t i = 0; i < CONFIG_MAX_DEVICES; i++)
    {
        if(check_mac(devices[i].mac_addr, mac_addr)){
            return i;
        }
    }
    return -1;
}