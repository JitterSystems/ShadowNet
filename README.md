🛡️ ShadowNet: Flow-Invariant Anonymity Protocol (Tor + Mixnet Techniques)

No longer rely on being unique to be anonymous like regular tor, NOW BEING UNIQUE IS THE TRUE ANONYMITY!

Version 2.0: Asynchronous Obfuscation Layer

ShadowNet is an advanced network hardening framework that transforms a standard workstation into a "Private Mixnet of One." By forcing all system traffic through a synchronous, timing-obfuscated, and size-uniform tunnel, it eliminates the behavioral metadata that state-level adversaries use to deanonymize users.
🛡️ Core Evolutionary Features
1. Asynchronous Message Queuing (SFQ) Randomized delay/reordering and shuffling

ShadowNet replaces standard linear packet release with Stochastic Fairness Queuing.

    The Logic: Instead of a predictable "tick-tock" delivery, packets are hashed into multiple internal "buckets" and released using a shuffling algorithm.

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

5. Constant Bit Rate (CBR) Shaping (1mbit) (Cover Traffic)

ShadowNet maintains a disciplined 1mbit pulse regardless of your actual activity.

    The Logic: If you are idle, the protocol maintains a "Hum" of cover traffic. If you are active, it throttles your data into that same 1mbit window.

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
