#ifndef WIFI_H
#define WIFI_H 
#define DEFAULT_WIFI_SSIS      "MASLOW_CNC"
#define DEFAULT_ESP_WIFI_PASS      "12345678"
#define DEFAULT_ESP_WIFI_CHANNEL   CONFIG_ESP_WIFI_CHANNEL
#define DEFAULT_MAX_STA_CONN       8
#define DEFAULT_ESP_WIFI_CHANNEL      6

void initialize_networking();
#endif