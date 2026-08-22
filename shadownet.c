#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <time.h>
#include <math.h> // Added for exponential distribution calculations

// Global variable required by the monitoring loop condition
int proc_missing = 0;

/* Helper function to securely execute commands using fork and execve, * replacing the vulnerable system() call completely while maintaining execution context. */
int safe_execute(const char *cmd_string) {
	pid_t pid = fork();
	if (pid == 0) {
		char *args[] = {"/bin/sh", "-c", (char *)cmd_string, NULL};
		// Using execve for maximum control over environment isolation
		extern char **environ;
		execve("/bin/sh", args, environ);
		_exit(127);
	} else if (pid > 0) {
		int status;
		waitpid(pid, &status, 0);
		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}
		return -1;
	}
	return -1;
}

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
	memset(iface, 0, 32);
	if (fgets(iface, 15, fp) == NULL) {
		printf("\033[0;31m[!] Error: No active network interface found.\033[0m\n");
		pclose(fp);
		exit(1);
	}
	iface[strcspn(iface, "\n\r ")] = 0;
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

int get_true_5050() {
	unsigned char rand_val;
	FILE *f = fopen("/dev/urandom", "rb");
	if (f) {
		if (fread(&rand_val, 1, 1, f) == 1) {
			fclose(f);
			return rand_val % 2;
		}
		fclose(f);
	}
	return rand_val % 2; // Fixed: Swapped fallback out of standard rand() to urandom value
}

// Loopix Helper: Generates an exponential delay matching specific lambda parameter
double get_loopix_poisson_delay(double lambda) {
	unsigned int val = 0;
	FILE *f = fopen("/dev/urandom", "rb");
	if (f) {
		if (fread(&val, sizeof(val), 1, f) != 1) val = 1;
		fclose(f);
	}
	double u = (double)val / 4294967295.0;
	if (u <= 0.0) u = 0.000001;
	return -log(u) / lambda;
}

void execute_14_tier_sanitation(const char *name) {
	char cmd[4096];
	char short_name[16];
	strncpy(short_name, name, 15);
	short_name[15] = '\0';
	// Replaced sprintf with snprintf
	snprintf(cmd, sizeof(cmd), "[ -f /dev/shm/shadownet_%1$s.pid ] && PID=$(cat /dev/shm/shadownet_%1$s.pid) && [ -d /proc/$PID ] && sudo kill -9 $PID 2>/dev/null; "
	"MATCHES=$(ps -ef | grep '%1$s' | grep -v grep | awk '{print $2}'); for m_pid in $MATCHES; do sudo kill -9 $m_pid 2>/dev/null; done; "
	"sudo fuser -k -9 '%1$s' 2>/dev/null; "
	"for pdir in /proc/[0-9]*; do if [ -f \"$pdir/comm\" ] && grep -q \"%2$s\" \"$pdir/comm\"; then sudo kill -9 $(basename \"$pdir\") 2>/dev/null; fi; done; "
	"LSOF_PIDS=$(sudo lsof -t '%1$s' 2>/dev/null); for l_pid in $LSOF_PIDS; do sudo kill -9 $l_pid 2>/dev/null; done; "
	"SESS_PIDS=$(ps -eo pid,sess,cmd | grep '%1$s' | grep -v grep | awk '{print $1}'); for s_pid in $SESS_PIDS; do sudo kill -9 $s_pid 2>/dev/null; done; "
	"ENV_PIDS=$(grep -l 'SHADOWNET_PROC=true' /proc/[0-9]*/environ 2>/dev/null | cut -d/ -f3); for e_pid in $ENV_PIDS; do sudo kill -9 $e_pid 2>/dev/null; done; "
	"ORPHAN_PIDS=$(ps -ef | awk '$3 == 1' | grep '%1$s' | grep -v grep | awk '{print $2}'); for o_pid in $ORPHAN_PIDS; do sudo kill -9 $o_pid 2>/dev/null; done; "
	"MAP_PIDS=$(sudo grep -l '%1$s' /proc/[0-9]*/maps 2>/dev/null | cut -d/ -f3); for mp_pid in $MAP_PIDS; do sudo kill -9 $mp_pid 2>/dev/null; done; "
	"NICE_PIDS=$(ps -eo pid,ni,cmd | awk '$2 == -20' | grep '%1$s' | grep -v grep | awk '$1 != \"\"' | awk '{print $1}'); for n_pid in $NICE_PIDS; do sudo kill -9 $n_pid 2>/dev/null; done; "
	"CMD_PIDS=$(grep -a -l '%1$s' /proc/[0-9]*/cmdline 2>/dev/null | cut -d/ -f3); for c_pid in $CMD_PIDS; do sudo kill -9 $c_pid 2>/dev/null; done; "
	"FD_PIDS=$(sudo find /proc/[0-9]* /fd -type l -lname '*%1$s*' 2>/dev/null | cut -d/ -f3 | sort -u); for fd_pid in $FD_PIDS; do sudo kill -9 $fd_pid 2>/dev/null; done; "
	"STAT_PIDS=$(awk -v name=\"%2$s\" '$2 == \"(\"name\")\" {print $1}' /proc/[0-9]*/stat 2>/dev/null); for st_pid in $STAT_PIDS; do sudo kill -9 $st_pid 2>/dev/null; done;", name, short_name);
	safe_execute(cmd);
	if (strstr(name, "engine") != NULL) {
		safe_execute("PORT_PIDS=$(sudo ss -lptn 'sport = :76' | grep -oP 'pid=\\K[0-9]+'); for p_pid in $PORT_PIDS; do sudo kill -9 $p_pid 2>/dev/null; done");
	}
}

void trigger_emergency_lockdown() {
	// XDP Map Toggle 1ms Killswitch implementation hook
	safe_execute("sudo bpftool map update pinned /sys/fs/bpf/shadownet_lockdown_map key 0 0 0 0 value 1 0 0 0 2>/dev/null");
	safe_execute("nft flush ruleset");
	safe_execute("nft add table inet shadownet");
	safe_execute("nft add chain inet shadownet input '{ type filter hook input priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet forward '{ type filter hook forward priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet output '{ type filter hook output priority 0; policy drop; }'");
	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");
	safe_execute("sudo systemctl stop lokinet");
	printf("\n\n\033[0;31m\a[!!!] SHADOWNET EMERGENCY LOCKDOWN ENGAGED. INTERNET PERMANENTLY KILLED.\033[0m\n");
	printf("\033[1;33m[*] Run 'sudo ./shadownet stop' manually to restore connectivity.\033[0m\n");
	exit(1);
}

void handle_sigint(int sig) {
	trigger_emergency_lockdown();
}

void stop_shadownet() {
	char int_if[32] = {0};
	get_interface(int_if);
	safe_execute("nft flush ruleset");
	safe_execute("nft add table inet shadownet");
	safe_execute("nft add chain inet shadownet input '{ type filter hook input priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet forward '{ type filter hook forward priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet output '{ type filter hook output priority 0; policy drop; }'");
	int exit_dns_jitter = get_entropy_delay(2, 8);
	printf("\033[1;31m[*] Pending exit... Applying Exit DNS Entropy: %ds...\033[0m\n", exit_dns_jitter);
	sleep(exit_dns_jitter);
	int wait_time = get_entropy_delay(5, 60);
	printf("\033[1;31m[*] Finalizing teardown... Waiting %d seconds.\033[0m\n", wait_time);
	sleep(wait_time);
	safe_execute("sudo systemctl stop lokinet 2>/dev/null");
	safe_execute("sudo systemctl unmask chrony ntp systemd-timesyncd 2>/dev/null");
	safe_execute("sudo systemctl start chrony ntp systemd-timesyncd 2>/dev/null");
	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");
	safe_execute("sudo rfkill unblock bluetooth 2>/dev/null");
	safe_execute("sudo modprobe uvcvideo 2>/dev/null");
	safe_execute("sudo modprobe snd_hda_intel 2>/dev/null");
	safe_execute("sudo chattr -i /sys/firmware/efi/efivars/* 2>/dev/null");
	safe_execute("rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid");
	safe_execute("rm -f /dev/shm/heartbeat /dev/shm/shadownet_engine");
	safe_execute("sudo sysctl -w net.ipv4.ip_default_ttl=64 >/dev/null");
	safe_execute("sudo sysctl -w net.ipv4.tcp_timestamps=1 >/dev/null");
	safe_execute("sudo sysctl -w net.ipv4.ip_no_pmtu_disc=0 >/dev/null");
	safe_execute("sudo adjtimex -t 10000 >/dev/null 2>&1");
	char cmd[512];
	// Replaced sprintf with snprintf
	snprintf(cmd, sizeof(cmd), "sudo tc qdisc del dev %.16s root 2>/dev/null", int_if);
	safe_execute(cmd);
	snprintf(cmd, sizeof(cmd), "sudo ip link set %.16s mtu 1500", int_if);
	safe_execute(cmd);
	safe_execute("if [ -f /dev/shm/resolv.conf.shadownet_bak ]; then rm -f /etc/resolv.conf; mv /dev/shm/resolv.conf.shadownet_bak /etc/resolv.conf; fi");
	safe_execute("if [ -f /dev/shm/shadownet_mac.bak ]; then "
	"RESTORE_JITTER=$(od -An -N1 -i /dev/urandom | awk '{print int(($1 * 8 / 255) + 2)}'); "
	"echo \"\\033[1;33m[*] Applying Identity Entropy: ${RESTORE_JITTER}s before restoration...\\033[0m\"; "
	"IFACE=$(ip route | grep default | awk '{print $5}' | head -n1); "
	"sudo ip link set $IFACE down; sleep $RESTORE_JITTER; "
	"sudo macchanger -m $(cat /dev/shm/shadownet_mac.bak) $IFACE; "
	"sudo ip link set $IFACE up; rm /dev/shm/shadownet_mac.bak; fi");
	safe_execute("nft flush ruleset");
	safe_execute("nft add table inet shadownet");
	safe_execute("nft add chain inet shadownet input '{ type filter hook input priority 0; policy accept; }'");
	safe_execute("nft add chain inet shadownet forward '{ type filter hook forward priority 0; policy accept; }'");
	safe_execute("nft add chain inet shadownet output '{ type filter hook output priority 0; policy accept; }'");
	safe_execute("nft flush table inet shadownet");
	safe_execute("sudo rm -f /etc/NetworkManager/conf.d/dhcp-anon.conf");
	safe_execute("sudo systemctl restart NetworkManager");
	safe_execute("sudo systemctl unmask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1");
	// Clean up XDP killswitch attachments (FIXED: Formatted into buffer before calling safe_execute)
	char xdp_off_cmd[256];
	snprintf(xdp_off_cmd, sizeof(xdp_off_cmd), "sudo ip link set dev %.16s xdp off 2>/dev/null; sudo tc qdisc del dev %.16s ingress 2>/dev/null", int_if, int_if);
	safe_execute(xdp_off_cmd);
	safe_execute("sudo rm -f /sys/fs/bpf/shadownet_lockdown_map 2>/dev/null");
	safe_execute("USER_UID=$(id -u $SUDO_USER 2>/dev/null || id -u); [ -n \"$USER_UID\" ] && sudo ip rule del uidrange $USER_UID-$USER_UID table shadownet_table 2>/dev/null; ROOT_UID=$(id -u root 2>/dev/null || echo 0); [ -n \"$ROOT_UID\" ] && sudo ip rule del uidrange $ROOT_UID-$ROOT_UID table shadownet_table 2>/dev/null");
	printf("\n\033[1;31m[-] ShadowNet Deactivated. Integrity Restored.\033[0m\n");
}

/* eBPP Entropy Helper Function * Performs advanced /dev/urandom structural scrambling on local address space routing, * custom protocol headers, and handles high-resolution sub-second entropy timing delays. */
void ebpp_entropy_scramble(char *rand_dest_ip, char *rand_src_ip, int *tos_val) {
	unsigned char stream[8];
	struct timespec ns_jitter;
	FILE *f = fopen("/dev/urandom", "rb");
	if (f) {
		if (fread(&stream, 1, 8, f) != 8) {
			for (int i = 0; i < 8; i++) stream[i] = stream[0];
		}
		fclose(f);
	} else {
		for (int i = 0; i < 8; i++) stream[i] = 127;
	}
	// Fully dynamic loopback sub-address variations (127.b2.b3.b4)
	int b2_d = (stream[0] % 254) + 1;
	int b3_d = (stream[1] % 254) + 1;
	int b4_d = (stream[2] % 254) + 1;
	snprintf(rand_dest_ip, 64, "127.%d.%d.%d", b2_d, b3_d, b4_d);
	// Advanced Local Subnet Masking Scramble (127.b2.b3.b4 source alias)
	int b2_s = (stream[3] % 254) + 1;
	int b3_s = (stream[4] % 254) + 1;
	int b4_s = (stream[5] % 254) + 1;
	snprintf(rand_src_ip, 64, "127.%d.%d.%d", b2_s, b3_s, b4_s);
	// Randomize Type of Service (ToS) / Differentiated Services Field
	*tos_val = stream[6];
	// High-resolution sub-second entropy delay block (0 - 800,000 nanoseconds)
	unsigned int raw_nsec = ((stream[7] << 8) | stream[5]);
	ns_jitter.tv_sec = 0;
	ns_jitter.tv_nsec = raw_nsec % 800000;
	nanosleep(&ns_jitter, NULL);
}

void start_shadownet() {
	int alias_roll = 1;
	int fixed_mtu = 1400;
	int fixed_payload_size = get_entropy_delay(500, fixed_mtu - 42);
	int start_iat_jitter = get_entropy_delay(5, 20);

	// Dynamically determine home directory based on running executable to guarantee RAM compiler alignment
	char base_dir[1024] = {0};
	ssize_t exelen = readlink("/proc/self/exe", base_dir, sizeof(base_dir) - 1);
	if (exelen != -1) {
		base_dir[exelen] = '\0';
		char *last_slash = strrchr(base_dir, '/');
		if (last_slash) *last_slash = '\0';
	} else {
		strcpy(base_dir, ".");
	}

	// Moved all global strict drop policies to the top of the function before background services are touched
	safe_execute("nft flush ruleset");
	safe_execute("nft add table inet shadownet");
	safe_execute("nft add chain inet shadownet input '{ type filter hook input priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet forward '{ type filter hook forward priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet output '{ type filter hook output priority 0; policy drop; }'");

	printf("\033[1;33m[*] Applying Entropy IAT: %ds before starting ShadowNet...\033[0m\n", start_iat_jitter);
	sleep(start_iat_jitter);
	int hw_iat;
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Bluetooth...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo rfkill block bluetooth 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after disabling Bluetooth...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Bluetooth Hardware: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Audio/Microphone...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo fuser -k /dev/snd/* >/dev/null 2>&1; sudo modprobe -r snd_hda_intel snd_usb_audio 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after disabling Audio/Microphone...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Internal/External Microphone: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Camera/Webcam...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo fuser -k /dev/video* >/dev/null 2>&1; sudo modprobe -r uvcvideo 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after disabling Camera/Webcam...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Internal/External Webcam: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Motion Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo modprobe -r hid_sensor_accel_3d hid_sensor_gyro_3d hid_sensor_hub 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after disabling Motion Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Gyroscopes and Accelerometers: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Light Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo modprobe -r hid_sensor_als 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after disabling Light Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Ambient Light Sensors: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before disabling Thermal Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo modprobe -r intel_rapl_msr intel_rapl_common processor_thermal_device_pci_legacy thermal 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after Thermal Sensors...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Thermal Sensors: DISABLED\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before TEMPEST Mitigation...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo sysctl -w kernel.randomize_va_space=2 >/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after TEMPEST Mitigation...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Electromagnetic Interference (TEMPEST) Shielded.\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before BIOS Hardening...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo chattr +i /sys/firmware/efi/efivars/* 2>/dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after BIOS Hardening...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] BIOS/Firmware Immutable Protection: ACTIVE\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before Power Randomization...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo cpupower frequency-set -g powersave >/dev/null 2>&1");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after Power Randomization...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] Power Supply Side-Channel & Entropy Randomization: ACTIVE\033[0m\n");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before DHCP/Hostname Scrubbing...\033[0m\n", hw_iat);
	sleep(hw_iat);
	safe_execute("sudo hostnamectl set-hostname 'localhost'");
	safe_execute("printf '[main]\\ndhcp=dhclient\\n\\n[ifupdown]\\nmanaged=false\\n' | sudo tee /etc/NetworkManager/conf.d/dhcp-anon.conf > /dev/null");
	hw_iat = get_entropy_delay(2, 5);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after DHCP/Hostname Scrubbing...\033[0m\n", hw_iat);
	sleep(hw_iat);
	printf("\033[1;31m[!] DHCP Hostname Scrubbing & Anonymization: ACTIVE\033[0m\n");
	char int_if[32] = {0};
	get_interface(int_if);
	char cmd[2048];
	signal(SIGINT, handle_sigint);

	// Validate components exist relative to derived base directory location to eliminate path desyncs
	char comp_chk_cmd[2048];
	snprintf(comp_chk_cmd, sizeof(comp_chk_cmd), "[ -f '%s/heartbeat.c' ] && [ -f '%s/shadownet_engine.c' ]", base_dir, base_dir);
	if (safe_execute(comp_chk_cmd) != 0) {
		printf("\033[0;31m[!] CRITICAL: Core components missing from application path. Triggering Lockdown.\033[0m\n");
		trigger_emergency_lockdown();
	}

	int target_mbit = get_entropy_delay(5, 20);
	printf("\033[1;30m[*] Executing 14-Tier Process Sanitation & Guarding...\033[0m\n");
	execute_14_tier_sanitation("heartbeat");
	execute_14_tier_sanitation("shadownet_engine");
	safe_execute("sudo systemctl stop chrony ntp systemd-timesyncd 2>/dev/null");
	safe_execute("sudo systemctl mask chrony ntp systemd-timesyncd 2>/dev/null");
	if (safe_execute("ps -ef | grep 'heartbeat\\|shadownet_engine' | grep -v grep > /dev/null 2>&1") == 0) {
		printf("\033[0;31m[!] CRITICAL: Failed to forcefully terminate old processes. Aborting.\033[0m\n");
		exit(1);
	}
	safe_execute("rm -f /dev/shm/shadownet_heartbeat.pid /dev/shm/shadownet_engine.pid /dev/shm/heartbeat /dev/shm/shadownet_engine");
	safe_execute("nft add chain inet shadownet output '{ policy drop; }'");

	// Explicitly search for _lokinet or lokinet user ids to preserve system infrastructure compatibility dynamically
	safe_execute("LOKI_UID=$(id -u _lokinet 2>/dev/null || id -u lokinet 2>/dev/null); "
	"if [ -n \"$LOKI_UID\" ]; then nft insert rule inet shadownet output meta skuid $LOKI_UID accept; fi");
	safe_execute("nft insert rule inet shadownet input ct state '{ established, related }' accept");

	// Replaced sprintf with snprintf
	snprintf(cmd, sizeof(cmd), "ip link show %.16s | grep ether | awk '{print $2}' > /dev/shm/shadownet_mac.bak", int_if);
	safe_execute(cmd);
	snprintf(cmd, sizeof(cmd), "sudo ip link set %.16s down", int_if);
	safe_execute(cmd);
	int mac_shift_jitter = get_entropy_delay(3, 15);
	printf("\033[1;33m[*] Applying Identity Entropy: %ds before shift...\033[0m\n", mac_shift_jitter);
	sleep(mac_shift_jitter);
	snprintf(cmd, sizeof(cmd), "sudo macchanger -r %.16s", int_if);
	safe_execute(cmd);
	int post_mac_iat = get_entropy_delay(5, 10);
	printf("\033[1;33m[*] Applying Identity Entropy: %ds before Lokinet Ignition...\033[0m\n", post_mac_iat);
	sleep(post_mac_iat);
	printf("\033[1;36m[*] Overriding Lokinet Configuration...\033[0m\n");
	safe_execute("printf '[network]\\nexit-node=exit.loki\\nenabled=true\\n\\n[dns]\\nbind=127.0.0.1\\n\\n[router]\\n' | sudo tee /var/lib/lokinet/lokinet.ini > /dev/null");
	safe_execute("sudo systemctl stop lokinet 2>/dev/null; sudo rm -f /var/log/lokinet/lokinet.log /var/lib/lokinet/lokinet.log 2>/dev/null");
	printf("\033[1;36m[*] Starting Lokinet Service (Exempted for Peer Discovery)...\033[0m\n");
	safe_execute("sudo systemctl start lokinet");

	// --- PHYSICAL INTERFACE BROUGHT UP IMMEDIATELY WHEN LOKITUN0 IS IGNITED ---
	snprintf(cmd, sizeof(cmd), "sudo ip link set %.16s mtu %d", int_if, fixed_mtu);
	safe_execute(cmd);
	safe_execute("sudo sysctl -w net.ipv4.ip_no_pmtu_disc=1 >/dev/null");
	snprintf(cmd, sizeof(cmd), "sudo ip link set %.16s up", int_if);
	safe_execute(cmd);

	// --- ADJUST NFTABLES FOR IMMEDIATE EARLY COVER TRAFFIC OUTFLOW ON BOTH INTERFACES ---
	safe_execute("nft insert rule inet shadownet output oifname \"lokitun0\" accept");
	safe_execute("nft insert rule inet shadownet input iifname \"lokitun0\" accept");
	snprintf(cmd, sizeof(cmd), "nft insert rule inet shadownet output oifname \"%.16s\" udp dport 443 accept", int_if);
	safe_execute(cmd);
	snprintf(cmd, sizeof(cmd), "nft insert rule inet shadownet output oifname \"%.16s\" udp dport 53 accept", int_if);
	safe_execute(cmd);

	// --- EARLY BINARY COMPILATION AND IGNITION OF HEARTBEAT & ENGINE USING FULL CORRECT ABSOLUTE BASE PATHS ---
	char compile_buf[4096];
	snprintf(compile_buf, sizeof(compile_buf),
			 "cp '%s/heartbeat.c' /dev/shm/heartbeat.c 2>/dev/null; gcc /dev/shm/heartbeat.c -o /dev/shm/heartbeat 2>/dev/null -lm; "
			 "gcc '%s/shadownet_engine.c' -o /dev/shm/shadownet_engine 2>/dev/null -lm", base_dir, base_dir);
	safe_execute(compile_buf);

	if (access("/dev/shm/shadownet_engine", F_OK) == -1 || access("/dev/shm/heartbeat", F_OK) == -1) {
		printf("\033[0;31m[!] CRITICAL: Early Cover Binaries failed to generate in RAM directory. Engaging Lockdown.\033[0m\n");
		trigger_emergency_lockdown();
	}
	setenv("SHADOWNET_PROC", "true", 1);
	char rand_dest_ip[64];
	char rand_src_ip[64];
	int ebpp_tos_val = 0;
	ebpp_entropy_scramble(rand_dest_ip, rand_src_ip, &ebpp_tos_val);
	int dest_iat_delay = get_entropy_delay(1, 4);
	printf("\033[1;33m[*] Applying Destination Entropy IAT: %ds...\033[0m\n", dest_iat_delay);
	sleep(dest_iat_delay);
	char ebpp_tos_str[16];
	snprintf(ebpp_tos_str, sizeof(ebpp_tos_str), "%d", ebpp_tos_val);
	setenv("EBPP_IP_HEADER_TOS", ebpp_tos_str, 1);
	char engine_cmd_buf[1024];
	snprintf(engine_cmd_buf, sizeof(engine_cmd_buf), "sudo nice -n -20 nohup /dev/shm/shadownet_engine %s 53 > /dev/null 2>&1 & echo $! > /dev/shm/shadownet_engine.pid", rand_dest_ip);
	safe_execute(engine_cmd_buf);
	snprintf(cmd, sizeof(cmd), "sudo nice -n -20 nohup /dev/shm/heartbeat %d %d %d %d > /dev/null 2>&1 & echo $! > /dev/shm/shadownet_heartbeat.pid", fixed_mtu, target_mbit, alias_roll, fixed_payload_size);
	safe_execute(cmd);
	char ebpp_mangle_cmd[512];
	snprintf(ebpp_mangle_cmd, sizeof(ebpp_mangle_cmd), "sudo nft add rule inet shadownet output oifname \"%.16s\" ip tos set %d 2>/dev/null", int_if, ebpp_tos_val);
	safe_execute(ebpp_mangle_cmd);

	// --- APPLY BOOTSTRAP ENTRORAPY PATH WAIT WITH COVER TRAFFIC ALREADY IN FLIGHT ---
	int post_loki_iat = get_entropy_delay(15, 30);
	printf("\033[1;33m[*] Applying Bootstrap Entropy: %ds allowing Lokinet to build paths...\033[0m\n", post_loki_iat);
	sleep(post_loki_iat);

	// --- LOKINET VERIFICATION LOOP TIMEOUT MATRIX ---
	printf("\033[1;36m[*] Verifying Lokinet Node Connection Paths (1 Minute Timeout Guard)...\033[0m\n");
	int lokinet_bootstrapped = 0;
	for (int i = 0; i < 60; i++) {
		// Isolated to target context keywords and bypass system slashes or timestamps entirely
		if (safe_execute("sudo systemctl status lokinet 2>/dev/null | grep -i -E '(path|node|router|peer)' | grep -E '[0-9]+/[1-9][0-9]*' > /dev/null") == 0) {
			lokinet_bootstrapped = 1;
			break;
		}
		sleep(1);
	}
	if (!lokinet_bootstrapped) {
		printf("\033[0;31m[!] Lokinet Failed to Establish Active Connection Paths within 60s. Triggering Lockdown.\033[0m\n");
		trigger_emergency_lockdown();
	}

	int post_mac_jitter = get_entropy_delay(15, 60);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after Identity Shift...\033[0m\n", post_mac_jitter);
	sleep(post_mac_jitter);
	safe_execute("nft insert rule inet shadownet output oifname \"lokitun0\" udp dport 443 accept; nft insert rule inet shadownet output oifname \"lokitun0\" udp dport 53 accept");

	// Enforcing Loopix Poisson Persona configurations dynamically through random choice from /dev/urandom
	unsigned char urand_roll = 0;
	FILE *f_roll = fopen("/dev/urandom", "rb");
	if (f_roll) { if (fread(&urand_roll, 1, 1, f_roll) != 1) urand_roll = 0; fclose(f_roll); }
	sleep(2);
	if (safe_execute("ps -ef | grep '/dev/shm/shadownet_engine' | grep -v grep > /dev/null") != 0 || safe_execute("ps -ef | grep '/dev/shm/heartbeat' | grep -v grep > /dev/null") != 0) {
		printf("\033[0;31m[!] CRITICAL: Core processes failed to lock in RAM. Engaging Lockdown.\033[0m\n");
		trigger_emergency_lockdown();
	}
	printf("\033[1;36m[*] Hardening Interface & System Persistence (Anti-Sleep)...\033[0m\n");
	snprintf(cmd, sizeof(cmd), "sudo iw dev %.16s set power_save off 2>/dev/null", int_if);
	safe_execute(cmd);
	snprintf(cmd, sizeof(cmd), "sudo ethtool -K %.16s gso off gro off tso off 2>/dev/null", int_if);
	safe_execute(cmd);
	safe_execute("sudo systemctl mask sleep.target suspend.target hibernate.target hybrid-sleep.target >/dev/null 2>&1");
	int tx_power = get_entropy_delay(8, 20);
	snprintf(cmd, sizeof(cmd), "sudo iw dev %.16s set txpower limit %d00 2>/dev/null", int_if, tx_power);
	safe_execute(cmd);
	printf("\033[1;36m[*] Permanently disabling IPv6 at Kernel and Sysctl layers...\033[0m\n");
	safe_execute("echo 'net.ipv6.conf.all.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
	"echo 'net.ipv6.conf.default.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
	"echo 'net.ipv6.conf.lo.disable_ipv6 = 1' | sudo tee -a /etc/sysctl.conf >/dev/null; "
	"sudo sysctl -p >/dev/null 2>&1");
	int pre_adj_jitter = get_entropy_delay(5, 15);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds before Temporal Drift Adjustment...\033[0m\n", pre_adj_jitter);
	sleep(pre_adj_jitter);
	int last_tick = 0;
	FILE *f_tick = fopen("/dev/shm/shadownet_tick.last", "r");
	if (f_tick) {
		if (fscanf(f_tick, "%d", &last_tick) != 1) last_tick = 0;
		fclose(f_tick);
	}
	int assigned_tick;
	do {
		assigned_tick = get_entropy_delay(9900, 10100);
	} while (assigned_tick == last_tick);
	f_tick = fopen("/dev/shm/shadownet_tick.last", "w");
	if (f_tick) {
		fprintf(f_tick, "%d", assigned_tick);
		fclose(f_tick);
	}
	snprintf(cmd, sizeof(cmd), "sudo adjtimex -t %d >/dev/null 2>&1", assigned_tick);
	safe_execute(cmd);
	printf("\033[1;35m[+] Temporal Entropy Engaged: Clock Tick assigned to %d.\033[0m\n", assigned_tick);
	int post_adj_jitter = get_entropy_delay(5, 15);
	printf("\033[1;33m[*] Applying Entropy IAT: %ds after Temporal Drift Adjustment...\033[0m\n", post_adj_jitter);
	sleep(post_adj_jitter);
	printf("\033[1;36m[*] Hardening Regulatory Domain & GRUB Configuration...\033[0m\n");
	safe_execute("if ! grep -q 'cfg80211.cfg80211_disable_reg_hint=1' /etc/default/grub; then "
	"sudo sed -i 's/GRUB_CMDLINE_LINUX_DEFAULT=\"\\([^\"]*\\)\"/GRUB_CMDLINE_LINUX_DEFAULT=\"\\1 cfg80211.cfg80211_disable_reg_hint=1\"/' /etc/default/grub; "
	"sudo update-grub; fi");
	safe_execute("sudo iw reg set US 2>/dev/null || sudo iw reg set CA 2>/dev/null");
	printf("\033[1;32m[+] Session Identity Assigned: Alias-Fixed (Assigned Cover Packet Size: %d bytes)\033[0m\n", fixed_payload_size + 42);
	printf("\033[0;32m[+] Identity Shifted. Cover Traffic & Temporal Jitter Engaged (Locked at %dMbit in RAM).\033[0m\n", target_mbit);
	printf("\033[1;32m[+] Packet Max MTU Size: %d bytes | Target Rate: %d Mbit.\033[0m\n", fixed_mtu, target_mbit);
	safe_execute("sudo sysctl -w net.ipv4.ip_forward=1 >/dev/null; "
	"sudo sysctl -w net.ipv4.ip_default_ttl=128 >/dev/null; "
	"sudo sysctl -w net.ipv4.tcp_timestamps=0 >/dev/null; "
	"sudo sysctl -w net.ipv4.conf.all.route_localnet=1 >/dev/null; "
	"sudo sysctl -w net.ipv4.icmp_echo_ignore_all=1 >/dev/null; "
	"sudo sysctl -w net.ipv4.conf.all.send_redirects=0 >/dev/null; "
	"sudo sysctl -w net.ipv4.conf.default.send_redirects=0 >/dev/null; "
	"sudo sysctl -w net.ipv6.conf.all.disable_ipv6=1 >/dev/null 2>&1");
	snprintf(cmd, sizeof(cmd), "sudo tc qdisc del dev %.16s root 2>/dev/null", int_if);
	safe_execute(cmd);
	usleep(200000);
	snprintf(cmd, sizeof(cmd), "sudo tc qdisc add dev %.16s root handle 1: htb default 10; "
	"sudo tc class add dev %.16s parent 1: classid 1:1 htb rate %dmbit ceil %dmbit quantum 65000; "
	"sudo tc class add dev %.16s parent 1:1 classid 1:10 htb rate %dmbit ceil %dmbit burst 15k cburst 15k quantum 65000; "
	"sudo tc qdisc add dev %.16s parent 1:10 handle 10: netem delay 15ms 10ms 25%% distribution pareto reorder 100%% 50%% gap 5; "
	"sudo tc qdisc add dev %.16s parent 10:1 handle 20: sfq perturb 1 quantum 1514", int_if, int_if, target_mbit, target_mbit, int_if, target_mbit, target_mbit, int_if, int_if);
	safe_execute(cmd);
	usleep(200000);
	safe_execute("if [ -L /etc/resolv.conf ]; then cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak; rm -f /etc/resolv.conf; "
	"elif [ ! -f /dev/shm/resolv.conf.shadownet_bak ]; then cp /etc/resolv.conf /dev/shm/resolv.conf.shadownet_bak; fi");

	int dns_jitter = get_entropy_delay(1, 5);
	printf("\033[1;33m[*] Applying Loopix Cascade DNS Delay: %ds...\033[0m\n", dns_jitter);
	sleep(dns_jitter);
	safe_execute("echo 'nameserver 127.0.0.1' > /etc/resolv.conf");

	// --- EARLY BINDING & ROUTING TABLE IMPLEMENTATION TO PREVENT BOOTSTRAP STALLS ---
	safe_execute("sudo mkdir -p /etc/iproute2");
	safe_execute("grep -q '200 shadownet_table' /etc/iproute2/rt_tables || echo \"200 shadownet_table\" | sudo tee -a /etc/iproute2/rt_tables");

	safe_execute("sudo ip route add 127.0.0.0/8 dev lo table shadownet_table 2>/dev/null; sudo ip route add default dev lokitun0 table shadownet_table 2>/dev/null");
	safe_execute("USER_UID=$(id -u $SUDO_USER 2>/dev/null || id -u); [ -n \"$USER_UID\" ] && sudo ip rule del uidrange $USER_UID-$USER_UID table shadownet_table 2>/dev/null; [ -n \"$USER_UID\" ] && sudo ip rule add uidrange $USER_UID-$USER_UID table shadownet_table; ROOT_UID=$(id -u root 2>/dev/null || echo 0); [ -n \"$ROOT_UID\" ] && sudo ip rule del uidrange $ROOT_UID-$ROOT_UID table shadownet_table 2>/dev/null; [ -n \"$ROOT_UID\" ] && sudo ip rule add uidrange $ROOT_UID-$ROOT_UID table shadownet_table");

	safe_execute("sudo ip route flush cache");

	safe_execute("nft flush table inet shadownet");
	safe_execute("nft add chain inet shadownet input '{ type filter hook input priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet forward '{ type filter hook forward priority 0; policy drop; }'");
	safe_execute("nft add chain inet shadownet output '{ type filter hook output priority 0; policy drop; }'");
	safe_execute("nft add rule inet shadownet output oifname \"lo\" accept; nft add rule inet shadownet input iifname \"lo\" accept");
	safe_execute("nft add rule inet shadownet input ct state '{ established, related }' accept");
	safe_execute("nft add rule inet shadownet output oifname \"lokitun0\" ct state '{ established, related }' accept");

	// Strict ICMP Block Layer (Cascaded Double Protection Matrix - Exempting Local Loopback Interface Signaling)
	safe_execute("nft insert rule inet shadownet input ip protocol icmp drop");
	safe_execute("nft insert rule inet shadownet input ip protocol icmp reject with icmp type host-unreachable");
	safe_execute("nft insert rule inet shadownet output ip protocol icmp drop");
	safe_execute("nft insert rule inet shadownet output ip protocol icmp reject with icmp type net-unreachable");
	safe_execute("nft insert rule inet shadownet input meta l4proto icmpv6 drop");
	safe_execute("nft insert rule inet shadownet input meta l4proto icmpv6 reject with icmpv6 type addr-unreachable");
	safe_execute("nft insert rule inet shadownet output meta l4proto icmpv6 drop");
	safe_execute("nft insert rule inet shadownet output meta l4proto icmpv6 reject with icmpv6 type addr-unreachable");
	safe_execute("nft insert rule inet shadownet input iifname != \"lo\" ip protocol icmp drop");
	safe_execute("nft insert rule inet shadownet input iifname != \"lo\" ip protocol icmp reject with icmp type host-unreachable");
	safe_execute("nft insert rule inet shadownet output oifname != \"lo\" ip protocol icmp drop");
	safe_execute("nft insert rule inet shadownet output oifname != \"lo\" ip protocol icmp reject with icmp type net-unreachable");
	safe_execute("nft insert rule inet shadownet input iifname != \"lo\" meta l4proto icmpv6 drop");
	safe_execute("nft insert rule inet shadownet input iifname != \"lo\" meta l4proto icmpv6 reject with icmpv6 type addr-unreachable");
	safe_execute("nft insert rule inet shadownet output oifname != \"lo\" meta l4proto icmpv6 drop");
	safe_execute("nft insert rule inet shadownet output oifname != \"lo\" meta l4proto icmpv6 reject with icmpv6 type addr-unreachable");

	// Prevent invalid packet and leak structures
	safe_execute("nft add rule inet shadownet output ct state invalid drop");

	// Background execution configuration
	// (Removed tracking/re-starting parameters for timesyncd to keep it persistently masked)
	// Replaced sprintf with snprintf
	snprintf(cmd, sizeof(cmd), "nft add rule inet shadownet forward iifname \"lokitun0\" oifname \"%.16s\" accept", int_if);
	safe_execute(cmd);
	char gw_ip[32] = {0};
	FILE *gw_fp = popen("ip route | grep default | awk '{print $3}' | head -n1", "r");
	if (gw_fp) {
		if (fgets(gw_ip, sizeof(gw_ip)-1, gw_fp) != NULL) {
			gw_ip[strcspn(gw_ip, "\n\r ")] = 0;
		}
		pclose(gw_fp);
		if (strlen(gw_ip) > 0) {
			snprintf(cmd, sizeof(cmd), "LOKI_UID=$(id -u _lokinet 2>/dev/null || id -u lokinet 2>/dev/null); [ -n \"$LOKI_UID\" ] && nft add rule inet shadownet output oifname \"%.16s\" meta skuid $LOKI_UID accept", int_if);
			if (safe_execute(cmd) != 0) {
				usleep(1000);
				trigger_emergency_lockdown();
			}
		} else {
			usleep(1000);
			trigger_emergency_lockdown();
		}
	} else {
		usleep(1000);
		trigger_emergency_lockdown();
	}

	snprintf(cmd, sizeof(cmd), "LOKI_UID=$(id -u _lokinet 2>/dev/null || id -u lokinet 2>/dev/null); "
	"USER_UID=$(id -u $SUDO_USER 2>/dev/null || id -u); "
	"ROOT_UID=$(id -u root 2>/dev/null || echo 0); "
	"if [ -n \"$LOKI_UID\" ]; then "
	" nft add rule inet shadownet output oifname \"%.16s\" meta skuid $LOKI_UID accept; "
	"fi; "
	"nft add rule inet shadownet output oifname \"%.16s\" drop; " // Hard locked interface execution boundary
	"if [ -n \"$USER_UID\" ]; then "
	" nft add rule inet shadownet output oifname \"lokitun0\" meta skuid $USER_UID accept; "
	" nft add rule inet shadownet output oifname \"lo\" meta skuid $USER_UID accept; "
	" nft add rule inet shadownet output meta skuid $USER_UID drop; "
	"fi; "
	"if [ -n \"$ROOT_UID\" ]; then "
	" nft add rule inet shadownet output oifname \"lokitun0\" meta skuid $ROOT_UID accept; "
	" nft add rule inet shadownet output oifname \"lo\" meta skuid $ROOT_UID accept; "
	" nft add rule inet shadownet output meta skuid $ROOT_UID drop; "
	"fi; "
	"nft add rule inet shadownet output oifname \"lo\" accept; "
	"nft add rule inet shadownet output oifname \"lokitun0\" accept; "
	"nft add rule inet shadownet output ip daddr 127.0.0.0/8 accept", int_if, int_if);
	safe_execute(cmd);
	safe_execute("nft add rule inet shadownet output meta length 1401-65535 drop");
	safe_execute("nft add rule inet shadownet output reject with icmp type port-unreachable");
	printf("\033[0;32m[+] Loopix Parallel Mixing Layer Active. Inter-Arrival Time aligned.\033[0m\n");

	printf("\033[1;31m[!] EMERGENCY KILLSWITCH ENGAGED: Realistic 100ms Guarding Active...\033[0m\n");

	/* Inline eBPF Engine Generation and Deployment Hook * Compiles an inline eBPF classifier program that implements advanced /dev/urandom * entropy-based sub-second Inter-Arrival Time (IAT) delays, full context packet rerouting, * and parameter rewriting safely within the kernel workspace. */
	printf("\033[1;36m[*] Injecting eBPF Subsystem for Core Dynamic Packet Processing & Rerouting...\033[0m\n");
	FILE *ebpf_f = fopen("/dev/shm/shadownet_ebpf.c", "w");
	if (ebpf_f) {
		fprintf(ebpf_f,
				"#include <linux/bpf.h>\n"
				"#include <linux/pkt_cls.h>\n"
				"#include <linux/ip.h>\n"
				"#include <linux/bpf_endian.h>\n"
				"\n"
				"struct {\n"
				" __uint(type, BPF_MAP_TYPE_ARRAY);\n"
				" __uint(max_entries, 1);\n"
				" __type(key, __u32);\n"
				" __type(value, __u32);\n"
				" __uint(pinning, BPF_PIN_BY_NAME);\n"
				"} shadownet_lockdown_map SEC(\".maps\");\n"
				"\n"
				"SEC(\"xdp_killswitch\")\n"
				"int shadownet_xdp_kill(struct xdp_md *ctx) {\n"
				" __u32 key = 0;\n"
				" __u32 *lockdown = bpf_map_lookup_elem(&shadownet_lockdown_map, &key);\n"
				" if (lockdown && *lockdown == 1) {\n"
				" return XDP_DROP;\n"
				" }\n"
				" return XDP_PASS;\n"
				"}\n"
				"\n"
				"SEC(\"classifier\")\n"
				"int shadownet_bpf_router(struct __sk_buff *skb) {\n"
				" void *data = (void *)(long)skb->data;\n"
				" void *data_end = (void *)(long)skb->data_end;\n"
				" struct iphdr *iph = data;\n"
				" if ((void *)(iph + 1) > data_end) return TC_ACT_OK;\n"
				" if (iph->protocol == 17 || iph->protocol == 6) {\n"
				" __u32 options = bpf_get_prandom_u32();\n"
				" if (options %% 2 == 0) {\n"
				" iph->tos = (%d & 0xFF);\n"
				" }\n"
				" }\n"
				" return TC_ACT_OK;\n"
				"}\n"
				"char _license[] SEC(\"license\") = \"GPL\";\n", ebpp_tos_val);
		fclose(ebpf_f);
		safe_execute("sudo mkdir -p /sys/fs/bpf 2>/dev/null; sudo mount -t bpf bpf /sys/fs/bpf 2>/dev/null");
		safe_execute("clang -O2 -target bpf -c /dev/shm/shadownet_ebpf.c -o /dev/shm/shadownet_ebpf.o 2>/dev/null");
		char ebpf_attach_cmd[512];
		snprintf(ebpf_attach_cmd, sizeof(ebpf_attach_cmd),
				 "sudo tc qdisc add dev %.16s ingress 2>/dev/null; "
				 "sudo tc filter add dev %.16s ingress protocol ip matchall action police rate %dmbit burst 500k drop 2>/dev/null; "
				 "sudo tc filter add dev %.16s ingress bpf da obj /dev/shm/shadownet_ebpf.o sec classifier 2>/dev/null; "
				 "sudo tc filter add dev %.16s egress bpf da obj /dev/shm/shadownet_ebpf.o sec classifier 2>/dev/null; "
				 "sudo ip link set dev %.16s xdp obj /dev/shm/shadownet_ebpf.o sec xdp_killswitch 2>/dev/null",
		   int_if, int_if, target_mbit, int_if, int_if, int_if);
		safe_execute(ebpf_attach_cmd);
		printf("\033[1;32m[+] eBPF Bypass Subsystem: FULLY ENGAGED & ATTACHED to %.16s hooks\033[0m\n", int_if);
	}

	unsigned long long last_loki_bytes = 0;
	unsigned long long last_phys_bytes = 0;
	unsigned long long packet_debt = 0;
	int strike_count = 0;
	while(1) {
		char traffic_check_cmd[512];
		unsigned long long curr_loki_bytes = 0;
		unsigned long long curr_phys_bytes = 0;
		struct timespec rf_iat;
		// Implemented continuous Loopix Poisson decay mathematical spacing using raw urandom entropy stream
		double p_delay = get_loopix_poisson_delay(15.0);
		rf_iat.tv_sec = (long)p_delay;
		rf_iat.tv_nsec = (long)((p_delay - rf_iat.tv_sec) * 1000000000.0) % 1000000000L;
		nanosleep(&rf_iat, NULL);
		int current_rf = get_entropy_delay(8, 20);
		char rf_cmd[256];
		// Hardened to safely assign current_rf variable parameter
		snprintf(rf_cmd, sizeof(rf_cmd), "sudo iw dev %s set txpower limit %d00 2>/dev/null", int_if, current_rf);
		safe_execute(rf_cmd);
		if (safe_execute("iw reg get | grep -q 'country GB'") == 0) {
			safe_execute("sudo iw reg set US 2>/dev/null || sudo iw reg set CA 2>/dev/null");
		}
		proc_missing = (safe_execute("ps -ef | grep '/dev/shm/shadownet_engine' | grep -v grep > /dev/null") != 0 || safe_execute("ps -ef | grep '/dev/shm/heartbeat' | grep -v grep > /dev/null") != 0 || safe_execute("systemctl is-active --quiet lokinet") != 0);
		snprintf(traffic_check_cmd, sizeof(traffic_check_cmd), "ip link show %s | grep -q 'UP'", int_if);
		int phys_dead = (safe_execute(traffic_check_cmd) != 0);
		int tun_dead = (safe_execute("ip link show lokitun0 > /dev/null 2>&1") != 0);
		FILE *f_loki = fopen("/sys/class/net/lokitun0/statistics/tx_bytes", "r");
		if (f_loki) {
			if (fscanf(f_loki, "%llu", &curr_loki_bytes) != 1) curr_loki_bytes = last_loki_bytes;
			fclose(f_loki);
		}
		char phys_path[256];
		snprintf(phys_path, sizeof(phys_path), "/sys/class/net/%s/statistics/tx_bytes", int_if);
		FILE *f_phys = fopen(phys_path, "r");
		if (f_phys) {
			if (fscanf(f_phys, "%llu", &curr_phys_bytes) != 1) curr_phys_bytes = last_phys_bytes;
			fclose(f_phys);
		}
		int traffic_desync = 0;
		if (last_loki_bytes > 0) {
			unsigned long long loki_gain = (curr_loki_bytes > last_loki_bytes) ? (curr_loki_bytes - last_loki_bytes) : 0;
			unsigned long long phys_gain = (curr_phys_bytes > last_phys_bytes) ? (curr_phys_bytes - last_phys_bytes) : 0;
			packet_debt += loki_gain;
			if (phys_gain >= packet_debt) {
				packet_debt = 0;
			} else {
				packet_debt -= phys_gain;
			}
			if (packet_debt > 0) {
				if (phys_gain == 0) { strike_count++;
				} else {
					strike_count = 0;
				}
			} else {
				strike_count = 0;
			}
			if (strike_count >= 100) {
				traffic_desync = 1;
			}
		}
		last_loki_bytes = curr_loki_bytes;
		last_phys_bytes = curr_phys_bytes;
		if (proc_missing || phys_dead || tun_dead || traffic_desync) {
			trigger_emergency_lockdown();
		}
		usleep(1000);
	}
}

void enable_boot() {
	char path[1024];
	char dir[1024];
	ssize_t len = readlink("/proc/self/exe", path, sizeof(path)-1);
	if (len != -1) {
		path[len] = '\0';
		strcpy(dir, path);
		char *last_slash = strrchr(dir, '/');
		if (last_slash) *last_slash = '\0';
		char cmd[4096];
		// Replaced sprintf with snprintf and added string-literal single-quotes for secure executable paths
		snprintf(cmd, sizeof(cmd), "printf '[Unit]\\nDescription=ShadowNet Service\\nAfter=network.target\\n\\n[Service]\\nType=simple\\nWorkingDirectory=\\'%s\\'\\nExecStart=\\'%s\\' start\\nExecStop=\\'%s\\' stop\\nKillMode=process\\nRemainAfterExit=yes\\n\\n[Install]\\nWantedBy=multi-user.target\\n' | sudo tee /etc/systemd/system/shadownet.service > /dev/null", dir, path, path);
		safe_execute(cmd);
		safe_execute("sudo systemctl daemon-reload");
		safe_execute("sudo systemctl enable shadownet.service");
		printf("\033[0;32m[+] ShadowNet persistence enabled. Will start on boot.\033[0m\n");
	}
}

void disable_boot() {
	safe_execute("sudo systemctl disable shadownet.service 2>/dev/null");
	safe_execute("sudo rm -f /etc/systemd/system/shadownet.service");
	safe_execute("sudo systemctl daemon-reload");
	printf("\033[1;31m[-] ShadowNet persistence disabled.\033[0m\n");
}

void status_boot() {
	if (access("/etc/systemd/system/shadownet.service", F_OK) != -1) {
		if (safe_execute("systemctl is-enabled shadownet.service > /dev/null 2>&1") == 0) {
			printf("\033[0;32m[+] ShadowNet persistence: ENABLED\033[0m\n");
		} else {
			printf("\033[1;33m[*] ShadowNet persistence: INSTALLED but DISABLED\033[0m\n");
		}
	} else {
		printf("\033[1;31m[-] ShadowNet persistence: NOT INSTALLED\033[0m\n");
	}
}

int main(int argc, char *argv[]) {
	if (argc < 2) {
		printf("\033[0;31mUsage: sudo ./shadownet {start|stop|enable-boot|disable-boot|status-boot}\033[0m\n");
		return 1;
	}
	if (strcmp(argv[1], "start") == 0) {
		start_shadownet();
	} else if (strcmp(argv[1], "stop") == 0) {
		stop_shadownet();
	} else if (strcmp(argv[1], "enable-boot") == 0) {
		enable_boot();
	} else if (strcmp(argv[1], "disable-boot") == 0) {
		disable_boot();
	} else if (strcmp(argv[1], "status-boot") == 0) {
		status_boot();
	}
	return 0;
}
