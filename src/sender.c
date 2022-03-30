#include <errno.h>
#include <stdbool.h>
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

int print_usage(char* prog_name)
{
    ERROR("Usage:\n\t%s [-f filename] [-s stats_filename] [-c] receiver_ip receiver_port", prog_name);
    return EXIT_FAILURE;
}

int main(int argc, char** argv)
{
    int opt;

    char* filename = NULL;
    char* stats_filename = NULL;
    char* receiver_ip = NULL;
    char* receiver_port_err;
    trtp_options_t options = { 0 };
    uint16_t receiver_port;

    while ((opt = getopt(argc, argv, "f:s:hc")) != -1) {
        switch (opt) {
        case 'f':
            filename = optarg;
            break;
        case 'h':
            return print_usage(argv[0]);
        case 's':
            stats_filename = optarg;
            break;
        case 'c':
            options.fec_enabled = true;
            break;
        default:
            return print_usage(argv[0]);
        }
    }

    if (optind + 2 != argc) {
        ERROR("Unexpected number of positional arguments");
        return print_usage(argv[0]);
    }

    receiver_ip = argv[optind];
    receiver_port = (uint16_t)strtol(argv[optind + 1], &receiver_port_err, 10);
    if (*receiver_port_err != '\0') {
        ERROR("Receiver port parameter is not a number");
        return print_usage(argv[0]);
    }

    INFO("Sender has following arguments: filename is %s, stats_filename is %s, fec_enabled is %d, receiver_ip is %s, receiver_port is %u",
        filename, stats_filename, options.fec_enabled, receiver_ip, receiver_port);

    struct sockaddr_storage receiver_addr = { 0 };
    const char* err = real_address(receiver_ip, (struct sockaddr*)&receiver_addr);
    if (err) {
        ERROR("Could not resolve hostname \"%s\": %s\n", receiver_ip, err);
        return EXIT_FAILURE;
    }

    int sfd = create_socket(NULL, -1, (struct sockaddr*)&receiver_addr, receiver_port);

    if (sfd < 0) {
        ERROR("Failed to create the socket!: %s", strerror(errno));
        return EXIT_FAILURE;
    }

    FILE* input = NULL;

    if (filename) {
        input = fopen(filename, "r");
    } else {
        input = stdin;
    }

    statistics_t statistics = {
        0,
    };

    exchange_trtp(sfd, input, NULL, &options, &statistics);

    if (filename) {
        fclose(input);
    }

    if (!write_sender_stats(stats_filename, &statistics))
        perror("Could not write stats file.");

    return EXIT_SUCCESS;
}
