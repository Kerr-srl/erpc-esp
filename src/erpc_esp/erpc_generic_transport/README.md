# eRPC generic transport

Sometimes (especially when using C instead of C++), it not straightforward to subclass `FramedTransport` and maybe it is more convenient to directly pass two functions: underlyingWrite and underlyingRead. This transport is exactly thought for this use case.
