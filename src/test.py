#!/usr/bin/env python3
from scapy.all import *
import sys

def perform_handshake(dst_ip, dst_port):
    # Generate a random source port
    src_port = RandShort()
    
    # Build IP and TCP (SYN) packet
    ip = IP(dst=dst_ip)
    syn = TCP(sport=src_port, dport=dst_port, flags='S', seq=1000)
    print(f"[*] Sending SYN to {dst_ip}:{dst_port}")
    
    # Send SYN and wait for SYN-ACK
    syn_ack = sr1(ip/syn, timeout=2, verbose=0)
    if not syn_ack:
        print("[-] No response. Exiting.")
        sys.exit(1)
    
    print("[+] Received SYN-ACK:")
    syn_ack.show()  # Displays the details of the SYN-ACK packet

    # Craft and send the ACK packet to complete the handshake
    ack = TCP(sport=src_port, dport=dst_port, flags='A',
              seq=syn.seq+1, ack=syn_ack.seq+1)
    send(ip/ack, verbose=0)
    print("[*] Sent ACK. TCP handshake completed.")

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <IP> <Port>")
        sys.exit(1)
        
    dst_ip = sys.argv[1]
    dst_port = int(sys.argv[2])
    perform_handshake(dst_ip, dst_port)