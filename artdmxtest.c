/* artdmxtest.c */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include <sys/time.h>

#define OpOutput 0x5000

#define VERSION_HI 0
#define VERSION_LO 14

#define ARTNET_PORT "6454"

#define TRUE 1
#define FALSE 0
typedef int bool;

unsigned char dmx_universe[512];

struct pkt_ArtOutput {
     char ID[8];
     unsigned short OpCode;
     unsigned char ProtVerHi;
     unsigned char ProtVerLo;
     unsigned char Sequence;
     unsigned char Physical;
     unsigned char SubUni;
     unsigned char Net;
     unsigned char LengthHi;
     unsigned char Length;
     unsigned char Data[512];
} __attribute__((packed));

#define DMXPKTLEN(channels) (channels+18)

int art_socket;

struct addrinfo *dmx_dest;

void setup_socket(char *universe_address)
{
	struct addrinfo hints, *res;
	struct sockaddr_in myaddr;
	struct timeval timeout;
	int rv;

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	int yes = 1;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

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

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(universe_address, ARTNET_PORT, &hints, &dmx_dest))
			!= 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		exit(1);
	}
}

void send_dmx(struct addrinfo *dest, char *data, int length)
{
	struct pkt_ArtOutput pkt;
	int i;
	int numbytes;
	static unsigned char dmx_sequence;

	strcpy(pkt.ID, "Art-Net");
	pkt.OpCode = OpOutput;
	pkt.ProtVerHi = VERSION_HI;
	pkt.ProtVerLo = VERSION_LO;
	pkt.Sequence = dmx_sequence++;
	if (dmx_sequence == 0)
		dmx_sequence++;
	pkt.Physical = 0;
	pkt.SubUni = 0;
	pkt.Net = 0;
	pkt.LengthHi = (length >> 8);
	pkt.Length = length & 0xff;
	memcpy(pkt.Data, data, length);

	if ((numbytes = sendto(art_socket, &pkt, DMXPKTLEN(length), 0,
             dest->ai_addr, dest->ai_addrlen)) == -1) {
		perror("sendto");
		exit(1);
	}
}

struct timeval delay_time;

void delay_setup(void)
{
	gettimeofday(&delay_time, NULL);
}

void frame_delay(unsigned int microseconds)
{
	struct timeval interval;
	struct timeval now;
	struct timespec int_ts;

	interval.tv_sec = microseconds / 1000000;
	interval.tv_usec = microseconds % 1000000;

	timeradd(&delay_time, &interval, &delay_time);

	gettimeofday(&now, NULL);

	do {
		timersub(&delay_time, &now, &interval);

		int_ts.tv_sec = interval.tv_sec;
		int_ts.tv_nsec = interval.tv_usec * 1000;

		nanosleep(&int_ts, NULL);

		gettimeofday(&now, NULL);
	} while (timercmp(&delay_time, &now, >));
}

void generate_pattern(char *dmx_universe)
{
	static int pos;
	int i;
	bool direction = FALSE;

	static int colour[450];

	for (i = 0; i < 150; i++) {
		float distance = ((float)abs(pos-i)) / 150;
		if (distance < 0.0)
			distance = 0.0;
		if (distance > 1.0)
			distance = 1.0;
		int intensity = (int) (255.0 * (1.0-distance));
		dmx_universe[3*i] = (colour[i]==0)?255:intensity;
		dmx_universe[3*i+1] = (colour[i]==1)?255:intensity;
		dmx_universe[3*i+2] = (colour[i]==2)?255:intensity;
	}

	if ((pos >= 0) && (pos < 150)) {
		colour[pos]++;
		if (colour[pos] > 2)
			colour[pos] = 0;
	}

	if (direction) {
		pos++;
		if (pos >= 2*150) {
			pos = -150;
		}
	} else {
		pos--;
		if (pos <= -150) {
			pos = 2*150;
		}
	}
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: artdmxtest <IP address>\n");
		exit(1);
	}
	setup_socket(argv[1]);
	delay_setup();
	while (1) {
		generate_pattern(dmx_universe);
		send_dmx(dmx_dest, dmx_universe, 450);
		frame_delay(20000);
	}
	return 0;
}

