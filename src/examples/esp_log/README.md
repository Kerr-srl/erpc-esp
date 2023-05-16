# ESP logging coexistence

Example of interacting with an ESP32 dev board via the console UART. The UART TX on the ESP32 is used both for logging and for sending eRPC payloads, which means that there is no need to wire an additional UART-to-USB.

Tested on:

* [ESP32-S2-Saola-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html#hardware-reference)
* [ESP32-S3-DevKitC-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/hw-reference/esp32s3/user-guide-devkitc-1.html)

## Usage

Check the documentation of the [erpc](../../erpc_esp/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../erpc_esp/erpc_tinyproto/README.md) component.

```bash
# Build and flash the firmware on the ESP32 target

# Communicate with the target via the serial port (e.g. /dev/ttyUSB0) using the host-side Python script
# This example uses 2M as baudrate for the UART console
# Use the --reset option to reset the ESP32 on script startup (just like idf.py monitor)
# Use the --print-logs option to print also ESP-IDF logs to stderr
$ python main/main.py -p /dev/ttyUSB0 -b 2000000 --reset --print-logs
# Obviously, if you don't like the logs of the Python scripts and
# the logs of the target firmware to be mixed together, you could e.g. redirect
# stderr to a file and then use `tail -f <file>` in another shell.
```
