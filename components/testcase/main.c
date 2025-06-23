/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "include/machine.h"
#include "machine.h"
#include "i2c_beltsensor.h"
#include "brushedmotor.h"
#include "driver/i2c_master.h"


void app_main(void)
{
    printf("Hello world!\n");

    /* Print chip information */
    esp_chip_info_t chip_info;
    uint32_t flash_size;
    esp_chip_info(&chip_info);
    printf("This is %s chip with %d CPU core(s), %s%s%s%s, ",
           CONFIG_IDF_TARGET,
           chip_info.cores,
           (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "WiFi/" : "",
           (chip_info.features & CHIP_FEATURE_BT) ? "BT" : "",
           (chip_info.features & CHIP_FEATURE_BLE) ? "BLE" : "",
           (chip_info.features & CHIP_FEATURE_IEEE802154) ? ", 802.15.4 (Zigbee/Thread)" : "");

    unsigned major_rev = chip_info.revision / 100;
    unsigned minor_rev = chip_info.revision % 100;
    printf("silicon revision v%d.%d, ", major_rev, minor_rev);
    if(esp_flash_get_size(NULL, &flash_size) != ESP_OK) {
        printf("Get flash size failed");
        return;
    }

    printf("%" PRIu32 "MB %s flash\n", flash_size / (uint32_t)(1024 * 1024),
           (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "embedded" : "external");

    printf("Minimum free heap size: %" PRIu32 " bytes\n", esp_get_minimum_free_heap_size());
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    maslow_initialize(); // Initialize the machine
    enable_cooling_fan(true); // Turn on the cooling fan
    /*
     vTaskDelay(10000 / portTICK_PERIOD_MS);
     brushedmotor_set_speed(2, 1, 128); 
     vTaskDelay(1000 / portTICK_PERIOD_MS);
    brushedmotor_set_speed(2, 1, 0); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);
      brushedmotor_set_speed(3, 1, 128); 
     vTaskDelay(1000 / portTICK_PERIOD_MS);
    brushedmotor_set_speed(3, 1, 0); 
    vTaskDelay(1000 / portTICK_PERIOD_MS);   
    */
    for (int i = 180; i >= 0; i--) {
        printf("Restarting in %d seconds...\n", i);
        printf("ADC Readings: BL: %d, BR: %d, TR: %d, TL: %d\n",
               adc_last_avg_readings[0],
               adc_last_avg_readings[1],
               adc_last_avg_readings[2],
               adc_last_avg_readings[3]);
       const int32_t *belt_positions = getBeltPositions();
        printf("Belt Positions: AS5600_1: %ld, AS5600_2: %ld, AS5600_3: %ld, AS5600_4: %ld\n",
               belt_positions[0],
               belt_positions[1],
               belt_positions[2],
               belt_positions[3]);  
               
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
    printf("Restarting now.\n");
    fflush(stdout);
    esp_restart();
}
