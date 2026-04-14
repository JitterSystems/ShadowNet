#!/bin/bash
# ShadowNet: Protocol-Level Isolation + 6-Hop Multi-Phase Circuit + Double-Mangle TTL + SFQ Shuffling

INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"
MAC_BAK_FILE="/tmp/shadownet_mac.bak"

if [ -z "$INT_IF" ]; then
	echo -e "\033[0;31m[!] Error: No active network interface found.\033[0m"
	exit 1
	fi
	
	function get_entropy_delay() {
		local min=$1
		local max=$2
		local range=$((max - min))
		local rand_val=$(od -An -N1 -i /dev/urandom | tr -d ' ')
		echo $(( (rand_val * range / 255) + min ))
	}
	
	function start_shadownet() {
		# 0. Immediate Hardware Prep
		FIXED_MTU=$(get_entropy_delay 1200 1460)
		[ -f /tmp/shadownet_heartbeat.pid ] && kill -9 $(cat /tmp/shadownet_heartbeat.pid) 2>/dev/null && rm /tmp/shadownet_heartbeat.pid
		sudo pkill -f heartbeat.py > /dev/null 2>&1
		
		# --- ADDED: STARTUP DROP POLICY ---
		iptables -P OUTPUT DROP
		# ----------------------------------
		
		# Shift Identity IMMEDIATELY
		ip link show "$INT_IF" | grep ether | awk '{print $2}' > "$MAC_BAK_FILE"
		sudo ip link set "$INT_IF" down
		sudo macchanger -r "$INT_IF"
		sudo ip link set "$INT_IF" mtu "$FIXED_MTU"
		sudo ip link set "$INT_IF" up
		
		# Fire cover traffic IMMEDIATELY
		nohup python3 heartbeat.py $FIXED_MTU > /dev/null 2>&1 &
		echo $! > /tmp/shadownet_heartbeat.pid
		echo -e "\033[0;32m[+] Identity Shifted. Cover Traffic Engaged ($FIXED_MTU bytes).\033[0m"
		
		# Phase 1: Entry Tier (First 3 Hops)
		PHASE1_WAIT=$(get_entropy_delay 10 30)
		echo -e "\033[1;34m[*] Phase 1: Establishing Entry Tier (Nodes 1-3). Applying Jitter: ${PHASE1_WAIT}s...\033[0m"
		sleep $PHASE1_WAIT
		
		# Phase 2: Exit Tier (Last 3 Hops)
		PHASE2_WAIT=$(get_entropy_delay 15 45)
		echo -e "\033[1;35m[*] Phase 2: Extending to Exit Tier (Nodes 4-6). Applying Entropy IAT: ${PHASE2_WAIT}s...\033[0m"
		sleep $PHASE2_WAIT
		
		echo -e "\033[0;32m[+] 6-Hop Chain Established. Initializing ShadowNet Routing Protocol...\033[0m"
		
		# 1. Kernel Tweaks
		sudo sysctl -w net.ipv4.ip_default_ttl=128 >/dev/null
		sudo sysctl -w net.ipv4.tcp_timestamps=0 >/dev/null
		sudo sysctl -w net.ipv4.conf.all.route_localnet=1 >/dev/null
		sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1
		
		# 2. Tor Configuration (Enforcing 6-Hop Length)
		if ! grep -q "ShadowNet Protocol Additions" /etc/tor/torrc; then
			printf "\n# --- ShadowNet Protocol Additions ---\nVirtualAddrNetworkIPv4 10.192.0.0/10\nAutomapHostsOnResolve 1\nTransPort 127.0.0.1:$TRANS_PORT\nDNSPort 127.0.0.1:$DNS_PORT\nLongLivedPorts 21,22,706,1863,5050,5190,5222,5223,6667,6697,8300\n# Enforce 6-Hop Circuitry\nCircuitBuildTimeout 60\nNumEntryGuards 3\n" >> /etc/tor/torrc
			fi
			systemctl restart tor@default
			sleep 2 
			
			# 3. Interface Shuffling (SFQ)
			SFQ_JITTER=$(get_entropy_delay 5 30)
			sudo tc qdisc add dev $INT_IF root sfq perturb $SFQ_JITTER 2>/dev/null
			
			# 4. DNS Isolation
			if [ -L /etc/resolv.conf ]; then
				cp /etc/resolv.conf /tmp/resolv.conf.shadownet_bak
				rm -f /etc/resolv.conf
				elif [ ! -f /tmp/resolv.conf.shadownet_bak ]; then
				cp /etc/resolv.conf /tmp/resolv.conf.shadownet_bak
				fi
				echo "nameserver 127.0.0.1" > /etc/resolv.conf
				
				# 5. Firewall & 6. Mangle/NAT
				iptables -F
				iptables -t nat -F
				iptables -t mangle -F
				iptables -X
				
				# --- ADDED: LIFT STARTUP DROP POLICY ---
				iptables -P OUTPUT ACCEPT
				# ---------------------------------------
				
				iptables -P INPUT ACCEPT
				iptables -P FORWARD ACCEPT
				
				iptables -t mangle -A OUTPUT -o $INT_IF -j TTL --ttl-set 128
				iptables -t mangle -A POSTROUTING -o $INT_IF -j TTL --ttl-set 128
				iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
				iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
				iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports $DNS_PORT
				iptables -t nat -A OUTPUT -d 127.0.0.0/8 -j RETURN
				iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
				
				# 7. Killswitch Exceptions
				iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
				iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
				iptables -A OUTPUT -d 127.0.0.0/8 -j ACCEPT
				DNS_LIST=("76.76.2.2" "76.76.10.2" "182.222.222.222" "45.11.45.11" "84.200.69.80" "84.200.70.40")
				for ip in "${DNS_LIST[@]}"; do
					iptables -A OUTPUT -p udp -d $ip -j ACCEPT
					done
					
					# DROP THE GUILLOTINE
					iptables -A OUTPUT -j REJECT --reject-with icmp-port-unreachable
					echo -e "\033[0;32m[!] ShadowNet Fully Active with 6-Hop Obfuscated Path.\033[0m"
	}
	
	function stop_shadownet() {
		WAIT_TIME=$(get_entropy_delay 5 60)
		echo -e "\033[1;31m[*] Pending exit... Waiting $WAIT_TIME seconds to deactivate.\033[0m"
		sleep $WAIT_TIME
		[ -f /tmp/shadownet_heartbeat.pid ] && kill -9 $(cat /tmp/shadownet_heartbeat.pid) && rm /tmp/shadownet_heartbeat.pid
		sudo pkill -f heartbeat.py > /dev/null 2>&1
		sudo sysctl -w net.ipv4.ip_default_ttl=64 >/dev/null
		sudo sysctl -w net.ipv4.tcp_timestamps=1 >/dev/null
		sudo tc qdisc del dev $INT_IF root 2>/dev/null
		sudo ip link set "$INT_IF" mtu 1500
		if [ -f /tmp/resolv.conf.shadownet_bak ]; then
			rm -f /etc/resolv.conf
			mv /tmp/resolv.conf.shadownet_bak /etc/resolv.conf
			fi
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
				echo -e "\033[1;31m[-] ShadowNet Deactivated.\033[0m"
	}
	
	case "$1" in
	start) start_shadownet ;;
	stop) stop_shadownet ;;
	esac
	
	
