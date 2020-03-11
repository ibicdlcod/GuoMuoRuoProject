from src_common import read_config
import os
import socket

config_dict = read_config.read_primary_config(os.path.join("config", "config.ini"), "network")
DEFAULT_SERVER_LISTEN = (config_dict["default_server_listen"]
                         if "default_server_listen" in config_dict.keys()
                         else socket.gethostname()
                         )
DEFAULT_PORT = int(config_dict["default_port"]) if "default_port" in config_dict.keys() else 1826
DEFAULT_HOST = config_dict["default_host"] if "default_host" in config_dict.keys() else socket.gethostname()
MAXIMUM_ALLOWED_CONNECTIONS = (int(config_dict["maximum_connections"])
                               if "maximum_connections" in config_dict.keys() else 20736)