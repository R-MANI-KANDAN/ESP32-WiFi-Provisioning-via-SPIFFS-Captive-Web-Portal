# 📶 ESP32 Wi-Fi Provisioning via SPIFFS Captive Web Portal

A **Wi-Fi provisioning system** for the ESP32 built using **ESP-IDF**. Instead of hardcoding Wi-Fi credentials in firmware, this project allows users to configure the SSID and password at runtime through a **captive web portal** — served directly from the ESP32's flash memory using **SPIFFS**.

> 🎬 A demo video (`demo video.mp4`) is included in the repository root.

---

## 🧠 How It Works

```
Power ON
    │
    ▼
Initialize SPIFFS
    │
    ▼
Check /spiffs/ssid.txt
    │
    ├── File exists & not empty?
    │       │
    │       ▼
    │   Load saved SSID + Password from SPIFFS
    │   Connect to Wi-Fi as Station (STA mode)
    │
    └── File empty / not found?
            │
            ▼
        Start as Wi-Fi Access Point (AP mode)
        Launch Captive Web Portal
            │
            ▼
        User connects to ESP32 AP
        Opens browser → Captive portal auto-launches
        User enters SSID + Password
            │
            ▼
        Credentials saved to /spiffs/ssid.txt
        ESP32 restarts → connects to Wi-Fi
```

---

## ✨ Features

- **Zero hardcoding** — Wi-Fi credentials are never baked into the firmware
- **Captive portal** — browser automatically redirects to the config page when connecting to the ESP32 AP
- **SPIFFS-backed persistence** — credentials are saved to flash and survive reboots
- **Auto-connect on boot** — if credentials already exist in SPIFFS, ESP32 connects directly without launching the portal
- **ESP-IDF native** — built entirely with the Espressif IoT Development Framework in C
- **Dev container support** — `.devcontainer` included for VS Code + Docker development environment

---

## 📁 Project Structure

```
ESP32-WiFi-Provisioning-via-SPIFFS-Captive-Web-Portal/
├── .devcontainer/              # VS Code Dev Container config (Docker-based)
├── .vscode/                    # VS Code workspace settings
├── main/
│   ├── main.c                  # Entry point: SPIFFS init, credential load, AP start
│   ├── spiffs.c / spiffs.h     # SPIFFS mount, file read/write, credential storage
│   ├── wifi.c  / wifi.h        # Wi-Fi AP mode, captive portal HTTP server
│   └── CMakeLists.txt          # Component build config
├── .gitignore
├── CMakeLists.txt              # Top-level project build file
├── Makefile                    # Legacy Make build support
├── partitions_example.csv      # Custom partition table (NVS + factory app + SPIFFS)
├── spiffs_example_test.py      # Python test script for SPIFFS behaviour
├── demo video.mp4              # Demo of the provisioning flow
└── README.md
```

---

## 🗂️ Partition Table

The custom partition layout (`partitions_example.csv`) allocates a dedicated **SPIFFS storage partition** for saving Wi-Fi credentials:

| Name       | Type | SubType | Offset   | Size    |
|------------|------|---------|----------|---------|
| nvs        | data | nvs     | 0x9000   | 24 KB   |
| phy_init   | data | phy     | 0xf000   | 4 KB    |
| factory    | app  | factory | 0x10000  | 1 MB    |
| storage    | data | spiffs  | —        | 960 KB  |

The `storage` partition is where `ssid.txt` (and optionally `pwd.txt`) are stored by the captive portal.

---

## 🔑 Key Source Files

### `main/main.c`
The application entry point. On boot it:
1. Initialises the SPIFFS filesystem with `spiffs_initialize()`
2. Runs a filesystem health check with `spiffs_check()`
3. Checks if `/spiffs/ssid.txt` exists and is non-empty
4. If credentials exist → loads them with `load_wifi1()` and displays them via `show_credentials()`
5. Starts the ESP32 in **AP mode** via `wifi_ap()` to serve the captive portal

### `main/spiffs.c`
Handles all filesystem operations:
- Mounting SPIFFS with `esp_vfs_spiffs_register()`
- Reading and writing credential files (`ssid.txt`, `pwd.txt`)
- Checking file existence and emptiness
- Reporting partition usage stats

### `main/wifi.c`
Manages Wi-Fi and the web portal:
- Configures ESP32 as a **SoftAP** (`WIFI_MODE_AP`)
- Starts an **HTTP server** to serve the provisioning web page
- Handles `GET /` to serve the HTML form
- Handles `POST /save` to receive and persist SSID + password to SPIFFS
- Implements **captive portal redirect** — intercepts DNS queries and redirects all traffic to the config page

---

## 🛠️ Requirements

### Hardware
- ESP32 Development Board (ESP32 DevKit v1 or similar)
- USB cable

### Software
- [ESP-IDF v5.x](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)
- Python 3.x (for build tools and test script)
- **OR** use the included `.devcontainer` with VS Code + Docker (no local ESP-IDF install needed)

---

## 🚀 Getting Started

### Option A — Local ESP-IDF Setup

**1. Install ESP-IDF**
```bash
git clone --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
./install.sh esp32
. ./export.sh
```

**2. Clone this repository**
```bash
git clone https://github.com/R-MANI-KANDAN/ESP32-WiFi-Provisioning-via-SPIFFS-Captive-Web-Portal.git
cd ESP32-WiFi-Provisioning-via-SPIFFS-Captive-Web-Portal
```

**3. Build, flash and monitor**
```bash
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```
> Replace `/dev/ttyUSB0` with your actual port (`COM3`, `/dev/tty.usbserial-*`, etc.)

---

### Option B — VS Code Dev Container (Recommended)

1. Open the repo folder in **VS Code**
2. When prompted, click **"Reopen in Container"**
3. The Docker-based ESP-IDF environment will build automatically
4. Run `idf.py build flash monitor` in the integrated terminal

---

## 📱 Using the Captive Portal

1. **Flash and power** the ESP32
2. On your phone or laptop, **connect to the Wi-Fi network** broadcasted by the ESP32 (default AP SSID: `ESP32-Config` or similar)
3. A **captive portal page** will automatically open in your browser (or navigate to `http://192.168.4.1`)
4. **Enter your Wi-Fi SSID and password** and submit
5. The ESP32 saves the credentials to SPIFFS, restarts, and connects to your network
6. On the next boot, the portal will not appear — the ESP32 connects directly using saved credentials

---

## 🧰 Tech Stack

| Layer | Technology |
|---|---|
| **MCU** | ESP32 (Xtensa LX6 dual-core) |
| **Framework** | ESP-IDF v5.x |
| **Language** | C |
| **Build System** | CMake + idf.py |
| **File System** | SPIFFS (Serial Peripheral Interface Flash File System) |
| **Networking** | lwIP, ESP HTTP Server, DNS server (captive portal) |
| **Dev Environment** | VS Code + Dev Container (Docker) |

---

## 📖 ESP-IDF APIs Used

| API | Purpose |
|---|---|
| `esp_vfs_spiffs_register()` | Mount SPIFFS partition |
| `esp_wifi_set_mode(WIFI_MODE_AP)` | Start ESP32 as Access Point |
| `httpd_start()` / `httpd_register_uri_handler()` | HTTP web server for portal |
| `esp_netif_init()` | Network interface initialisation |
| `nvs_flash_init()` | NVS flash for Wi-Fi driver storage |
| `ESP_LOGI / ESP_LOGE` | Logging |

---

