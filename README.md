ShadowNet: Flow-Invariant Anonymity Protocol (TOR + Mixnet technique)

ShadowNet is an advanced network hardening framework for Kali Linux and Parrot OS. It transforms a standard workstation into a "Private Mixnet of One" by forcing all system traffic through a synchronous, timing-obfuscated, and size-uniform Tor tunnel.

Unlike standard transparent proxies, ShadowNet eliminates Behavioral Metadata—the timing and size patterns that state-level adversaries (like the NSA) use to deanonymize encrypted traffic.
🛡️ Key Features

1. Synchronous Time-Slotting (CBR)

ShadowNet implements Constant Bit Rate (CBR) flow. By using the Linux kernel's Traffic Control (tc) subsystem, it releases data in a perfectly rhythmic "heartbeat" (1mbit pulse). This ensures your network signature remains a flat line, regardless of whether you are idle or active.

2. Sphinx-Style MTU Clamping

To defeat Packet Size Analysis, ShadowNet uses iptables mangle rules to clamp the Maximum Segment Size (MSS) to exactly 1200 bytes. Every "slice" of data moving across the wire is physically identical, making it impossible to distinguish a text message from a file transfer.

3. Binary Pattern Obfuscation

Defeats Deep Packet Inspection (DPI) by injecting non-functional, randomized padding into Tor protocol headers. This prevents automated systems from identifying the unique binary "fingerprint" of the Tor onion routing protocol.

4. Background "Hum" Cover Traffic

ShadowNet maintains a constant 1mbit stream of jittered data. This creates a "noise floor" that masks the start and end of your actual communications. To an observer, your connection is "always busy," hiding your true intent.

5. Deterministic Latency Masking

Adds a fixed 100ms processing delay to normalize your hardware's response time. This prevents "Hardware Fingerprinting," where an adversary guesses your CPU speed based on how fast your machine responds to network requests.

6. 🛡️ Advanced Anonymity: Secure Distributed Time Sync (sdwdate)

Anti-Fingerprinting: It eliminates "Clock Skew" (the unique ms offset of your CPU) that can be used to track your identity across different networks.

Zero-Leak Proxying: Unlike standard NTP which uses UDP port 123, this system fetches time over encrypted TLS/Onion connections within the Tor network.

Distributed Consensus: It doesn't trust a single server; it calculates the median time from multiple high-trust sources to prevent an adversary from feeding you "fake time" to de-sync your connection.

Temporal Masking: It resets the system clock during the initial "Heartbeat" phase, ensuring your machine looks like a generic, perfectly-synced node before a single packet is sent.

7. Anti-Leak Protection (DNS & TCP)

Transparent Redirection: Every TCP connection is hijacked at the kernel level and forced into the Tor TransPort.

DNS Shielding: All Port 53 (UDP) queries are intercepted and resolved internally via Tor’s encrypted DNSPort.


8. OS-Fingerprint Morphing (ip_default_ttl=128)

What it is: Changes your "Time To Live" value from the Linux default (64) to the Windows default (128).

The Benefit: To any automated sensor or ISP, your encrypted packets now look like they are coming from a standard Windows 10/11 home PC. It makes you a "needle in a haystack" of billions of users instead of a "Linux privacy user."

9. Hardware Clock-Skew Defense (tcp_timestamps=0)

What it is: Disables the TCP timestamp field in your packet headers.

The Benefit: Every CPU has a tiny, unique physical vibration (clock-skew). Advanced adversaries like the NSA use these timestamps to "fingerprint" your specific motherboard, allowing them to track your laptop even if you change your IP. Setting this to 0 deletes that physical serial number.

10. TCP Stack Hardening (tcp_rfc1337=1)

What it is: Implements a technical safeguard against "TIME-WAIT Assassination."

The Benefit: It prevents an adversary from using duplicate or "stale" packets to forcibly reset your connection. It makes your TCP "handshake" much more resilient against remote manipulation.

11. Hardware ID Randomization (macchanger)

What it is: A logic block that detects if you have the macchanger tool installed.

The Benefit: If installed, it automatically gives your network card a new, random MAC address every time you start the script. This ensures that even your local router cannot track your physical hardware history.


BONUS:

Disables ipv6 

Kernel Log Silencing (Anti-Forensics)

Hostname Masking

Chrono-Anonymization (Secure Time Sync)

Entropy Scrambling

The "Bridge" Exception (This allows the Tor process (and only the Tor process) to reach the internet so it can actually build the tunnel for your traffic.)

Volatile Memory Purge (When you shut down ShadowNet, this wipes the RAM caches of your session metadata)
