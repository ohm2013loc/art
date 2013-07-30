/* artdmx.h */

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define TRUE 1
#define FALSE 0
typedef int bool;

extern struct addrinfo *dmx_dest;

void setup_socket(char *universe_address);
void send_dmx(struct addrinfo *dest, char *data, int length);
void delay_setup(void);
void frame_delay(unsigned int microseconds);

