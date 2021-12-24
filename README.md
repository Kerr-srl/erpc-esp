# ESP32-eRPC

Set of utilities components for working with [eRPC](https://github.com/EmbeddedRPC/erpc) on ESP32 family MCUs.

For a ready to use ESP32 project take look at the [hello world example](https://github.com/Kerr-srl/erpc-esp/tree/dev/src/examples/hello_world).

## Build and installing

The typical workflow is to include this repository as submodule in the main ESP32 project and then add the path to `src/components` of this repository to `EXTRA_COMPONENT_DIRS`. Then check out the README of the components that you want to use.

## Examples

Examples are available in the `src/examples/` folder.

## Support

Due to limited time, the only supported machine is "my machine", which runs Ubuntu 20.04. I hope that the repository can work with any Linux distributions.
On the other hand, there is no plan to support Windows and macOS.
