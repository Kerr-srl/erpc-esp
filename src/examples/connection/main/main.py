import os
import argparse
import sys
import threading
import time
from typing import Union
import erpc
import erpc_tinyproto
import serial
from enum import Enum
import logging

from rich.logging import RichHandler
from rich.console import Console
from rich.layout import Layout

import gen.hello_world_with_connection_host as host
import gen.hello_world_with_connection_target as target

FORMAT = "%(message)s"
logging.basicConfig(
    level="NOTSET", format=FORMAT, datefmt="[%X]", handlers=[RichHandler(
        rich_tracebacks=True, omit_repeated_times=False,
    )]
)

log = logging.getLogger("rich")


class Role(Enum):
    host = "host",
    target = "target",


def server_thread_fn(server: erpc.simple_server.SimpleServer, role: Role):
    class hello_world_host_handler(host.interface.Ihello_world_host):
        def say_hello_to_host(self, name, i):
            response = f"Hello {name}"
            log.info(
                f"Received call ({i}) from target. Responding with \"{response}\"")
            return response

    class hello_world_target_handler(target.interface.Ihello_world_target):
        def say_hello_to_target(self, name, i):
            response = f"Hello {name}"
            log.info(
                f"Received call ({i}) from host. Responding with \"{response}\"")
            return response

    if role == Role.host:
        service = host.server.hello_world_hostService(
            hello_world_host_handler())
    elif role == Role.target:
        service = target.server.hello_world_targetService(
            hello_world_target_handler())
    else:
        assert False

    server.add_service(service)
    server.run()


def client_thread_fn(client_manager: erpc.client.ClientManager, role: Role):
    if role == Role.host:
        client = target.client.hello_world_targetClient(client_manager)
        i = 0
        while True:
            log.info(f"Calling target ({i})...")
            response = client.say_hello_to_target("Host", i)
            i = i + 1
            log.info(f"Target response: {response}")
            time.sleep(1)
    elif role == Role.target:
        client = host.client.hello_world_hostClient(client_manager)
        i = 0
        while True:
            log.info(f"Calling host ({i})...")
            response = client.say_hello_to_host("Target", i)
            i = i + 1
            log.info(f"Host response: {response}")
            time.sleep(1)
    else:
        assert False


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


def main(port: str, baudrate: int, role: Role, reset: bool, log_file_path: Union[str, None]):
    serial_port = serial.serial_for_url(port, baudrate=baudrate, timeout=0.01)

    log_file = None
    if log_file_path:
        log_file = open(log_file_path, "w")

    if reset:
        reset_esp32(serial_port)

    def write_func(data: bytearray):
        try:
            written = serial_port.write(data)
        except Exception:
            log.exception("Unable to write")
            written = 0

        return written

    def read_func(max_count):
        try:
            data = serial_port.read(max_count)
        except Exception:
            data = bytes()
            log.exception("Unable to read")
        text = data.decode('utf-8', errors="ignore")
        if log_file:
            log_file.write(text)
            log_file.flush()
        else:
            print(text, end='', file=sys.stderr)
        data = bytearray(data)
        return data

    # Create shared transport
    tinyproto_transport = erpc_tinyproto.TinyprotoTransport(
        read_func, write_func, receive_timeout=5, send_timeout=5)
    log.info("Opening transport")
    tinyproto_transport.connect(timeout=10)

    arbitrator = erpc.arbitrator.TransportArbitrator(
        tinyproto_transport, erpc.basic_codec.BasicCodec())

    # Create client that uses the shared transport
    client_manager = erpc.client.ClientManager(
        arbitrator.shared_transport, erpc.basic_codec.BasicCodec)
    client_manager.arbitrator = arbitrator

    # Create server that uses the shared transport
    server = erpc.simple_server.SimpleServer(
        arbitrator, erpc.basic_codec.BasicCodec)

    if role == Role.host:
        log.info("Running as host")
    elif role == Role.target:
        log.info("Running as target")
    else:
        assert False

    server_thread = threading.Thread(
        target=server_thread_fn, name='SERVER', args=(server, role))
    client_thread = threading.Thread(
        target=client_thread_fn, name='CLIENT', args=(client_manager, role))

    log.info("Starting server...")
    server_thread.start()
    log.info("Starting client...")
    client_thread.start()

    server_thread.join()
    client_thread.join()
    tinyproto_transport.close()

    serial_port.close()


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description='ESP32-eRPC demo')
    arg_parser.add_argument('-p', '--port', help='Serial port', required=True)
    arg_parser.add_argument('-b', '--bd', default='115200',
                            help='Baud rate (default value is 115200)')
    arg_parser.add_argument('-r', '--role', choices=list(r.name.lower() for r in Role), default=Role.host.name,
                            help='Whether to act as host or as target')
    arg_parser.add_argument('-c', '--reset', default=False, action='store_true',
                            help='Whether to reset the ESP32 target. Should not be used when using a virtual serial port (e.g. created using socat)')
    arg_parser.add_argument('--log', dest="log_file", default=None, type=str,
                            help='Redirect output of the target into the given file')
    args = arg_parser.parse_args()
    main(args.port, args.bd, Role[args.role], args.reset, args.log_file)
