// spiffs_storage.c

#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_err.h"
#include "esp_log.h"
#include "esp_spiffs.h"

static const char *TAG_SPIFFS = "SPIFFS";

static esp_vfs_spiffs_conf_t conf = {
    .base_path = "/spiffs",
    .partition_label = NULL,
    .max_files = 8,
    .format_if_mount_failed = true
};

static esp_err_t ret;

void spiffs_initialize(void)
{
    ESP_LOGI(TAG_SPIFFS, "Initializing SPIFFS");

    ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG_SPIFFS, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG_SPIFFS, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG_SPIFFS, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }
}

void spiffs_check(void)
{
#ifdef CONFIG_EXAMPLE_SPIFFS_CHECK_ON_START
    ESP_LOGI(TAG_SPIFFS, "Performing SPIFFS_check().");
    ret = esp_spiffs_check(conf.partition_label);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SPIFFS, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
        return;
    } else {
        ESP_LOGI(TAG_SPIFFS, "SPIFFS_check() successful");
    }
#endif

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_SPIFFS, "Failed to get SPIFFS partition information (%s). Formatting...", esp_err_to_name(ret));
        esp_spiffs_format(conf.partition_label);
        return;
    } else {
        ESP_LOGI(TAG_SPIFFS, "Partition size: total: %d, used: %d", total, used);
    }

    if (used > total) {
        ESP_LOGW(TAG_SPIFFS, "Used bytes > total bytes. Performing SPIFFS_check().");
        ret = esp_spiffs_check(conf.partition_label);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG_SPIFFS, "SPIFFS_check() failed (%s)", esp_err_to_name(ret));
            return;
        } else {
            ESP_LOGI(TAG_SPIFFS, "SPIFFS_check() successful");
        }
    }
}

void spiffs_unmount(void)
{
    esp_vfs_spiffs_unregister(conf.partition_label);
    ESP_LOGI(TAG_SPIFFS, "SPIFFS unmounted");
}

/* ---------- Generic helpers ---------- */

bool spiffs_file_not_empty(const char *path)
{
    struct stat st;
    if (stat(path, &st) != 0) {
        return false;
    }
    return st.st_size > 0;
}

static bool spiffs_read(const char *path, char *buf, size_t max_len)
{
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGW("SPIFFS", "Failed to open %s", path);
        return false;
    }

    if (!fgets(buf, max_len, f)) {
        fclose(f);
        return false;
    }
    fclose(f);

    // 🔴 REMOVE NEWLINE
    buf[strcspn(buf, "\n")] = 0;

    return true;
}

static void spiffs_write(const char *path, const char *str)
{
    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG_SPIFFS, "Failed to open %s for writing", path);
        return;
    }
    fprintf(f, "%s", str);
    fclose(f);
}
void show_credentials(void)
{
    char ssid[32] = {0};
    char pwd[64]  = {0};

    FILE *f;

    f = fopen("/spiffs/ssid.txt", "r");
    if (f) {
        fgets(ssid, sizeof(ssid), f);
        fclose(f);
    } else {
        ESP_LOGE("SPIFFS", "ssid.txt not found");
        return;
    }

    f = fopen("/spiffs/password.txt", "r");
    if (f) {
        fgets(pwd, sizeof(pwd), f);
        fclose(f);
    } else {
        ESP_LOGE("SPIFFS", "password.txt not found");
        return;
    }

    ESP_LOGI("SPIFFS", "Stored SSID     : %s", ssid);
    ESP_LOGI("SPIFFS", "Stored Password : %s", pwd);
}


bool load_wifi1(char *ssid, size_t ssid_len, char *pwd, size_t pwd_len)
{
    if (!spiffs_file_not_empty("/spiffs/ssid.txt") ||
        !spiffs_file_not_empty("/spiffs/password.txt")) {
        return false;
    }

    bool ok1 = spiffs_read("/spiffs/ssid.txt", ssid, ssid_len);
    bool ok2 = spiffs_read("/spiffs/password.txt", pwd, pwd_len);

    if (!ok1 || !ok2) {
        ESP_LOGW(TAG_SPIFFS, "Failed to read WiFi1 credentials");
        return false;
    }

    return true;
}

void save_wifi1(const char *ssid, const char *pwd)
{
    spiffs_write("/spiffs/ssid.txt", ssid);
    spiffs_write("/spiffs/password.txt", pwd);
    ESP_LOGI(TAG_SPIFFS, "Saved WiFi1 credentials to SPIFFS");
}
