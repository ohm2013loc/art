/* artmonitor.c */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#define OpPoll 0x2000
#define OpPollReply 0x2100

#define VERSION_HI 0
#define VERSION_LO 14

#define ARTNET_PORT "6454"

#define TRUE 1
#define FALSE 0
typedef int bool;

struct pkt_ArtPoll {
     char ID[8];
     unsigned short OpCode;
     unsigned char ProtVerHi;
     unsigned char ProtVerLo;
     unsigned char TalkToMe;
     unsigned char Priority;
} __attribute__((packed));

struct pkt_ArtPollReply {
     char ID[8];
     unsigned short OpCode;
     unsigned char IPAddress[4];
     unsigned short Port;
     unsigned char VersInfoH;
     unsigned char VersInfo;
     unsigned char NetSwitch;
     unsigned char SubSwitch;
     unsigned char OemHi;
     unsigned char Oem;
     unsigned char UbeaVersion;
     unsigned char Status1;
     unsigned char EstaManLo;
     unsigned char EstaManHi;
     unsigned char ShortName[18];
     unsigned char LongName[64];
     unsigned char NodeReport[64];
     unsigned char NumPortsHi;
     unsigned char NumPortsLo;
     unsigned char PortTypes[4];
     unsigned char GoodInput[4];
     unsigned char GoodOutput[4];
     unsigned char SwIn[4];
     unsigned char SwOut[4];
     unsigned char SwVideo;
     unsigned char SwMacro;
     unsigned char SwRemote;
     unsigned char Spare1;
     unsigned char Spare2;
     unsigned char Spare3;
     unsigned char Style;
     unsigned char MAC[6];
} __attribute__((packed));

struct device_entry {
	bool present;
	unsigned char ip[4];
};

struct device_entry device_table[256];
struct device_entry prev_table[256];
unsigned char arduino_mac[6] = {0xcc, 0xb5, 0x5a, 0x00, 0x00, 0x00};

int art_socket;

FILE *f;

void setup_socket(void)
{
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr;
	struct timeval timeout;

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	int yes = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
/*	hints.ai_flags = AI_PASSIVE; */

	/* getaddrinfo(NULL, ARTNET_PORT, &hints, &res); */
	getaddrinfo("255.255.255.255", ARTNET_PORT, &hints, &res);

	art_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
	if (art_socket == -1) {
		perror("socket");
		exit(1);
	}

	if (bind(art_socket, res->ai_addr, res->ai_addrlen) == -1) {
		perror("bind");
		exit(1);
	}

	if (setsockopt(art_socket,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int))
		== -1) {
	    perror("setsockopt");
	    exit(1);
	} 

	if (setsockopt(art_socket,SOL_SOCKET,SO_BROADCAST,&yes,sizeof(int))
		== -1) {
	    perror("setsockopt");
	    exit(1);
	} 

	if (setsockopt(art_socket,SOL_SOCKET,SO_RCVTIMEO,&timeout,
			sizeof(timeout)) == -1) {
	    perror("setsockopt");
	    exit(1);
	} 
}

void send_poll(char *broadcast_address)
{
	struct pkt_ArtPoll pkt;
	struct addrinfo hints, *dest;
	int i;
	int rv;
	int numbytes;

	strcpy(pkt.ID, "Art-Net");
	pkt.OpCode = OpPoll;
	pkt.ProtVerHi = VERSION_HI;
	pkt.ProtVerLo = VERSION_LO;
	pkt.TalkToMe = 0;
	pkt.Priority = 0xff;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(broadcast_address, ARTNET_PORT, &hints, &dest))
			!= 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}

	if ((numbytes = sendto(art_socket, &pkt, sizeof(pkt), 0,
             dest->ai_addr, dest->ai_addrlen)) == -1) {
		perror("sendto");
		exit(1);
	}
}

void wait_replies(void)
{
	size_t bytes;
	unsigned char buf[512];
	struct pkt_ArtPollReply *pkt = (struct pkt_ArtPollReply *)&buf;
	int i;

	while (1) {
		bytes = recv(art_socket, &buf, sizeof(buf), 0);
		if (bytes == -1) {
			if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
				break;
			perror("recv");
			exit(1);
		}
		if (bytes == 0)
			return;
		//printf("Received %d bytes\n", (int)bytes);
		if (strncmp(pkt->ID, "Art-Net", strlen("Art-Net")) != 0) {
			printf("Received invalid packet\n");
			continue;
		}
		if (pkt->OpCode != OpPollReply) {
			printf("Received packet with unexpected opcode\n");
			continue;
		}
		for (i = 0; i < 5; i++) {
			if (pkt->MAC[i] != arduino_mac[i])
				printf("WARNING: Non-Arduino MAC detected: "
					"%02x:%02x:%02x:%02x:%02x:%02x\n",
					pkt->MAC[0], pkt->MAC[1], pkt->MAC[2],
					pkt->MAC[3], pkt->MAC[4], pkt->MAC[5]);
		}
		if (device_table[pkt->MAC[5]].present) {
			printf("WARNING: Duplicate MAC byte %02x ignored\n",
			       	pkt->MAC[5]);
		} else {
			device_table[pkt->MAC[5]].present = TRUE;
			device_table[pkt->MAC[5]].ip[0] = pkt->IPAddress[0];
			device_table[pkt->MAC[5]].ip[1] = pkt->IPAddress[1];
			device_table[pkt->MAC[5]].ip[2] = pkt->IPAddress[2];
			device_table[pkt->MAC[5]].ip[3] = pkt->IPAddress[3];
		}
	}
}



void check_table(FILE *f) {
	unsigned int mac, ip0, ip1, ip2, ip3;
	int i, j;
	int total_changed = 0;
	int total_missing = 0;
	int total_new = 0;
	int total_same = 0;

	while (fscanf(f, "%x: %d.%d.%d.%d", &mac, &ip0, &ip1, &ip2, &ip3)
			== 5) {
		prev_table[mac].present = TRUE;
		prev_table[mac].ip[0] = ip0;
		prev_table[mac].ip[1] = ip1;
		prev_table[mac].ip[2] = ip2;
		prev_table[mac].ip[3] = ip3;
	}
	for (i = 0; i < 256; i++) {
	    if (device_table[i].present) {
	        if (prev_table[i].present) {
		    for(j = 0; j < 4; j++) {
			if (device_table[i].ip[j] != prev_table[i].ip[j]) {
			    total_changed++;
			    printf("Device %02x changed IP address from " 
				"%d.%d.%d.%d to %d.%d.%d.%d\n", i,
				prev_table[i].ip[0],
				prev_table[i].ip[1],
				prev_table[i].ip[2],
				prev_table[i].ip[3],
				device_table[i].ip[0],
				device_table[i].ip[1],
				device_table[i].ip[2],
				device_table[i].ip[3]);
			    break;
			}
		    }
		    total_same++;
	        } else {
		    total_new++;
		    printf("New device %02x on IP address %d.%d.%d.%d\n", i,
			device_table[i].ip[0],
			device_table[i].ip[1],
			device_table[i].ip[2],
			device_table[i].ip[3]);
		}
	    } else {
	        if (prev_table[i].present) {
		    total_missing++;
		    printf("Missing device %02x last seen on IP address "
			"%d.%d.%d.%d\n", i,
			prev_table[i].ip[0],
			prev_table[i].ip[1],
			prev_table[i].ip[2],
			prev_table[i].ip[3]);
		    device_table[i].present = TRUE;
		    for (j = 0; j < 4; j++)
			device_table[i].ip[j] = prev_table[i].ip[j];
		} else {
		    /* Nothing to do. Device never seen. */
		}
	    }
	}
	printf("%d new, %d missing, %d changed, %d unchanged, %d active\n",
	       total_new, total_missing, total_changed, total_same,
	       total_new + total_changed + total_same);
}

void dump_table(FILE *f) {
	unsigned int count = 0;
	int i;

	for (i = 0; i < 256; i++) {
		if (device_table[i].present) {
			fprintf(f, "%02x: %d.%d.%d.%d\n", i,
				device_table[i].ip[0],
				device_table[i].ip[1],
				device_table[i].ip[2],
				device_table[i].ip[3]);
			count++;
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: artpoll <IP address>\n");
		exit(1);
	}
	setup_socket();
	send_poll(argv[1]);
	wait_replies();
	f = fopen("field-devices", "r+");
	if (!f) {
		perror("fopen");
		exit(1);
	}
	check_table(f);
	rewind(f);
	dump_table(f);
	close(f);
	return 0;
}

