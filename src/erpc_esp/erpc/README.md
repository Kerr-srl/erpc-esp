# erpc

The `erpc` component is the core component provided by this repository. This component takes care of:

* Including the relevant eRPC runtime library in CMake.
* Ensuring that the required eRPC tools are installed.
* Providing an applicable subset of eRPC compile time configurations through ESP-IDF's Kconfig interface.

Note that this repository contains [our fork of eRPC](https://github.com/Kerr-srl/erpc/tree/master) as submodule. The forked eRPC is what will be included in the build by this component.
Our fork provides CMake integration. In particular check out the documentation of the `ERPCAddIDLTarget` CMake function in [FindERPC.cmake](https://github.com/Kerr-srl/erpc/blob/master/cmake/FindERPC.cmake).

Since this repository is simply some utilities to make eRPC easier to use with ESP32, you obviously need also to consult the documentation of [eRPC](https://github.com/EmbeddedRPC/erpc).

## Dependencies

This component requires the eRPC tools `erpcgen` to be installed, i.e. available in PATH. Since no precompiled binaries are available, you should build `erpcgen`. You can follow [eRPC's documentation](https://github.com/EmbeddedRPC/erpc#building-and-installing).

Furthermore, if you're interested in using the Python bindings of eRPC, don't forget to install the eRPC python library. As always see [eRPC's documentation](https://github.com/EmbeddedRPC/erpc#installing-for-python).
