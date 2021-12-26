import time
import threading
import queue

import erpc
import tinyproto


class TinyprotoError(Exception):
    pass


class TinyprotoDisconnectedError(TinyprotoError):
    pass


class TinyprotoTimeoutError(TinyprotoError):
    pass


class TinyprotoTransport(erpc.transport.FramedTransport):
    def __init__(self, read_func, write_func, send_timeout: float = 0.5, receive_timeout: float = 0.5, **kwargs):
        """
        TinyprotoTransport constructor

        :param read_func tinyproto Fd read function
        :param write_func tinyproto Fd write function
        :param send_timeout send timeout in seconds.
        :param receive_timeout receive timeout in seconds.
        """
        super(TinyprotoTransport, self).__init__()
        self._proto = tinyproto.Fd()
        self._write_func = write_func
        self._read_func = read_func
        self._rx_fifo = queue.Queue(0)
        self._sent_event = threading.Condition()
        self._send_timeout = send_timeout
        self._receive_timeout = receive_timeout

        def on_read(data):
            for byte in data:
                self._rx_fifo.put(byte, block=True)

        def on_send(data):
            with self._sent_event:
                self._sent_event.notify()

        self._proto.on_send = on_send
        self._proto.on_read = on_read

    def connect(self, timeout: float = None):
        """
        Establish connection

        :param timeout connection timeout in seconds.
        """

        self._proto.begin()
        self._rx_thread = threading.Thread(
            target=TinyprotoTransport.rx_thread, name='TinyprotoTransport RX', args=(self,))
        self._tx_thread = threading.Thread(
            target=TinyprotoTransport.tx_thread, name='TinyprotoTransport TX', args=(self,))
        self._rx_thread.start()
        self._tx_thread.start()

        if timeout is None:
            timeout = float('inf')

        while self._proto.get_status() != 0:
            time.sleep(0.01)
            timeout -= 0.01
            if (timeout <= 0):
                raise TinyprotoTimeoutError("Connection timed out")

    def close(self):
        self._rx_thread.join()
        self._tx_thread.join()
        self._proto.end()

    @staticmethod
    def rx_thread(this):
        while True:
            this._proto.run_rx(this._read_func)

    @staticmethod
    def tx_thread(this):
        while True:
            this._proto.run_tx(this._write_func)

    def _base_send(self, data):
        ret = self._proto.send(data)
        if ret != 0:
            if ret == -1:
                raise TinyprotoError("Send request cancelled")
            elif ret == -2:
                if self._proto.get_status() == 0:
                    raise TinyprotoError("Send request timed out")
                else:
                    raise TinyprotoDisconnectedError("Link is disconnected")
            elif ret == -3:
                raise TinyprotoError("Data too large")
            else:
                assert False
        with self._sent_event:
            sent = self._sent_event.wait(timeout=self._send_timeout)
            if not sent:
                raise TinyprotoTimeoutError("Frame send timeout")

    def _base_receive(self, count):
        bytes = []
        timeout = self._receive_timeout
        last = time.time()
        while count > 0:
            try:
                byte = self._rx_fifo.get(block=True, timeout=timeout)
            except queue.Empty:
                raise TinyprotoTimeoutError("Frame receive timeout")
            timeout -= (time.time() - last)
            last = time.time()
            bytes.append(byte)
            count -= 1

        return bytearray(bytes)
