#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "networking.h"



void initialize_wifi();



void initialize_networking() {
    initialize_wifi();
}
void initialize_wifi() {
  wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_ESP_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_ESP_WIFI_SSID),
            .channel = DEFAULT_ESP_WIFI_CHANNEL,
            .password = DEFAULT_ESP_WIFI_PASS,
            .max_connection = DEFAULT_MAX_STA_CONN,
#ifdef CONFIG_ESP_WIFI_SOFTAP_SAE_SUPPORT
            .authmode = WIFI_AUTH_WPA3_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
#else            
            .authmode = WIFI_AUTH_WPA2_PSK,
#endif
            .pmf_cfg = {
                    .required = true,
            },
        },
    };

    //initialize dhcp infrastructure
    ESP_ERROR_CHECK(esp_netif_init());                      // Initialize network stack
    ESP_ERROR_CHECK(esp_event_loop_create_default());       // Required for Wi-Fi and netif events
    esp_netif_t *netif_ap = esp_netif_create_default_wifi_ap();                     // Creates AP netif + DHCP server

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}