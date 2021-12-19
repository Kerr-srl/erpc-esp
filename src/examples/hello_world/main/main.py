import argparse
from enum import Enum
import threading
import time
import erpc

import gen.hello_world_host as host
import gen.hello_world_target as target


class Role(Enum):
    host = "host",
    target = "target",


def server_thread_fn(server: erpc.simple_server.SimpleServer, role: Role):
    class hello_world_host_handler(host.interface.Ihello_world_host):
        def say_hello_to_host(self, name, i):
            response = f"Hello {name}"
            print(
                f"Received call ({i}) from target. Responding with \"{response}\"")
            return response

    class hello_world_target_handler(target.interface.Ihello_world_target):
        def say_hello_to_target(self, name, i):
            response = f"Hello {name}"
            print(
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
            print(f"Calling target ({i})...")
            response = client.say_hello_to_target("Host", i)
            i = i + 1
            print(f"Target response: {response}")
    elif role == Role.target:
        client = host.client.hello_world_hostClient(client_manager)
        i = 0
        while True:
            print(f"Calling host ({i})...")
            response = client.say_hello_to_host("Target", i)
            i = i + 1
            print(f"Host response: {response}")
    else:
        assert False


def main(port: str, baudrate: int, role: Role):
    # Create shared transport
    serial_transport = erpc.transport.SerialTransport(port, baudrate)
    arbitrator = erpc.arbitrator.TransportArbitrator(
        serial_transport, erpc.basic_codec.BasicCodec())

    # Create client that uses the shared transport
    client_manager = erpc.client.ClientManager(
        arbitrator.shared_transport, erpc.basic_codec.BasicCodec)
    client_manager.arbitrator = arbitrator

    # Create server that uses the shared transport
    server = erpc.simple_server.SimpleServer(
        arbitrator, erpc.basic_codec.BasicCodec)

    # Create server and client threads
    server_thread = threading.Thread(
        target=server_thread_fn, name='SERVER', args=(server, role))
    client_thread = threading.Thread(
        target=client_thread_fn, name='CLIENT', args=(client_manager, role))
    print("Starting server...")
    server_thread.start()
    print("Starting client...")
    client_thread.start()
    pass


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description='ESP32-eRPC demo')
    arg_parser.add_argument('-p', '--port', help='Serial port', required=True)
    arg_parser.add_argument('-b', '--bd', default='115200',
                            help='Baud rate (default value is 115200)')
    arg_parser.add_argument('-r', '--role', choices=list(r.name.lower() for r in Role), default=Role.host.name,
                            help='Whether to act as host or as target')
    args = arg_parser.parse_args()
    main(args.port, args.bd, Role[args.role])
