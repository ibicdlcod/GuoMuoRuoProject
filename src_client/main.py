from src_common import read_config
import os
import socket

config_dict = read_config.read_primary_config(os.path.join("config", "config.ini"), "network")
DEFAULT_HOST = config_dict["default_host"] if "default_host" in config_dict.keys() else socket.gethostname()
DEFAULT_PORT = int(config_dict["default_port"]) if "default_port" in config_dict.keys() else 1826


if __name__ == '__main__':
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect((DEFAULT_HOST, DEFAULT_PORT))
        s.sendall(b"Hello, world")
        data = s.recv(1024)

    print("Received", repr(data))