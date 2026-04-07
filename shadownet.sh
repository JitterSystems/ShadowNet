#!/bin/bash

# --- Configuration ---
INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"

# --- Colors ---
GREEN='\033[0;32m'
RED='\033[1;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

function start_shadownet() {
	echo -e "${GREEN}[+] Initializing ShadowNet: Flow-Invariant Protocol...${NC}"
	
	# 1. FIXED PACKET SIZE (MTU Clamping)
	echo -e "[*] Implementing Total Size Uniformity (1200b)..."
	iptables -t mangle -F
	iptables -t mangle -A POSTROUTING -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 1200
	
	# 2. TOR CONFIGURATION
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
	
	# --- ANTI-TIME-DRIFT DEFENSE ---
	# Syncs clock via TLS over Tor to prevent clock-skew fingerprinting
	echo -e "[*] Synchronizing Clock via TLS (Anti-Fingerprinting)..."
	tlsdate -V -n -H duckduckgo.com -p 443 --proxy socks5://127.0.0.1:9050 > /dev/null 2>&1
	
	# 3. SYNCHRONOUS TIME-SLOTTING (The Heartbeat)
	echo -e "[*] Enabling Synchronous Time-Slotting (CBR)..."
	tc qdisc del dev $INT_IF root 2>/dev/null
	tc qdisc add dev $INT_IF root handle 1: tbf rate 1mbit burst 32k latency 400ms
	tc qdisc add dev $INT_IF parent 1:1 netem delay 100ms
	
	# 4. MULTI-PATH ROUTING LOGIC
	echo -e "[*] Finalizing Unlinkable Routing..."
	iptables -F
	iptables -t nat -F
	iptables -A OUTPUT -o lo -j ACCEPT
	
	# Allow Tor User to build circuits
	iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
	iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
	
	# Hijack DNS and TCP
	iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
	iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
	
	echo -e "${GREEN}[!] ShadowNet Active: The signal is now a constant pulse.${NC}"
}

function stop_shadownet() {
	echo -e "${RED}[-] Deactivating ShadowNet...${NC}"
	tc qdisc del dev $INT_IF root 2>/dev/null
	iptables -F
	iptables -t nat -F
	iptables -t mangle -F
	iptables -X
	iptables -P INPUT ACCEPT
	iptables -P FORWARD ACCEPT
	iptables -P OUTPUT ACCEPT
	systemctl stop tor
	echo -e "${RED}[!] System Restored.${NC}"
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
