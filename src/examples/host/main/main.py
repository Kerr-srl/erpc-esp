import argparse
from subprocess import PIPE, Popen
import sys
import threading
import time
from typing import Optional, TypeVar

import erpc
import erpc_esp.erpc_tinyproto as erpc_tinyproto

import gen.hello_world_host as host
import gen.hello_world_target as target


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


class ServerThread(StoppableThread):
    def __init__(
        self,
        server: erpc.simple_server.SimpleServer,
        transport: erpc_tinyproto.TinyprotoTransport,
        *args,
        **kwargs,
    ):
        super(ServerThread, self).__init__(*args, **kwargs)
        self._server = server
        self._transport = transport

    def run(self):
        class hello_world_host_handler(host.interface.Ihello_world_host):
            def say_hello_to_host(self, name, i):
                print(f'Received call ({i}) from "{name}"')

        service = host.server.hello_world_hostService(hello_world_host_handler())
        self._server.add_service(service)

        print(f"eRPC server: Waiting connection")
        self._transport.wait_connected()

        while not self.stopped():
            try:
                self._server.run()
            except erpc_tinyproto.TinyprotoRecoverableError as e:
                if isinstance(e, erpc_tinyproto.TinyprotoTimeoutError):
                    print(f"eRPC server: {e}")
                elif isinstance(e, erpc_tinyproto.TinyprotoDisconnectedError):
                    print("eRPC server: Waiting until connected")
                    self._transport.wait_connected()
                elif (
                    isinstance(e, erpc_tinyproto.TinyprotoClosedError) or self.stopped()
                ):
                    break
                else:
                    raise e

    def stop(self):
        super().stop()
        self._server.stop()


def interact(locals):
    # Copied from https://stackoverflow.com/a/50918009
    import code
    import readline
    import rlcompleter

    readline.set_completer(rlcompleter.Completer(locals).complete)
    readline.parse_and_bind("tab: complete")
    console = code.InteractiveConsole(locals)
    console.interact("INTERACT WITH THE PROGRAM VIA THE 'client' OBJECT!")


class ClientThread(StoppableThread):
    def __init__(
        self,
        client_manager: erpc.client.ClientManager,
        transport: erpc_tinyproto.TinyprotoTransport,
        repl: bool = False,
        *args,
        **kwargs,
    ):
        super(ClientThread, self).__init__(*args, **kwargs)
        self._client_manager = client_manager
        self._transport = transport
        self._repl = repl

    def run(self):
        client = target.client.hello_world_targetClient(self._client_manager)
        i = 0

        print(f"eRPC client: Waiting connection")
        self._transport.wait_connected()

        if self._repl:
            interact(locals())
            self.stop()

        while not self.stopped():
            print(f"Calling target ({i})...")
            try:
                client.say_hello_to_target("Python host", i)
            except erpc_tinyproto.TinyprotoRecoverableError as e:
                if isinstance(e, erpc_tinyproto.TinyprotoTimeoutError):
                    print(f"eRPC client: {e}")
                elif isinstance(e, erpc_tinyproto.TinyprotoDisconnectedError):
                    print("eRPC client: Waiting until connected")
                    self._transport.wait_connected()
                elif (
                    isinstance(e, erpc_tinyproto.TinyprotoClosedError) or self.stopped()
                ):
                    break
                else:
                    raise e

            i = i + 1


T = TypeVar("T")


def _assert_not_none(obj: Optional[T]) -> T:
    assert obj is not None
    return obj


def main(program: str, repl: bool):
    esp_app = Popen(
        program,
        # Unbuffered pipe
        bufsize=0,
        stdin=PIPE,
        stdout=PIPE,
        stderr=sys.stderr,
    )

    def write_func(data: bytearray):
        try:
            written = _assert_not_none(esp_app.stdin).write(data)
            _assert_not_none(esp_app.stdin).flush()
        except Exception as e:
            print(e)
            raise e
        return written

    def read_func(max_count):
        data = _assert_not_none(esp_app.stdout).read(max_count)
        return data

    # Create shared transport
    tinyproto_transport = erpc_tinyproto.TinyprotoTransport(
        read_func, write_func, send_timeout=5
    )

    tinyproto_transport.open()

    arbitrator = erpc.arbitrator.TransportArbitrator(
        tinyproto_transport, erpc.basic_codec.BasicCodec()
    )

    # Create client that uses the shared transport
    client_manager = erpc.client.ClientManager(
        arbitrator.shared_transport, erpc.basic_codec.BasicCodec
    )
    client_manager.arbitrator = arbitrator

    # Create server that uses the shared transport
    server = erpc.simple_server.SimpleServer(arbitrator, erpc.basic_codec.BasicCodec)

    server_thread = ServerThread(server, tinyproto_transport)
    client_thread = ClientThread(client_manager, tinyproto_transport, repl)

    try:
        print("Starting server...")
        server_thread.start()
        print("Starting client...")
        client_thread.start()
        while server_thread.is_alive() and client_thread.is_alive():
            time.sleep(0.01)
    except (KeyboardInterrupt, SystemExit):
        pass
    finally:
        tinyproto_transport.disconnect()
        tinyproto_transport.close()
        server_thread.stop()
        client_thread.stop()
        server_thread.join()
        client_thread.join()
        esp_app.terminate()
        print(f"ESP-APP terminated with return code: {esp_app.returncode}")


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="ESP32-eRPC demo")
    arg_parser.add_argument("program", help="The program to interact with via eRPC")
    arg_parser.add_argument(
        "--repl",
        default=False,
        action="store_true",
        help="Interact with the program via REPL, instead of sending burst eRPC calls",
    )
    args = arg_parser.parse_args()
    main(args.program, args.repl)
