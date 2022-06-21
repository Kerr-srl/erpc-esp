from enum import IntFlag, auto
import time
import threading
import queue
from typing import Callable

import erpc
import tinyproto


class TinyprotoUnRecoverableError(Exception):
    def __init__(self, msg="TinyProto generic unrecovarble error", *args, **kwargs):
        super().__init__(msg, *args, **kwargs)


class TinyprotoTxThreadDead(TinyprotoUnRecoverableError):
    def __init__(self, msg, *args, **kwargs):
        super().__init__(f"TinyProto error: TX thread dead. {msg}", *args, **kwargs)


class TinyprotoRxThreadDead(TinyprotoUnRecoverableError):
    def __init__(self, msg, *args, **kwargs):
        super().__init__(f"TinyProto error: RX thread dead. {msg}", *args, **kwargs)


class TinyprotoRecoverableError(Exception):
    def __init__(self, msg="TinyProto generic recoverable error", *args, **kwargs):
        super().__init__(msg, *args, **kwargs)


class TinyprotoDisconnectedError(TinyprotoRecoverableError):
    def __init__(self, msg, *args, **kwargs):
        super().__init__(f"TinyProto error: disconnected. {msg}", *args, **kwargs)


class TinyprotoTimeoutError(TinyprotoRecoverableError):
    def __init__(self, msg, *args, **kwargs):
        super().__init__(f"TinyProto error: timeout. {msg}", *args, **kwargs)


class TinyprotoClosedError(TinyprotoRecoverableError):
    def __init__(self, msg, *args, **kwargs):
        super().__init__(f"TinyProto error: closed. {msg}", *args, **kwargs)


class EventFlags(object):
    def __init__(self):
        self._cond = threading.Condition()
        self._event_flags: IntFlag = IntFlag(0)

    def get_bits(self) -> IntFlag:
        self._cond.acquire()
        val = self._event_flags
        self._cond.release()
        return val

    def update_bits(self, op: Callable[[IntFlag], IntFlag]):
        self._cond.acquire()
        self._event_flags = op(self._event_flags)
        self._cond.notify_all()
        self._cond.release()

    def set_bits(self, bits: IntFlag):
        self.update_bits(lambda flags: flags | bits)

    def clear_bits(self, bits: IntFlag):
        self.update_bits(lambda flags: flags & (~bits))

    def wait_bits(
        self,
        set: IntFlag,
        cleared: IntFlag,
        all_set: bool = False,
        all_cleared: bool = False,
        clear_and_set: bool = False,
        timeout=None,
        op: Callable[[IntFlag], IntFlag] = None,
    ) -> IntFlag:
        """
        Wait on bits in the event flags value to assume a certain value

        :param set IntFlag: bitflags to be waited to be set
        :param cleared IntFlag: bitflags to be waited to be cleared
        :param all_set bool: if true, wait for all the bitflags specified
         in``set`` to be set
        :param all_cleared bool: if true, wait for all the bitflags specified
         in``cleared`` to be cleared
        :param clear_and_set bool: if true, the event flags value must pass
         both the conditions of set bits and cleared bits.
        :param timeout float: Optional timeout
        :param op Callable[[IntFlag], IntFlag]: Manipulate the event flags
         value right after the wait is unblocked. If wait is unblocked due to
         timeout, this function is not called.
        :rtype IntFlag: the new event flags that caused the wait to be
        unblocked and before ``op`` is applied. If wait is unblocked due to
        timeout, the old event flags is returned.
        """

        def predicate():
            set_bits = self._event_flags & set
            cleared_bits = (~self._event_flags) & cleared
            set_satisfied = False
            if all_set:
                set_satisfied = set_bits == set
            else:
                set_satisfied = set_bits >= min(set, 1)
            clear_satisfied = False
            if all_cleared:
                clear_satisfied = cleared_bits == cleared
            else:
                clear_satisfied = cleared_bits >= min(cleared, 1)

            if clear_and_set:
                return set_satisfied and clear_satisfied
            else:
                return set_satisfied or clear_satisfied

        self._cond.acquire()
        did_not_timeout = self._cond.wait_for(predicate, timeout)
        val = self._event_flags
        if did_not_timeout and op:
            self._event_flags = op(val)
        self._cond.release()
        return val

    def wait_bits_set(self, set: IntFlag, all_set: bool = False, timeout=None):
        return self.wait_bits(
            set,
            IntFlag(0),
            all_set=all_set,
            # Cleared bits condition will always be true
            clear_and_set=True,
            timeout=timeout,
        )

    def wait_bits_cleared(self, cleared: IntFlag, all_cleared: bool, timeout=None):
        return self.wait_bits(
            IntFlag(0),
            cleared,
            all_cleared=all_cleared,
            # Set bits condition will always be true
            clear_and_set=True,
            timeout=timeout,
        )


class StoppableThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    def __init__(self, *args, **kwargs):
        super(StoppableThread, self).__init__(*args, **kwargs)
        self._stop_event = threading.Event()

    def stop(self):
        self._stop_event.set()

    def stopped(self):
        return self._stop_event.is_set()


class _EventFlags(IntFlag):
    OPENED = auto()
    CONNECTED = auto()
    NEW_FRAME_RX_PENDING = auto()


class TinyprotoTransport(erpc.transport.Transport):
    class RxThread(StoppableThread):
        # Why is the string "TinyprotoTransport" used as type annotation?
        # See https://stackoverflow.com/a/33533514
        def __init__(self, transport: "TinyprotoTransport", *args, **kwargs):
            super(TinyprotoTransport.RxThread, self).__init__(*args, **kwargs)
            self.transport = transport

        def run(self):
            while not self.stopped():
                self.transport._proto.run_rx(self.transport._read_func)

    class TxThread(StoppableThread):
        def __init__(self, transport: "TinyprotoTransport", *args, **kwargs):
            super(TinyprotoTransport.TxThread, self).__init__(*args, **kwargs)
            self.transport = transport

        def run(self):
            while not self.stopped():
                self.transport._proto.run_tx(self.transport._write_func)

    def __init__(
        self,
        read_func,
        write_func,
        send_timeout: float = 0.5,
        receive_timeout: float = 0.5,
    ):
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
        self._event_flags = EventFlags()
        self._rx_fifo = queue.Queue(0)
        self._send_timeout = send_timeout
        self._receive_timeout = receive_timeout
        self._rx_thread = TinyprotoTransport.RxThread(
            self,
            name="TinyprotoTransport RX",
        )
        self._tx_thread = TinyprotoTransport.TxThread(
            self,
            name="TinyprotoTransport TX",
        )

    def open(self):
        """
        Open the transport.
        """

        def on_read(data):
            self._event_flags.set_bits(_EventFlags.NEW_FRAME_RX_PENDING)
            self._rx_fifo.put(data, block=True)

        def on_connect_event(address, connected):
            if connected:
                self._event_flags.set_bits(_EventFlags.CONNECTED)
            else:
                # Reset protocol on disconnection
                # self._proto.begin()
                self._event_flags.clear_bits(_EventFlags.CONNECTED)

        self._proto.on_read = on_read
        self._proto.on_connect_event = on_connect_event

        self._proto.begin()
        self._rx_thread.start()
        self._tx_thread.start()

        self._event_flags.set_bits(_EventFlags.OPENED)

    def close(self):
        """
        Close the transport.
        """
        self._event_flags.clear_bits(_EventFlags.OPENED)
        self._rx_thread.stop()
        self._tx_thread.stop()
        self._rx_thread.join()
        self._tx_thread.join()
        self._proto.end()

    def wait_connected(self, timeout: float = None):
        """
        Wait connection to be established

        :param timeout connection timeout in seconds.
        """
        assert self._event_flags.get_bits() & _EventFlags.OPENED

        event_flags = self._event_flags.wait_bits(
            _EventFlags.CONNECTED, _EventFlags.OPENED, timeout=timeout
        )
        if (event_flags & _EventFlags.OPENED) == 0:
            raise TinyprotoClosedError("Connection failed")
        elif (event_flags & _EventFlags.CONNECTED) == 0:
            raise TinyprotoTimeoutError("Connection failed")

    def disconnect(self):
        self._proto.disconnect()

    @property
    def connected(self):
        return self._proto.get_status() == 0

    def send(self, data):
        event_flags = self._event_flags.get_bits()
        if (event_flags & _EventFlags.OPENED) == 0:
            raise TinyprotoClosedError("TX failure")
        if not (event_flags & _EventFlags.CONNECTED):
            raise TinyprotoDisconnectedError("TX failure")
        if not self._tx_thread.is_alive():
            raise TinyprotoTxThreadDead("TX failure")

        ret = self._proto.send(data)
        if ret != 0:
            if (self._event_flags.get_bits() & _EventFlags.OPENED) == 0:
                raise TinyprotoClosedError("TX failure")
            if ret == -1:
                raise TinyprotoRecoverableError("Send request cancelled")
            elif ret == -2:
                if self._proto.get_status() == 0:
                    raise TinyprotoTimeoutError("TX timed out")
                else:
                    raise TinyprotoDisconnectedError("TX failure")
            elif ret == -3:
                raise TinyprotoRecoverableError("Data too large")
            else:
                assert False

    def receive(self):
        if self._rx_fifo.qsize() > 0:
            try:
                return self._rx_fifo.get(block=False)
            except queue.Empty:
                raise AssertionError("Queue must contain at least one item")
        event_flags = self._event_flags.wait_bits(
            _EventFlags.NEW_FRAME_RX_PENDING,
            _EventFlags.CONNECTED | _EventFlags.OPENED,
            timeout=self._receive_timeout,
            # Clear new frame rx pending
            op=lambda flags: flags & ~(_EventFlags.NEW_FRAME_RX_PENDING),
        )
        if (event_flags & _EventFlags.CONNECTED) == 0:
            raise TinyprotoDisconnectedError("RX failure")
        if (event_flags & _EventFlags.OPENED) == 0:
            raise TinyprotoClosedError("RX failure")
        if event_flags & _EventFlags.NEW_FRAME_RX_PENDING:
            try:
                return self._rx_fifo.get(block=False)
            except queue.Empty:
                raise AssertionError("Queue must contain at least one item")
        if not self._rx_thread.is_alive():
            raise TinyprotoTxThreadDead("RX failure")

        # Arrived here. Nothing happened and timeout expired
        raise TinyprotoTimeoutError("RX failure")
