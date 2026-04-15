#!/usr/bin/python3
import sys
import os
import time
import random
from scapy.all import Ether, IP, UDP, Raw, conf, raw, DNS, DNSQR

def get_entropy_jitter():
    """Pulls a raw byte from /dev/urandom and converts it to a chaotic micro-delay."""
    random_byte = ord(os.urandom(1))
    return (random_byte / 255.0) * 0.040 + 0.010

def get_dns_iat():
    """Generates a longer entropy delay specifically for DNS decoupling."""
    return random.uniform(0.5, 3.0)

def generate_cover_traffic(p_size=1200):
    destinations = [
        "76.76.2.2", "76.76.10.2", "182.222.222.222", 
        "45.11.45.11", "84.200.69.80", "84.200.70.40"
    ]
    
    # Standard Cover Payload
    payload = b"\x00" * (p_size - 42)
    prepared_packets = []
    
    for ip in destinations:
        pkt = (Ether() / 
               IP(dst=ip, ttl=128) / 
               UDP(sport=443, dport=443) / 
               Raw(load=payload))
        prepared_packets.append(raw(pkt))
    
    # --- ADDED: DNS ENTROPY DECOUPLING PACKETS ---
    fake_domains = ["google.com", "bing.com", "duckduckgo.com", "protonmail.com", "github.com"]
    dns_packets = []
    for ip in destinations:
        dns_pkt = (Ether() / 
                   IP(dst=ip, ttl=128) / 
                   UDP(sport=random.randint(49152, 65535), dport=53) / 
                   DNS(rd=1, qd=DNSQR(qname=random.choice(fake_domains))))
        dns_packets.append(raw(dns_pkt))
    # ----------------------------------------------

    s = conf.L2socket(iface=conf.iface)
    last_dns_time = time.time()

    while True:
        try:
            curr_time = time.time()
            
            # Inject entropy-timed DNS noise to mask real DNS requests
            if curr_time - last_dns_time > get_dns_iat():
                s.send(random.choice(dns_packets))
                last_dns_time = curr_time
            
            # Standard Heartbeat
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
