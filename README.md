# ESP32-eRPC

Set of utilities components for working with [eRPC](https://github.com/EmbeddedRPC/erpc) on ESP32 family MCUs.

For a ready to use ESP32 project take look at the [hello world example](https://github.com/Kerr-srl/erpc-esp/tree/dev/src/examples/hello_world).

NOTE: in this repository, when we say ESP32, we usually mean any MCU of the ESP32 family (more precisely, anything that is supported by ESP-IDF).

## Build and installing

The typical workflow is to include this repository as submodule in the main ESP32 project and then add the path to `src/erpc_esp` of this repository to `EXTRA_COMPONENT_DIRS`. Then check out the README of the components that you want to use.

Some components in this repository offer also the corresponding Python modules. You need to install the python package provided in this repository:

```bash
# Assuming you are in the root of the repository
$ pip install .
```

## Examples

Examples are available in the `src/examples/` folder.

Start with the [hello_world](./src/examples/hello_world/) example, which shows:

* How two ESP32 can communicate with each other over a secondary UART.
* How a ESP32 can communicate with a Python program on the host over a secondary UART.

The next step is the [multiple_interfaces](./src/examples/multiple_interfaces/) example, which is similar to the hello_world example but shows the use of multiple eRPC interfaces.

Both examples are based on vanilla UART (i.e. connection-less) and work only when the eRPC server side is already up and running before the eRPC client makes the first eRPC call.

To solve this limitation, try the [connection](./src/examples/connection) examples, where a connection-oriented reliable protocol is used as eRPC transport. Before starting the communication, the eRPC client and server perform the necessary handshakes to establish a connection.

If you want to use the secondary UART (e.g. because you don't have to UART-USB adapter), then the [esp_log](./src/examples/esp_log/) and [socket](./src/examples/socket/) examples may interest you. [esp_log](./src/examples/esp_log/) extends [connection](./src/examples/connection) and shows how a ESP32 development board can communicate with a Python program on the host over the primary UART (accessible directly using the UART-USB bridge of the devboard). [socket](./src/examples/socket/) shows how to use WiFi+socket as eRPC transport.

If you don't have any ESP32, then the [host](./src/examples/host/) example may interest you. It shows how to use ESP-IDF on the host and how to use POSIX standard I/O (via `read()` and `write()`) as eRPC transport.

## Support

Due to limited time, the only supported machine is "my machine", which runs Ubuntu 20.04. I hope that the repository can work with any Linux distributions.
On the other hand, there is no plan to support Windows and macOS. They may work, but who knows.

## Alternatives

* [Pigweed RPC](https://pigweed.dev/pw_rpc/) by Google. It's a much more refined solution, but making Pigweed work on ESP32 is quite challenging, due to how Pigweed manages its dependencies.
