/* dmxmain.c */

#include <stdlib.h>
#include <stdio.h>

#include "artdmx.h"

unsigned char dmx_universe[512];

void generate_pattern(unsigned char *dmx_universe);
void init_pattern(unsigned char *dmx_universe);

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <IP address>\n", argv[0]);
		exit(1);
	}
	setup_socket(argv[1]);
	init_pattern(dmx_universe);
	delay_setup();
	while (1) {
		generate_pattern(dmx_universe);
		send_dmx(dmx_dest, dmx_universe, 450);
		frame_delay(20000);
	}
	return 0;
}

