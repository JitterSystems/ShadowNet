#!/bin/bash

# --- Colors for Output ---
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${YELLOW}[*] ShadowNet Dependency Installer Initializing...${NC}"

# Check for root privileges
if [[ $EUID -ne 0 ]]; then
	echo -e "${RED}[!] Error: This script must be run as root (sudo).${NC}"
	exit 1
	fi
	
	# 1. Update Package Lists
	echo -e "${GREEN}[+] Updating system package lists...${NC}"
	apt-get update -y
	
	# 2. Install Core Dependencies
	# tor: The anonymity network
	# iptables-persistent: To manage firewall rules across reboots
	# iproute2: Provides the 'tc' command for traffic shaping
	# curl: For verifying the connection
	echo -e "${GREEN}[+] Installing Tor, IPTables-Persistent, and Traffic Control tools...${NC}"
	DEBIAN_FRONTEND=noninteractive apt-get install -y tor iptables-persistent iproute2 curl
	
	# 3. Enable and Start Tor Service
	echo -e "${GREEN}[+] Enabling Tor service...${NC}"
	systemctl enable tor
	systemctl start tor
	
	# 4. Finalizing Environment
	echo -e "${GREEN}[+] Setting permissions for ShadowNet...${NC}"
	if [ -f "shadownet.sh" ]; then
		chmod +x shadownet.sh
		echo -e "${GREEN}[+] shadownet.sh is now executable.${NC}"
		else
			echo -e "${YELLOW}[!] Warning: shadownet.sh not found in current directory.${NC}"
			fi
			
			echo -e "${YELLOW}--------------------------------------------------${NC}"
			echo -e "${GREEN}[V] Setup Complete!${NC}"
			echo -e "${YELLOW}[i] Usage: sudo ./shadownet.sh start${NC}"
			echo -e "${YELLOW}--------------------------------------------------${NC}"
