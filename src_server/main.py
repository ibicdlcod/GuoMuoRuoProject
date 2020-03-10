from src_common import read_config

dict1 = read_config.read_primary_config("config\\config.ini", "network")
print(dict1)