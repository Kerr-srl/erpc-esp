# Hello world example

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

### Communication between two ESP32, one as host and one as target

Assuming that:

* the "host ESP32" is connected to the PC at port /dev/ttyUSB0
* the "target ESP32" is connected to the PC at port /dev/ttyUSB1
* "host ESP32" and "target ESP32" are connected via the other UART (the one that is not used for flashing and logging)

```bash
# Build and flash the "host" firmware
idf.py -Bbuild/host build -DHOST=TRUE
ESPPORT=/dev/ttyUSB0 idf.py -Bbuild/host flash

# Build and flash the "target" firmware
idf.py -Bbuild/target build -DHOST=FALSE
ESPPORT=/dev/ttyUSB1 idf.py -Bbuild/target flash

# Watch them communicate with each other
ESPPORT=/dev/ttyUSB0 idf.py -Bbuild/host monitor
ESPPORT=/dev/ttyUSB1 idf.py -Bbuild/target monitor
```

## Problem of connectionless UART communication

Reset the ESP32, start the Python script after 2 seconds. You'll see that the Python script (the host) successfully calls the ESP32 (the target) and gets a reply back, while the calls that the ESP32 is supposed to make every second don't happen. This is because the first call from the ESP32 was lost since the Python script was not up running yet.

Fundamentally, we need a sort of handshake mechanism, we need to establish a connection before communicating.

Considerations:

* UART hardware flow control. Note that usually on ESP32 dev boards, only the RTS and DTR are connected to the USB-UART bridge and these signals are used for automatic flashing
  (without requiring setting some pin to HIGH or LOW). See some discussion here https://www.esp32.com/viewtopic.php?t=5731.
  But maybe using UART hardware flow control for connection establishment is an abuse of this feature.
* I don't think UART software flow (XON/XOFF) control can be used for the objective of connection establishment. It can only be used for flow control when the devices are already communicating.
* Use a proper connection-oriented protocol (alla TCP) over UART. See [connection](../connection)
