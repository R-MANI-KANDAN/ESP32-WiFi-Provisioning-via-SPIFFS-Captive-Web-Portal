// wifi_ap.c

#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "nvs_flash.h"

#include "driver/gpio.h"
#include "esp_http_server.h"
#include "cJSON.h"

#include "tcpip_adapter.h"
#include "lwip/err.h"
#include "lwip/sys.h"

// SPIFFS
#include "spiffs.h"

/* ---------------- GPIO ---------------- */
#define LED_G 2

/* ---------------- WiFi ---------------- */
static EventGroupHandle_t wifi_event_group;
#define CONNECTED_BIT BIT0

static const char *TAG = "WIFI_PROVISION";

/* ---------------- WiFi Event Handler ---------------- */

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch (event->event_id) {

    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;

    case SYSTEM_EVENT_STA_GOT_IP:
        ESP_LOGI(TAG, "Connected to WiFi, got IP");
        gpio_set_level(LED_G, 1);
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;

    case SYSTEM_EVENT_STA_DISCONNECTED:
        ESP_LOGW(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;

    default:
        break;
    }
    return ESP_OK;
}

/* ---------------- HTTP PAGE ---------------- */

static esp_err_t root_get_handler(httpd_req_t *req)
{
    httpd_resp_sendstr(req,
        "<!DOCTYPE html><html>"
        "<head><meta name='viewport' content='width=device-width, initial-scale=1'>"
        "<title>ESP32 WiFi Setup</title></head>"
        "<body style='font-family:Arial;text-align:center;'>"
        "<h2>ESP32 WiFi Provisioning</h2>"
        "<form id='f'>"
        "SSID:<br><input name='ssid'><br><br>"
        "Password:<br><input type='password' name='pwd'><br><br>"
        "<button>Save & Connect</button>"
        "</form>"
        "<p id='msg'></p>"
        "<script>"
        "document.getElementById('f').onsubmit=function(e){"
        "e.preventDefault();"
        "fetch('/connect',{method:'POST',headers:{'Content-Type':'application/json'},"
        "body:JSON.stringify({ssid:this.ssid.value,pwd:this.pwd.value})})"
        ".then(()=>{document.getElementById('msg').innerText='Saved! Rebooting...';});};"
        "</script>"
        "</body></html>"
    );
    return ESP_OK;
}

static const httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_get_handler
};

/* ---------------- POST /connect ---------------- */

static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char buf[256];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) return ESP_FAIL;
    buf[len] = 0;

    cJSON *root = cJSON_Parse(buf);
    if (!root) return ESP_FAIL;

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pwd  = cJSON_GetObjectItem(root, "pwd");

    if (!cJSON_IsString(ssid) || !cJSON_IsString(pwd)) {
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    save_wifi1(ssid->valuestring, pwd->valuestring);
    ESP_LOGI(TAG, "WiFi credentials saved");

    cJSON_Delete(root);

    httpd_resp_sendstr(req, "OK");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();

    return ESP_OK;
}

static const httpd_uri_t connect_uri = {
    .uri = "/connect",
    .method = HTTP_POST,
    .handler = connect_post_handler
};

/* ---------------- Web Server ---------------- */

static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &connect_uri);
    }
    return server;
}

/* ---------------- STA MODE ---------------- */

static void start_sta_mode(const char *ssid, const char *pwd)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {0};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pwd, sizeof(wifi_config.sta.password));

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    xEventGroupWaitBits(wifi_event_group,
                        CONNECTED_BIT,
                        false,
                        true,
                        portMAX_DELAY);
}

/* ---------------- AP MODE ---------------- */

static void start_ap_mode(void)
{
    tcpip_adapter_init();
    ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t ap_config = {
        .ap = {
            .ssid = "ESP32_Setup",
            .ssid_len = strlen("ESP32_Setup"),
            .password = "12345678",
            .max_connection = 1,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP Mode started: ESP32_Setup");
}

/* ---------------- PUBLIC ENTRY ---------------- */

void wifi_ap(void)
{
    // LEDs
    gpio_set_direction(LED_G, GPIO_MODE_OUTPUT);

    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    char ssid[32] = {0};
    char pwd[64]  = {0};

    if (load_wifi1(ssid, sizeof(ssid), pwd, sizeof(pwd))) {
        ESP_LOGI(TAG, "Credentials found → STA mode");
        start_sta_mode(ssid, pwd);
        unlink("/spiffs/ssid.txt");
        unlink("/spiffs/password.txt");
        spiffs_unmount();
    } else {
        ESP_LOGI(TAG, "No credentials → AP provisioning");
        start_ap_mode();
        start_webserver();
    }
}