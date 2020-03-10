from src_common import read_config
import os

dict1 = read_config.read_primary_config(os.path.join("config", "config.ini"), "network")
print(dict1)