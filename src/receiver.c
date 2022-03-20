#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "create_socket.h"
#include "exchange_trtp.h"
#include "log.h"
#include "real_address.h"
#include "statistics.h"
#include "wait_for_client.h"

int print_usage(char* prog_name)
{
    ERROR("Usage:\n\t%s [-s stats_filename] listen_ip listen_port", prog_name);
    return EXIT_FAILURE;
}

int main(int argc, char** argv)
{
    int opt;

    char* stats_filename = NULL;
    char* listen_ip = NULL;
    char* listen_port_err;
    uint16_t listen_port;

    while ((opt = getopt(argc, argv, "s:h")) != -1) {
        switch (opt) {
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    listen_ip = argv[optind];
    listen_port = (uint16_t)strtol(argv[optind + 1], &listen_port_err, 10);
    if (*listen_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    INFO("Receiver has following arguments: stats_filename=\"%s\", listen_ip=\"%s\", listen_port=%u",
        stats_filename, listen_ip, listen_port);

    struct sockaddr_storage listen_addr;
    const char* err = real_address(listen_ip, (struct sockaddr*)&listen_addr);
    if (err) {
        ERROR("Could not resolve hostname \"%s\": %s\n", listen_ip, err);
        return EXIT_FAILURE;
    }

    int sfd = create_socket((struct sockaddr*)&listen_addr, listen_port, NULL, -1);

    if (sfd > 0 && wait_for_client(sfd) < 0) { /* Connected */
        ERROR("Could not connect the socket after the first message.\n");
        close(sfd);
        return EXIT_FAILURE;
    }

    if (sfd < 0) {
        ERROR("Failed to create the socket.: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    statistics_t statistics = {
        0,
    };

    exchange_trtp(sfd, NULL, stdout, &statistics);

    if (!write_receiver_stats(stats_filename, &statistics))
        perror("Could not write stats file.");

    return EXIT_SUCCESS;
}