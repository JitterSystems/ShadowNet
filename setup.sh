#!/bin/bash
# ShadowNet Dependency Installer (C-Stack Optimized)
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

if [[ $EUID -ne 0 ]]; then
	echo -e "${RED}[!] Error: Run as root.${NC}"
	exit 1
	fi
	
	echo -e "${GREEN}[+] Installing Sovereign Dependencies...${NC}"
	apt-get update -y
	
	# 1. Fixed bind9-dnsutils for newer Parrot/Debian
	# 2. Removed tlsdate (replaced by htpdate or just system tools)
	# 3. Added build-essential to ensure gcc is ready
	DEBIAN_FRONTEND=noninteractive apt-get install -y \
	tor iptables-persistent iproute2 curl \
	macchanger haveged net-tools bind9-dnsutils \
	adjtimex ethtool tshark build-essential
	
	# Permissions for your new C files
	chmod +x shadownet.c
	chmod +x heartbeat.c
	chmod +x shadownet_engine.c
	
	echo -e "${GREEN}[V] Setup Complete.${NC}"
	echo -e "${GREEN}[*] Next Step: gcc shadownet.c -o shadownet${NC}"
	echo -e "${GREEN}[*] Then: sudo ./shadownet start${NC}"
