/*
 * The Bank takes commands from the ATM, handled by
 * bank_process_remote_command.
 *
 * Feel free to update the struct and the processing as you desire
 */

#ifndef __BANK_H__
#define __BANK_H__

#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <signal.h>
#include "../util/hash_table.h"
#include "../util/list.h"

typedef struct _Bank
{
    // Networking state
    int sockfd;
    struct sockaddr_in bank_addr;
    struct sockaddr_in remote_addr;
    char *authfile;
    HashTable *accounts;

    // Protocol state
    // TODO add more, as needed

} Bank;

Bank* bank_create(char *port, char *auth_file);
ssize_t bank_send(Bank *bank, char *data, size_t data_len);
ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len);
void bank_process_remote_command(Bank *bank, char *command, size_t len);
void bank_exit();

#endif

