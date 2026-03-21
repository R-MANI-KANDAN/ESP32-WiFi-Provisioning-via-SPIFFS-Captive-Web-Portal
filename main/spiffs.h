void spiffs_initialize(void);
void spiffs_check(void);
void spiffs_unmount(void);
bool load_wifi1(char *ssid, size_t ssid_len, char *pwd, size_t pwd_len);
void save_wifi1(const char *ssid, const char *pwd);
bool spiffs_file_not_empty(const char *path) ;
bool spiffs_read(const char *path, char *buf, size_t max_len);
void spiffs_write(const char *path, const char *str) ;
void show_credentials(void) ;