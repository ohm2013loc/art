/* multi-midi.c */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/soundcard.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <alsa/asoundlib.h>

#include "artdmx.h"
#include "dmxmain.h"

#define MIDI_PORTNAME "hw:1,0,0"

int chan[128];

snd_rawmidi_t *midiin = NULL;

unsigned char midi_buf[1024];

void init_pattern(char *dmx_universe)
{
	int mode = SND_RAWMIDI_NONBLOCK;
	int status;

	if ((status = snd_rawmidi_open(&midiin, NULL, MIDI_PORTNAME, mode))
			< 0) {
		fprintf(stderr, "Problem opening MIDI input: %s",
		       snd_strerror(status));
		exit(1);
	}

	memset(dmx_universe, 0, 450);
}

void process_message(unsigned char *buf, int len)
{
	switch (buf[0]) {
	case 0xB0:
		chan[buf[1]] = buf[2];
		//printf("Set channel %02x to %d\n", buf[1], buf[2]);
		break;
	default:
		printf("Unknown MIDI message: %02x %02x %02x\n",
		       buf[0], buf[1], buf[2]);
		break;
	}
}

void process_midi(unsigned char byte)
{
	static unsigned char status = 0;
	static unsigned int p = 0;
	static unsigned char buf[8];
	int msg_len;

	if (byte & 0x80) {
		/* status */
		p = 0;
		buf[p++] = byte;
	} else {
		/* data */
		if (p == 0)
			p++; /* running status */
		buf[p++] = byte;
	}
	switch (buf[0] & 0xf0) {
		case 0x80:
		case 0x90:
		case 0xA0:
		case 0xB0:
		case 0xC0:
		case 0xE0:
			msg_len = 3;
			break;
		case 0xD0:
			msg_len = 2;
			break;
		default:
			msg_len = 0;
			break;
	}
	if (p >= msg_len) {
		process_message(buf, msg_len);
	}
}

#define SET_LED(ch, r, g, b) do {		\
		int c = 149-ch;			\
		dmx_universe[3*c] = r;		\
		dmx_universe[3*c+1] = g;	\
		dmx_universe[3*c+2] = b;	\
	} while (0)


void generate_pattern(char *dmx_universe)
{
	int i;
	int bytes;

	bytes = snd_rawmidi_read(midiin, &midi_buf, sizeof(midi_buf));

	for (i = 0; i < bytes; i++) {
		process_midi(midi_buf[i]);
	}

	for (i = 0; i < 150; i++) {
		SET_LED(i, 0, 0, 0);
		if (i < chan[0])
			SET_LED(i, 255, 0, 0);
		if (i < chan[1])
			SET_LED(i, 0, 255, 0);
		if (i < chan[2])
			SET_LED(i, 0, 0, 255);
		if (i < chan[3])
			SET_LED(i, 255, 255, 0);
		if (i < chan[4])
			SET_LED(i, 255, 0, 255);
		if (i < chan[5])
			SET_LED(i, 0, 255, 255);
		if (i < chan[6])
			SET_LED(i, 255, 255, 255);
		if (i < chan[7])
			SET_LED(i, 0, 0, 0);

		if ((i < chan[0x10]) && (i > (chan[0x10] - 10)))
			SET_LED(i, 255, 0, 0);
		if ((i < chan[0x11]) && (i > (chan[0x11] - 10)))
			SET_LED(i, 0, 255, 0);
		if ((i < chan[0x12]) && (i > (chan[0x12] - 10)))
			SET_LED(i, 0, 0, 255);
		if ((i < chan[0x13]) && (i > (chan[0x13] - 10)))
			SET_LED(i, 255, 255, 0);
		if ((i < chan[0x14]) && (i > (chan[0x14] - 10)))
			SET_LED(i, 255, 0, 255);
		if ((i < chan[0x15]) && (i > (chan[0x15] - 10)))
			SET_LED(i, 0, 255, 255);
		if ((i < chan[0x16]) && (i > (chan[0x16] - 10)))
			SET_LED(i, 255, 255, 255);
		if ((i < chan[0x17]) && (i > (chan[0x17] - 10)))
			SET_LED(i, 0, 0, 0);
	}
}

