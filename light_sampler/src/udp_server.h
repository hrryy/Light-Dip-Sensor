#ifndef _UDP_SERVER_H_
#define _UDP_SERVER_H_

#include <stdbool.h>

// UDP server listening on port 12345
#define UDP_PORT 12345
#define UDP_MAX_PACKET_SIZE 1500

/**
 * Initialize and start UDP server thread
 * 
 * @return true on success, false on failure
 */
bool UdpServer_init(void);

/**
 * Stop UDP server thread and cleanup
 */
void UdpServer_cleanup(void);

#endif // _UDP_SERVER_H_