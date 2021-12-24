# Hello world example

## Usage

Check the documentation of the [erpc](../../components/erpc/README.md) component.

```bash
# Build and flash the firmware on the ESP32 target
idf.py build
idf.py flash

# Communicate with the target via UART (e.g. at /dev/ttyUSB0) using the host-side Python script
python main/main.py -p /dev/ttyUSB0
```

## Problem of connectionless UART communication

Reset the ESP32, start the Python script after 6 seconds. You'll see that the Python script (the host) successfully calls the ESP32 (the target) and gets a reply back, while the calls that the ESP32 is supposed to make every five seconds don't happen. This is because the first call from the ESP32 was lost since the Python script was not up running yet.

Fundamentally, we need a sort of handshake mechanism, we need to build a connection before communicating.

Considerations:

* UART hardware flow control. Note that usually on ESP32 dev boards, only the RTS and DTR are connected to the USB-UART bridge and these signals are used for automatic flashing
  (without requiring setting some pin to HIGH or LOW). See some discussion here https://www.esp32.com/viewtopic.php?t=5731.
  But maybe using UART hardware flow control for connection establishment is an abuse of this feature.
* I don't think UART software flow (XON/XOFF) control can be used for the objective of connection establishment. It can only be used for flow control when the devices are already communicating.
* Use a proper connection-oriented protocol (alla TCP) over UART. See [connection](../connection)
