from src_common import network_default
import socket
import sys
import selectors
import types

sel = selectors.DefaultSelector()


def accept_wrapper(sock):
    conn_acc, addr_acc = sock.accept()
    # the following should be convert to log
    print(f"accepted connection from {addr_acc}")
    conn_acc.setblocking(False)
    data_acc = types.SimpleNamespace(addr=addr_acc, inb = b"", outb = b"")
    events = selectors.EVENT_READ | selectors.EVENT_WRITE
    sel.register(conn_acc, events, data=data_acc)


def service_connection(key, mask):
    sock = key.fileobj
    data_serv = key.data
    if mask & selectors.EVENT_READ:
        recv_data = sock.recv(network_default.MAXIMUM_DATA_BLOCK_SIZE)
        if recv_data:
            data_serv.outb += recv_data
        else:
            # the following should be converted to log
            print(f"closing connection to {data_serv.addr}")
            sel.unregister(sock)
            sock.close()
    if mask & selectors.EVENT_WRITE:
        if data_serv.outb:
            print(f"echoing: {repr(data_serv.outb)} to {data_serv.addr}")
            sent = sock.send(data_serv.outb)
            data_serv.outb = data_serv.outb[sent:]


if __name__ == '__main__':
    print("Args: ", end="")
    for arg in sys.argv:
        print(arg, end="\t")
    print()
    SERVER_LISTEN = sys.argv[1] if len(sys.argv) > 1 else network_default.DEFAULT_SERVER_LISTEN
    PORT = int(sys.argv[2]) if len(sys.argv) > 2 else network_default.DEFAULT_PORT

    l_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    l_sock.bind((SERVER_LISTEN, PORT))
    l_sock.listen()
    print(f"listening on host {SERVER_LISTEN} and port {PORT}")
    l_sock.setblocking(False)
    sel.register(l_sock, selectors.EVENT_READ, data=None)

    try:
        while True:
            events = sel.select(timeout=None)
            for key, mask in events:
                if key.data is None:
                    accept_wrapper(key.fileobj)
                else:
                    service_connection(key, mask)
    except KeyboardInterrupt:
        print("caught keyboard interrupt, exiting")
    finally:
        sel.close()
    # with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    #     s.bind((network_default.DEFAULT_SERVER_LISTEN, network_default.DEFAULT_PORT))
    #     s.listen(network_default.MAXIMUM_ALLOWED_CONNECTIONS)
    #     conn, addr = s.accept()
    #     with conn:
    #         print('Connected by', addr)
    #         while True:
    #             data = conn.recv(1024)
    #             if not data:
    #                 break
    #             conn.sendall(data)
