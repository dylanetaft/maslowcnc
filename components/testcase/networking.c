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
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>


#define PORT 23

int _listen_sock = -1; // Socket for listening to incoming connections

void initialize_wifi();
void initialize_socket();
TaskHandle_t server_taskhandle = NULL; // Handle for the server task

void server_task(void *param) {
    assert(listen(_listen_sock, 1) == 0); // Start listening for incoming connections
    struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
    socklen_t addr_len = sizeof(source_addr);
    while (1) {
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            continue; // If accept fails, continue to the next iteration
        }
    }

}


void initialize_socket() {
        int ip_protocol = IPPROTO_IPV6;
        int addr_family = AF_INET6; // Use IPv6 address family
        struct sockaddr_storage dest_addr;

        struct sockaddr_in6 *dest_addr_ip6 = (struct sockaddr_in6 *)&dest_addr;
        bzero(&dest_addr_ip6->sin6_addr.un, sizeof(dest_addr_ip6->sin6_addr.un));
        dest_addr_ip6->sin6_family = AF_INET6;
        dest_addr_ip6->sin6_port = htons(PORT);
        _listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
        assert(_listen_sock >= 0);
        int no = 0;
        assert((setsockopt(_listen_sock, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) == 0));
        assert(setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt) == 0));

        struct sockaddr_in6 *bind_addr = (struct sockaddr_in6 *)&dest_addr;
        assert((bind(_listen_sock, (struct sockaddr *)bind_addr, sizeof(*bind_addr)) == 0));
        xTaskCreate(server_taskhandle, "tcp_server", 8096, NULL , 5, NULL);
}

void initialize_networking() {
    initialize_wifi();
    initialize_socket();
}
void initialize_wifi() {
  wifi_config_t wifi_config = {
        .ap = {
            .ssid = DEFAULT_WIFI_SSID,
            .ssid_len = strlen(DEFAULT_WIFI_SSID),
            .channel = DEFAULT_WIFI_CHANNEL,
            .password = DEFAULT_WIFI_PASS,
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