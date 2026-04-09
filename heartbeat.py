#!/usr/bin/python3
from scapy.all import Ether, IP, UDP, Raw, conf, raw
import time
import random
import socket

def generate_cover_traffic(p_size=1200):
    # --- Performance Optimization ---
    # We pre-build the packet template to save CPU cycles in the loop
    payload = b"\x00" * (p_size - 42)
    pkt_template = (Ether() / 
                   IP(dst="1.1.1.1", ttl=128) / 
                   UDP(sport=443, dport=443) / 
                   Raw(load=payload))
    
    # Pre-serialize the packet into raw bytes
    pkt_bytes = raw(pkt_template)
    
    # Open a persistent Layer 2 socket for direct hardware access
    # This bypasses the overhead of reopening sockets every loop
    s = conf.L2socket(iface=conf.iface)

    while True:
        try:
            # Send a batch of 15 packets to ensure we stay above 100kbit
            for _ in range(15):
                s.send(pkt_bytes)
            
            # Randomized Jitter (0.04s to 0.12s) 
            # Slightly tightened for better consistency
            time.sleep(random.uniform(0.04, 0.12))
        except Exception:
            # Auto-recovery: Re-open socket if it stalls
            time.sleep(1)
            s = conf.L2socket(iface=conf.iface)

if __name__ == "__main__":
    generate_cover_traffic()
