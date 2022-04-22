#include "exchange_trtp.h"

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "log.h"
#include "utils.h"
#include "window.h"

#define SFD_IN 0
#define SFD_OUT 1

bool read_file(FILE* input, window_t* send_window)
{
    char buffer[MAX_PAYLOAD_SIZE];

    if (!send_window->write_finished && !window_is_full(send_window)) {
        uint16_t nb_read = fread(buffer, 1, MAX_PAYLOAD_SIZE, input);

        if (nb_read == 0) {
            if (ferror(input)) {
                ERROR("Couldn't read from input: %s", strerror(errno));
                return false;
            }
            if (feof(input)) {
                send_window->write_finished = true;
            }
        }

        DEBUG("%3d byte(s) read from input", nb_read);

        if (!window_add_data_pkt(send_window, buffer, nb_read)) {
            ERROR("Attempt to add data pkt to a full window");
            return false;
        }
    }

    return true;
}

bool write_file(FILE* output, window_t* recv_window)
{
    pkt_t* pkt;
    while ((pkt = window_slide_if_possible(recv_window, 1))) {
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

bool trtp_send(const int sfd, window_t* send_window, window_t* recv_window, statistics_t* statistics)
{
    pkt_t* pkt = NULL;
    bool from_window = false;
    pkt = next_pkt(recv_window, statistics); // Return ACK or NACK
    if (!pkt) {
        pkt = next_pkt(send_window, statistics); // Return DATA or FEC
        from_window = true;
    }
    if (!pkt)
        return true;

    char buffer[PKT_MAX_LEN];
    size_t buffer_len = PKT_MAX_LEN;
    pkt_status_code ret = pkt_encode(pkt, buffer, &buffer_len);

    if (ret != PKT_OK) {
        ERROR("Packet encoding failed. status=%d", ret);
        pkt_del(pkt);
        return false;
    }

    ssize_t send_len = send(sfd, buffer, buffer_len, 0);

    DEBUG("%3ld byte(s) sent. type=%d, seqnum=%d", send_len, pkt_get_type(pkt), pkt_get_seqnum(pkt));

    if (send_len < 0) {
        ERROR("Couldn't send to sfd: %s", strerror(errno));
        pkt_del(pkt);
        if (from_window) {
            int sleep_time = 2 << send_window->connection_retry_attempt;
            INFO("sleeping for %d seconds", sleep_time);
            sleep(sleep_time);
            send_window->connection_retry_attempt++;
            return send_window->is_new_connection && send_window->connection_retry_attempt < 5;
        }
        return false;
    }

    update_stats_from_valid_pkt_sent(pkt, statistics);
    pkt_del(pkt);

    return true;
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
        DEBUG("Received a corrupted packet. error=%d", ret);
        if (ret == E_LENGTH) {
            DEBUG("recv_len=%ld", recv_len);
        }
        pkt_del(pkt);
        statistics->packet_ignored++;
        return true;
    }

    window_update_from_received_pkt(send_window, pkt, statistics);
    window_update_from_received_pkt(recv_window, pkt, statistics);
    update_stats_from_valid_pkt_received(pkt, statistics);
    pkt_del(pkt);

    return true;
}

void exchange_trtp(const int sfd, FILE* input, FILE* output, const trtp_options_t* const options, statistics_t* statistics)
{
    bool stop = false;
    window_t* send_window = NULL;
    window_t* recv_window = NULL;

    int input_poll_index = -1;
    int output_poll_index = -1;
    int fds_len = 2;

    if (input) {
        input_poll_index = fds_len;
        fds_len++;

        send_window = window_new(SEND_WINDOW, 5);
        if (!send_window) {
            stop = true;
        } else {
            send_window->peer_size = 1;
        }
    }

    if (output) {
        output_poll_index = fds_len;
        fds_len++;

        recv_window = window_new(RECV_WINDOW, 5);
        if (!recv_window) {
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
        if (recv_window && recv_window->shutdown_time != -1) {
            long current_time = get_time_in_milliseconds();
            if (recv_window->shutdown_time < current_time) {
                DEBUG("Shuting down gracefully");
                recv_window->read_finished = true;
            }
        }
        if (window_closed(send_window) && window_closed(recv_window)) {
            stop = true;
            continue;
        }

        // Wait 1 seconds
        int ret = poll(fds, fds_len, 1000);

        // Check if poll actually succeed
        if (ret == -1) {
            stop = true;
            continue;
        } else if (ret > 0) {

            if (input_poll_index != -1 && fds[input_poll_index].revents & POLLIN) {
                fds[input_poll_index].revents = 0;

                if (options->fec_enabled) {
                    window_add_fec_pkt_if_needed(send_window);
                }

                if (!read_file(input, send_window)) {
                    stop = true;
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
