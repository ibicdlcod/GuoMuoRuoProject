from src_common import network_default
import socket

if __name__ == '__main__':
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((network_default.DEFAULT_SERVER_LISTEN, network_default.DEFAULT_PORT))
        s.listen(network_default.MAXIMUM_ALLOWED_CONNECTIONS)
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                conn.sendall(data)
