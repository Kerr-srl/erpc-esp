# eRPC tinyproto transport

The `erpc_tinyproto` component provides a eRPC transport based on [tinyproto](https://github.com/lexus2k/tinyproto), which is a library that provided an ARQ-like protocol layer.

Advantages of this transport:

* Proper connection-oriented communication, unlike e.g. bare UART.
* Log coexistence: rpc payloads can be mixed with `esp_log`. Everything is passed to tinyproto, which decides what to keep and what to discard.

Limitations:

* Limitations: limited throughput. Need more investigation on how to improve this aspect.

## CMake setup

Ensure that the `tinyproto` submodule has been downloaded.
Then add the directory of the `tinyproto` submodule to `EXTRA_COMPONENT_DIRS`

## Python dependencies installation

This component provides also the tinyproto-based transport as Python binding. It is contained in the Python package provided by this repository.

An additional dependency is the Python binding of Tinyproto itself. You must install it.

```bash
# Install tinyproto
$ pip install ../../../external/tinyproto/
```
