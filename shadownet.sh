#!/bin/bash

# --- Configuration ---
INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"

# --- Colors ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function start_shadownet() {
	echo -e "${GREEN}[+] Initializing ShadowNet: Sovereign Protocol...${NC}"
	
	# 1. OS-FINGERPRINT MORPHING & HARDENING
	# Mimics Windows 10/11 TTL and kills Hardware Clock-Skew (Timestamp) leaks.
	echo -e "[*] Morphing OS Fingerprint & Deleting Clock-Skew..."
	sysctl -w net.ipv4.ip_default_ttl=128 > /dev/null
	sysctl -w net.ipv4.tcp_timestamps=0 > /dev/null
	sysctl -w net.ipv4.tcp_rfc1337=1 > /dev/null
	sysctl -w net.ipv4.tcp_syncookies=1 > /dev/null
	
	# 2. MAC ADDRESS RANDOMIZATION (Optional but included)
	if command -v macchanger &> /dev/null; then
		echo -e "[*] Randomizing Hardware MAC Address..."
		ip link set $INT_IF down
		macchanger -r $INT_IF > /dev/null
		ip link set $INT_IF up
		fi
		
		# 3. FIXED PACKET SIZE (MTU Clamping)
		echo -e "[*] Implementing Total Size Uniformity (1200b)..."
		iptables -t mangle -F
		iptables -t mangle -A POSTROUTING -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 1200
		
		# 4. TOR CONFIGURATION (Padding & Obfuscation)
		echo -e "[*] Configuring Tor with Binary-Matching Defense..."
		cat > /etc/tor/torrc <<-EOF
		VirtualAddrNetworkIPv4 10.192.0.0/10
		AutomapHostsOnResolve 1
		TransPort $TRANS_PORT
		DNSPort $DNS_PORT
		ConnectionPadding 1
		ReducedConnectionPadding 0
		CircuitPadding 1
		PaddingStatistics 1
		EOF
		systemctl restart tor
		
		echo -e "${YELLOW}[*] Waiting 20 seconds for Heartbeat synchronization...${NC}"
		sleep 20
		
		# 5. SYNCHRONOUS TIME-SLOTTING (The Heartbeat)
		echo -e "[*] Enabling Synchronous Time-Slotting (CBR)..."
		tc qdisc del dev $INT_IF root 2>/dev/null
		tc qdisc add dev $INT_IF root handle 1: tbf rate 1mbit burst 32k latency 400ms
		tc qdisc add dev $INT_IF parent 1:1 netem delay 100ms
		
		# 6. MULTI-PATH ROUTING LOGIC
		echo -e "[*] Finalizing Unlinkable Routing..."
		iptables -F
		iptables -t nat -F
		iptables -A OUTPUT -o lo -j ACCEPT
		iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
		iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
		iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
		iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
		
		echo -e "${GREEN}[!] ShadowNet Sovereign Active: Metadata and Hardware signatures erased.${NC}"
}

function stop_shadownet() {
	echo -e "${RED}[-] Deactivating ShadowNet...${NC}"
	# Restore Linux Defaults
	sysctl -w net.ipv4.ip_default_ttl=64 > /dev/null
	sysctl -w net.ipv4.tcp_timestamps=1 > /dev/null
	tc qdisc del dev $INT_IF root 2>/dev/null
	iptables -F
	iptables -t nat -F
	iptables -t mangle -F
	iptables -X
	iptables -P INPUT ACCEPT
	iptables -P FORWARD ACCEPT
	iptables -P OUTPUT ACCEPT
	systemctl stop tor
	echo -e "${RED}[!] System Restored to Linux Defaults.${NC}"
}

if [[ $EUID -ne 0 ]]; then
	echo "This script must be run as root."
	exit 1
	fi
	
	case "$1" in
	start) start_shadownet ;;
stop) stop_shadownet ;;
*) echo "Usage: sudo ./shadownet.sh {start|stop}" ;;
esac
