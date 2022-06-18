import os
import sys
import threading

import IPython
from traitlets.config.loader import Config
import common

CTRL_E = "\x05"
CMD_IPYTHON = 20

if __name__ == "__main__":
    args = common.parse_extension_args()

    hide_erpc = args.hide_erpc

    with common.idf_monitor_modules():

        import idf_monitor
        import idf_monitor_base.constants as constants

        #
        #
        OriginalLogger = idf_monitor.Logger

        class Logger(OriginalLogger):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            @classmethod
            def printer(cls, str):
                print(
                    str.decode("utf-8", errors="replace"),
                    end="",
                    file=sys.stderr,
                    flush=True,
                )

            def print(self, string, console_printer=None):
                super().print(
                    string,
                    console_printer if console_printer is not None else Logger.printer,
                )

        idf_monitor.Logger = Logger

        #
        # Override ConsoleParser to add an addtional command
        #
        OriginalConsoleParser = idf_monitor.ConsoleParser

        class ConsoleParser(OriginalConsoleParser):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            def _handle_menu_key(self, c):
                ret = None
                if c == CTRL_E:
                    ret = (constants.TAG_CMD, CMD_IPYTHON)
                else:
                    return super()._handle_menu_key(c)
                self._pressed_menu_key = False
                return ret

        idf_monitor.ConsoleParser = ConsoleParser

        stdin_lock = threading.Lock()

        #
        # Override ConsoleReader with a version that cooperatively shares
        # the console input with IPython.
        #
        OriginalConsoleReader = idf_monitor.ConsoleReader

        class ConsoleReader(OriginalConsoleReader):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            def run(self):
                with stdin_lock:
                    super().run()

        idf_monitor.ConsoleReader = ConsoleReader

        #
        # Override SerialHandler to handle the additional commands that we inserted
        # in ConsoleParser
        #
        OriginalSerialHandler = idf_monitor.SerialHandler

        class SerialHandler(OriginalSerialHandler):
            def __init__(self, *args, **kwargs):
                super().__init__(*args, **kwargs)

            def handle_commands(
                self, cmd, chip, run_make_func, console_reader, serial_reader
            ):
                if cmd == CMD_IPYTHON:
                    # Stop console_reader
                    console_reader.stop()
                    # Acquire stdin lock
                    stdin_lock.acquire()
                    # Restart console_reader, so that the rest of
                    # the original idf.monitor still works and doesn't
                    # handle the death of console_reader
                    # But console_reader will be blocked waiting for stdin_lock
                    console_reader.start()

                    config = Config()
                    config.InteractiveShell.confirm_exit = False

                    def ipython_thread_fn():
                        IPython.start_ipython(argv=[], config=config)
                        # Release stdin. console_reader can start working again
                        stdin_lock.release()

                    ipython_thread = threading.Thread(
                        target=ipython_thread_fn, name="ipython"
                    )
                    ipython_thread.start()
                else:
                    super().handle_commands(
                        cmd, chip, run_make_func, console_reader, serial_reader
                    )

        idf_monitor.SerialHandler = SerialHandler

        idf_monitor.main()
