/* stars.c */

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "artdmx.h"
#include "dmxmain.h"

#define SET_LED(ch, r, g, b) do {               \
                int c = 149-ch;                 \
                dmx_universe[3*c] = r;          \
                dmx_universe[3*c+1] = g;        \
                dmx_universe[3*c+2] = b;        \
        } while (0)


void init_pattern(char *dmx_universe)
{
	int i;

	memset(dmx_universe, 0, 450);
}

void generate_pattern(char *dmx_universe)
{
	int i;
	static int count;

	for (i = 0; i < 450; i++) {
		int n = (unsigned char)dmx_universe[i];
		n -= 2;
		if (n < 0)
			n = 0;
		dmx_universe[i] = n;
	}

	if (count++ >= 10) {
		int r;
		count = 0;
		r = rand();
		r = r / (RAND_MAX/150);
		if (r >= 150)
			r = 149;
		SET_LED(r, 255, 255, 255);
	}
}

