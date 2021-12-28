# eRPC hello world using Tinyproto as transport

Example of connection-oriented communication over UART using Tinyproto

## Features

* Connection establishment
* ESP32 log coexistence, which means that there is no need to wire an additional UART. (Although it's pretty ugly)

## Usage

Check the documentation of the [erpc](../../components/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../components/erpc_tinyproto/README.md) component.

```bash
# Build and flash the firmware on the ESP32 target
idf.py build
idf.py flash

# Communicate with the target via UART (e.g. at /dev/ttyUSB0) using the host-side Python script
python main/main.py -p /dev/ttyUSB0
```
