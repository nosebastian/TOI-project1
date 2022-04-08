#ifndef __MQTT_H__
#define __MQTT_H__

void mqtt_init(void);

typedef struct measurement {
    float temp;
    int light;
} measurement_t;

void mqtt_publish_temp_light(char *dev, measurement_t measurement);

#endif