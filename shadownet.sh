#!/bin/bash
# ShadowNet: Protocol-Level Isolation + 6-Hop Multi-Phase Circuit + Double-Mangle TTL + SFQ Shuffling

INT_IF=$(ip route | grep default | awk '{print $5}' | head -n1)
TOR_UID=$(id -u debian-tor)
TRANS_PORT="9040"
DNS_PORT="5353"
MAC_BAK_FILE="/dev/shm/shadownet_mac.bak"

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
		# --- STRICT FILE PRESENCE GATE ---
		if [ ! -f "./heartbeat.py" ] || [ ! -f "./shadownet_engine.c" ]; then
			echo -e "\033[0;31m[!] CRITICAL: heartbeat.py or shadownet_engine.c missing from $(pwd). Aborting.\033[0m"
			exit 1
			fi
			# ---------------------------------
			
			FIXED_MTU=$(get_entropy_delay 1200 1460)
			
			# --- 14-METHOD PERMANENT RESTART & CONSISTENCY GUARD (NO PKILL) ---
			echo -e "\033[1;30m[*] Executing 14-Tier Process Sanitation & Guarding...\033[0m"
			for name in "heartbeat.py" "shadownet_engine"; do
				[ -f /dev/shm/shadownet_${name%.*}.pid ] && PID=$(cat /dev/shm/shadownet_${name%.*}.pid) && [ -d /proc/$PID ] && sudo kill -9 $PID 2>/dev/null
				MATCHES=$(ps -ef | grep "$name" | grep -v grep | awk '{print $2}')
				for m_pid in $MATCHES; do sudo kill -9 $m_pid 2>/dev/null; done
					sudo fuser -k -9 "$name" 2>/dev/null
					for pdir in /proc/[0-9]*; do
						if [ -f "$pdir/comm" ] && grep -q "${name:0:15}" "$pdir/comm"; then
							sudo kill -9 $(basename "$pdir") 2>/dev/null
							fi
							done
							LSOF_PIDS=$(sudo lsof -t "$name" 2>/dev/null)
							for l_pid in $LSOF_PIDS; do sudo kill -9 $l_pid 2>/dev/null; done
								SESS_PIDS=$(ps -eo pid,sess,cmd | grep "$name" | grep -v grep | awk '{print $1}')
								for s_pid in $SESS_PIDS; do sudo kill -9 $s_pid 2>/dev/null; done
									if [[ "$name" == "shadownet_engine" ]]; then
										PORT_PIDS=$(sudo ss -lptn 'sport = :76' | grep -oP 'pid=\K[0-9]+')
										for p_pid in $PORT_PIDS; do sudo kill -9 $p_pid 2>/dev/null; done
											fi
											ENV_PIDS=$(grep -l "SHADOWNET_PROC=true" /proc/[0-9]*/environ 2>/dev/null | cut -d/ -f3)
											for e_pid in $ENV_PIDS; do sudo kill -9 $e_pid 2>/dev/null; done
												ORPHAN_PIDS=$(ps -ef | awk '$3 == 1' | grep "$name" | grep -v grep | awk '{print $2}')
												for o_pid in $ORPHAN_PIDS; do sudo kill -9 $o_pid 2>/dev/null; done
													MAP_PIDS=$(sudo grep -l "$name" /proc/[0-9]*/maps 2>/dev/null | cut -d/ -f3)
													for mp_pid in $MAP_PIDS; do sudo kill -9 $mp_pid 2>/dev/null; done
														NICE_PIDS=$(ps -eo pid,ni,cmd | awk '$2 == -20' | grep "$name" | grep -v grep | awk '{print $1}')
														for n_pid in $NICE_PIDS; do sudo kill -9 $n_pid 2>/dev/null; done
															CMD_PIDS=$(grep -a -l "$name" /proc/[0-9]*/cmdline 2>/dev/null | cut -d/ -f3)
															for c_pid in $CMD_PIDS; do sudo kill -9 $c_pid 2>/dev/null; done
																FD_PIDS=$(sudo find /proc/[0-9]*/fd -type l -lname "*$name*" 2>/dev/null | cut -d/ -f3 | sort -u)
																for fd_pid in $FD_PIDS; do sudo kill -9 $fd_pid 2>/dev/null; done
																	STAT_PIDS=$(awk -v name="${name:0:15}" '$2 == "("name")" {print $1}' /proc/[0-9]*/stat 2>/dev/null)
																	for st_pid in $STAT_PIDS; do sudo kill -9 $st_pid 2>/dev/null; done
																		done
																		
																		# --- STRICT PROCESS DEATH GATE ---
																		for name in "heartbeat.py" "shadownet_engine"; do
																			if ps -ef | grep "$name" | grep -v grep > /dev/null 2>&1; then
																				echo -e "\033[0;31m[!] CRITICAL: Failed to forcefully terminate old $name. Aborting to prevent signal overlap.\033[0m"
																				exit 1
																				fi
																				done
																				# ---------------------------------
																				
																				rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid
																				rm -f /dev/shm/heartbeat.py /dev/shm/shadownet_engine
																				# -------------------------------------------------------------------
																				
																				iptables -P OUTPUT DROP
																				ip link show "$INT_IF" | grep ether | awk '{print $2}' > "$MAC_BAK_FILE"
																				sudo ip link set "$INT_IF" down
																				
																				MAC_SHIFT_JITTER=$(get_entropy_delay 3 15)
																				echo -e "\033[1;33m[*] Applying Identity Entropy: ${MAC_SHIFT_JITTER}s before shift...\033[0m"
																				sleep $MAC_SHIFT_JITTER
																				
																				sudo macchanger -r "$INT_IF"
																				sudo ip link set "$INT_IF" mtu "$FIXED_MTU"
																				sudo ip link set "$INT_IF" up
																				
																				echo -e "\033[1;36m[*] Hardening Interface & System Persistence (Anti-Sleep)...\033[0m"
																				sudo iw dev "$INT_IF" set power_save off 2>/dev/null
																				sudo ethtool -K "$INT_IF" gso off gro off tso off 2>/dev/null
																				sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1
																				
																				# --- PERMANENT IPV6 KERNEL & SYSCTL BLINDING (KALI/PARROT) ---
																				echo -e "\033[1;36m[*] Permanently disabling IPv6 at Kernel and Sysctl layers...\033[0m"
																				if ! grep -q "net.ipv6.conf.all.disable_ipv6 = 1" /etc/sysctl.conf; then
																					echo "net.ipv6.conf.all.disable_ipv6 = 1" | sudo tee -a /etc/sysctl.conf >/dev/null
																					echo "net.ipv6.conf.default.disable_ipv6 = 1" | sudo tee -a /etc/sysctl.conf >/dev/null
																					echo "net.ipv6.conf.lo.disable_ipv6 = 1" | sudo tee -a /etc/sysctl.conf >/dev/null
																					sudo sysctl -p >/dev/null 2>&1
																					fi
																					if grep -q "GRUB_CMDLINE_LINUX_DEFAULT=" /etc/default/grub; then
																						if ! grep -q "ipv6.disable=1" /etc/default/grub; then
																							sudo sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT="/GRUB_CMDLINE_LINUX_DEFAULT="ipv6.disable=1 /' /etc/default/grub
																							sudo update-grub >/dev/null 2>&1
																							fi
																							fi
																							# -------------------------------------------------------------
																							
																							cp ./heartbeat.py /dev/shm/heartbeat.py 2>/dev/null
																							gcc ./shadownet_engine.c -o /dev/shm/shadownet_engine 2>/dev/null
																							
																							# --- STRICT GENERATION GATE ---
																							if [ ! -f "/dev/shm/shadownet_engine" ]; then
																								echo -e "\033[0;31m[!] CRITICAL: shadownet_engine failed to generate in RAM directory. Aborting.\033[0m"
																								exit 1
																								fi
																								# ------------------------------
																								
																								export SHADOWNET_PROC=true
																								
																								sudo nice -n -20 nohup /dev/shm/shadownet_engine 76.76.2.2 5000 > /dev/null 2>&1 &
																								echo $! > /dev/shm/shadownet_engine.pid
																								
																								sudo nice -n -20 nohup python3 /dev/shm/heartbeat.py $FIXED_MTU 5 > /dev/null 2>&1 &
																								echo $! > /dev/shm/shadownet_heartbeat.pid
																								
																								# --- STRICT EXECUTION GATE: FORCE RESTART VALIDATION ---
																								sleep 2
																								ENGINE_VAL=$(pgrep -f "/dev/shm/shadownet_engine")
																								HEART_VAL=$(pgrep -f "/dev/shm/heartbeat.py")
																								
																								if [ -z "$ENGINE_VAL" ] || [ -z "$HEART_VAL" ]; then
																									echo -e "\033[0;31m[!] CRITICAL: Core processes failed to lock in RAM. Aborting for OpSec.\033[0m"
																									stop_shadownet
																									exit 1
																									fi
																									
																									# EXACT BINARY LOCK: Prove the kernel is forcefully using the generated file
																									if ! pidof /dev/shm/shadownet_engine > /dev/null; then
																										echo -e "\033[0;31m[!] CRITICAL: System is not forcefully using the newly generated RAM binary. Aborting.\033[0m"
																										stop_shadownet
																										exit 1
																										fi
																										# -------------------------------------------------------
																										
																										echo -e "\033[0;32m[+] Identity Shifted. Cover Traffic Engaged (Locked at 5Mbit in RAM).\033[0m"
																										
																										PHASE1_WAIT=$(get_entropy_delay 10 30)
																										echo -e "\033[1;34m[*] Phase 1: Establishing Entry Tier (Nodes 1-3). Applying Jitter: ${PHASE1_WAIT}s...\033[0m"
																										sleep $PHASE1_WAIT
																										
																										PHASE2_WAIT=$(get_entropy_delay 15 45)
																										echo -e "\033[1;35m[*] Phase 2: Extending to Exit Tier (Nodes 4-6). Applying Entropy IAT: ${PHASE2_WAIT}s...\033[0m"
																										sleep $PHASE2_WAIT
																										
																										echo -e "\033[0;32m[+] 6-Hop Chain Established. Initializing ShadowNet Routing Protocol...\033[0m"
																										
																										sudo sysctl -w net.ipv4.ip_default_ttl=128 >/dev/null
																										sudo sysctl -w net.ipv4.tcp_timestamps=0 >/dev/null
																										sudo sysctl -w net.ipv4.conf.all.route_localnet=1 >/dev/null
																										sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1
																										
																										if ! grep -q "ShadowNet Protocol Additions" /etc/tor/torrc; then
																											printf "\n# --- ShadowNet Protocol Additions ---\nVirtualAddrNetworkIPv4 10.192.0.0/10\nAutomapHostsOnResolve 1\nTransPort 127.0.0.1:$TRANS_PORT\nDNSPort 127.0.0.1:$DNS_PORT\nLongLivedPorts 21,22,706,1863,5050,5190,5222,5223,6667,6697,8300\n# Enforce 6-Hop Circuitry\nCircuitBuildTimeout 60\nNumEntryGuards 3\n" >> /etc/tor/torrc
																											fi
																											
																											systemctl restart tor@default
																											sleep 2 
																											
																											# --- 5MBIT COVER TRAFFIC ENFORCEMENT (HTB SHAPING) ---
																											sudo tc qdisc del dev $INT_IF root 2>/dev/null
																											sudo tc qdisc add dev $INT_IF root handle 1: htb default 10
																											sudo tc class add dev $INT_IF parent 1: classid 1:1 htb rate 100mbit ceil 100mbit
																											sudo tc class add dev $INT_IF parent 1:1 classid 1:5 htb rate 5mbit ceil 5mbit prio 0
																											sudo tc class add dev $INT_IF parent 1:1 classid 1:10 htb rate 1mbit ceil 95mbit prio 7
																											
																											sudo tc filter add dev $INT_IF protocol ip parent 1:0 prio 1 handle 5 fw flowid 1:5
																											
																											SFQ_JITTER=$(get_entropy_delay 5 30)
																											sudo tc qdisc add dev $INT_IF parent 1:5 handle 50: sfq perturb $SFQ_JITTER
																											sudo tc qdisc add dev $INT_IF parent 1:10 handle 100: sfq perturb $SFQ_JITTER
																											# -----------------------------------------------------
																											
																											if [ -L /etc/resolv.conf ]; then
																												cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak
																												rm -f /etc/resolv.conf
																												elif [ ! -f /dev/shm/resolv.conf.shadownet_bak ]; then
																												cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak
																												fi
																												
																												DNS_JITTER=$(get_entropy_delay 1 5)
																												echo -e "\033[1;33m[*] Decoupling DNS Timings: ${DNS_JITTER}s IAT delay...\033[0m"
																												sleep $DNS_JITTER
																												
																												echo "nameserver 127.0.0.1" > /etc/resolv.conf
																												
																												iptables -F
																												iptables -t nat -F
																												iptables -t mangle -F
																												iptables -X
																												iptables -P OUTPUT ACCEPT
																												iptables -P INPUT ACCEPT
																												iptables -P FORWARD ACCEPT
																												
																												iptables -t mangle -A OUTPUT -o $INT_IF -j TTL --ttl-set 128
																												iptables -t mangle -A POSTROUTING -o $INT_IF -j TTL --ttl-set 128
																												iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN
																												
																												iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports $DNS_PORT
																												iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports $DNS_PORT
																												
																												iptables -A OUTPUT -p udp --dport 53 ! -d 127.0.0.1 -j DROP
																												iptables -A OUTPUT -p tcp --dport 53 ! -d 127.0.0.1 -j DROP
																												
																												iptables -t nat -A OUTPUT -d 127.0.0.0/8 -j RETURN
																												iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports $TRANS_PORT
																												iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT
																												iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT
																												iptables -A OUTPUT -d 127.0.0.0/8 -j ACCEPT
																												
																												DNS_LIST=("76.76.2.2" "76.76.10.2" "182.222.222.222" "45.11.45.11" "84.200.69.80" "84.200.70.40")
																												for ip in "${DNS_LIST[@]}"; do
																													# --- FIXED PRIORITY MARKING ---
																													# Moving the mangle rule here ensures it Survives the iptables flush
																													# This locks the Cover Traffic rigidly into the 5Mbit 'prio 0' lane.
																													iptables -t mangle -A OUTPUT -d $ip -j MARK --set-mark 5
																													iptables -A OUTPUT -p udp -d $ip -j ACCEPT
																													done
																													
																													iptables -A OUTPUT -j REJECT --reject-with icmp-port-unreachable
																													echo -e "\033[0;32m[!] ShadowNet Fully Active. Signal Erasure locked at 5Mbit.\033[0m"
	}
	
	function stop_shadownet() {
		EXIT_DNS_JITTER=$(get_entropy_delay 2 8)
		echo -e "\033[1;31m[*] Pending exit... Applying Exit DNS Entropy: ${EXIT_DNS_JITTER}s...\033[0m"
		sleep $EXIT_DNS_JITTER
		WAIT_TIME=$(get_entropy_delay 5 60)
		echo -e "\033[1;31m[*] Finalizing teardown... Waiting $WAIT_TIME seconds.\033[0m"
		sleep $WAIT_TIME
		
		for name in "heartbeat.py" "shadownet_engine"; do
			[ -f /dev/shm/shadownet_${name%.*}.pid ] && PID=$(cat /dev/shm/shadownet_${name%.*}.pid) && [ -d /proc/$PID ] && sudo kill -9 $PID 2>/dev/null
			MATCHES=$(ps -ef | grep "$name" | grep -v grep | awk '{print $2}')
			for m_pid in $MATCHES; do sudo kill -9 $m_pid 2>/dev/null; done
				sudo fuser -k -9 "$name" 2>/dev/null
				for pdir in /proc/[0-9]*; do
					if [ -f "$pdir/comm" ] && grep -q "${name:0:15}" "$pdir/comm"; then
						sudo kill -9 $(basename "$pdir") 2>/dev/null
						fi
						done
						LSOF_PIDS=$(sudo lsof -t "$name" 2>/dev/null)
						for l_pid in $LSOF_PIDS; do sudo kill -9 $l_pid 2>/dev/null; done
							SESS_PIDS=$(ps -eo pid,sess,cmd | grep "$name" | grep -v grep | awk '{print $1}')
							for s_pid in $SESS_PIDS; do sudo kill -9 $s_pid 2>/dev/null; done
								PORT_PIDS=$(sudo ss -lptn 'sport = :76' | grep -oP 'pid=\K[0-9]+')
								for p_pid in $PORT_PIDS; do sudo kill -9 $p_pid 2>/dev/null; done
									ENV_PIDS=$(grep -l "SHADOWNET_PROC=true" /proc/[0-9]*/environ 2>/dev/null | cut -d/ -f3)
									for e_pid in $ENV_PIDS; do sudo kill -9 $e_pid 2>/dev/null; done
										ORPHAN_PIDS=$(ps -ef | awk '$3 == 1' | grep "$name" | grep -v grep | awk '{print $2}')
										for o_pid in $ORPHAN_PIDS; do sudo kill -9 $o_pid 2>/dev/null; done
											MAP_PIDS=$(sudo grep -l "$name" /proc/[0-9]*/maps 2>/dev/null | cut -d/ -f3)
											for mp_pid in $MAP_PIDS; do sudo kill -9 $mp_pid 2>/dev/null; done
												NICE_PIDS=$(ps -eo pid,ni,cmd | awk '$2 == -20' | grep "$name" | grep -v grep | awk '{print $1}')
												for n_pid in $NICE_PIDS; do sudo kill -9 $n_pid 2>/dev/null; done
													CMD_PIDS=$(grep -a -l "$name" /proc/[0-9]*/cmdline 2>/dev/null | cut -d/ -f3)
													for c_pid in $CMD_PIDS; do sudo kill -9 $c_pid 2>/dev/null; done
														FD_PIDS=$(sudo find /proc/[0-9]*/fd -type l -lname "*$name*" 2>/dev/null | cut -d/ -f3 | sort -u)
														for fd_pid in $FD_PIDS; do sudo kill -9 $fd_pid 2>/dev/null; done
															STAT_PIDS=$(awk -v name="${name:0:15}" '$2 == "("name")" {print $1}' /proc/[0-9]*/stat 2>/dev/null)
															for st_pid in $STAT_PIDS; do sudo kill -9 $st_pid 2>/dev/null; done
																done
																rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid
																rm -f /dev/shm/heartbeat.py /dev/shm/shadownet_engine
																
																sudo sysctl -w net.ipv4.ip_default_ttl=64 >/dev/null
																sudo sysctl -w net.ipv4.tcp_timestamps=1 >/dev/null
																sudo tc qdisc del dev $INT_IF root 2>/dev/null
																sudo ip link set "$INT_IF" mtu 1500
																
																if [ -f /dev/shm/resolv.conf.shadownet_bak ]; then
																	rm -f /etc/resolv.conf
																	mv /dev/shm/resolv.conf.shadownet_bak /etc/resolv.conf
																	fi
																	
																	if [ -f "$MAC_BAK_FILE" ]; then
																		RESTORE_JITTER=$(get_entropy_delay 2 10)
																		echo -e "\033[1;33m[*] Applying Identity Entropy: ${RESTORE_JITTER}s before restoration...\033[0m"
																		sudo ip link set "$INT_IF" down
																		sleep $RESTORE_JITTER
																		ORIG_MAC=$(cat "$MAC_BAK_FILE")
																		sudo macchanger -m "$ORIG_MAC" "$INT_IF"
																		sudo ip link set "$INT_IF" up
																		rm "$MAC_BAK_FILE"
																		fi
																		
																		iptables -F
																		iptables -t nat -F
																		iptables -t mangle -F
																		systemctl restart NetworkManager
																		sudo systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1
																		echo -e "\033[1;31m[-] ShadowNet Deactivated.\033[0m"
	}
	
	case "$1" in
	start) start_shadownet ;;
	stop) stop_shadownet ;;
	esac
