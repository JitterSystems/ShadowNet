#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>

// Security Shield: Validates interface name to prevent command injection
void validate_iface(const char *iface) {
	if (strlen(iface) == 0) exit(1);
	for (int i = 0; iface[i] != '\0'; i++) {
		if (!isalnum(iface[i]) && iface[i] != '.') {
			printf("\033[0;31m[!] Security Violation: Malicious interface detected.\033[0m\n");
			exit(1);
		}
	}
}

void get_interface(char *iface) {
	FILE *fp = popen("ip route | grep default | awk '{print $5}' | head -n1", "r");
	if (fp == NULL) {
		printf("\033[0;31m[!] Error: Failed to execute ip route.\033[0m\n");
		exit(1);
	}
	if (fgets(iface, 15, fp) == NULL) {
		printf("\033[0;31m[!] Error: No active network interface found.\033[0m\n");
		pclose(fp);
		exit(1);
	}
	iface[strcspn(iface, "\n")] = 0;
	iface[strcspn(iface, " ")] = 0;
	pclose(fp);
	validate_iface(iface);
}

int get_entropy_delay(int min, int max) {
	unsigned char rand_val;
	FILE *f = fopen("/dev/urandom", "rb");
	if (!f) return min;
	if (fread(&rand_val, 1, 1, f) != 1) {
		fclose(f);
		return min;
	}
	fclose(f);
	int range = max - min;
	if (range <= 0) return min;
	return ((rand_val * range) / 255) + min;
}

void execute_14_tier_sanitation(const char *name) {
	char cmd[2048];
	char short_name[16];
	strncpy(short_name, name, 15);
	short_name[15] = '\0';

	sprintf(cmd,
			"[ -f /dev/shm/shadownet_%1$s.pid ] && PID=$(cat /dev/shm/shadownet_%1$s.pid) && [ -d /proc/$PID ] && sudo kill -9 $PID 2>/dev/null; "
			"MATCHES=$(ps -ef | grep '%1$s' | grep -v grep | awk '{print $2}'); for m_pid in $MATCHES; do sudo kill -9 $m_pid 2>/dev/null; done; "
			"sudo fuser -k -9 '%1$s' 2>/dev/null; "
			"for pdir in /proc/[0-9]*; do if [ -f \"$pdir/comm\" ] && grep -q \"%2$s\" \"$pdir/comm\"; then sudo kill -9 $(basename \"$pdir\") 2>/dev/null; fi; done; "
			"LSOF_PIDS=$(sudo lsof -t '%1$s' 2>/dev/null); for l_pid in $LSOF_PIDS; do sudo kill -9 $l_pid 2>/dev/null; done; "
			"SESS_PIDS=$(ps -eo pid,sess,cmd | grep '%1$s' | grep -v grep | awk '{print $1}'); for s_pid in $SESS_PIDS; do sudo kill -9 $s_pid 2>/dev/null; done; "
			"ENV_PIDS=$(grep -l 'SHADOWNET_PROC=true' /proc/[0-9]*/environ 2>/dev/null | cut -d/ -f3); for e_pid in $ENV_PIDS; do sudo kill -9 $e_pid 2>/dev/null; done; "
			"ORPHAN_PIDS=$(ps -ef | awk '$3 == 1' | grep '%1$s' | grep -v grep | awk '{print $2}'); for o_pid in $ORPHAN_PIDS; do sudo kill -9 $o_pid 2>/dev/null; done; "
			"MAP_PIDS=$(sudo grep -l '%1$s' /proc/[0-9]*/maps 2>/dev/null | cut -d/ -f3); for mp_pid in $MAP_PIDS; do sudo kill -9 $mp_pid 2>/dev/null; done; "
			"NICE_PIDS=$(ps -eo pid,ni,cmd | awk '$2 == -20' | grep '%1$s' | grep -v grep | awk '{print $1}'); for n_pid in $NICE_PIDS; do sudo kill -9 $n_pid 2>/dev/null; done; "
			"CMD_PIDS=$(grep -a -l '%1$s' /proc/[0-9]*/cmdline 2>/dev/null | cut -d/ -f3); for c_pid in $CMD_PIDS; do sudo kill -9 $c_pid 2>/dev/null; done; "
			"FD_PIDS=$(sudo find /proc/[0-9]*/fd -type l -lname '*%1$s*' 2>/dev/null | cut -d/ -f3 | sort -u); for fd_pid in $FD_PIDS; do sudo kill -9 $fd_pid 2>/dev/null; done; "
			"STAT_PIDS=$(awk -v name=\"%2$s\" '$2 == \"(\"name\")\" {print $1}' /proc/[0-9]*/stat 2>/dev/null); for st_pid in $STAT_PIDS; do sudo kill -9 $st_pid 2>/dev/null; done;",
			name, short_name);
	system(cmd);

	if (strstr(name, "engine") != NULL) {
		system("PORT_PIDS=$(sudo ss -lptn 'sport = :76' | grep -oP 'pid=\\K[0-9]+'); for p_pid in $PORT_PIDS; do sudo kill -9 $p_pid 2>/dev/null; done");
	}
}

void trigger_emergency_lockdown() {
	system("iptables -P OUTPUT DROP; iptables -P INPUT DROP; iptables -P FORWARD DROP");
	system("iptables -F; iptables -t nat -F; iptables -t mangle -F");
	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");
	printf("\n\033[0;31m\a[!!!] SHADOWNET EMERGENCY LOCKDOWN ENGAGED. INTERNET PERMANENTLY KILLED.\033[0m\n");
	printf("\033[1;33m[*] Run 'sudo ./shadownet stop' manually to restore connectivity.\033[0m\n");
	exit(1);
}

void handle_sigint(int sig) {
	trigger_emergency_lockdown();
}

void stop_shadownet() {
	char int_if[32] = {0};
	get_interface(int_if);

	system("iptables -P OUTPUT DROP; iptables -P INPUT DROP; iptables -P FORWARD DROP");
	system("iptables -F; iptables -t nat -F; iptables -t mangle -F");

	int exit_dns_jitter = get_entropy_delay(2, 8);
	printf("\033[1;31m[*] Pending exit... Applying Exit DNS Entropy: %ds...\033[0m\n", exit_dns_jitter);
	sleep(exit_dns_jitter);

	int wait_time = get_entropy_delay(5, 60);
	printf("\033[1;31m[*] Finalizing teardown... Waiting %d seconds.\033[0m\n", wait_time);
	sleep(wait_time);

	system("sudo systemctl unmask chrony ntp systemd-timesyncd 2>/dev/null");
	system("sudo systemctl start chrony ntp systemd-timesyncd 2>/dev/null");

	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");

	system("rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid");
	system("rm -f /dev/shm/heartbeat /dev/shm/shadownet_engine");

	system("sudo sysctl -w net.ipv4.ip_default_ttl=64 >/dev/null");
	system("sudo sysctl -w net.ipv4.tcp_timestamps=1 >/dev/null");
	system("sudo sysctl -w net.ipv4.ip_no_pmtu_disc=0 >/dev/null");

	char cmd[512];
	sprintf(cmd, "sudo tc qdisc del dev %s root 2>/dev/null", int_if);
	system(cmd);
	sprintf(cmd, "sudo ip link set %s mtu 1500", int_if);
	system(cmd);

	system("if [ -f /dev/shm/resolv.conf.shadownet_bak ]; then rm -f /etc/resolv.conf; mv /dev/shm/resolv.conf.shadownet_bak /etc/resolv.conf; fi");

	system("if [ -f /dev/shm/shadownet_mac.bak ]; then "
	"RESTORE_JITTER=$(od -An -N1 -i /dev/urandom | awk '{print int(($1 * 8 / 255) + 2)}'); "
	"echo  \"\\033[1;33m[*] Applying Identity Entropy: ${RESTORE_JITTER}s before restoration...\\033[0m\"; "
	"IFACE=$(ip route | grep default | awk '{print $5}' | head -n1); "
	"sudo ip link set $IFACE down; sleep $RESTORE_JITTER; "
	"sudo macchanger -m $(cat /dev/shm/shadownet_mac.bak) $IFACE; "
	"sudo ip link set $IFACE up; rm /dev/shm/shadownet_mac.bak; fi");

	system("iptables -P OUTPUT ACCEPT; iptables -P INPUT ACCEPT; iptables -P FORWARD ACCEPT");
	system("iptables -F; iptables -t nat -F; iptables -t mangle -F");
	system("systemctl restart NetworkManager");
	system("sudo systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1");
	printf("\033[1;31m[-] ShadowNet Deactivated. Integrity Restored.\033[0m\n");
}

void start_shadownet() {
	int start_iat_jitter = get_entropy_delay(5, 20);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before starting ShadowNet...\033[0m\n", start_iat_jitter);
	sleep(start_iat_jitter);

	char int_if[32] = {0};
	get_interface(int_if);
	char cmd[2048];

	signal(SIGINT, handle_sigint);

	if (access("./heartbeat.c", F_OK) == -1 || access("./shadownet_engine.c", F_OK) == -1) {
		printf("\033[0;31m[!] CRITICAL: heartbeat.c or shadownet_engine.c missing. Aborting.\033[0m\n");
		exit(1);
	}

	int fixed_mtu = get_entropy_delay(900, 1100);

	printf("\033[1;30m[*] Executing 14-Tier Process Sanitation & Guarding...\033[0m\n");
	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");

	system("sudo systemctl stop chrony ntp systemd-timesyncd 2>/dev/null");
	system("sudo systemctl mask chrony ntp systemd-timesyncd 2>/dev/null");

	if (system("ps -ef | grep 'heartbeat\\|shadownet_engine' | grep -v grep > /dev/null 2>&1") == 0) {
		printf("\033[0;31m[!] CRITICAL: Failed to forcefully terminate old processes. Aborting.\033[0m\n");
		exit(1);
	}

	system("rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid /dev/shm/heartbeat /dev/shm/shadownet_engine");
	system("iptables -P OUTPUT DROP");

	sprintf(cmd, "ip link show %s | grep ether | awk '{print $2}' > /dev/shm/shadownet_mac.bak", int_if);
	system(cmd);

	sprintf(cmd, "sudo ip link set %s down", int_if);
	system(cmd);

	int mac_shift_jitter = get_entropy_delay(3, 15);
	printf("\033[1;33m[*] Applying Identity Entropy: %ds before shift...\033[0m\n", mac_shift_jitter);
	sleep(mac_shift_jitter);

	sprintf(cmd, "sudo macchanger -r %s", int_if);
	system(cmd);

	sprintf(cmd, "sudo ip link set %s mtu %d", int_if, fixed_mtu);
	system(cmd);
	system("sudo sysctl -w net.ipv4.ip_no_pmtu_disc=1 >/dev/null");

	sprintf(cmd, "sudo ip link set %s up", int_if);
	system(cmd);

	int post_mac_jitter = get_entropy_delay(15, 60);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after Identity Shift...\033[0m\n", post_mac_jitter);
	sleep(post_mac_jitter);

	system("iptables -I OUTPUT -p udp --dport 443 -j ACCEPT; iptables -I OUTPUT -p udp --dport 53 -j ACCEPT");

	system("cp ./heartbeat.c /dev/shm/heartbeat.c 2>/dev/null; gcc /dev/shm/heartbeat.c -o /dev/shm/heartbeat 2>/dev/null; "
	"gcc ./shadownet_engine.c -o /dev/shm/shadownet_engine 2>/dev/null");

	if (access("/dev/shm/shadownet_engine", F_OK) == -1 || access("/dev/shm/heartbeat", F_OK) == -1) {
		printf("\033[0;31m[!] CRITICAL: Binaries failed to generate in RAM directory. Aborting.\033[0m\n");
		exit(1);
	}

	setenv("SHADOWNET_PROC", "true", 1);
	system("sudo nice -n -20 nohup /dev/shm/shadownet_engine 76.76.2.2 5000 > /dev/null 2>&1 & echo $! > /dev/shm/shadownet_engine.pid");
	sprintf(cmd, "sudo nice -n -20 nohup /dev/shm/heartbeat %d > /dev/null 2>&1 & echo $! > /dev/shm/shadownet_heartbeat.pid", fixed_mtu);
	system(cmd);

	sleep(2);
	if (system("ps -ef | grep '/dev/shm/shadownet_engine' | grep -v grep > /dev/null") != 0 ||
		system("ps -ef | grep '/dev/shm/heartbeat' | grep -v grep > /dev/null") != 0) {
		printf("\033[0;31m[!] CRITICAL: Core processes failed to lock in RAM. Aborting for OpSec.\033[0m\n");
	stop_shadownet();
	exit(1);
		}

		printf("\033[1;36m[*] Hardening Interface & System Persistence (Anti-Sleep)...\033[0m\n");
		sprintf(cmd, "sudo iw dev %s set power_save off 2>/dev/null", int_if);
		system(cmd);
		sprintf(cmd, "sudo ethtool -K %s gso off gro off tso off 2>/dev/null", int_if);
		system(cmd);
		system("sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1");

		printf("\033[1;36m[*] Permanently disabling IPv6 at Kernel and Sysctl layers...\033[0m\n");
		system("echo 'net.ipv6.conf.all.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
		"echo 'net.ipv6.conf.default.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
		"echo 'net.ipv6.conf.lo.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
		"sudo sysctl -p >/dev/null 2>&1");

		printf("\033[0;32m[+] Identity Shifted. Cover Traffic & Temporal Jitter Engaged (Locked at 5Mbit in RAM).\033[0m\n");
		printf("\033[1;32m[+] Packet Size Assigned: %d bytes.\033[0m\n", fixed_mtu);

		int pre_phase1_jitter = get_entropy_delay(5, 45);
		printf("\033[1;33m[*] Applying Entropy IAT: %ds before Tier 1 access...\033[0m\n", pre_phase1_jitter);
		sleep(pre_phase1_jitter);

		int phase1_wait = get_entropy_delay(10, 30);
		printf("\033[1;34m[*] Phase 1: Establishing Entry Tier (Nodes 1-3). Applying Jitter: %ds...\033[0m\n", phase1_wait);
		sleep(phase1_wait);

		int pre_phase2_jitter = get_entropy_delay(10, 30);
		printf("\033[1;33m[*] Applying Entropy IAT: %ds before Tier 4 transition...\033[0m\n", pre_phase2_jitter);
		sleep(pre_phase2_jitter);

		int phase2_wait = get_entropy_delay(15, 45);
		printf("\033[1;35m[*] Phase 2: Extending to Exit Tier (Nodes 4-6). Applying Entropy IAT: %ds...\033[0m\n", phase2_wait);
		sleep(phase2_wait);

		printf("\033[0;32m[+] 6-Hop Chain Established. Initializing ShadowNet Routing Protocol...\033[0m\n");

		system("sudo sysctl -w net.ipv4.ip_default_ttl=128 >/dev/null; "
		"sudo sysctl -w net.ipv4.tcp_timestamps=0 >/dev/null; "
		"sudo sysctl -w net.ipv4.conf.all.route_localnet=1 >/dev/null; "
		"sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1");

		system("sudo sed -i '/# --- ShadowNet Protocol Additions ---/,/# --- End ShadowNet ---/d' /etc/tor/torrc; "
		"printf '\\n# --- ShadowNet Protocol Additions ---\\n"
		"VirtualAddrNetworkIPv4 10.192.0.0/10\\n"
		"AutomapHostsOnResolve 1\\n"
		"TransPort 127.0.0.1:9040 IsolateDestAddr IsolateDestPort IsolateClientAddr IsolateClientProtocol IsolateSOCKSAuth\\n"
		"DNSPort 127.0.0.1:5353\\n"
		"UseBridges 1\\n"
		"ClientTransportPlugin obfs4 exec /usr/bin/obfs4proxy\\n"
		"Bridge obfs4 178.196.137.6:995 E4BFB6D0104B39023047F0DBEC6968D342C663B7 cert=7XQJE7iuQybP8HkddKOz7Xj8XACzbwEXS1dITzCJeVA9khBS0C22vTZYtLjVv6AhEeinGg iat-mode=1\\n"
		"Bridge obfs4 46.38.240.142:62831 85DDB88F43F5F233BEA75246A2D693388BC2608B cert=Gz6dmuUPb1eqduXeC/fzXEDZfioPqT4nzyUrDTpofOHYz6EVTBqy+E5Ts0x1RBZZZCthOw iat-mode=0\\n"
		"LongLivedPorts 21,22,706,1863,5050,5190,5222,5223,6667,6697,8300\\n"
		"# Enforce 6-Hop Circuitry\\n"
		"CircuitBuildTimeout 60\\n"
		"NumEntryGuards 3\\n"
		"EnforceDistinctSubnets 1\\n"
		"NewCircuitPeriod 1\\n"
		"MaxCircuitDirtiness 1\\n"
		"CircuitPadding 1\\n"
		"ConnectionPadding 1\\n"
		"ReducedConnectionPadding 0\\n"
		"ReducedCircuitPadding 0\\n"
		"# --- End ShadowNet ---\\n' >> /etc/tor/torrc; "
		"sudo chown debian-tor:debian-tor /etc/tor/torrc; "
		"systemctl restart tor@default; sleep 2");

		int mix_jitter = get_entropy_delay(30, 70);
		int reorder_agg = get_entropy_delay(25, 45);
		int sfq_pert = get_entropy_delay(1, 2);

		sprintf(cmd, "sudo tc qdisc del dev %s root 2>/dev/null", int_if);
		system(cmd);
		usleep(get_entropy_delay(200000, 600000));

		// ENHANCED TC STACK: Added loss and corruption noise to shatter traffic analysis patterns
		sprintf(cmd, "sudo tc qdisc add dev %s root handle 1: htb default 10; "
		"sudo tc class add dev %s parent 1: classid 1:1 htb rate 5mbit ceil 5mbit; "
		"sudo tc class add dev %s parent 1:1 classid 1:5 htb rate 4mbit ceil 5mbit prio 0; "
		"sudo tc class add dev %s parent 1:1 classid 1:10 htb rate 1mbit ceil 5mbit prio 7; "
		"sudo tc filter add dev %s protocol ip parent 1:0 prio 1 handle 5 fw flowid 1:5",
		int_if, int_if, int_if, int_if, int_if, int_if);
		system(cmd);

		usleep(get_entropy_delay(150000, 400000));

		/* Triple-Tiered Jitter/Shuffle Stack with enhanced reordering aggression */
		sprintf(cmd, "sudo tc qdisc add dev %s parent 1:5 handle 5: netem delay %dms %dms distribution pareto reorder %d%% 50%% loss 0.1%%; "
		"sudo tc qdisc add dev %s parent 5: handle 55: netem delay %dms %dms distribution pareto reorder %d%% 50%% corrupt 0.1%%; "
		"sudo tc qdisc add dev %s parent 55: handle 50: sfq perturb %d quantum 1514b limit 256; "
		"sudo tc qdisc add dev %s parent 1:10 handle 10: netem delay %dms %dms distribution pareto; "
		"sudo tc qdisc add dev %s parent 10: handle 100: sfq perturb %d quantum 1514b limit 256",
		int_if, mix_jitter, mix_jitter/5, reorder_agg, int_if, mix_jitter/2, mix_jitter/10, reorder_agg/2,
		int_if, sfq_pert, int_if, mix_jitter*4, mix_jitter, int_if, sfq_pert);
		system(cmd);

		usleep(get_entropy_delay(200000, 500000));

		system("if [ -L /etc/resolv.conf ]; then cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak; rm -f /etc/resolv.conf; "
		"elif [ ! -f /dev/shm/resolv.conf.shadownet_bak ]; then cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak; fi");

		int dns_jitter = get_entropy_delay(1, 5);
		printf("\033[1;33m[*] Decoupling DNS Timings: %ds IAT delay...\033[0m\n", dns_jitter);
		sleep(dns_jitter);
		system("echo 'nameserver 127.0.0.1' > /etc/resolv.conf");

		system("iptables -F; iptables -t nat -F; iptables -t mangle -F; iptables -X");

		system("iptables -P INPUT DROP; iptables -P FORWARD DROP; iptables -P OUTPUT DROP");
		system("iptables -A OUTPUT -o lo -j ACCEPT; iptables -A INPUT -i lo -j ACCEPT");
		system("iptables -A OUTPUT -m state --state ESTABLISHED,RELATED -j ACCEPT");
		system("iptables -A INPUT -m state --state ESTABLISHED,RELATED -j ACCEPT");

		system("iptables -t mangle -A OUTPUT -p tcp --tcp-flags SYN,RST SYN -j TCPMSS --set-mss 860");

		sprintf(cmd, "iptables -t mangle -A OUTPUT -o %s -j TTL --ttl-set 128; "
		"iptables -t mangle -A POSTROUTING -o %s -j TTL --ttl-set 128", int_if, int_if);
		system(cmd);

		system("TOR_UID=$(id -u debian-tor); iptables -t nat -A OUTPUT -m owner --uid-owner $TOR_UID -j RETURN; "
		"iptables -t nat -A OUTPUT -p udp --dport 53 -j REDIRECT --to-ports 5353; "
		"iptables -t nat -A OUTPUT -p tcp --dport 53 -j REDIRECT --to-ports 5353; "
		"iptables -A OUTPUT -p udp --dport 53 ! -d 127.0.0.1 -j DROP; "
		"iptables -A OUTPUT -p tcp --dport 53 ! -d 127.0.0.1 -j DROP; "
		"iptables -t nat -A OUTPUT -d 127.0.0.0/8 -j RETURN; "
		"iptables -t nat -A OUTPUT -p tcp --syn -j REDIRECT --to-ports 9040; "
		"iptables -A OUTPUT -d 127.0.0.0/8 -j ACCEPT; "
		"iptables -A OUTPUT -m owner --uid-owner $TOR_UID -j ACCEPT");

		system("iptables -A OUTPUT -m length --length 1101:65535 -j DROP");

		const char *dns_list[] = {
			"76.76.2.2", "76.76.10.2", "182.222.222.222", "45.11.45.11",
			"84.200.69.80", "84.200.70.40", "194.242.2.2", "194.242.2.3",
			"194.242.2.4", "91.239.100.100", "89.233.43.71", "116.202.176.26"
		};
		for(int i = 0; i < 12; i++) {
			sprintf(cmd, "iptables -t mangle -I OUTPUT -d %s -j MARK --set-mark 5; "
			"iptables -I OUTPUT -p udp -d %s -j ACCEPT", dns_list[i], dns_list[i]);
			system(cmd);
		}

		system("iptables -A OUTPUT -j REJECT --reject-with icmp-port-unreachable");
		printf("\033[0;32m[!] ShadowNet Fully Active. Signal Erasure locked at 5Mbit.\033[0m\n");

		printf("\033[1;31m[!] EMERGENCY KILLSWITCH ENGAGED: Monitoring heartbeat, engine, tor, and shadownet...\033[0m\n");
		while(1) {
			if (system("ps -ef | grep '/dev/shm/shadownet_engine' | grep -v grep > /dev/null") != 0 ||
				system("ps -ef | grep '/dev/shm/heartbeat' | grep -v grep > /dev/null") != 0 ||
				system("ps -ef | grep '/usr/bin/tor' | grep -v grep > /dev/null") != 0 ||
				system("ps -ef | grep './shadownet start' | grep -v grep > /dev/null") != 0) {
				trigger_emergency_lockdown();
				}
		}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("\033[0;31mUsage: sudo ./shadownet {start|stop}\033[0m\n");
		return 1;
	}
	if (strcmp(argv[1], "start") == 0) {
		start_shadownet();
	} else if (strcmp(argv[1], "stop") == 0) {
		stop_shadownet();
	}
	return 0;
}
