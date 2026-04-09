#!/usr/bin/python3
from scapy.all import Ether, IP, UDP, Raw, sendp, conf, raw
import time
import random

def generate_cover_traffic(p_size=1200):
    conf.L3socket = conf.L2socket 
    iface = conf.iface
    batch_size = 10
    payload = b"\x00" * (p_size - 42)
    
    while True:
        try:
            for _ in range(batch_size):
                # Using 1.1.1.1 as a "Ghost Destination" for the Fog.
                # TTL 128 is set to remain consistent with the Windows Mask.
                pkt = (Ether() / 
                       IP(dst="1.1.1.1", ttl=128, id=random.randint(1, 65535)) / 
                       UDP(sport=443, dport=443) / 
                       Raw(load=payload))
                sendp(raw(pkt), iface=iface, verbose=False)
            time.sleep(random.uniform(0.05, 0.15))
        except Exception:
            continue

if __name__ == "__main__":
    generate_cover_traffic()
