🛡️ ShadowNet: Flow-Invariant Anonymity Protocol (Tor + Mixnet Techniques)

No longer rely on being unique to be anonymous like regular tor, NOW BEING UNIQUE IS THE TRUE ANONYMITY!

For Kali Linux/Parrot OS Only

Asynchronous Obfuscation Layer:

ShadowNet is an advanced network hardening framework that transforms a standard workstation into a "Private Mixnet of One." By forcing all system traffic through a synchronous, timing-obfuscated, and size-uniform tunnel, it eliminates the behavioral metadata that state-level adversaries use to deanonymize users.
🛡️ Core Evolutionary Features
1. Asynchronous Message Queuing (SFQ)/Jitter Randomized delay/reordering and shuffling

ShadowNet replaces standard linear packet release with Stochastic Fairness Queuing.

    The Logic: Instead of a predictable "tick-tock" delivery, packets are hashed into multiple internal "buckets" and released using a shuffling algorithm. The jitter also delays the start up connection/disconnection randomly, the NSA won't know when you just first connected to ShadowNet and when you disconnected, it's all delayed.

    The Benefit: It destroys Timing Correlation Attacks. By re-shuffling the internal order of packets every 10 seconds (perturb 10), it ensures that the rhythm of data leaving your home never matches the rhythm of data exiting a Tor node.

2. Multi-Tiered Decoy Handshakes

ShadowNet creates a "TLS Noise Floor" before establishing its primary secure tunnel.

    The Logic: Upon initialization, the protocol executes background handshakes with high-traffic, "safe" global CDNs (Google, Cloudflare, Microsoft).

    The Benefit: To an ISP, your initial connection looks like standard web browsing. This masks the "Start-up Signature" of the Tor protocol, blending your entry node connection into a flurry of unremarkable HTTPS traffic.

3. Hardware Clock-Drift Mimicry

ShadowNet moves beyond "Perfect Time Sync" to simulate physical hardware imperfections.

    The Logic: Using adjtimex, the protocol introduces a microscopic, random oscillation (drift) into the system clock.

    The Benefit: Virtual machines and automated bots often have "perfect" millisecond-accurate clocks. Real physical laptops have tiny vibrations that cause time to drift. Mimicking this drift prevents Clock-Skew Fingerprinting, making your machine look like an actual physical device rather than an anonymized instance.

4. Sphinx-Style MTU Clamping (Fixed 1200b)

To defeat Packet Size Analysis, ShadowNet uses kernel-level mangle rules to clamp the Maximum Segment Size (MSS) to exactly 1200 bytes.

    The Benefit: Every "slice" of data moving across the wire is physically identical. An observer cannot distinguish a 1KB text message from a 10MB file transfer because every packet "envelope" weighs exactly the same.

5. Constant Bit Rate (CBR) Shaping (100kbit-1mbit) (Cover Traffic)

ShadowNet maintains a disciplined 100kbit-1mbit pulse regardless of your actual activity.

    The Logic: If you are idle, the protocol maintains a "Hum" of cover traffic. If you are active, it throttles your data into that same 100kbit-1mbit window.

    The Benefit: Your network signature remains a flat line. An adversary cannot see "spikes" in traffic that would indicate when you are actively using the computer versus when it is sitting idle.

🛡️ Anti-Forensic & Leak Protection
6. The "WebRTC Killer" Firewall

WebRTC is the primary vector for IP leaks in modern browsers. ShadowNet implements a Strict UDP Reject policy.

    The Benefit: It blocks all non-DNS UDP traffic. Since WebRTC requires random UDP ports to discover your "real" IP, this firewall rule effectively "blinds" the browser's ability to leak your identity.

7. OS-Fingerprint Morphing (ttl=128)

ShadowNet modifies the kernel's default IP behavior to mimic a standard Windows workstation.

    The Logic: Changes the "Time To Live" (TTL) from 64 (Linux) to 128 (Windows) and disables TCP Timestamps.

    The Benefit: You become a "needle in a haystack" of billions of Windows users. To automated network sensors, your traffic looks like it's coming from a standard home PC rather than a specialized privacy OS.

8. Secure Distributed Time Sync (Chrono-Anonymization)

    Zero-Leak Proxying: Fetches time over encrypted TLS/Onion connections, avoiding the suspicious UDP Port 123 (NTP).

    Distributed Consensus: Calculates the median time from multiple high-trust sources to prevent "Time-Warp" attacks where an adversary feeds you fake time to de-sync your encryption.

9. Volatile Memory & Entropy Scrambling

    Entropy Harvesting: Restarts haveged to ensure the system has maximum randomness for encryption keys.

    Memory Purge: Upon deactivation, the script drops system caches and clears volatile metadata, leaving no "residue" of the session in RAM.

🚀 Quick Start

    Install Dependencies: sudo ./setup.sh

    Initialize ShadowNet: sudo ./shadow.sh start

    Verify Anonymity: Check your IP and run a WebRTC leak test.

    Deactivate: sudo ./shadow.sh stop (Restores system to original state).

    Note: ShadowNet is designed for high-latency, high-security environments. By prioritizing Flow-Invariance over speed, it provides protection against the world's most advanced traffic analysis systems.

    KILL SWITCH IS ENABLED! All non tor traffic is blocked by default! If the connection fails when browsing, your internet will be killed. This will be prevent ip leaks.

    MAC ADDRESS SPOOFING: Spoofs mac address randomly for each session.


THE DIAGNOSTIC TEST:

1. Jitter & SFQ Randomization Test

This verifies that the kernel is actively reordering packets to break timing-based correlation (breaking the "metronome" of your data).

    Command:
    Bash

    tc -s qdisc show dev wlo1

    ping -c 20 127.0.0.1 

    What to look for: The output must show qdisc sfq with perturb 10sec. Check the sent and backlog statistics; if they are incrementing, the "Shuffling" engine is live. Also when you try the ping command, it shoud return with a randomized delayed timing.

2. TLS Noise Floor Stability (The "Hum" Test)

This confirms the Volumetric Masking is high enough to hide your actual browsing spikes.

    Command:
    Bash

    nload (interface) 
    
    The Wifi interface for Parrot is usually 'wlo1' for Kali linux it's 'wlan0'

    The Goal: Monitor the "Outgoing" rate while idle. It should maintain a steady baseline above 100 kbit/s-1mbit. If it drops to 20-70 kbit/s, the noise floor has "stuttered" and requires a script restart.

3. Sphinx Structural Uniformity (Packet Lengths)

This ensures every outgoing packet looks identical in size, defeating "Packet Size Fingerprinting".

    Command:
    Bash

    sudo tcpdump -i (interface) -n -c 20 'host 1.1.1.1'

    The Goal: Every packet in the output must show an identical length (verified at 1158 or 1186 in your environment). Any deviation in length while idle means the "Sphinx Clamping" is compromised.

4. Chrono-Anonymization (Temporal Fingerprint)

Verifies that your hardware isn't leaking unique CPU timing or uptime data.

    Command:
    Bash

    cat /proc/sys/net/ipv4/tcp_timestamps && ping -c 3 127.0.0.1

    The Goal: tcp_timestamps must return 0. The ping must return ttl=128 to match the Windows mimicry signature.

5. MAC Identity Integrity

Ensures the Layer 2 spoofing is actually active on the hardware.

    Command:
    Bash

    cat /sys/class/net/wlo1/address && ethtool -P (interface)

    The Goal: The first address (Active) must not match the second address (Permanent). 

  6.  Volatile RAM (Anti-Forensics):

Bash

ls -l /tmp/shadownet_mac.bak && df -h /tmp

Sovereign Requirement: The backup file must exist in /tmp. Note: Use df -h /tmp (without the trailing slash) to verify it is mounted as tmpfs so it wipes instantly on power-off.

7. Killswitch Integrity:

Start the shadownet tool

# Run in Terminal 1:

while true; do curl --connect-timeout 2 -s https://check.torproject.org/api/ip | grep -oE "\b([0-9]{1,3}\.){3}[0-9]{1,3}\b" || echo "BLOCK ENGAGED"; sleep 1; done

# Run in Terminal 2:

sudo systemctl stop tor

If you see the tor ip it will continue to spam it, just turn tor off and you will see it say 'BLOCK ENGAGED'. This is when
you know the kill-switch is working when it prevents your ip leaking after a sudden fail/disconnect

2. WebRTC & Local IP Leak Test (Browser Level)

This verifies that your browser cannot be "tricked" into revealing your real local IP or hardware MAC via JavaScript.

    Command: While ShadowNet is active, run this in your terminal to see what the system "thinks" is the only valid route:
    Bash

ip route get 1.1.1.1

The Goal: It should show traffic being routed through 127.0.0.1 or your specified TRANS_PORT gateway.

Verification: Open your browser and visit a leak-test site (like browserleaks.com/webrtc). Under WebRTC Local IP, it should show "N/A", "Timed out", or a Tor-internal IP (10.x.x.x). It must never show your real local IP (192.168.xxx.xxx)
    

🛡️ Summary of Logic

Diagnostic	Targeted Vulnerability	Sovereign Requirement

TC/SFQ	Timing Analysis	perturb 10sec active

NLOAD	Activity Correlation	Baseline > 100 kbit/s

TCPDUMP	Packet Size Fingerprinting	Fixed Length (1158/1186) 1200bytes

SYSCTL	Temporal/Uptime Leakage	Timestamps = 0

IP/ETH	Physical ID Tracking	Active != Permanent
