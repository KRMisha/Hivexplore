import socket


def get_local_ip() -> str:
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        DUMMY_IP = '10.255.255.255'
        DUMMY_PORT = 1
        sock.connect((DUMMY_IP, DUMMY_PORT))
        ip_address = sock.getsockname()[0]
    except Exception: # pylint: disable=broad-except
        ip_address = '127.0.0.1'
    finally:
        sock.close()
    return ip_address
