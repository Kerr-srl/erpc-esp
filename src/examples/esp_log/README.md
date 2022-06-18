# ESP logging coexistence

Example of interacting with an ESP32 dev board via the console UART. The UART TX on the ESP32 is used both for logging and for sending eRPC payloads, which means that there is no need to wire an additional UART-to-USB.

Tested on:

* [ESP32-S2-Saola-1](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s2/hw-reference/esp32s2/user-guide-saola-1-v1.2.html#hardware-reference)

## Usage

Check the documentation of the [erpc](../../components/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../components/erpc_tinyproto/README.md) component.

```bash
# Flash and monitor the firmware on the ESP32 target
# We use the monitorf idf.py action provided in this repository.
# With the --hide-erpc option, eRPC payload logs are not shown to the serial
# monitor
$ idf.py flash monitorf --hide-erpc

# Communicate with the target via the PTY slave (e.g. /dev/pts/1) using the host-side Python script
$ python main/main.py -p /dev/pts/1
```

Obviously, if you don't need `idf.py monitor`, then you can also you the Python script alone. In such case some other options supported by the Python script may be useful.

```bash
# Communicate with the target via the PTY slave (e.g. /dev/pts/1) using the host-side Python script
# This example uses 2M as baudrate for the UART console
# Use the -c to reset the ESP32 on script startup (just like idf.py monitor)
$ python main/main.py -p /dev/ttyUSB0 -b 2000000 --reset --
```
