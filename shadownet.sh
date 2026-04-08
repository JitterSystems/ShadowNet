#!/bin/bash
# ShadowNet: Flow-Invariant Anonymity Protocol (1Mbit Force-Batch)

INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"

function start_shadownet() {
	echo -e "\033[0;32m[+] Initializing ShadowNet: Force-Batching 1Mbit...\033[0m"
	
	# OS Hardening
	sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1
	
	# HARDWARE LOCK: Set MTU to 1200 to match Scapy pulses
	ip link set dev $INT_IF mtu 1200
	ethtool -K $INT_IF gso off tso off gro off lro off >/dev/null 2>&1
	
	# Launch High-Velocity Noise
	nohup python3 heartbeat.py > /dev/null 2>&1 &
	echo $! > /tmp/shadownet_heartbeat.pid
	
	echo -e "\033[1;33m[*] Satiating the line (20s delay)...\033[0m"
	sleep 20
	
	# Firewall (Open Egress + WebRTC Block)
	iptables -F
	iptables -t nat -F
	iptables -A OUTPUT -o lo -j ACCEPT
	iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
	iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
	iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
	iptables -A OUTPUT -p udp ! --dport $DNS_PORT -j REJECT
	iptables -t nat -A OUTPUT -p tcp ! -d 127.0.0.1 -j REDIRECT --to-ports $TRANS_PORT
	
	echo -e "\033[0;32m[!] ShadowNet Active. Check nload for 1000 kBit/s.\033[0m"
}

function stop_shadownet() {
	[ -f /tmp/shadownet_heartbeat.pid ] && kill -9 $(cat /tmp/shadownet_heartbeat.pid) && rm /tmp/shadownet_heartbeat.pid
	ip link set dev $INT_IF mtu 1500 # Restore standard MTU
	iptables -F
	iptables -t nat -F
	systemctl restart NetworkManager
}

case "$1" in
start) start_shadownet ;;
stop) stop_shadownet ;;
esac
