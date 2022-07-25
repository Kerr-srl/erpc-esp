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

        :param set IntFlag: bitflags to be waited to be set. Pass IntFlag(0) to
        not wait on any bit setting event.
        :param cleared IntFlag: bitflags to be waited to be cleared. Pass
        IntFlag(0) to not wait on any bit clearing event.
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
            if set != 0:
                if all_set:
                    set_satisfied = set == set_bits
                else:
                    set_satisfied = set_bits >= set
            clear_satisfied = False
            if cleared != 0:
                if all_cleared:
                    clear_satisfied = cleared == cleared_bits
                else:
                    clear_satisfied = cleared_bits >= cleared
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
    POSSIBLE_NEW_TX_PENDING = auto()
    NEW_DISCONNECTION_EVENT_PENDING = auto()


class TinyprotoTransport(erpc.transport.Transport):
    class RxThread(StoppableThread):
        # Why is the string "TinyprotoTransport" used as type annotation?
        # See https://stackoverflow.com/a/33533514
        def __init__(self, transport: "TinyprotoTransport", *args, **kwargs):
            super(TinyprotoTransport.RxThread, self).__init__(*args, **kwargs)
            self.transport = transport

        def run(self):
            while not self.stopped():
                read_bytes = self.transport._read_func(2048)
                if len(read_bytes) > 0:
                    self.transport._proto.rx(read_bytes)
                    # Something was received. Potentially there is need to send
                    # ACK.
                    self.transport._event_flags.set_bits(
                        _EventFlags.POSSIBLE_NEW_TX_PENDING
                    )

    class TxThread(StoppableThread):
        def __init__(self, transport: "TinyprotoTransport", *args, **kwargs):
            super(TinyprotoTransport.TxThread, self).__init__(*args, **kwargs)
            self.transport = transport

        def run(self):
            while not self.stopped():
                # Get TX data
                to_send = self.transport._proto.tx()
                if len(to_send) > 0:
                    # Send it
                    self.transport._write_func(to_send)
                    # Yield. Don't starve other threads.
                    time.sleep(0.0001)
                else:
                    # When there is nothing to send, wait on this event flag
                    # until there's something else to send. Don't just busy
                    # loop, which could lead to starvation of other threads.
                    self.transport._event_flags.wait_bits(
                        _EventFlags.POSSIBLE_NEW_TX_PENDING,
                        IntFlag(0),
                        # But we also want to wake up from time to time
                        # because:
                        # * we need to check whether the thread should be
                        # stopped;
                        # * TinyProto may need to send something on its own
                        # (e.g. some periodc keep alive frames), which are not
                        # handled by the POSSIBLE_NEW_TX_PENDING flag.
                        timeout=0.01,
                        # Clear on exit
                        op=lambda flags: flags & ~(_EventFlags.POSSIBLE_NEW_TX_PENDING),
                    )

    def __init__(
        self,
        read_func,
        write_func,
        send_timeout: float = 0.5,
        receive_timeout: float = None,
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
            self._rx_fifo.put(data, block=True)
            self._event_flags.set_bits(_EventFlags.NEW_FRAME_RX_PENDING)

        def on_connect_event(address, connected):
            if connected:
                self._event_flags.set_bits(_EventFlags.CONNECTED)
            else:
                # Reset protocol on disconnection
                # self._proto.begin()
                self._event_flags.clear_bits(_EventFlags.CONNECTED)
                self._event_flags.set_bits(_EventFlags.NEW_DISCONNECTION_EVENT_PENDING)

        self._proto.on_read = on_read
        self._proto.on_connect_event = on_connect_event

        ret = self._proto.begin()
        if ret != 0:
            raise TinyprotoUnRecoverableError(
                f"Unable to begin tinyproto. Error code {ret}"
            )

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

        NOTE that when tinyproto is already connected, calling this
        would immediately return, but it is not always true that the
        connection is still on. Disconnection detection takes a while,
        depending on the keep alive timeout.
        Check also wait_disconnection

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

    def wait_disconnection(self, timeout: float = None):
        """
        Wait for disconnection to happen.

        :param timeout timeout in seconds.
        """
        assert self._event_flags.get_bits() & _EventFlags.OPENED

        event_flags = self._event_flags.wait_bits(
            _EventFlags.NEW_DISCONNECTION_EVENT_PENDING,
            _EventFlags.OPENED,
            timeout=timeout,
            # Clear new disconnection event pending
            op=lambda flags: flags & ~(_EventFlags.NEW_DISCONNECTION_EVENT_PENDING),
        )
        if (event_flags & _EventFlags.OPENED) == 0:
            raise TinyprotoClosedError("Connection closed")
        elif (event_flags & _EventFlags.NEW_DISCONNECTION_EVENT_PENDING) == 0:
            raise TinyprotoTimeoutError("Disconnection didn't happen")

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
        else:
            # Successfully queued some data to be sent. Unblock the TX thread
            # immediately, if it is waiting for new TX data.
            self._event_flags.set_bits(_EventFlags.POSSIBLE_NEW_TX_PENDING)

    def receive(self):
        if self._rx_fifo.qsize() > 0:
            try:
                self._event_flags.clear_bits(_EventFlags.NEW_FRAME_RX_PENDING)
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
