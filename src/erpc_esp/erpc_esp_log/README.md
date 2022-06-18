# erpc_esp_log

This component contains some utility code that makes it possible to have ESP-IDF logs and eRPC communication coexist on the same channel.

The idea is very simple: the bytes of the eRPC outbound payload are serialized in a hex string and send out using `ESP_LOGI`. There are for sure better alternatives, which may be provided in future. 
