#include "exchange_trtp.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "packet.h"

#define SFD_IN 0
#define SFD_OUT 1

typedef struct {
    pkt_t* pkts;
    size_t size;
    int next_seqnum;
    bool EOF_reached;
} window_t;

bool read_file(FILE* input, char* buffer, size_t* buffer_len, bool* EOF_reached)
{
    if (*buffer_len == 0 && !*EOF_reached) {
        size_t nb_read = fread(buffer, 1, MAX_PAYLOAD_SIZE, input);
        if (nb_read == 0) {
            if (feof(input)) {
                *EOF_reached = true;
            }
            if (ferror(input)) {
                ERROR("Couldn't read from input: %s", strerror(errno));
                return false;
            }
        }

        DEBUG("%ld byte(s) read from input", nb_read);

        *buffer_len = nb_read;
    }

    return true;
}

bool write_file(FILE* output, char* buffer, size_t* buffer_len)
{
    if (*buffer_len) {
        fwrite(buffer, 1, *buffer_len, output);
        fflush(output);

        *buffer_len = 0;
    }
    return true;
}

bool trtp_send(const int sfd, char* input_buffer, size_t* input_buffer_len,
    window_t* window, statistics_t* statistics)
{
    pkt_t pkt;

    if (window->EOF_reached) {
        pkt_set_type(&pkt, PTYPE_DATA);
        statistics->data_sent++;
        WARNING("EOF_reached");
    } else {
        pkt_set_type(&pkt, PTYPE_DATA);
        pkt_set_tr(&pkt, 0);
        pkt_set_window(&pkt, 0x1C);
        pkt_set_seqnum(&pkt, 0x7b);
        pkt_set_timestamp(&pkt, 0x17);
        pkt_set_payload(&pkt, input_buffer, *input_buffer_len);
        *input_buffer_len = 0;
        statistics->data_sent++;
    }

    char buffer[PKT_MAX_LEN];
    size_t buffer_len = PKT_MAX_LEN;
    pkt_status_code ret = pkt_encode(&pkt, buffer, &buffer_len);

    if (ret != PKT_OK) {
        return false;
    }

    ssize_t send_len = send(sfd, buffer, buffer_len, 0);

    DEBUG("%ld byte(s) sent. Expected: %ld, header=%ld", send_len, buffer_len, PKT_MAX_HEADERLEN);

    if (send_len < 0) {
        ERROR("Couldn't send to sfd: %s", strerror(errno));
        return false;
    }

    return !window->EOF_reached;
}

bool trtp_recv(const int sfd, char* output_buffer, size_t* output_buffer_len,
    window_t* window, statistics_t* statistics)
{
    WARNING("RECV");
    if (window) {
        WARNING("WINDOW");
    }

    char buffer[PKT_MAX_LEN];
    ssize_t recv_len = recv(sfd, buffer, PKT_MAX_LEN, 0);
    if (recv_len < 0) {
        ERROR("Couldn't read from sfd: %s", strerror(errno));
        return false;
    }

    pkt_t pkt;

    pkt_status_code ret = pkt_decode(buffer, recv_len, &pkt);

    if (ret != PKT_OK) {
        return false;
    }

    uint16_t length = pkt_get_length(&pkt);

    if (length == 0 && pkt_get_type(&pkt) == PTYPE_DATA) {
        WARNING("End of transfert ptk");
        return false;
    }

    const char* payload = pkt_get_payload(&pkt);
    statistics->data_received++;

    if (payload) {
        memcpy(output_buffer, payload, length);
        *output_buffer_len = length;
    }

    return true;
}

pkt_t* set_window_size(window_t* window)
{
    if (window->pkts == NULL)
        return (pkt_t*)malloc(window->size * sizeof(pkt_t));
    return (pkt_t*)realloc(window->pkts, window->size * sizeof(pkt_t));
}

void exchange_trtp(const int sfd, FILE* input, FILE* output, statistics_t* statistics)
{
    char send_buffer[MAX_PAYLOAD_SIZE];
    size_t send_buffer_len = 0;
    window_t send_window = { 0 };
    send_window.size = 1;
    set_window_size(&send_window);

    char recv_buffer[MAX_PAYLOAD_SIZE];
    size_t recv_buffer_len = 0;
    window_t recv_window = { 0 };
    recv_window.size = 1;
    set_window_size(&recv_window);

    int input_poll_index = -1;
    int output_poll_index = -1;
    int fds_len = 2;

    if (input) {
        input_poll_index = fds_len;
        fds_len++;
    }

    if (output) {
        output_poll_index = fds_len;
        fds_len++;
    }

    while (1) {
        // The structure for four events
        struct pollfd fds[4];

        // Monitor sfd for input
        fds[SFD_IN].fd = sfd;
        fds[SFD_IN].events = POLLIN;

        // Monitor sfd for output
        fds[SFD_OUT].fd = sfd;
        fds[SFD_OUT].events = POLLOUT;

        // Monitor input for input
        if (input_poll_index != -1) {
            fds[input_poll_index].fd = STDIN_FILENO;
            fds[input_poll_index].events = POLLIN;
        }

        // Monitor output for output
        if (output_poll_index != -1) {
            fds[output_poll_index].fd = STDOUT_FILENO;
            fds[output_poll_index].events = POLLOUT;
        }

        // Wait 1 seconds
        int ret = poll(fds, fds_len, 1000); // TODO: Check this for network latency

        // Check if poll actually succeed
        if (ret == -1)
            return;
        else if (ret > 0) {

            if (input_poll_index != -1 && fds[input_poll_index].revents & POLLIN) {
                fds[input_poll_index].revents = 0;

                if (!read_file(input, send_buffer, &send_buffer_len, &send_window.EOF_reached))
                    return;
            }

            if (output_poll_index != -1 && fds[output_poll_index].revents & POLLOUT) {
                fds[output_poll_index].revents = 0;

                if (!write_file(output, recv_buffer, &recv_buffer_len))
                    return;
            }

            if (fds[SFD_IN].revents & POLLIN) {
                fds[SFD_IN].revents = 0;

                if (!trtp_recv(sfd, recv_buffer, &recv_buffer_len, &recv_window, statistics))
                    return;
            }

            if (fds[SFD_OUT].revents & POLLOUT) {
                fds[SFD_OUT].revents = 0;

                if (!trtp_send(sfd, send_buffer, &send_buffer_len, &send_window, statistics))
                    return;
            }
        }
    }
}
