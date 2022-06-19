# Hello world example

## Usage

Check the documentation of the [erpc](../../erpc_esp/erpc/README.md) component.

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

Reset the ESP32, start the Python script after 2 seconds. You'll see that the Python script (the host) successfully calls the ESP32 (the target) and gets a reply back, while the calls that the ESP32 is supposed to make every second don't happen. The same problem happens in the case of the 2 ESP32 communicating with each other. This is because the first call from the ESP32 was lost since the Python script was not up running yet. So the eRPC client on the ESP32 is stuck waiting for the response of its first RPC call, which the server didn't receive. There are a few ways to solve this problem:

* Use `oneway` RPC procedures. Acceptable if you don't care about the response from the eRPC server. Applicable only to RPC functions that don't return anything.
* Add a timeout for each eRPC invocation. eRPC doesn't support specifying timeout for each RPC call, since that would mean to add a timeout parameter to each remote procedure's signature, which breaks the abstraction that a RPC library is supposed to give (i.e. being able to treat remote procedures as local procedures). What you can do is:
    * Use eRPC's pre and post RPC call hooks. It is AFAIK undocumented and the Python library does not have this feature yet. For more information you can look in the header files of eRPC and at the [PR](https://github.com/EmbeddedRPC/erpc/pull/131) in which this feature was introduced.
    * Create your own transport, in which you block on reception only for a limited amount of time and then you return a timeout error.
* If using a connectionless protocol, include a sort of handshake mechanism to at least ensure that both sides are alive.
    * The UART hardware flow control may be used for this purpose, but that requires connecting a few more wires. Note that usually on ESP32 dev boards, only the RTS and DTR are connected to the USB-UART bridge and these signals are used for automatic flashing. See some discussion here https://www.esp32.com/viewtopic.php?t=5731.
    * In practise this means implementing a connection establishment logic at the application level, rather than at the transport level.
    * esp_log shows an example of this application-level handshake approach.
* Use a connected-oriented protocol
    * Note that using a connection-oriented protocol is complementary with the aforementioned point of setting a timeout on each eRPC call, since communication might get stuck not only because the other side was not alive, but also due to message corruption during communication. OTOH, if you use a connection-oriented *reliable* transport (such as TCP), then the application level timeout is no more useful for the purpose of detecting missed eRPC calls. It can still be used to handle application level errors though (e.g. the server received the call from the eRPC client but it is taking too long to respond or it is stuck, for whatever reason).
* Use a proper connection-oriented reliable protocol (alla TCP) over UART. See the [connection](../connection) example.
    * Note that conceptually a connection-oriented reliable protocol is not enough to ensure "flawless" RPC communication. It could happen that a client's RPC call arrives reliably on the server side and then the server breaks down and resets, losing all its state. A naive application logic would lead the client's RPC call to be stuck waiting for a response forever.

NOTE: When a peer disconnects (e.g. due to reset) and re-connects within a short amount of time, some protocol may decide to silently re-establish the connection. This is a problem. On a well-implemented connection-oriented reliable eRPC transport, when the server re-starts and tries to establish a new connection, the re-connection must be detected and explicitly handled, instead of silently re-connecting. Explicitly handling of re-connection allows having special logic to first unblock all the existing client RPC calls, which can be treated as erroneous (due to IO link failure).
