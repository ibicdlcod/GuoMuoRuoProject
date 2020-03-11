from src_common import network_default
import socket


if __name__ == '__main__':
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((network_default.DEFAULT_HOST, network_default.DEFAULT_PORT))
        s.sendall(b"Hello, world")
        data = s.recv(1024)

    print("Received", repr(data))
