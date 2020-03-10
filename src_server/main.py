from src_common import read_config
import os
import socket

config_dict = read_config.read_primary_config(os.path.join("config", "config.ini"), "network")
DEFAULT_HOST = config_dict["default_host"] if "default_server_listen" in config_dict.keys() else socket.gethostname()
DEFAULT_PORT = int(config_dict["default_port"]) if "default_port" in config_dict.keys() else 1826
print(DEFAULT_HOST, DEFAULT_PORT)

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((DEFAULT_HOST, DEFAULT_PORT))
    s.listen()
    conn, addr = s.accept()
    with conn:
        print('Connected by', addr)
        while True:
            data = conn.recv(1024)
            if not data:
                break
            conn.sendall(data)