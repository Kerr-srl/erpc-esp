# eRPC hello world using Tinyproto as transport

Example of connection-oriented communication over UART using Tinyproto

## Usage

Check the documentation of the [erpc](../../components/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../components/erpc_tinyproto/README.md) component.

### Communication between two ESP32, one as host and one as target

Assuming that:

* the "host ESP32" is connected to the PC at port /dev/ttyUSB0
* the "target ESP32" is connected to the PC at port /dev/ttyUSB1
* "host ESP32" and "target ESP32" are connected via the other UART (the one that is not used for flashing and logging)

```bash
# Build, flash and monitor the "host" firmware
idf.py -Bbuild/host -DHOST=TRUE -p /dev/ttyUSB0 -Bbuild/host flash monitor

# Build, flash and monitor the "target" firmware
idf.py -Bbuild/target -DHOST=FALSE -p /dev/ttyUSB1 -Bbuild/target flash monitor
```

You can try to disconnect the UARTs and re-connect. RPC communication should resume on re-connection.
