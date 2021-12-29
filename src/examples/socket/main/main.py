import argparse
from enum import Enum
import logging
import threading
import time

import erpc
from rich.logging import RichHandler
import serial

import gen.hello_world_with_socket_host as host
import gen.hello_world_with_socket_target as target

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
    elif role == Role.target:
        client = host.client.hello_world_hostClient(client_manager)
        i = 0
        while True:
            log.info(f"Calling host ({i})...")
            response = client.say_hello_to_host("Target", i)
            i = i + 1
            log.info(f"Host response: {response}")
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


def main(host_ip: str, host_port: int, role: Role):
    transport = erpc.transport.TCPTransport(host_ip, host_port, role == Role.host)
    arbitrator = erpc.arbitrator.TransportArbitrator(
        transport, erpc.basic_codec.BasicCodec())

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

    transport.close()

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description='ESP32-eRPC demo')
    arg_parser.add_argument('host_ip', help='Destination IP address')
    arg_parser.add_argument('-p', '--port', default=28080,
                            help='Port number. Default to 28080')
    arg_parser.add_argument('-r', '--role', choices=list(r.name.lower() for r in Role), default=Role.host.name,
                            help='Whether to act as host or as target')
    args = arg_parser.parse_args()
    main(args.host_ip, args.port, Role[args.role])
