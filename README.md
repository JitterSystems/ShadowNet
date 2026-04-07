ShadowNet: Flow-Invariant Anonymity Protocol

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

6. Anti-Leak Protection (DNS & TCP)

Transparent Redirection: Every TCP connection is hijacked at the kernel level and forced into the Tor TransPort.

DNS Shielding: All Port 53 (UDP) queries are intercepted and resolved internally via Tor’s encrypted DNSPort.

🚀 Installation

Ensure you are running a Debian-based rolling distribution (Kali/Parrot).

Clone the repository:
Bash

git clone https://github.com/gothamblvck-coder/ShadowNet.git

cd shadownet

Run the dependency setup:

Bash

sudo chmod +x setup.sh
sudo ./setup.sh

🛠️ Usage

Start ShadowNet

Initialize the uniformity protocol and establish the invariant flow:
Bash

sudo ./shadownet.sh start

Note: The script will wait 20 seconds to allow Tor to bootstrap its circuits before applying traffic shaping.
Verify Connection

Check if your traffic is successfully exiting the Tor network:
Bash

curl https://check.torproject.org

Stop ShadowNet

Flush all rules and restore the system to a standard "clearnet" state:
Bash

sudo ./shadownet.sh stop

📊 Comparison: ShadowNet vs. Others
Attack Vector	Standard Tor	ShadowNet	Nym Mixnet
Size Analysis	Vulnerable	Immune (Fixed 1200b)	Immune (Sphinx)
Timing Analysis	Vulnerable	Immune (Time-Slotting)	Immune (Shuffling)
DPI Fingerprinting	Visible	Obfuscated	Variable
Ease of Use	High	High (System-wide)	Low (Per-app)
⚠️ Disclaimer

ShadowNet is a powerful tool for privacy and security research. However, no software can protect against human error. Avoid logging into personally identifiable accounts (Google, Facebook, etc.) while the protocol is active. This tool is provided "as is" for educational purposes.

"There is nothing that the Sovereigns haven't seen."
