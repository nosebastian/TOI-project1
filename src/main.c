#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "nvs_flash.h"


#include "driver/gpio.h"


#define GPIO_LED_RED  2
#define GPIO_LED1     13
#define GPIO_LED2     12
#define GPIO_LED3     14


void hello_task(void *pvParameter)
{
    printf("Hello world!\n");
    for (int i = 1; 1 ; i++) {
        printf("Running %d seconds...\n", i);
        vTaskDelay(1000 / portTICK_RATE_MS);
    }
}

void blink_task(void *pvParameter) {
    gpio_pad_select_gpio(GPIO_LED_RED);
    gpio_pad_select_gpio(GPIO_LED1);
    gpio_pad_select_gpio(GPIO_LED2);
    gpio_pad_select_gpio(GPIO_LED3);


    /* Set the GPIO as a push/pull output */
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED_RED, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED1, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED2, GPIO_MODE_OUTPUT));
    ESP_ERROR_CHECK(gpio_set_direction(GPIO_LED3, GPIO_MODE_OUTPUT));

    int cnt = 0;

    while(1) {
        /* Blink off (output low) */
        ESP_ERROR_CHECK(gpio_set_level(GPIO_LED_RED, cnt & 0x01));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_LED1, cnt & 0x01));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_LED2, cnt & 0x02));
        ESP_ERROR_CHECK(gpio_set_level(GPIO_LED3, cnt & 0x04));
        vTaskDelay(500 / portTICK_RATE_MS);
        cnt++;
    }
}
void app_main() {
    ESP_ERROR_CHECK(nvs_flash_init());
    xTaskCreate(&hello_task, "hello_task", 2048, NULL, 5, NULL);
    xTaskCreate(&blink_task, "blink_task", 512, NULL, 5, NULL);
}