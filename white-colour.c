/* white-colour.c */

#include <string.h>

#include "artdmx.h"
#include "dmxmain.h"

int pos;
bool direction = FALSE;
int colour[150];

void init_pattern(char *dmx_universe)
{
	int i;

	memset(dmx_universe, 0, 450);
	for (i = 0; i < 150; i++)
		colour[i] = 3;
	if (direction)
		pos = 0;
	else
		pos = -150;
}

void generate_pattern(char *dmx_universe)
{
	int i;

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
		if (colour[pos] > 3)
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

