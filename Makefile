# Makefile

all: artpoll artdmxtest white-colour multi-midi stars police artmonitor

artpoll: artpoll.c
artmonitor: artmonitor.c
artdmxtest: artdmxtest.c

white-colour: white-colour.c artdmx.c dmxmain.c
stars: stars.c artdmx.c dmxmain.c
multi-midi: multi-midi.c artdmx.c dmxmain.c -lasound
police: police.c artdmx.c dmxmain.c

