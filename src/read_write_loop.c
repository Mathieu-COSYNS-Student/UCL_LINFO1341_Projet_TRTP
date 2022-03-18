#include "read_write_loop.h"
#include <poll.h>
#include <sys/select.h>

#define MAX_SEGEMENT_SIZE 1024

char buffer[MAX_SEGEMENT_SIZE];

int read_from_stdin_and_write_socket(int socket)
{
    char read_buffer[MAX_SEGEMENT_SIZE];
    int ret = fread(0, buffer, sizeof(buffer));
    if (ret < 0) {
        perror("couldn't read from stdin");
    }
    ssize_t written = write(socket, read_buffer, MAX_SEGEMENT_SIZE));
    fflush(0);
}

int receive_messages(int socket, char* bufferReceived)
{
    ssize_t amount_read = read(socket, bufferReceived, MAX_SEGEMENT_SIZE);
    if (amount_read == -1) {
        printf("could not read on the socket");
    }
}

void read_write_loop(const int sfd)
{

    while (1) {
    }
}
