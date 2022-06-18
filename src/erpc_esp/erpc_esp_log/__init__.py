import re
from typing import Optional

ERPC_LOG_PATTERN = re.compile(r"I\s\(\d+\)\serpc:\s\[([0-9a-fA-F]+)\]")


class ErpcEspLogFilter(object):
    """
    Helper methods to extract eRPC payloads from ESP LOG input stream
    """

    def feed_line(self, line: bytes) -> Optional[bytes]:
        """
        Feed a line and returns potential eRPC payload bytes
        """
        str = line.decode("utf-8", errors="ignore")
        match = ERPC_LOG_PATTERN.search(str)
        if match:
            payload = match.group(1)
            bytes = bytearray.fromhex(payload)
            return bytes
        return None
