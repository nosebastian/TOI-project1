#ifndef __PEER_NET_H__
#define __PEER_NET_H__
#include <stdint.h>

void wifi_init(void);
void espnow_init(void);
void peer_net_task(void *pvParameter);
void add_peer(const uint8_t * mac_addr);

#endif