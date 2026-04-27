#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <fcntl.h>

unsigned short csum(unsigned short *ptr, int nbytes) {
	long sum;
	unsigned short oddbyte;
	short answer;
	sum = 0;
	while(nbytes > 1) {
		sum += *ptr++;
		nbytes -= 2;
	}
	if(nbytes == 1) {
		oddbyte = 0;
		*((u_char*)&oddbyte) = *(u_char*)ptr;
		sum += oddbyte;
	}
	sum = (sum >> 16) + (sum & 0xffff);
	sum += (sum >> 16);
	answer = (short)~sum;
	return answer;
}

double get_entropy_jitter() {
	unsigned char rand_byte;
	FILE *f = fopen("/dev/urandom", "rb");
	fread(&rand_byte, 1, 1, f);
	fclose(f);
	return ((double)rand_byte / 255.0) * 0.040 + 0.010;
}

double get_dns_iat() {
	unsigned int rand_val;
	FILE *f = fopen("/dev/urandom", "rb");
	fread(&rand_val, sizeof(rand_val), 1, f);
	fclose(f);
	return 0.5 + ((double)(rand_val % 2500) / 1000.0);
}

int main(int argc, char *argv[]) {
	// This is the ceiling MTU from shadownet.c
	int max_mtu = (argc > 1) ? atoi(argv[1]) : 1100;
	const char *destinations[] = {"76.76.2.2", "76.76.10.2", "182.222.222.222", "45.11.45.11", "84.200.69.80", "84.200.70.40"};
	const char *fake_domains[] = {"google.com", "bing.com", "duckduckgo.com", "protonmail.com", "github.com"};
	int num_dests = 6;
	int num_domains = 5;

	srand(time(NULL));

	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(sock < 0) exit(1);
	int one = 1;
	setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one));

	char packet[4096];
	struct iphdr *iph = (struct iphdr *) packet;
	struct udphdr *udph = (struct udphdr *) (packet + sizeof(struct iphdr));

	struct sockaddr_in sin;
	sin.sin_family = AF_INET;

	struct timespec req, rem;
	time_t last_dns_time = time(NULL);

	while(1) {
		time_t curr_time = time(NULL);
		int dest_idx = rand() % num_dests;
		sin.sin_addr.s_addr = inet_addr(destinations[dest_idx]);

		// 1. DNS Entropy Logic
		if(difftime(curr_time, last_dns_time) > get_dns_iat()) {
			memset(packet, 0, 4096);
			iph->ihl = 5; iph->version = 4; iph->tos = 0;
			iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + 32;
			iph->id = htons(rand() % 65535); iph->frag_off = 0; iph->ttl = 128;
			iph->protocol = IPPROTO_UDP; iph->daddr = sin.sin_addr.s_addr;
			iph->check = csum((unsigned short *) packet, iph->tot_len);
			udph->source = htons(49152 + (rand() % 16383));
			udph->dest = htons(53); udph->len = htons(sizeof(struct udphdr) + 32);
			char *dns_data = packet + sizeof(struct iphdr) + sizeof(struct udphdr);
			dns_data[0] = rand() % 255; dns_data[1] = rand() % 255; dns_data[2] = 0x01;
			strcpy(dns_data + 12, fake_domains[rand() % num_domains]);
			sendto(sock, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin));
			last_dns_time = curr_time;
		}

		// 2. Heartbeat with PER-PACKET JITTER Logic
		int burst_size = 10 + (rand() % 13);
		for(int b = 0; b < burst_size; b++) {
			// Updated range ensures even at max jitter, we stay under the 1100 ceiling
			int jittered_payload_size = (rand() % (1100 - 500 + 1)) + 500 - 42;
			if (jittered_payload_size < 64) jittered_payload_size = 64;

			memset(packet, 0, 4096);
			iph->ihl = 5; iph->version = 4; iph->tos = 0;
			iph->tot_len = sizeof(struct iphdr) + sizeof(struct udphdr) + jittered_payload_size;
			iph->id = htons(rand() % 65535); iph->frag_off = 0; iph->ttl = 128;
			iph->protocol = IPPROTO_UDP;
			iph->daddr = sin.sin_addr.s_addr;
			iph->check = csum((unsigned short *) packet, iph->tot_len);

			udph->source = htons(443);
			udph->dest = htons(443);
			udph->len = htons(sizeof(struct udphdr) + jittered_payload_size);
			udph->check = 0;

			struct timespec micro_req;
			micro_req.tv_sec = 0;
			micro_req.tv_nsec = (rand() % 30000);
			nanosleep(&micro_req, NULL);

			sendto(sock, packet, iph->tot_len, 0, (struct sockaddr *)&sin, sizeof(sin));
		}

		double jitter = get_entropy_jitter();
		req.tv_sec = 0;
		req.tv_nsec = (long)(jitter * 1000000000.0);
		nanosleep(&req, &rem);
	}
	return 0;
}
