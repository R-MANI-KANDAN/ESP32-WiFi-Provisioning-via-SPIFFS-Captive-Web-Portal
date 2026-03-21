#include <stdio.h>
#include "esp_log.h"
#include "spiffs.h"
#include "wifi.h"


static const char *TAG = "APP_MAIN";

void app_main(void)
{
    ESP_LOGI(TAG, "Application started");

    spiffs_initialize();
    spiffs_check();
    if (spiffs_file_not_empty("/spiffs/ssid.txt")) {
        ESP_LOGI(TAG, "ssid.txt exists and is not empty");
    }

    char ssid[32] = {0};
    char pwd[64]  = {0};

    load_wifi1(ssid, sizeof(ssid), pwd, sizeof(pwd));
    show_credentials();

   

    wifi_ap();
    

}
