/* police.c */

#include <string.h>
#include <math.h>

#include "artdmx.h"
#include "dmxmain.h"

#define min(a, b) ((a)<(b)?(a):(b))

float pos;

#define SET_LED(ch, r, g, b) do {               \
                int c = 149-ch;                 \
                dmx_universe[3*c] = r;          \
                dmx_universe[3*c+1] = g;        \
                dmx_universe[3*c+2] = b;        \
        } while (0)

void init_pattern(char *dmx_universe)
{
	memset(dmx_universe, 0, 450);
	pos = 0;
}

void generate_pattern(char *dmx_universe)
{
	int i;
	float place;
	float p2;

	for (i = 0; i < 150; i++) {
		float distance, val;
		float d1, d2, d3;
		place = (i % 7) + pos;
		if (place >= 7.0)
			place -= 7.0;
		distance = fabs(3.0 - place);
		val = 2.0 - distance;
		if (val < 0.0)
			val = 0.0;
		val = val / 2.0;
		SET_LED(i, 0, 0, (int)(val*255.0));
	}

	pos = pos + 0.2;
	if (pos >= 7.0)
		pos = 0.0;
}

