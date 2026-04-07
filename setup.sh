#!/bin/bash
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

if [[ $EUID -ne 0 ]]; then
	echo -e "${RED}[!] Error: Run as root.${NC}"
	exit 1
	fi
	
	echo -e "${GREEN}[+] Installing dependencies...${NC}"
	apt-get update -y
	DEBIAN_FRONTEND=noninteractive apt-get install -y tor iptables-persistent iproute2 curl tlsdate macchanger haveged net-tools
	
	chmod +x shadow.sh
	echo -e "${GREEN}[V] Setup Complete.${NC}"
