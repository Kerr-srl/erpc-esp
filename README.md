# ESP32-eRPC

Set of utilities components for working with [eRPC](https://github.com/EmbeddedRPC/erpc) on ESP32 family MCUs.

For a ready to use ESP32 project take look at the [hello world example](https://github.com/Kerr-srl/erpc-esp/tree/dev/src/examples/hello_world).

## Build and installing

The typical workflow is to include this repository as submodule in the main ESP32 project and then add the path to `src/erpc_esp` of this repository to `EXTRA_COMPONENT_DIRS`. Then check out the README of the components that you want to use.

Some components in this repository offer also the corresponding Python modules. You need to install the python package provided in this repository:

```bash
# Assuming you are in the root of the repository
$ pip install .
```

## Utility idf.py actions

This repository offers some utility idf.py actions that make life easier when working with idf.py and eRPC at the same time. The extension actions are defined in the [`idf_ext.py`](./idf_ext.py) file. `idf.py` looks for action extensions inside a `idf_ext.py` file that resides in the current working directory or that resides in paths listed in the `IDF_EXTRA_ACTIONS_PATH` environment variable.

The utilities actions are:

* `monitori` (monitor "interactive"): extends the `idf.py monitor` action by providing an additional shortcut (Ctrl+E), that can be used to create a temporary Python REPL.
* `monitorf` (monitor "forward"): extends the `idf.py monitor` action by forwarding all the serial I/O to a PTY (pseudo-terminal). This allows you to monitor your firmware over the serial port using `idf.py monitorf` and at the same time interacting with it over the PTY using eRPC.
    * The created PTY is printed in yellow in the first lines of the command `idf.py monitorf`. Something along the lines of "Use /dev/pts/xxx pseudoterminal".
    * Additional options to `idf.py monitor`:
        * `--hide-erpc`: hide eRPC payload logs. See [erpc_esp_log](./src/erpc_esp/erpc_esp_log/).
* Caveat: the implementation of these two actions are quite hackish, since ESP-IDF doesn't provide good ways to extend existing actions. Tested on:

* ESP-IDF v4.4.1

## Examples

Examples are available in the `src/examples/` folder.

## Support

Due to limited time, the only supported machine is "my machine", which runs Ubuntu 20.04. I hope that the repository can work with any Linux distributions.
On the other hand, there is no plan to support Windows and macOS. They may work, but who knows.

## Alternatives

* [Pigweed RPC](https://pigweed.dev/pw_rpc/) by Google
