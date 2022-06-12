# esp_log

This example shows how to use [ESP-IDF logging](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/log.html) as a transport for eRPC. This allows logs to coexist with eRPC outputs.
This example also showcases how you can interactively execute eRPC commands via a Python REPL, while still having all the handy features of `idf.py monitor`.

## Usage

Check the documentation of the [erpc](../../components/erpc/README.md) component.

### Communication between ESP32 and PC

```bash
# Build and flash the firmware on the ESP32 target
idf.py build
idf.py flash

# Connect the other UART of the board with the PC through an USB-UART bridge.
# Let's say the USB-UART bridge is connected on port /dev/ttyACM0
# We can communicate with the target using the host-side Python script
python main/main.py -p /dev/ttyACM0
```
