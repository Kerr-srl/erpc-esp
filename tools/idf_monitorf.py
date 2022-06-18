import os
import pty
import queue
import select
import sys
import termios
import threading
import time
import tty

from serial.serialutil import SerialException

import common

import erpc_esp.erpc_esp_log as erpc_esp_log

if __name__ == "__main__":
    args = common.parse_extension_args()

    arg_hide_erpc = args.hide_erpc
    erpc_esp_log_filter = erpc_esp_log.ErpcEspLogFilter()

    with common.idf_monitor_modules():

        import idf_monitor
        import idf_monitor_base.constants as constants
        from idf_monitor_base.output_helpers import yellow_print, red_print

        OriginalSerialReader = idf_monitor.SerialReader

        class SerialReader(OriginalSerialReader):
            current_serial = None

            def start(self):
                SerialReader.current_serial = self.serial
                super().start()

            def stop(self):
                # When the serial reader thread is stopped for any reason
                # (e.g. flashing). Set this to None
                SerialReader.current_serial = None
                super().stop()

        idf_monitor.SerialReader = SerialReader

        #
        #
        OriginalLogger = idf_monitor.Logger

        class Logger(OriginalLogger):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                # open the pseudoterminal
                self.master, self.slave = pty.openpty()

                yellow_print(
                    f"Created ({os.ttyname(self.master)},{os.ttyname(self.slave)}) pty pair"
                )
                yellow_print(f"Use {os.ttyname(self.slave)} pseudoterminal")

                os.set_blocking(self.master, False)
                # Don't echo back. See https://stackoverflow.com/a/46670094
                tty.setraw(self.master, termios.TCSANOW)

                def master_reader_thread_fn(master, stop_event):
                    while not stop_event.is_set():
                        try:
                            (readable, _, _) = select.select([master], [], [], 0.001)
                            if master in readable:
                                bytes = os.read(master, 1)
                                if SerialReader.current_serial:
                                    SerialReader.current_serial.write(bytes)
                        except OSError as e:
                            if not stop_event.is_set():
                                red_print(
                                    f"Unable to read data from PTS {os.strerror(e.errno)}"
                                )
                            break

                def master_writer_thread_fn(
                    master, stop_event, input_queue: queue.SimpleQueue
                ):
                    last_warn_ms = 0
                    WARN_EVERY_SECONDS = 2
                    WARN_ON_QUEUE_SIZE = 5000

                    while not stop_event.is_set():
                        str = input_queue.get()
                        if str is None:
                            break
                        try:
                            while True:
                                (_, writable, _) = select.select([], [master], [], 0.1)
                                if (
                                    input_queue.qsize() > WARN_ON_QUEUE_SIZE
                                    and time.time() - last_warn_ms > WARN_EVERY_SECONDS
                                ):
                                    red_print(
                                        f"A lot of data has queued up waiting to be read from {os.ttyname(self.slave)}"
                                    )
                                    last_warn_ms = time.time()
                                if master in writable:
                                    os.write(master, str)
                                    break
                        except OSError as e:
                            if not stop_event.is_set():
                                red_print(
                                    f"Unable to forward data to PTS {os.strerror(e.errno)}"
                                )
                            break

                self.master_threads_stop_event = threading.Event()

                self.master_reader_thread = threading.Thread(
                    target=master_reader_thread_fn,
                    name="PTY master reader",
                    args=(self.master, self.master_threads_stop_event),
                )
                self.master_reader_thread.start()

                # We use a separate thread to write to the PTS because
                # it has a limited input queue. If no program opens the PTS
                # then its queue will fill up very quickly.
                #
                # Other solutions could be:
                # * Flush PTS input queue using `termios.tcflush(self.slave,
                # termios.TCIFLUSH)` before every write. But this works only if
                # the program that opens the PTS reads from the PTS quickly
                # enough, otherwise data will be lost.
                # * Write to the PTS only if some programs has opened it. We
                # tried to fllow https://stackoverflow.com/a/3490197 and to
                # close(self.slave) after openpty, and then rely on POLLHUP
                # event to determine whether some program opened the PTS.
                # It works, BUT it turns out that close(self.slave) makes it
                # imposible to get input from the PTS via os.read(self.master).
                self.master_writer_input_queue = queue.SimpleQueue()
                self.master_reader_thread = threading.Thread(
                    target=master_writer_thread_fn,
                    name="PTY master writer",
                    args=(
                        self.master,
                        self.master_threads_stop_event,
                        self.master_writer_input_queue,
                    ),
                )
                self.master_reader_thread.start()

            def stop_logging(self):
                super().stop_logging()
                self.master_threads_stop_event.set()
                self.master_writer_input_queue.put(None)

                # Flush PTS's input. Otherwise, even if we close the self.master
                # the os.read(self.master) still blocks...
                termios.tcflush(self.slave, termios.TCIFLUSH)
                os.close(self.slave)

                os.close(self.master)

            def get_printer(self):
                def printer(str):
                    # Forward everything to PTY
                    self.master_writer_input_queue.put(str)

                    # Decide whether to print the line to serial monitor
                    do_print = True
                    if arg_hide_erpc:
                        received = erpc_esp_log_filter.feed_line(str)
                        if received is not None:
                            do_print = False
                    if do_print:
                        print(
                            str.decode("utf-8", errors="replace"),
                            end="",
                            file=sys.stderr,
                            flush=True,
                        )

                return printer

            def print(self, string, console_printer=None):
                super().print(
                    string,
                    console_printer
                    if console_printer is not None
                    else self.get_printer(),
                )

        idf_monitor.Logger = Logger

        idf_monitor.main()
