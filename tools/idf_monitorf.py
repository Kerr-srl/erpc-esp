import errno
import os
import pty
import select
import sys
import termios
import threading
import tty

import serial
from serial.threaded import ReaderThread, Protocol

from common import add_path

if __name__ == "__main__":
    with add_path(os.path.join(os.environ["IDF_PATH"], "tools")):

        import idf_monitor
        import idf_monitor_base.constants as constants
        from idf_monitor_base.output_helpers import yellow_print

        OriginalSerialReader = idf_monitor.SerialReader

        class SerialReader(OriginalSerialReader):
            current_serial = None

            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)
                SerialReader.current_serial = self.serial

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

                # Inspired by https://stackoverflow.com/a/3490197
                # Note that due to openpty the self.slave fd is already open
                # We just need to close it.
                os.close(self.slave)
                self.master_hup_poll = select.poll()
                self.master_hup_poll.register(self.master)
                self.master_hup = True

                # Don't echo back. See https://stackoverflow.com/a/46670094
                tty.setraw(self.master, termios.TCSANOW)

                def master_reader_thread_fn(master, stop_event):
                    while not stop_event.is_set():
                        try:
                            bytes = os.read(master, 1)
                        except OSError as e:
                            break

                        if SerialReader.current_serial:
                            SerialReader.current_serial.write(bytes)

                self.master_reader_thread_stop_event = threading.Event()
                self.master_reader_thread = threading.Thread(
                    target=master_reader_thread_fn,
                    name="PTY master reader",
                    args=(self.master, self.master_reader_thread_stop_event),
                )
                self.master_reader_thread.start()

            def stop_logging(self):
                super().stop_logging()
                # Close ptys.
                # Reader thread will automatically unblock and terminate
                os.close(self.master)
                os.close(self.slave)
                self.master_reader_thread_stop_event.set()

            def get_printer(self):
                def printer(str):
                    res = self.master_hup_poll.poll(0.0001)
                    if len(res) > 0:
                        if res[0][1] & select.POLLHUP:
                            self.master_hup = True
                        else:
                            self.master_hup = False
                    if not self.master_hup:
                        # If the self.master fd has not the POLLHUP event
                        # it means that somebody opened the slave pty.
                        os.write(self.master, str)
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
