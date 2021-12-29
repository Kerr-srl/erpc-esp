# hello world based on sockets

## Usage

Modify the following macros in `main/main.c`.

```c
#define CONFIG_EXAMPLE_WIFI_SSID "myssid"
#define CONFIG_EXAMPLE_WIFI_PASSWORD "mypassword"
#define CONFIG_EXAMPLE_HOST_IP_ADDRESS "hostip"
```

Then use the following commands

```bash
# Build and flash the firmware on the ESP32 target
idf.py build
idf.py flash

# Listen for incoming TCP connections
$ python main/main.py 0.0.0.0
```
