#!/usr/bin/python3
import sys
import os
import time
import random
from scapy.all import Ether, IP, UDP, Raw, conf, raw

def get_entropy_jitter():
    """Pulls a raw byte from /dev/urandom and converts it to a chaotic micro-delay."""
    random_byte = ord(os.urandom(1))
    return (random_byte / 255.0) * 0.040 + 0.010

def generate_cover_traffic(p_size=1200):
    destinations = [
        "76.76.2.2", "76.76.10.2", "182.222.222.222", 
        "45.11.45.11", "84.200.69.80", "84.200.70.40"
    ]
    
    payload = b"\x00" * (p_size - 42)
    prepared_packets = []
    
    for ip in destinations:
        pkt = (Ether() / 
               IP(dst=ip, ttl=128) / 
               UDP(sport=443, dport=443) / 
               Raw(load=payload))
        prepared_packets.append(raw(pkt))
    
    s = conf.L2socket(iface=conf.iface)

    while True:
        try:
            pkt_bytes = random.choice(prepared_packets)
            burst_size = random.randint(10, 22)
            for _ in range(burst_size):
                s.send(pkt_bytes)
            
            time.sleep(get_entropy_jitter())
            
        except Exception:
            time.sleep(1)
            s = conf.L2socket(iface=conf.iface)

if __name__ == "__main__":
    session_mtu = int(sys.argv[1]) if len(sys.argv) > 1 else 1200
    generate_cover_traffic(p_size=session_mtu)
