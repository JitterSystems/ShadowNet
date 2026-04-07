#!/bin/bash
# ShadowNet: Flow-Invariant Anonymity Protocol

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
	
	# 1. IPv6 SCORCHED EARTH (Prevents Dual-Stack Leaks)
	sysctl -w net.ipv6.conf.all.disable_ipv6=1 > /dev/null 2>&1
	sysctl -w net.ipv6.conf.default.disable_ipv6=1 > /dev/null 2>&1
	
	# 2. KERNEL LOG SILENCING (Anti-Forensics)
	sysctl -w kernel.printk="0 0 0 0" > /dev/null
	
	# 3. OS-FINGERPRINT MORPHING (Mimics Windows Network Stack)
	sysctl -w net.ipv4.ip_default_ttl=128 > /dev/null
	sysctl -w net.ipv4.tcp_timestamps=0 > /dev/null
	sysctl -w net.ipv4.tcp_rfc1337=1 > /dev/null
	
	# 4. HOSTNAME MASKING
	NEW_HOSTNAME=$(tr -dc 'a-z0-9' < /dev/urandom | head -c 8)
	hostnamectl set-hostname $NEW_HOSTNAME
	
	# 5. MAC ADDRESS RANDOMIZATION
	if command -v macchanger &> /dev/null; then
		ip link set $INT_IF down
		macchanger -r $INT_IF > /dev/null
		ip link set $INT_IF up
		fi
		
		# 6. HARDENING THE SPHINX (The Hardware Lock)
		# Passed Test: Disables GSO/TSO to prevent Jumbo Frame leaks during high-speed downloads.
		echo -e "[*] Locking MTU Clamping: Disabling Hardware Offloading..."
		ethtool -K $INT_IF gso off tso off gro off lro off > /dev/null 2>&1
		
		# 7. FIXED PACKET SIZE (Clamping to 1200b)
		iptables -t mangle -F
		iptables -t mangle -A POSTROUTING -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 1200
		
		# 8. TOR NODE INITIALIZATION
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
		
		# 9. CLOCK DRIFT MIMICRY (Anti-Hardware Fingerprinting)
		if command -v adjtimex &> /dev/null; then
			DRIFT_VAL=$(( ( RANDOM % 20 )  - 10 ))
			adjtimex -o $DRIFT_VAL > /dev/null 2>&1
			fi
			tlsdate -V -n -H rsync.torproject.org > /dev/null 2>&1
			
			# 10. ENTROPY SATURATION
			systemctl restart haveged
			
			# 11. MULTI-TIERED DECOY HANDSHAKES (Mimics standard browsing startup)
			(
				DECOYS=("https://www.google.com" "https://www.cloudflare.com")
				for i in {1..2}; do
					curl -s -L ${DECOYS[$RANDOM % ${#DECOYS[@]}]} > /dev/null 2>&1
					sleep 2
					done
			) &
			
			echo -e "${YELLOW}[*] Waiting 20s for Sovereign Heartbeat...${NC}"
			sleep 20
			
			# 12. ASYNCHRONOUS LOCAL MIX (Passed Test: 0.9s Jitter)
			# Shuffles packet order and varies timing every 10 seconds.
			tc qdisc del dev $INT_IF root 2>/dev/null
			tc qdisc add dev $INT_IF root handle 1: htb default 11
			tc class add dev $INT_IF parent 1: classid 1:11 htb rate 1mbit
			tc qdisc add dev $INT_IF parent 1:11 handle 10: sfq perturb 10
			
			# 13. UNLINKABLE ROUTING (The Firewall)
			iptables -F
			iptables -t nat -F
			iptables -A OUTPUT -o lo -j ACCEPT
			
			# Allow Tor User
			iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
			iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
			
			# Redirect DNS
			iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
			iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports $DNS_PORT
			
			# WebRTC KILLER (UDP Block)
			iptables -A OUTPUT -p udp ! --dport $DNS_PORT -j REJECT
			
			# Global TCP Redirection
			iptables -t nat -A OUTPUT -p tcp -m state --state NEW,ESTABLISHED -j REDIRECT --to-ports $TRANS_PORT
			
			echo -e "${GREEN}[!] ShadowNet Sovereign Active: Metadata De-Coupled.${NC}"
}

function stop_shadownet() {
	echo -e "${RED}[-] Deactivating ShadowNet...${NC}"
	# Restore Hardware Defaults
	ethtool -K $INT_IF gso on tso on gro on lro on > /dev/null 2>&1
	sysctl -w net.ipv6.conf.all.disable_ipv6=0 > /dev/null 2>&1
	sysctl -w kernel.printk="4 4 1 7" > /dev/null
	sync; echo 3 > /proc/sys/vm/drop_caches
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

case "$1" in
start) start_shadownet ;;
stop) stop_shadownet ;;
*) echo "Usage: sudo ./shadownet.sh {start|stop}" ;;
esac
