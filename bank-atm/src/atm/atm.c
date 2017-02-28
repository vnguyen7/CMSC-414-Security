#include "atm.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <src/util/hash_table.h>
#include <src/util/list.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>

int decrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
  unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;
    /* Create context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        exit(255);
    /* Init for 256 bit AES. IV is 128 bit.*/
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        exit(255);
    // Decrypt message in blocks
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext,
            ciphertext_len))
        exit(255);
    plaintext_len = len;
    // Last step of encryption
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        exit(255);
    plaintext_len += len;
    // Free Memory
    EVP_CIPHER_CTX_free(ctx);
    return plaintext_len;
}

int encrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
    unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;
    // Initialize context
    if(!(ctx = EVP_CIPHER_CTX_new()))
        exit(255);
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv))
        exit(255);
    // Encrypt message
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        exit(255);
    ciphertext_len = len;
    // Finalize encryption
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        exit(255);
    ciphertext_len += len;
    // Free Memory
    EVP_CIPHER_CTX_free(ctx);
    return ciphertext_len;
}

ATM* atm_create(char *auth_file, char *port, char *ip)
{
    ATM *atm = (ATM*) malloc(sizeof(ATM));
    if(atm == NULL)
    {
        perror("Could not allocate ATM");
        exit(1);
    }

    // Might want to change this
    unsigned short bank_port = atoi(port);

    // Set up the network state
    atm->sockfd=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&atm->bank_addr, sizeof(atm->bank_addr));
    atm->bank_addr.sin_family = AF_INET;
    atm->bank_addr.sin_addr.s_addr=inet_addr(ip);
    atm->bank_addr.sin_port=htons(bank_port);
    atm->authfile = auth_file;
    // Set up the protocol state
    // TODO set up more, as needed
    return atm;
}

void atm_free(ATM *atm)
{
    if(atm != NULL)
    {
        close(atm->sockfd);
        free(atm);
    }
}

ssize_t atm_send(ATM *atm, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(atm->sockfd, data, data_len, 0,
                  (struct sockaddr*) &atm->bank_addr, sizeof(atm->bank_addr));
}

ssize_t atm_recv(ATM *atm, char *data, size_t max_data_len)
{
    // Returns the number of bytes received; negative on error
    return recvfrom(atm->sockfd, data, max_data_len, 0, NULL, NULL);
}

void atm_process_command(ATM *atm, char *command)
{
    // TODO: Implement the ATM's side of the ATM-bank protocol2
    /*
     * The following is a toy example that simply sends the
     * user's command to the bank, receives a message from the
     * bank, and then prints it to stdout.
     */
    //char recvline[10000];
    int n;
    char balance_received[16];
    char msg_error[6];
    
    FILE * fp;
    char * auth_line = NULL;
    size_t length = 0;
    ssize_t read;

    fp = fopen(atm->authfile, "r");
    if (fp == NULL)
        exit(63);

    // Read in key from auth file
    while ((read = getline(&auth_line, &length, fp)) != -1) {}
    unsigned char *key = (unsigned char *) auth_line;
    
    //unsigned char *iv = (unsigned char *) "01234567890123456";
    //unsigned char *iv2;
    //Randomize 128 bit IV
    unsigned char iv[16];
    int rc = RAND_bytes(iv, sizeof(iv));
    if(rc != 1)
        exit(255); // Failed to create key
    //unsigned char *iv2 = buffer;
    /*atm_send(atm, (char*)iv2, sizeof(iv2));
    n = atm_recv(atm,msg_error,10000);
    msg_error[n]=0;*/
    unsigned char *plaintext;
    
    //unsigned char *plaintext = (unsigned char *) command;
    //decryptedtext_len = decrypt((unsigned char *) command, ciphertext_len, 
    //    key, iv, decryptedtext);
    //decryptedtext[decryptedtext_len] = '\0';

    int i = 0;
    // Place cmds back into an array:
    // acc;auth_file;ip;port;card;mode;modearg
    char *cmds[7];
    *cmds = (char *) malloc(strlen(command) +1);
    char *tok = strtok(command, ";");
    while(tok != NULL) {
        cmds[i] = (char *) malloc(strlen(tok) + 1);
        strcpy(cmds[i], tok);
        tok = strtok(NULL, ";");
        i++;
    }
    double amount;
    switch (*cmds[5]) {
        case 'n':
            sscanf(cmds[6],"%lf", &amount);
            // If card exists or balance is less than 10, exit
            if (access(cmds[4], F_OK) != -1 || amount < 10)
                exit(255);
            else {
                /* Send the command to the bank in the format mode;acc;modearg
                   Commands are currently parsed in the following format:
                   0  ;    1    ;2 ; 3  ;  4 ; 5  ;   6
                   acc;auth_file;ip;port;card;mode;modearg
                */
                int size = strlen(cmds[5]) + 1 + strlen(cmds[0]) + 1 + 
                    strlen(cmds[6]) + 1;
                char *to_bank = calloc(1, size);
                snprintf(to_bank, size, "%s;%s;%s", cmds[5], cmds[0], cmds[6]);
                
                //Encrypt the command
                plaintext = (unsigned char *) to_bank;
                unsigned char ciphertext[10000];
                int ciphertext_len = encrypt(plaintext, strlen(to_bank), key, 
                    iv, ciphertext);
                
                //Append IV to front
                /*
                int new_size = ciphertext_len + sizeof(buffer);
                char *final_text = malloc(new_size);
                snprintf(final_text, new_size, "%s%s", buffer, ciphertext);
                printf("cipher %s\n", ciphertext);*/
                
                /* Decrypt the ciphertext */
                /*
                unsigned char decryptedtext[10000];
                int decryptedtext_len;
                decryptedtext_len = decrypt(ciphertext, ciphertext_len, key, 
                    iv, decryptedtext);
                decryptedtext[decryptedtext_len] = '\0';
                printf("Decrypted: %s", decryptedtext);*/
                // Send command to bank
                //atm_send(atm, (char *) ciphertext, strlen((char *) ciphertext));
                
                atm_send(atm, (char *) ciphertext, ciphertext_len);
                atm_send(atm, (char *) iv, sizeof(iv));
                //atm_send(atm, final_text, new_size);
                
                // ATM receives a response from Bank
                //n = atm_recv(atm,recvline,10000);
               // recvline[n]=0;
                // Print what ATM receives to stdout
                //fputs(recvline,stdout);
                n = atm_recv(atm,msg_error,10000);
                msg_error[n]=0;
                // If account exist from bank's end, exit.
                if (strcmp(msg_error, "false") == 0){ 
                    exit(255);
                } else { // Otherwise, create a card and print initial balance
                    FILE *card;
                    card = fopen(cmds[4], "w");
                    fwrite(cmds[0], strlen(cmds[0]) * sizeof(cmds[0]), 1, card);
                    fwrite("\n", 1, 1, card);
                    // generate nounce, fix later
                    /*
                    unsigned char buffer[32];
                    int rc = RAND_bytes(buffer, sizeof(buffer));
                    if(rc != 1)
                        exit(255); // Failed to create nonce
                    fwrite(buffer, 32, 1, input);
                    fwrite("\n", 1, 1, input);
                    fclose(input);
                    */
                    fclose(card);
                    printf("{\"initial_balance\": %0.2f, \"account\": \"%s\"}\n",
                        amount, cmds[0]);
                    //printf("{\"account\": \"%s\", \"initial_balance\": %0.2f}\n",
                      //  cmds[0], amount);
                    fflush(stdout);
                }
            }
            break;
        case 'd':
            sscanf(cmds[6],"%lf", &amount);
            // If card does not exist or amount is 0 or less, exit
            if (access(cmds[4], F_OK) == -1 || amount <= 0){
                exit(255);
            } else {
                // Make sure the account matches the card
                FILE *card;
                card = fopen(cmds[4], "r");
                char acc[strlen(cmds[4]) * sizeof(cmds[4])];
                fscanf(card, "%[^\n]", acc);
                if (strcmp(acc, cmds[0]) != 0)
                    exit(255);
                /* Send the command to the bank in the format mode;acc;modearg
                   Commands are currently parsed in the following format:
                   0  ;    1    ;2 ; 3  ;  4 ; 5  ;   6
                   acc;auth_file;ip;port;card;mode;modearg
                */ 
                int size = strlen(cmds[5]) + 1 + strlen(cmds[0]) + 1 + 
                    strlen(cmds[6]) + 1;
                char *to_bank = calloc(1, size);
                snprintf(to_bank, size, "%s;%s;%s", cmds[5], cmds[0], cmds[6]);
                
                //Encrypt the command
                plaintext = (unsigned char *) to_bank;
                unsigned char ciphertext[10000];
                int ciphertext_len = encrypt(plaintext, strlen(to_bank), key, 
                    iv, ciphertext);
                
                // Send command to bank
                atm_send(atm, (char *) ciphertext, ciphertext_len);
                atm_send(atm, (char *) iv, sizeof(iv));
                // ATM receives a response from Bank
               // n = atm_recv(atm,recvline,10000);
                //recvline[n]=0;
                // Print what ATM receives to stdout
                //fputs(recvline,stdout);
                n = atm_recv(atm,msg_error,10000);
                msg_error[n]=0;
                if (strcmp(msg_error, "false") == 0){ // If acc doesn't exist
                    exit(255);
                } else { // Otherwise print amount deposited
                    printf("{\"account\": \"%s\", \"deposit\": %0.2f}\n", 
                        cmds[0], amount);
                    fflush(stdout);
                }
            }
            break;
        case 'w':
            sscanf(cmds[6],"%lf", &amount);
            // If card does not exist or amount is 0 or less, exit
            if (access(cmds[4], F_OK) == -1 || amount <= 0){
                exit(255);
            } else {
                // Make sure the account matches the card
                FILE *card;
                card = fopen(cmds[4], "r");
                char acc[strlen(cmds[4]) * sizeof(cmds[4])];
                fscanf(card, "%[^\n]", acc);
                if (strcmp(acc, cmds[0]) != 0)
                    exit(255);
                /* Send the command to the bank in the format mode;acc;modearg
                   Commands are currently parsed in the following format:
                   0  ;    1    ;2 ; 3  ;  4 ; 5  ;   6
                   acc;auth_file;ip;port;card;mode;modearg
                */ 
                int size = strlen(cmds[5]) + 1 + strlen(cmds[0]) + 1 + 
                    strlen(cmds[6]) + 1;
                char *to_bank = calloc(1, size);
                snprintf(to_bank, size, "%s;%s;%s", cmds[5], cmds[0], cmds[6]);
                
                //Encrypt the command
                plaintext = (unsigned char *) to_bank;
                unsigned char ciphertext[10000];
                int ciphertext_len = encrypt(plaintext, strlen(to_bank), key, 
                    iv, ciphertext);
                
                // Send command to bank
                atm_send(atm, (char *) ciphertext, ciphertext_len);
                atm_send(atm, (char *) iv, sizeof(iv));
                // ATM receives a response from Bank
                //n = atm_recv(atm,recvline,10000);
               // recvline[n]=0;
                // Print what ATM receives to stdout
                //fputs(recvline,stdout);

                n = atm_recv(atm,msg_error,10000);
                msg_error[n]=0;
                if (strcmp(msg_error, "false") == 0){ // If acc doesn't exist
                    exit(255);
                } else { // Otherwise print amount deposited
                    printf("{\"account\": \"%s\", \"withdraw\": %0.2f}\n", 
                        cmds[0], amount);
                    fflush(stdout);
                }
            }
            break;
        case 'g':
            // If card does not exist, exit
            if (access(cmds[4], F_OK) == -1)
                exit(255);
            else {
                // Make sure the account matches the card
                FILE *card;
                card = fopen(cmds[4], "r");
                char acc[strlen(cmds[4]) * sizeof(cmds[4])];
                fscanf(card, "%[^\n]", acc);
                if (strcmp(acc, cmds[0]) != 0) 
                    exit(255);
                /* Send the command to the bank in the format mode;acc;modearg
                   Commands are currently parsed in the following format:
                   0  ;    1    ;2 ; 3  ;  4 ; 5  ;   6
                   acc;auth_file;ip;port;card;mode;modearg
                */
                int size = strlen(cmds[5]) + 1 + strlen(cmds[0]) + 1 + 1;
                char *to_bank = calloc(1, size);
                snprintf(to_bank, size, "%s;%s", cmds[5], cmds[0]);

                //Encrypt the command
                plaintext = (unsigned char *) to_bank;
                unsigned char ciphertext[10000];
                int ciphertext_len = encrypt(plaintext, strlen(to_bank), key, 
                    iv, ciphertext);
                
                // Send command to bank
                atm_send(atm, (char *) ciphertext, ciphertext_len);
                atm_send(atm, (char *) iv, sizeof(iv));
                // ATM receives a response from Bank
                //n = atm_recv(atm,recvline,10000);
                //recvline[n]=0;
                // Print what ATM receives to stdout
                //fputs(recvline,stdout);

                n = atm_recv(atm,msg_error,10000);
                msg_error[n]=0;
                if (strcmp(msg_error, "false") == 0){ // If acc doesn't exist
                    exit(255);
                } else { // Otherwise print account balance
                    n = atm_recv(atm,balance_received,10000);
                    balance_received[n]=0;
                    sscanf(balance_received,"%lf", &amount);
                    printf("{\"account\": \"%s\", \"balance\": %0.2f}\n", 
                        cmds[0], amount);
                    fflush(stdout);
                }
            } 
            break;
    }
}
