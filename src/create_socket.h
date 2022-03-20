#ifndef __CREATE_SOCKET_H_
#define __CREATE_SOCKET_H_

#include <netinet/in.h> /* * sockaddr */
#include <sys/types.h> /* sockaddr */

/* Creates a socket and initialize it
 * @source_addr: if !NULL, the source address that should be bound to this socket
 * @src_port: if >0, the port on which the socket is listening
 * @dest_addr: if !NULL, the destination address to which the socket should send data
 * @dst_port: if >0, the destination port to which the socket should be connected
 * @return: a file descriptor number representing the socket,
 *         or -1 in case of error (explanation will be printed on stderr)
 */
int create_socket(struct sockaddr* source_addr,
    int src_port,
    struct sockaddr* dest_addr,
    int dst_port);

#endif
