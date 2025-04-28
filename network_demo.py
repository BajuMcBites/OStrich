import socket
import time 

"""
todo: stuff to look at
- _initialize (doesn't mattter)
- dequeue (udp specific dequeue)
- udp specific send function
- server socket accept
- client bind
- server can be in python :P
"""

def connect_to_localhost_tcp(port):
    host = '127.0.0.1'  # localhost
    
    try:
        # Create a TCP socket
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:

            s.connect((host, port))

            # 1. Enable/disable Nagle’s algorithm
            #    - TCP_NODELAY=1 → disable Nagle (send small writes immediately)
            # s.setsockopt(socket.IPPROTO_TCP, socket.TCP_NODELAY, 1)

            # 2. Tune the send/receive buffer sizes
            #    (defaults vary by OS; bump them if you want larger in‑kernel buffers)
            # s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 4*1440)
            # s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1440)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1514)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_SNDBUF, 1440)

            # 3. Enable TCP keep‑alives on the socket
            #    (detects a dead peer after the keep‑alive idle/time/count thresholds)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            sent_bytes = 0
            recv_bytes = 0
            message = bytearray(500)
            start = time.time()
            
            for i in range(1000):
                s.send(message)

                sent_bytes += len(message)
                print(f'{i}, len(message) = {len(message)}')

                response = s.recv(1024)
                recv_bytes += len(response)

            print(f'took {time.time() - start} seconds, sent_bytes = {sent_bytes}, recv_bytes = {recv_bytes}')

    except Exception as e:
        # ignore
        return

def connect_to_localhost_udp(port):
    host = '127.0.0.1'  # localhost
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
        # disable checksum on receiving
        s.setsockopt(socket.IPPROTO_IP, socket.IP_RECVOPTS, 1)

        for i in range(1000):
            s.sendto(bytearray(512), (host, port))
            print(f'sent {i}')
            time.sleep(0.0000001)
            response, _ = s.recvfrom(5)
            # print(f'response = {response}')

if __name__ == '__main__':
    port = 25566
    connect_to_localhost_udp(port)
