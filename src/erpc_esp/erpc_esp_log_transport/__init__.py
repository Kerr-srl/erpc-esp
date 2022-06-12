import re
from typing import Optional

import erpc.transport as transport

ERPC_LOG_PATTERN = re.compile(r"I\s\(\d+\)\serpc:\s\[([0-9a-fA-F]+)\]")


class EspLogFilter(object):
    def feed_line(self, line: bytes) -> Optional[bytes]:
        str = line.decode("utf-8", errors="ignore")
        match = ERPC_LOG_PATTERN.search(str)
        if match:
            payload = match.group(1)
            bytes = bytearray.fromhex(payload)
            return bytes
        return None


class SerialEspLogTransport(transport.SerialTransport):
    def __init__(self, url, baudrate, **kwargs):
        super(SerialEspLogTransport, self).__init__(url, baudrate, **kwargs)
        self.log_filter = EspLogFilter()

    def _base_receive(self, count):
        assert count > 0
        recv_data = bytearray()

        while True:
            line = self._serial.readline()
            erpc_data = self.log_filter.feed_line(line)
            if erpc_data:
                recv_data.extend(erpc_data)
                if len(recv_data) >= count:
                    break

        if len(recv_data) > count:
            raise IOError("Received too many bytes")

        return recv_data
