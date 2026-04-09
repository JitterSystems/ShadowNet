#!/bin/bash
# ShadowNet: Protocol-Level Isolation + Double-Mangle TTL + SFQ Shuffling + Kalitorify-Grade Killswitch + MAC Spoofing

INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"
MAC_BAK_FILE="/tmp/shadownet_mac.bak"

if [ -z "$INT_IF" ]; then
	echo -e "\033[0;31m[!] Error: No active network interface found.\033[0m"
	exit 1
	fi
	
	function start_shadownet() {
		echo -e "\033[0;32m[+] Initializing ShadowNet on $INT_IF...\033[0m"
		
		# 0. MAC Address Spoofing
		echo -e "\033[0;32m[+] Spoofing MAC address for $INT_IF...\033[0m"
		ip link show "$INT_IF" | grep ether | awk '{print $2}' > "$MAC_BAK_FILE"
		sudo ip link set "$INT_IF" down
		sudo macchanger -r "$INT_IF"
		sudo ip link set "$INT_IF" up
		sleep 2
		
		# 1. Kernel Tweaks (Chrono-Leak & TTL Masking)
		sudo sysctl -w net.ipv4.ip_default_ttl=128 >/dev/null
		sudo sysctl -w net.ipv4.tcp_timestamps=0 >/dev/null
		sudo sysctl -w net.ipv4.conf.all.route_localnet=1 >/dev/null
		sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1
		
		# 2. Tor Configuration
		if ! grep -q "ShadowNet Protocol Additions" /etc/tor/torrc; then
			printf "\n# --- ShadowNet Protocol Additions ---\nVirtualAddrNetworkIPv4 10.192.0.0/10\nAutomapHostsOnResolve 1\nTransPort 127.0.0.1:$TRANS_PORT\nDNSPort 127.0.0.1:$DNS_PORT\n" >> /etc/tor/torrc
			fi
			systemctl restart tor@default
			
			# Wait for Tor to bind
			sleep 2 
			
			# 3. Interface Shuffling (SFQ)
			sudo tc qdisc add dev $INT_IF root sfq perturb 10 2>/dev/null
			
			# 4. DNS Isolation
			if [ -L /etc/resolv.conf ]; then
				cp /etc/resolv.conf /tmp/resolv.conf.shadownet_bak
				rm -f /etc/resolv.conf
				elif [ ! -f /tmp/resolv.conf.shadownet_bak ]; then
				cp /etc/resolv.conf /tmp/resolv.conf.shadownet_bak
				fi
				echo "nameserver 127.0.0.1" > /etc/resolv.conf
				
				# 5. Firewall Reset & Policy Setup
				iptables -F
				iptables -t nat -F
				iptables -t mangle -F
				iptables -X
				iptables -P INPUT ACCEPT
				iptables -P FORWARD ACCEPT
				iptables -P OUTPUT ACCEPT
				
				# --- THE OVERRIDE: DOUBLE-GATE TTL MANGLE ---
				iptables -t mangle -A OUTPUT -o $INT_IF -j TTL --ttl-set 128
				iptables -t mangle -A POSTROUTING -o $INT_IF -j TTL --ttl-set 128
				
				# --- 6. NAT ROUTING ---
				iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
				iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
				iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports $DNS_PORT
				iptables -t nat -A OUTPUT -d 127.0.0.0/8 -j RETURN
				iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
				
				# --- 7. STATEFUL KILLSWITCH ---
				iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
				iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
				iptables -A OUTPUT -d 127.0.0.0/8 -j ACCEPT
				iptables -A OUTPUT -p udp -d 1.1.1.1 -j ACCEPT
				iptables -A OUTPUT -p raw -d 1.1.1.1 -j ACCEPT
				
				# DROP THE GUILLOTINE
				iptables -A OUTPUT -j REJECT --reject-with icmp-port-unreachable
				
				# 8. Start Heartbeat
				nohup python3 heartbeat.py > /dev/null 2>&1 &
				echo $! > /tmp/shadownet_heartbeat.pid
				
				echo -e "\033[0;32m[!] ShadowNet Active. Kalitorify-Grade Killswitch Engaged.\033[0m"
	}
	
	function stop_shadownet() {
		[ -f /tmp/shadownet_heartbeat.pid ] && kill -9 $(cat /tmp/shadownet_heartbeat.pid) && rm /tmp/shadownet_heartbeat.pid
		
		sudo sysctl -w net.ipv4.ip_default_ttl=64 >/dev/null
		sudo sysctl -w net.ipv4.tcp_timestamps=1 >/dev/null
		sudo tc qdisc del dev $INT_IF root 2>/dev/null
		
		# Restore DNS
		if [ -f /tmp/resolv.conf.shadownet_bak ]; then
			rm -f /etc/resolv.conf
			mv /tmp/resolv.conf.shadownet_bak /etc/resolv.conf
			fi
			
			# Restore Original MAC
			if [ -f "$MAC_BAK_FILE" ]; then
				echo -e "\033[1;33m[*] Restoring original MAC address...\033[0m"
				ORIG_MAC=$(cat "$MAC_BAK_FILE")
				sudo ip link set "$INT_IF" down
				sudo macchanger -m "$ORIG_MAC" "$INT_IF"
				sudo ip link set "$INT_IF" up
				rm "$MAC_BAK_FILE"
				fi
				
				iptables -F
				iptables -t nat -F
				iptables -t mangle -F
				systemctl restart NetworkManager
				echo -e "\033[1;31m[-] ShadowNet Deactivated. Defaults Restored.\033[0m"
	}
	
	case "$1" in
	start) start_shadownet ;;
	stop) stop_shadownet ;;
	esac
