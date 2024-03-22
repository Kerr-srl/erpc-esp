# erpc

The `erpc` component is the core component provided by this repository. This component takes care of:

* Including the relevant eRPC static library and making it available through CMake.
* Ensuring that the required eRPC tools are installed.
* Providing an applicable subset of eRPC compile time configurations through ESP-IDF's Kconfig interface.
* Providing utility CMake functions, in [erpc_utils.cmake](./erpc_utils.cmake). E.g.:
    * `erpc_add_idl_target`: takes care of automatically invoking `erpcgen` and exposing the generated sources as linkable CMake static library targets.

Note that this repository contains [eRPC](https://github.com/EmbeddedRPC/erpc) as submodule. The submodule points to the version of eRPC that we support.

Since this repository is simply some utilities to make eRPC easier to use with ESP32, you obviously need to also consult the documentation of [eRPC](https://github.com/EmbeddedRPC/erpc).

## Dependencies

This component requires the eRPC tools `erpcgen` to be installed, i.e. available in PATH. Since no precompiled binaries are available, you should build `erpcgen`. You can follow [eRPC's documentation](https://github.com/EmbeddedRPC/erpc#building-and-installing).

Furthermore, if you're interested in using the Python bindings of eRPC, don't forget to install the eRPC python library. As always see [eRPC's documentation](https://github.com/EmbeddedRPC/erpc#installing-for-python).
