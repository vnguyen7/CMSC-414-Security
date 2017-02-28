/* 
 * The main program for the ATM.
 *
 * You are free to change this as necessary.
 */
#include "atm.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <regex.h>
#include <string.h>

//./atm -s bank.auth -i 127.0.0.1 -p 3000 -c potato -a user -n 500
//./atm -s bank.auth -c bob.card -a bob -n 1000.00
int main(int argc, char**argv)
{
    //handle command line parameters somewhere here.
    // Guideline 5 implementation
    char *account = NULL;
    char *auth_file = NULL;
    char *port = NULL;
    char *ip = NULL;
    char *card = NULL;
    char *mode = NULL;
    char *modearg = NULL;
    int option;
    // Compile regex for valid filename
    regex_t file_regex;
    if (regcomp(&file_regex, "^([_\\-\\.0-9a-z]*)$", REG_EXTENDED) != 0)
        exit(255);
    regex_t numbers;
    // Compile regex for valid numbers
    if (regcomp(&numbers, "^(0|[1-9][0-9]*)$", REG_EXTENDED) != 0)
        exit(255);
    regex_t dollar;
    // Compile regex for valid numbers
    if (regcomp(&dollar, "^(0|[1-9]+[0-9]*)(\\.[0-9]{2})$", REG_EXTENDED) != 0)
        exit(255);
    char *sub_ip;
    double d;
    // Input Validation
    while ((option = getopt (argc, argv, "a:s:i:p:c:n:d:w:g")) != -1) {
        if (optarg != NULL && strlen(optarg) > 4096) // Args cannot exceed 4096
            exit(255);
        switch (option) {
            case 'a':
                if (account == NULL) {
                    account = optarg;
                } else // Case where account command is duplicated
                    exit(255);
                // Same restrictions as file except 1-250 length and . and ..
                // are valid account names.
                if ((regexec(&file_regex, account, 0, NULL, 0) != 0) || 
                        strlen(account) < 1 || strlen(account) > 250)
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
            case 'i':
                if (ip == NULL)
                    ip = optarg;
                else // Case where ip command is duplicated
                    exit(255);
                sub_ip = (char *) malloc(strlen(optarg)+1);
                strncat(sub_ip, optarg, strlen(optarg));
                sub_ip = strtok(sub_ip, ".");
                while (sub_ip != NULL) { // Check if each subsection is valid
                    if ((regexec(&numbers, sub_ip, 0, NULL, 0) != 0) || 
                            atoi(sub_ip) > 255 || atoi(sub_ip) < 0)
                        exit(255);
                    sub_ip = strtok(NULL, ".");
                }
                free(sub_ip);
                break;
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
            case 'c':
                if (card == NULL) 
                    card = optarg;
                else // Case where card command is duplicated
                    exit(255);
                if ((regexec(&file_regex, card, 0, NULL, 0) != 0) || 
                        strlen(card) < 1 || strlen(card) > 255)
                    exit(255);
                break;
            case 'n':
                if (mode == NULL && modearg == NULL ){
                    modearg = optarg;
                    mode = "n";
                }else{
                    exit(255);
                }
                sscanf(modearg, "%lf", &d);
                if ((regexec(&dollar, modearg, 0, NULL, 0) != 0) || 
                    d < 0.00 || d > 4294967295.99) {
                    exit(255);
                } 
                break;
            case 'd':
                if (mode == NULL && modearg == NULL ){
                    modearg = optarg;
                    mode = "d";
                }else{
                    exit(255);
                }
                sscanf(modearg, "%lf", &d);
                if ((regexec(&dollar, modearg, 0, NULL, 0) != 0) || 
                    d < 0.00 || d > 4294967295.99) {
                    exit(255);
                } 
                break;
            case 'w':
                if (mode == NULL && modearg == NULL ){
                    modearg = optarg;
                    mode = "w";
                }else{
                    exit(255);
                }
                sscanf(modearg, "%lf", &d);
                if ((regexec(&dollar, modearg, 0, NULL, 0) != 0) || 
                    d < 0.00 || d > 4294967295.99) {
                    exit(255);
                } 
                break;
            case 'g':
                if (mode == NULL) {
                    mode = "g";
                } else
                    exit(255);
                break;
            case '?':
                exit(255);
        }
    }
    //Default values if none are passed in the command line
    if (strcmp(mode, "g") == 0 && argc != optind)
        exit(255);
    if (account == NULL)
        exit(255);
    if (auth_file == NULL)
        auth_file = "bank.auth";
    if (access(auth_file, F_OK | R_OK) == -1) // If file can't be opened or is
        exit(255);                            // invalid, exit.
    if (ip == NULL)
        ip = "127.0.0.1";
    if (port == NULL) 
        port = "3000";
    if (modearg == NULL)
        modearg = "";
    char card_name[256];
    if (card == NULL) {
        strncpy(card_name, account,strlen(account)+1);
        if (strlen(card_name) <= 255) {
            strcat(card_name, ".card");
            card = card_name;
        } else
            exit(255);
    }

    ATM *atm = atm_create(auth_file, port, ip);
    //account;auth_file;ip;port;card;mode;modearg
    //char **commands = (char**) malloc(2*sizeof(char*));

    int size = strlen(account) + strlen(auth_file) + strlen(ip) + strlen(port) +
        strlen(card) + strlen(mode) + strlen(modearg) + 7;
    char *cmds = calloc(1, size);
    snprintf(cmds, size, "%s;%s;%s;%s;%s;%s;%s", account, auth_file, ip, port,
        card, mode, modearg);
    
    atm_process_command(atm, cmds);
    //free(parcmd);
    //free(parsed_command);
    //parcmd = NULL;
    //parsed_command = NULL;
    return EXIT_SUCCESS;
}