/* 
 * The main program for the Bank.
 *
 * You are free to change this as necessary.
 */

#include <string.h>
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include "bank.h"
#include <unistd.h>
#include <regex.h>

int main(int argc, char**argv)
{
    int n;
    char recvline[10000];

    //catch SIGTERM in order to exit cleanly
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = bank_exit;
    sigaction(SIGTERM, &action, NULL);

    //Probably you should put something here to handle the command-line args
    // Guideline 5 implementation
    char *port = NULL;
    char *auth_file = NULL;
    int option;
    regex_t numbers;
    // Compile regex for valid numbers
    if (regcomp(&numbers, "^(0|[1-9][0-9]*)$", REG_EXTENDED) != 0)
        exit(255);
    // Compile regex for valid filename
    regex_t file_regex;
    if (regcomp(&file_regex, "^([_\\-\\.0-9a-z]*)$", REG_EXTENDED) != 0)
        exit(255);
    // Input Validation
    while ((option = getopt (argc, argv, "p:s:")) != -1) {
        if (optarg != NULL && strlen(optarg) > 4096) // Args cannot exceed 4096
            exit(255);
        switch (option) {
            case 'p':
                if (port == NULL)
                    port = optarg;
                else // Case where command is duplicated
                    exit(255);
                // Check if port numbers are between 1024 and 65535
                if (regexec(&numbers, port, 0, NULL, 0) != 0 || atoi(port) <
                        1024 || atoi(port) > 65535)
                    exit(255);
                break;
            case 's':
                if (auth_file == NULL)
                    auth_file = optarg;
                else // Case where command is duplicated
                    exit(255);
                // Check if the filename is valid
                if ((regexec(&file_regex, auth_file, 0, NULL, 0) != 0) || 
                        strlen(auth_file) < 1 || strlen(auth_file) > 255 ||
                        (strcmp(".", auth_file) == 0) || 
                        (strcmp("..", auth_file) == 0))
                    exit(255);
                break;
            case '?':
                exit(255);
        }
    }
    //Default values if none are passed in the command line
    if (auth_file == NULL)
        auth_file = "bank.auth";
    if (port == NULL) 
        port = "3000";

    Bank *bank = bank_create(port, auth_file);

    fflush(stdout);

    while(1)
    {   

        n = bank_recv(bank, recvline, 10000);
        bank_process_remote_command(bank, recvline, n);
    }

    return EXIT_SUCCESS;
}