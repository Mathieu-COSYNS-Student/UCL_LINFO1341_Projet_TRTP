#ifndef __STATISTICS_H_
#define __STATISTICS_H_

#include "packet.h"
struct statistics {
    long data_sent; // Nombre de paquet de type PTYPE_DATA envoyés.
    long data_received; // Nombre de paquet de type PTYPE_DATA valides reçus.
    long data_truncated_received; // Nombre de paquet de type PTYPE_DATA valides reçus avec le champ TR à 1.
    long fec_sent; // Nombre de paquet de type PTYPE_FEC envoyés.
    long fec_received; // Nombre de paquet de type PTYPE_FEC valides reçus.
    long ack_sent; // Nombre de paquet de type PTYPE_ACK envoyés.
    long ack_received; // Nombre de paquet de type PTYPE_ACK valides reçus.
    long nack_sent; // Nombre de paquet de type PTYPE_ACK envoyés.
    long nack_received; // Nombre de paquet de type PTYPE_ACK valides reçus.
    long packet_ignored; // Nombre de paquet ignorés.
    // sender uniquement
    long min_rtt; // Temps minimum en millisecondes entre l’envoie d’un paquet PTYPE_DATA et la réception de l’acquittement correspondant.
    long max_rtt; // Temps maximum en millisecondes entre l’envoie d’un paquet PTYPE_DATA et la réception de l’acquittement correspondant.
    long packet_retransmitted; // Nombre de paquets PTYPE_DATA retransmis à la suite d’une perte, troncation ou corruption.
    // receiver uniquement
    long packet_duplicated; // Nombre de paquets PTYPE_DATA reçus étant déjà dans le buffer de réception.
    long packet_recovered; // Nombre de paquets PTYPE_DATA récupérés grâce aux paquets PTYPE_FEC.
};

typedef struct statistics statistics_t;

int write_sender_stats(const char* pathname, statistics_t* statistics);

int write_receiver_stats(const char* pathname, statistics_t* statistics);

void update_stats_from_valid_pkt_sent(pkt_t* pkt, statistics_t* statistics);

void update_stats_from_valid_pkt_received(pkt_t* pkt, statistics_t* statistics);

#endif /* __STATISTICS_H_ */