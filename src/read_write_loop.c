#include "read_write_loop.h"

#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_SEGMENT_SIZE 1024

void read_write_loop(const int sfd)
{
    char send_buffer[MAX_SEGMENT_SIZE];
    char recv_buffer[MAX_SEGMENT_SIZE];

    while (1) {
        // The structure for four events
        struct pollfd fds[4];

        // Monitor stdin for input
        fds[0].fd = STDIN_FILENO;
        fds[0].events = POLLIN;

        // Monitor stdout for output
        fds[1].fd = STDOUT_FILENO;
        fds[1].events = POLLOUT;

        // Monitor sfd for input
        fds[2].fd = sfd;
        fds[2].events = POLLIN;

        // Monitor sfd for output
        fds[3].fd = sfd;
        fds[3].events = POLLOUT;

        // Wait 1 seconds
        int ret = poll(fds, 4, 1000);

        // Check if poll actually succeed
        if (ret == -1)
            return;
        else if (ret > 0) {
            // printf("fds[0]: %d, fds[3]: %d\r\n", fds[0].revents & POLLIN, fds[3].revents & POLLOUT);
            // If we detect the event, zero it out so we can reuse the structure
            if (fds[0].revents & POLLIN && fds[3].revents & POLLOUT) {
                fds[0].revents = 0;
                fds[3].revents = 0;
                // input event on stdin and output event on sfd

                ret = fread(send_buffer, 1, sizeof(send_buffer), stdin);
                if (ret < 0) {
                    perror("Couldn't read from stdin");
                    return;
                }
                send(sfd, send_buffer, ret, 0);
                printf("SENT\n");
            }

            if (fds[1].revents & POLLOUT && fds[2].revents & POLLIN) {
                fds[1].revents = 0;
                fds[2].revents = 0;
                // input event on sfd and output event on stdout
                ssize_t recv_len = recv(sfd, recv_buffer, MAX_SEGMENT_SIZE, 0);
                if (recv_len < 0) {
                    perror("Couldn't read from sfd");
                    return;
                }
                fflush(stdout);
                fwrite(recv_buffer, 1, recv_len, stdout);
                fflush(stdout);
            }
        }
    }
}
