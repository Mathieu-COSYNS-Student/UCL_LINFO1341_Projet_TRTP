#include "exchange_trtp.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "window.h"

#define SFD_IN 0
#define SFD_OUT 1

bool read_file(FILE* input, window_t* send_window)
{
    char buffer[MAX_PAYLOAD_SIZE];
    bool full = window_is_full(send_window);

    if (!send_window->write_finished && !full) {
        uint16_t nb_read = fread(buffer, 1, MAX_PAYLOAD_SIZE, input);

        if (nb_read == 0) {
            if (ferror(input)) {
                ERROR("Couldn't read from input: %s", strerror(errno));
                return false;
            }
        }

        DEBUG("%d byte(s) read from input", nb_read);

        if (!window_add_data_pkt(send_window, buffer, nb_read)) {
            ERROR("Attempt to add data pkt to a full window");
            return false;
        }

        if (feof(input)) {
            if (!window_add_data_pkt(send_window, NULL, 0)) {
                ERROR("Attempt to add data pkt to a full window");
                return false;
            }
            send_window->write_finished = true;
        }
    }

    return true;
}

bool write_file(FILE* output, window_t* recv_window)
{
    pkt_t* pkt;
    while ((pkt = window_remove_first_pkt(recv_window))) {
        const char* payload = pkt_get_payload(pkt);
        uint16_t length = pkt_get_length(pkt);

        if (length > 0) {
            fwrite(payload, 1, length, output);
            fflush(output);
        } else if (length == 0) {
            recv_window->write_finished = true;
        }

        pkt_del(pkt);
    }
    return true;
}

pkt_t* next_pkt_to_send(window_t* send_window, window_t* recv_window)
{
    if (window_has_pkt_to_send(recv_window)) {
        pkt_t* pkt = pkt_new();
        int seqnum = window_seqnum(recv_window);
        pkt_set_type(pkt, PTYPE_ACK);
        pkt_set_window(pkt, recv_window->size);
        pkt_set_seqnum(pkt, window_next_seqnum(recv_window, seqnum));
        pkt_set_timestamp(pkt, recv_window->pkts[seqnum].timestamp);

        if (recv_window->write_finished) {
            recv_window->read_finished = true;
        }
        return pkt;
    } else if (window_has_pkt_to_send(send_window)) {
        return next_pkt(send_window);
    }

    return NULL;
}

bool trtp_send(const int sfd, window_t* send_window, window_t* recv_window, statistics_t* statistics)
{
    pkt_t* pkt;
    if (!(pkt = next_pkt_to_send(send_window, recv_window)))
        return true;

    char buffer[PKT_MAX_LEN];
    size_t buffer_len = PKT_MAX_LEN;
    pkt_status_code ret = pkt_encode(pkt, buffer, &buffer_len);
    pkt_del(pkt);

    if (ret != PKT_OK) {
        return false;
    }

    ssize_t send_len = send(sfd, buffer, buffer_len, 0);

    DEBUG("%ld byte(s) sent. Expected: %ld", send_len, buffer_len);

    if (send_len < 0) {
        ERROR("Couldn't send to sfd: %s", strerror(errno));
        return false;
    }

    update_stats_from_valid_pkt_sent(pkt, statistics);

    return true;
}

void update_state_from_recv_pkt(pkt_t* pkt, window_t* send_window, window_t* recv_window)
{
    ptypes_t pkt_type = pkt_get_type(pkt);

    if (pkt_type == PTYPE_DATA) {
        recv_window->pkts[pkt_get_seqnum(pkt)] = *pkt;
        recv_window->pkts_status[pkt_get_seqnum(pkt)] = PKT_TO_ACK;
        uint16_t length = pkt_get_length(pkt);

        if (length == 0 && pkt_get_type(pkt) == PTYPE_DATA) {
            recv_window->write_finished = true;
            return;
        }
    } else if (pkt_type == PTYPE_ACK) {
        uint8_t seqnum = pkt_get_seqnum(pkt);
        send_window->pkts_status[seqnum] = PKT_ACK_OK;
    }
}

bool trtp_recv(const int sfd, window_t* send_window, window_t* recv_window, statistics_t* statistics)
{
    char buffer[PKT_MAX_LEN];
    ssize_t recv_len = recv(sfd, buffer, PKT_MAX_LEN, 0);
    if (recv_len < 0) {
        ERROR("Couldn't read from sfd: %s", strerror(errno));
        return false;
    }

    pkt_t* pkt = pkt_new();

    pkt_status_code ret = pkt_decode(buffer, recv_len, pkt);

    if (ret != PKT_OK) {
        return false;
    }

    DEBUG("Packet received.");
    update_state_from_recv_pkt(pkt, send_window, recv_window);
    update_stats_from_valid_pkt_received(pkt, statistics);
    pkt_del(pkt);

    return true;
}

void exchange_trtp(const int sfd, FILE* input, FILE* output, const trtp_options_t* const options, statistics_t* statistics)
{
    bool stop = false;
    window_t* send_window = window_new(SEND_WINDOW);
    window_t* recv_window = window_new(RECV_WINDOW);

    int input_poll_index = -1;
    int output_poll_index = -1;
    int fds_len = 2;

    if (input) {
        input_poll_index = fds_len;
        fds_len++;
        if (!window_set_size(send_window, 5)) {
            stop = true;
        }
        send_window->peer_size = 1;
    }

    if (output) {
        output_poll_index = fds_len;
        fds_len++;
        if (!window_set_size(recv_window, 5)) {
            stop = true;
        }
    }

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
        fds[input_poll_index].fd = fileno(input);
        if (fds[input_poll_index].fd < 0) {
            stop = true;
        }
        fds[input_poll_index].events = POLLIN;
    }

    // Monitor output for output
    if (output_poll_index != -1) {
        fds[output_poll_index].fd = fileno(output);
        if (fds[output_poll_index].fd < 0) {
            stop = true;
        }
        fds[output_poll_index].events = POLLOUT;
    }

    while (!stop) {
        if (window_closed(send_window) && window_closed(recv_window)) {
            stop = true;
            continue;
        }

        // Wait 1 seconds
        int ret = poll(fds, fds_len, 1000); // TODO: Check this for network latency

        // Check if poll actually succeed
        if (ret == -1) {
            stop = true;
            continue;
        } else if (ret > 0) {

            if (input_poll_index != -1 && fds[input_poll_index].revents & POLLIN) {
                fds[input_poll_index].revents = 0;

                if (options->fec_enabled) {
                    window_add_fec_pkt_if_needed(send_window);
                    // TODO also check bettewen last DATA and closing DATA
                }

                if (!read_file(input, send_window)) {
                    stop = true;
                }

                if (options->fec_enabled) {
                    window_add_fec_pkt_if_needed(send_window);
                }
            }

            if (output_poll_index != -1 && fds[output_poll_index].revents & POLLOUT) {
                fds[output_poll_index].revents = 0;

                if (!write_file(output, recv_window)) {
                    stop = true;
                }
            }

            if (fds[SFD_IN].revents & POLLIN) {
                fds[SFD_IN].revents = 0;

                if (!trtp_recv(sfd, send_window, recv_window, statistics)) {
                    stop = true;
                }
            }

            if (fds[SFD_OUT].revents & POLLOUT) {
                fds[SFD_OUT].revents = 0;

                if (!trtp_send(sfd, send_window, recv_window, statistics)) {
                    stop = true;
                }
            }
        }
    }

    window_del(send_window);
    window_del(recv_window);
}
