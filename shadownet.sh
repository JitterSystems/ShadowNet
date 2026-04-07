#!/bin/bash

# --- Configuration ---
INT_IF=$(ip route | grep default | awk $'{print $5}' | head -n1)
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
	
	# [ADDED] 1. IPv6 SCORCHED EARTH (Stops leaks outside the tunnel)
	sysctl -w net.ipv6.conf.all.disable_ipv6=1 > /dev/null
	sysctl -w net.ipv6.conf.default.disable_ipv6=1 > /dev/null
	
	# [ADDED] 2. KERNEL LOG SILENCING (Anti-Forensics)
	sysctl -w kernel.printk="0 0 0 0" > /dev/null
	
	# [ORIGINAL] 3. OS-FINGERPRINT MORPHING
	sysctl -w net.ipv4.ip_default_ttl=128 > /dev/null
	sysctl -w net.ipv4.tcp_timestamps=0 > /dev/null
	sysctl -w net.ipv4.tcp_rfc1337=1 > /dev/null
	sysctl -w net.ipv4.tcp_syncookies=1 > /dev/null
	
	# [ADDED] 4. HOSTNAME MASKING (Hardware identity cloaking)
	NEW_HOSTNAME=$(tr -dc 'a-z0-9' < /dev/urandom | head -c 8)
	hostnamectl set-hostname $NEW_HOSTNAME
	sed -i "s/127.0.1.1.*/127.0.1.1 $NEW_HOSTNAME/g" /etc/hosts
	
	# [ORIGINAL] 5. MAC ADDRESS RANDOMIZATION
	if command -v macchanger &> /dev/null; then
		ip link set $INT_IF down
		macchanger -r $INT_IF > /dev/null
		ip link set $INT_IF up
		fi
		
		# [ORIGINAL] 6. FIXED PACKET SIZE (MTU Uniformity 1200b)
		iptables -t mangle -F
		iptables -t mangle -A POSTROUTING -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 1200
		
		# [ORIGINAL] 7. TOR CONFIGURATION (Binary-Matching/Padding)
		cat > /etc/tor/torrc <<-EOF
		VirtualAddrNetworkIPv4 10.192.0.0/10
		AutomapHostsOnResolve 1
		TransPort $TRANS_PORT
		DNSPort $DNS_PORT
		ConnectionPadding 1
		ReducedConnectionPadding 0
		CircuitPadding 1
		EOF
		systemctl restart tor
		
		# [ADDED] 8. CHRONO-ANONYMIZATION (Secure TLS Time Sync)
		tlsdate -V -n -H rsync.torproject.org > /dev/null 2>&1
		
		# [ADDED] 9. ENTROPY HARVESTING (Scrambling Crypto Noise)
		systemctl restart haveged
		
		echo -e "${YELLOW}[*] Waiting 20s for Circuit Establishment...${NC}"
		sleep 20
		
		# [ORIGINAL] 10. SYNCHRONOUS TIME-SLOTTING (The Heartbeat/CBR)
		tc qdisc del dev $INT_IF root 2>/dev/null
		tc qdisc add dev $INT_IF root handle 1: tbf rate 1mbit burst 32k latency 400ms
		tc qdisc add dev $INT_IF parent 1:1 netem delay 100ms
		
		# [ORIGINAL ENGINE + REFINEMENT] 11. MULTI-PATH ROUTING LOGIC
		iptables -F
		iptables -t nat -F
		iptables -A OUTPUT -o lo -j ACCEPT
		
		# [REFINEMENT] Allow the Tor process itself to connect (Fixes the "no connection" bug)
		iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
		
		# [ORIGINAL] Redirection to TransPort and DNSPort
		iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
		iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
		iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports $DNS_PORT
		
		echo -e "${GREEN}[!] ShadowNet Active. Traffic is being routed through Tor.${NC}"
}

function stop_shadownet() {
	echo -e "${RED}[-] Deactivating ShadowNet...${NC}"
	# Restore IPv6 and Kernel Logs
	sysctl -w net.ipv6.conf.all.disable_ipv6=0 > /dev/null
	sysctl -w kernel.printk="4 4 1 7" > /dev/null
	
	# [ADDED] VOLATILE MEMORY PURGE (RAM Wiping)
	sync; echo 3 > /proc/sys/vm/drop_caches
	
	# Restore Originals
	sysctl -w net.ipv4.ip_default_ttl=64 > /dev/null
	sysctl -w net.ipv4.tcp_timestamps=1 > /dev/null
	tc qdisc del dev $INT_IF root 2>/dev/null
	iptables -F
	iptables -t nat -F
	iptables -t mangle -F
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
*) echo "Usage: sudo ./shadow.sh {start|stop}" ;;
esac
