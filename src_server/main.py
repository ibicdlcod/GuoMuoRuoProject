from src_common import read_config
import os
import socket

config_dict = read_config.read_primary_config(os.path.join("config", "config.ini"), "network")
DEFAULT_SERVER_LISTEN = (config_dict["default_server_listen"]
                if "default_server_listen" in config_dict.keys()
                else socket.gethostname()
                         )
DEFAULT_PORT = int(config_dict["default_port"]) if "default_port" in config_dict.keys() else 1826
MAXIMUM_ALLOWED_CONNECTIONS = (int(config_dict["maximum_connections"])
                               if "maximum_connections" in config_dict.keys() else 20736)


if __name__ == '__main__':
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.bind((DEFAULT_SERVER_LISTEN, DEFAULT_PORT))
        s.listen(MAXIMUM_ALLOWED_CONNECTIONS)
        conn, addr = s.accept()
        with conn:
            print('Connected by', addr)
            while True:
                data = conn.recv(1024)
                if not data:
                    break
                conn.sendall(data)