import argparse
from enum import Enum
import threading
import time
import erpc
import erpc_esp.erpc_esp_log_transport as erpc_esp_log_transport
from examples.connection.main.gen.hello_world_with_connection_target.client import (
    hello_world_targetClient,
)

import gen.hello_world_host as host
import gen.hello_world_target as target

connected_event = threading.Event()

class StoppableThread(threading.Thread):
    """Thread class with a stop() method. The thread itself has to check
    regularly for the stopped() condition."""

    def __init__(self, *args, **kwargs):
        super(StoppableThread, self).__init__(*args, **kwargs)
        self._stop = threading.Event()

    def stop(self):
        self._stop.set()

    def stopped(self):
        return self._stop.is_set()


def server_thread_fn(server: erpc.simple_server.SimpleServer):
    class hello_world_host_handler(host.interface.Ihello_world_host):
        def say_hello_to_host(self, name, i):
            response = f"Hello {name}"
            print(f'Received call ({i}) from target. Responding with "{response}"')
            return response

        def connect(self):
            print(f"Received connection request from target.")
            connected_event.set()

    service = host.server.hello_world_hostService(hello_world_host_handler())

    server.add_service(service)

    print("Starting server...")
    server.run()


def client_thread_fn(client: hello_world_targetClient):
    print("Starting client...")
    # Create client that uses the shared transport
    client_manager = erpc.client.ClientManager(
        arbitrator.shared_transport, erpc.basic_codec.BasicCodec
    )
    client_manager.arbitrator = arbitrator
    client = target.client.hello_world_targetClient(client_manager)

    i = 0
    connected_event.wait()
    print(f"Sending ack!")
    client.connect_ack()
    while True:
        print(f"Calling target ({i})...")
        response = client.say_hello_to_target("Host", i)
        i = i + 1
        print(f"Target response: {response}")


def main(port: str, baudrate: int):
    # Create shared transport
    serial_transport = erpc_esp_log_transport.SerialEspLogTransport(port, baudrate)
    arbitrator = erpc.arbitrator.TransportArbitrator(
        serial_transport, erpc.basic_codec.BasicCodec()
    )

    # Create server that uses the shared transport
    server = erpc.simple_server.SimpleServer(arbitrator, erpc.basic_codec.BasicCodec)

    client_thread = threading.Thread(
        target=client_thread_fn, name="CLIENT", args=(client,)
    )

    # Create server and client threads
    server_thread = threading.Thread(
        target=server_thread_fn,
        name="SERVER",
        args=(server, client_thread),
    )
    server_thread.start()
    pass


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="ESP32-eRPC demo")
    arg_parser.add_argument("-p", "--port", help="Serial port", required=True)
    arg_parser.add_argument(
        "-b", "--bd", default="115200", help="Baud rate (default value is 115200)"
    )
    args = arg_parser.parse_args()
    main(args.port, args.bd)
