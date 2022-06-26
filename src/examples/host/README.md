# Host ESP-IDF app

Example of building an ESP-IDF app for the host platform.

## Usage

Check the documentation of the [erpc](../../erpc_esp/erpc/README.md) component.
Check the documentation of the [erpc_tinyproto](../../erpc_esp/erpc_tinyproto/README.md) component.

This example contains nothing specific to this repository, but having an example that shows that the components provided by this repository can be made to work also on the host is useful.

This example uses stdout to print logs and stderr as channel to exchange eRPC data with the Python script.

```bash
# Build the project
$ idf.py build
# Communicate with the built firmware using Python
$ python main/main.py build/host.elf
```
