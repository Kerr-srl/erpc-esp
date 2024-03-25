# Example with multiple interfaces

Example of using multiple eRPC interfaces

## Usage

Check the documentation of the [erpc](../../erpc_esp/erpc/README.md) component.

### Communication between ESP32 and PC

```bash
# Build and flash the firmware on the ESP32 target
idf.py flash

# Connect the other UART of the board with the PC through an USB-UART bridge.
# Let's say the USB-UART bridge is connected on port /dev/ttyACM0
# We can communicate with the target using the host-side Python script
python main/main.py -p /dev/ttyACM0
```
