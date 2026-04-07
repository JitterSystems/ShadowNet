#!/bin/bash
# ShadowNet Dependency Installer
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m'

if [[ $EUID -ne 0 ]]; then
	echo -e "${RED}[!] Error: Run as root.${NC}"
	exit 1
	fi
	
	echo -e "${GREEN}[+] Installing Sovereign Dependencies...${NC}"
	apt-get update -y
	DEBIAN_FRONTEND=noninteractive apt-get install -y tor iptables-persistent iproute2 curl tlsdate macchanger haveged net-tools dnsutils adjtimex ethtool tshark
	
	chmod +x shadownet.sh
	echo -e "${GREEN}[V] Setup Complete. Use 'sudo ./shadownet.sh start' to initialize.${NC}"
