#!/usr/bin/python3
from scapy.all import Ether, IP, UDP, Raw, conf, raw
import time
import random

def generate_cover_traffic(p_size=1200):
    # Destination list for rotation (1.1.1.1 removed)
    destinations = [
        "76.76.2.2", 
        "76.76.10.2", 
        "182.222.222.222", 
        "45.11.45.11", 
        "84.200.69.80", 
        "84.200.70.40"
    ]
    
    # --- PERFORMANCE OPTIMIZATION: PRE-SERIALIZATION ---
    # Convert all packets to raw binary blobs once at startup
    payload = b"\x00" * (p_size - 42)
    prepared_packets = []
    
    for ip in destinations:
        pkt = (Ether() / 
               IP(dst=ip, ttl=128) / 
               UDP(sport=443, dport=443) / 
               Raw(load=payload))
        prepared_packets.append(raw(pkt))
    
    # Open persistent Layer 2 socket for direct hardware access (Max Speed)
    s = conf.L2socket(iface=conf.iface)

    while True:
        try:
            # Pick a pre-built binary blob from the pool
            pkt_bytes = random.choice(prepared_packets)
            
            # Send batch of 15 packets to sustain 1Mbit/s
            for _ in range(15):
                s.send(pkt_bytes)
            
            # Static jitter to keep the line saturated at ~1Mbit/s
            time.sleep(random.uniform(0.02, 0.05))
            
        except Exception:
            # Recovery: wait and re-open socket if interface resets
            time.sleep(1)
            s = conf.L2socket(iface=conf.iface)

if __name__ == "__main__":
    generate_cover_traffic()
