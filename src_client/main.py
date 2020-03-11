from src_common import network_default
import sys
import socket
import selectors
import types

sel = selectors.DefaultSelector()


def start_connections(conn_host, conn_port, number_of_connections, in_messages):
    server_addr = (conn_host, conn_port)
    for i in range(0, number_of_connections):
        connection_id = i + 1
        print("starting connection", connection_id, "to", server_addr)
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.setblocking(False)
        sock.connect_ex(server_addr)
        conn_events = selectors.EVENT_READ | selectors.EVENT_WRITE
        conn_data = types.SimpleNamespace(
            connid=connection_id,
            msg_total=sum(len(m) for m in in_messages),
            recv_total=0,
            messages=list(in_messages),
            outb=b"",
        )
        sel.register(sock, conn_events, data=conn_data)


def service_connection(serv_key, serv_mask):
    sock = serv_key.fileobj
    serv_data = serv_key.data
    if serv_mask & selectors.EVENT_READ:
        recv_data = sock.recv(1024)  # Should be ready to read
        if recv_data:
            print("received", repr(recv_data), "from connection", serv_data.connid)
            serv_data.recv_total += len(recv_data)
        if not recv_data or serv_data.recv_total == serv_data.msg_total:
            print("closing connection", serv_data.connid)
            sel.unregister(sock)
            sock.close()
    if serv_mask & selectors.EVENT_WRITE:
        if not serv_data.outb and serv_data.messages:
            serv_data.outb = serv_data.messages.pop(0)
        if serv_data.outb:
            print("sending", repr(serv_data.outb), "to connection", serv_data.connid)
            sent = sock.send(serv_data.outb)  # Should be ready to write
            serv_data.outb = serv_data.outb[sent:]


if __name__ == '__main__':
    messages = [b"I love you", b"I want to be with you"]

    print("Args: ", end="")
    for arg in sys.argv:
        print(arg, end="\t")
    print()
    HOST = sys.argv[1] if len(sys.argv) > 1 else network_default.DEFAULT_HOST
    PORT = int(sys.argv[2]) if len(sys.argv) > 2 else network_default.DEFAULT_PORT
    MAX_CONN_NUM = int(sys.argv[3]) if len(sys.argv) > 3 else 2

    start_connections(HOST, PORT, MAX_CONN_NUM, messages)

    try:
        while True:
            events = sel.select(timeout=1)
            if events:
                for key, mask in events:
                    service_connection(key, mask)
            # Check for a socket being monitored to continue.
            if not sel.get_map():
                break
    except KeyboardInterrupt:
        print("caught keyboard interrupt, exiting")
    finally:
        sel.close()

