#include "wait_for_client.h"

#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "create_socket.h"
#include "log.h"

int wait_for_client(int sfd)
{
    // Receive a message through the socket
    struct sockaddr_storage client_addr; // allocate the peer's address on the stack. It will be initialized when we receive a message
    socklen_t client_addr_len = sizeof(struct sockaddr_storage); // variable that will contain the length of the peer's address
    ssize_t n_received = recvfrom(sfd, NULL, 0, MSG_PEEK, (struct sockaddr*)&client_addr, &client_addr_len);
    if (n_received == -1) {
        ERROR("Could not receive the message: %s", strerror(errno));
        return -1;
    }

    int err = connect(sfd, (struct sockaddr*)&client_addr, client_addr_len);
    if (err == -1) {
        ERROR("Could not connect the socket: %s", strerror(errno));
        return -1;
    }

    return 0;
}
