import argparse
import threading
import time
import erpc

import gen.hello_world_host as host
import gen.hello_world_target as target


def server_thread_fn(server: erpc.simple_server.SimpleServer):
    class hello_world_target_handler(host.interface.Ihello_world_host):
        def say_hello_to_target(self, name):
            return f"Hello {name}"

    service = target.server.hello_world_targetService(
        hello_world_target_handler())
    server.add_service(service)
    server.run()
    print("Server stopped")


def client_thread_fn(client_manager: erpc.client.ClientManager):
    while True:
        print("Calling host...")
        client = host.client.hello_world_hostClient(client_manager)
        response = client.say_hello_to_host("Fake target")
        print(f"Host response: {response}")
        time.sleep(5)


def main(port: str, baudrate: int):
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
        target=server_thread_fn, name='SERVER', args=(server,))
    client_thread = threading.Thread(
        target=client_thread_fn, name='CLIENT', args=(client_manager,))
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
    args = arg_parser.parse_args()
    main(args.port, args.bd)
