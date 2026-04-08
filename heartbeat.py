#!/usr/bin/python3
from scapy.all import Ether, IP, UDP, Raw, sendp, conf, raw
import time
import random

def generate_cover_traffic(bitrate_mbps=1, p_size=1200):
    conf.L3socket = conf.L2socket 
    iface = conf.iface
    print(f"[*] ShadowNet: Boosting Pulse to {bitrate_mbps} Mbit/s on {iface}")
    
    # 1 Mbit/s = 125,000 bytes/s
    # 125,000 / 1200 per packet = ~104 packets per second
    # We will send in batches of 10 to overcome Python sleep jitter
    batch_size = 10
    packets_per_second = 104
    batches_per_second = packets_per_second / batch_size
    interval = 1.0 / batches_per_second

    # Pre-build fixed 1200b packet
    payload = b"\x00" * (p_size - 42)
    template_pkt = (Ether()/IP(dst="1.1.1.1")/
                    UDP(sport=443, dport=443)/Raw(load=payload))
    packet_bytes = raw(template_pkt)

    while True:
        try:
            # Fire the batch
            for _ in range(batch_size):
                sendp(packet_bytes, iface=iface, verbose=False)
            
            # Jitter the sleep between batches
            time.sleep(max(0, interval + random.uniform(-0.005, 0.005)))
        except KeyboardInterrupt:
            break
        except:
            continue

if __name__ == "__main__":
    generate_cover_traffic()
