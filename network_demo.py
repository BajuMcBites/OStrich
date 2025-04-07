import socket

def connect_to_localhost(port):
    host = '127.0.0.1'  # localhost
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((host, port))

            for i in range(10):
                message = f"Hello from client {i}!"
                s.sendall(message.encode())
                print(f"Sent: {message}")

                response = s.recv(1024)
                print(f"Received: {response.decode()}")

    except Exception as e:
        # ignore
        return

if __name__ == '__main__':
    port = 2222
    connect_to_localhost(port)
