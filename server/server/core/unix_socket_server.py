import socket

# TODO: Make this class's interface similar to WebSocketServer's (bind, send, etc)
class UnixSocketServer:
    # TODO: Make this async, like WebSocketServer does
    def serve(self):
        socket_ = socket.socket(socket.AF_UNIX, socket.SOCK_SEQPACKET)
        try:
            socket_.connect('/tmp/hivexplore/socket.sock')
        except (FileNotFoundError, socket.timeout) as exc:
            print('UnixSockerServer error:', exc)
            return

        try:
            while True:
                try:
                    input_ = input('> ') + '\n'
                    socket_.send(input_.encode('utf-8'))
                except KeyboardInterrupt as exc:
                    break
        finally:
            socket_.close()
