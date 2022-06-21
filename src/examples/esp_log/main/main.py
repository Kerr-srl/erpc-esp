import argparse
import sys
import threading
import time

import serial

import erpc
import erpc_esp.erpc_tinyproto as erpc_tinyproto
import erpc_esp.erpc_esp_log as erpc_esp_log

import gen.hello_world_with_esp_log_host as host
import gen.hello_world_with_esp_log_target as target


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


class ClientThread(StoppableThread):
    def __init__(
        self,
        client_manager: erpc.client.ClientManager,
        transport: erpc_tinyproto.TinyprotoTransport,
        *args,
        **kwargs,
    ):
        super(ClientThread, self).__init__(*args, **kwargs)
        self._client_manager = client_manager
        self._transport = transport

    def run(self):
        client = target.client.hello_world_targetClient(self._client_manager)
        i = 0

        print(f"eRPC client: Waiting connection")
        self._transport.wait_connected()

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


def reset_esp32(serial_port: serial.Serial):
    # Code copied from idf.py monitor to reset the target
    # Tested on ESP32-S2-Saola-1 dev board
    serial_port.dtr = False
    serial_port.rts = False
    serial_port.dtr = serial_port.dtr
    serial_port.setRTS(True)
    serial_port.setDTR(serial_port.dtr)
    time.sleep(0.2)
    serial_port.setRTS(False)
    serial_port.setDTR(serial_port.dtr)


def main(port: str, baudrate: int, reset: bool, print_logs: bool):
    serial_port = serial.serial_for_url(port, baudrate=baudrate, timeout=0.01)
    serial_port.reset_input_buffer()
    serial_port.reset_output_buffer()

    if reset:
        reset_esp32(serial_port)

    def write_func(data: bytearray):
        try:
            written = serial_port.write(data)
        except Exception as e:
            print(e)
            raise e
        return written

    erpc_esp_log_filter = erpc_esp_log.ErpcEspLogFilter()

    def read_func(max_count):
        while True:
            try:
                data = serial_port.readline()
            except Exception as e:
                print(e)
                raise e
            received = erpc_esp_log_filter.feed_line(data)
            if received:
                assert len(received) <= max_count
                return received
            elif print_logs and len(data) > 0:
                print(data.decode("utf-8", errors="ignore"), end="", file=sys.stderr)

    # Create shared transport
    tinyproto_transport = erpc_tinyproto.TinyprotoTransport(
        read_func, write_func, receive_timeout=5, send_timeout=5
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
    client_thread = ClientThread(client_manager, tinyproto_transport)

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
        serial_port.close()


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="ESP32-eRPC demo")
    arg_parser.add_argument("-p", "--port", help="Serial port", required=True)
    arg_parser.add_argument(
        "-b", "--bd", default="115200", help="Baud rate (default value is 115200)"
    )
    arg_parser.add_argument(
        "-c",
        "--reset",
        default=False,
        action="store_true",
        help="Whether to reset the ESP32 target. Should not be used when using a virtual serial port (e.g. created using socat)",
    )
    arg_parser.add_argument(
        "--print-logs",
        default=False,
        action="store_true",
        help="Whether to print ESP-IDF logs",
    )
    args = arg_parser.parse_args()
    main(args.port, args.bd, args.reset, args.print_logs)
