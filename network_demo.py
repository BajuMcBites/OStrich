import socket
import time 

def connect_to_localhost(port):
    host = '127.0.0.1'  # localhost
    
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

            s.connect((host, port))

            # 1. Enable/disable Nagle’s algorithm
            #    - TCP_NODELAY=1 → disable Nagle (send small writes immediately)
            s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            # 2. Tune the send/receive buffer sizes
            #    (defaults vary by OS; bump them if you want larger in‑kernel buffers)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 8*1024)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 8*1024)

            # 3. Enable TCP keep‑alives on the socket
            #    (detects a dead peer after the keep‑alive idle/time/count thresholds)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            sent_bytes = 0
            recv_bytes = 0
            message = ("0" * 500).encode()
            start = time.time()
            
            for i in range(1000):
                s.send(message)

                sent_bytes += len(message)
                print(f'{i}')

                # response = s.recv(1024)
                # recv_bytes += len(response)

            print(f'took {time.time() - start} seconds, sent_bytes = {sent_bytes}, recv_bytes = {recv_bytes}')

    except Exception as e:
        # ignore
        return
    

if __name__ == '__main__':
    port = 25565
    connect_to_localhost(port)
