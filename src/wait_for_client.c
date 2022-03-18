#include <arpa/inet.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "create_socket.h"
#include "wait_for_client.h"

/* Block the caller until a message is received on sfd,
 * and connect the socket to the source addresse of the received message.
 * @sfd: a file descriptor to a bound socket but not yet connected
 * @return: 0 in case of success, -1 otherwise
 * @POST: This call is idempotent, it does not 'consume' the data of the message,
 * and could be repeated several times blocking only at the first call.
 */
int wait_for_client(int sfd)
{
    // Receive a message through the socket
    struct sockaddr_storage client_addr; // allocate the peer's address on the stack. It will be initialized when we receive a message
    socklen_t client_addr_len = sizeof(struct sockaddr_storage); // variable that will contain the length of the peer's address
    ssize_t n_received = recvfrom(sfd, NULL, 0, 0, (struct sockaddr*)&client_addr, &client_addr_len);
    if (n_received == -1) {
        ERROR("Could not receive the message.");
        perror("Error");
        return -1;
    }

    int err = connect(sfd, (struct sockaddr*)&client_addr, client_addr_len);
    if (err == -1) {
        ERROR("Cound not connect the socket.");
        perror("Error");
        return -1;
    }

    return 0;
}
