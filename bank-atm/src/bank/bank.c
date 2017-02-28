#include "bank.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <math.h>

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

static Bank *bank;

Bank* bank_create(char *port, char *auth_file)
{
    // TODO: replace with a real port.
    unsigned short bank_port = atoi(port);

    bank = (Bank*) malloc(sizeof(Bank));
    if(bank == NULL)
    {
        perror("Could not allocate Bank");
        exit(1);
    } 

    HashTable *ht = hash_table_create(10000);

    // Set up the network state
    bank->sockfd=socket(AF_INET,SOCK_DGRAM,0);
    bzero(&bank->bank_addr, sizeof(bank->bank_addr));
    bank->bank_addr.sin_family = AF_INET;
    bank->bank_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    bank->bank_addr.sin_port = htons(bank_port);
    bank->accounts = ht;
    bank->authfile = NULL;
    bind(bank->sockfd,(struct sockaddr *)&bank->bank_addr, sizeof(bank->bank_addr));

    // Set up the protocol state
    // TODO set up more, as needed

    // Set up the auth file as needed.
    FILE *input;
    if (access(auth_file, F_OK) != -1)
        exit(255);
    else { // Otherwise, create auth file
        input = fopen(auth_file, "w");
        bank->authfile = auth_file;
        fwrite(port, strlen(port), 1, input);
        fwrite("\n", sizeof(char), 1, input);
        fwrite("127.0.0.1", strlen("127.0.0.1"), 1, input);
        fwrite("\n", sizeof(char), 1, input);
        //Create 256 bit key
        unsigned char buffer[32];
        int rc = RAND_bytes(buffer, sizeof(buffer));
        if(rc != 1)
            exit(63); // Failed to create key
        fwrite(buffer, sizeof(buffer), 1, input);
        fclose(input);
        printf("created\n");
        fflush(stdout);
    }
    return bank;
}

ssize_t bank_send(Bank *bank, char *data, size_t data_len)
{
    // Returns the number of bytes sent; negative on error
    return sendto(bank->sockfd, data, data_len, 0,
                  (struct sockaddr*) &bank->remote_addr, sizeof(bank->remote_addr));
}

ssize_t bank_recv(Bank *bank, char *data, size_t max_data_len)
{   

    // Returns the number of bytes received; negative on error
    socklen_t addrlen = sizeof(bank->remote_addr); 
    return recvfrom(bank->sockfd, data, max_data_len, 0, (struct sockaddr*) &bank->remote_addr, &addrlen);
}

void bank_process_remote_command(Bank *bank, char *command, size_t len)
{
    // TODO: Implement the bank side of the ATM-bank protocol
	/*
	 * The following is a toy example that simply receives a
	 * string from the ATM, prepends "Bank got: " and echoes 
	 * it back to the ATM before printing it to stdout. Of 
	 * course, you will need JSON output to stdout.
	 */

    command[len]=0;
    int n; 
    char IV_from_atm[10000];
    
    n = bank_recv(bank, IV_from_atm, 10000);
    IV_from_atm[n] = '\0';
    
    
    FILE * fp;
    char * auth_line = NULL;
    size_t length = 0;
    ssize_t read;
    fp = fopen(bank->authfile, "r");
    if (fp == NULL) {
        exit(63);
    }

    // Read in key from auth file
    while ((read = getline(&auth_line, &length, fp)) != -1) {}
    unsigned char *key = (unsigned char *) auth_line;
    
    int i;

    unsigned char decryptedtext[10000];
    // Lengths of the text returned
    int decryptedtext_len, ciphertext_len = strlen(command);
    //unsigned char *plaintext = (unsigned char *) command;
    decryptedtext_len = decrypt((unsigned char *) command,
        ciphertext_len, key, (unsigned char *) IV_from_atm,
        decryptedtext);
    decryptedtext[decryptedtext_len] = '\0';
    
    i = 0;
    char *cmds[3];
    // mode;acc;modearg
    *cmds = (char *) malloc(decryptedtext_len);
    char *tok = strtok((char *) decryptedtext, ";");
    while(tok != NULL) {
        cmds[i] = (char *) malloc(strlen(tok) + 1);
        strcpy(cmds[i], tok);
        tok = strtok(NULL, ";");
        i++;
    }
    
    double amount;
    double curr_bal;
    double new_bal;
    char new_bal_str[16];
    char msg_to_send[6];
    switch (*cmds[0]) {
        case 'n':
            if (hash_table_size(bank->accounts) != 0){
                if(hash_table_find(bank->accounts, cmds[1]) == NULL){
                    hash_table_add(bank->accounts, cmds[1], cmds[2]);
               
                    sscanf(hash_table_find(bank->accounts, cmds[1]), "%lf", 
                        &amount);
                    //printf("{\"account\":\"%s\",\"initial_balance:\":%0.2f}\n",
                      //      cmds[1], amount);
                    printf("{\"initial_balance\": %0.2f,\"account\": \"%s\"}\n",
                        amount, cmds[1]);
                    sprintf(msg_to_send, "%s", "true");
                    bank_send(bank, msg_to_send, strlen(msg_to_send));  
                    fflush(stdout);
                } else {
                    /*account already exists*/
                    sprintf(msg_to_send, "%s", "false");
                    bank_send(bank, msg_to_send, strlen(msg_to_send));
                }
            } else {
                hash_table_add(bank->accounts, cmds[1], cmds[2]);
                // Print JSON
                
                sscanf(hash_table_find(bank->accounts, cmds[1]), "%lf", &amount);
                //printf("{\"initial_balance\":%0.2f, \"account\": \"%s\"}\n",
                        //amount, cmds[1]);
                printf("{\"initial_balance\": %0.2f, \"account\": \"%s\"}\n",
                    amount, cmds[1]);
                fflush(stdout);
                sprintf(msg_to_send, "%s", "true");
                bank_send(bank, msg_to_send, strlen(msg_to_send));  
                fflush(stdout);
            }
            break;
        case 'd':
            if (hash_table_size(bank->accounts) != 0){
                /*account does not exist*/
                if (hash_table_find(bank->accounts, cmds[1]) == NULL){
                    sprintf(msg_to_send, "%s", "false");
                    bank_send(bank, msg_to_send, strlen(msg_to_send));   
                } else {
                    sscanf(hash_table_find(bank->accounts, cmds[1]), "%lf", 
                        &curr_bal);
                    sscanf(cmds[2], "%lf", &amount);
                    //fprintf(stderr, "\nDeposit\n");
                    //fprintf(stderr, "name: %s\n", cmds[1]);
                    //fprintf(stderr, "cur bal %lf\n", curr_bal);
                    //fprintf(stderr, "amount %lf\n", amount);
                    new_bal = curr_bal + amount;
                    
                    //fprintf(stderr, "newbal * 100 %lf\n", new_bal);
                    //fprintf(stderr, "newbal * 100 %lf\n", new_bal * 100);
                    //double temp = round(new_bal * 100.00)/100.00;
                    //new_bal = temp;
                    //double temp = roundf(new_bal * 100) / 100;
                    //new_bal = temp;
                    //unsigned int temp = curr_bal * 100;
                    //unsigned int temp2 = amount * 100;
                    //new_bal = temp + temp2;
                    //new_bal /= 100;
                    //curr_bal *= 100;
                    //amount *= 100;
                    //new_bal = (unsigned int) curr_bal +  (unsigned int) amount;
                    //new_bal = roundf(new_bal * 1000) / 1000;
                    //fprintf(stderr, "rounded: %lf\n", new_bal);
                    //new_bal /= 100;
                    //amount /= 100;
                    //fprintf(stderr, "new bal2: %lf\n", new_bal);
                    snprintf(new_bal_str, sizeof(new_bal), "%f", new_bal);
                    hash_table_del(bank->accounts, cmds[1]);
                    hash_table_add(bank->accounts, cmds[1], new_bal_str);

                    //fprintf(stderr, "amount");
                    printf("{\"account\": \"%s\", \"deposit\": %0.2f}\n",
                        cmds[1], amount);
                    fflush(stdout);
                    sprintf(msg_to_send, "%s", "true");
                    bank_send(bank, msg_to_send, strlen(msg_to_send)); 
                    
                }
            } else {//no accounts exist in bank
                sprintf(msg_to_send, "%s", "false");
                bank_send(bank, msg_to_send, strlen(msg_to_send)); 
            }
            break;
        case 'w':
            if (hash_table_size(bank->accounts) != 0){
                /*account does not exist*/
                if (hash_table_find(bank->accounts, cmds[1]) == NULL){
                    sprintf(msg_to_send, "%s", "false");
                    bank_send(bank, msg_to_send, strlen(msg_to_send));
                } else {
                    sscanf(hash_table_find(bank->accounts, cmds[1]), "%lf", 
                        &curr_bal);
                    /*fprintf(stderr, "\nWithdraw\n");
                    fprintf(stderr, "name: %s\n", cmds[1]);
                    fprintf(stderr, "cur bal %lf\n", curr_bal);*/
                    sscanf(cmds[2], "%lf", &amount);
                    //fprintf(stderr, "amount %lf\n", amount);
                    new_bal = curr_bal - amount;
                    //fprintf(stderr, "new bal: %lf\n", new_bal);
                    if (new_bal < 0){
                        sprintf(msg_to_send, "%s", "false");
                        bank_send(bank, msg_to_send, strlen(msg_to_send));  
                    } else {
                        snprintf(new_bal_str, sizeof(new_bal), "%f", new_bal);
                        hash_table_del(bank->accounts, cmds[1]);
                        hash_table_add(bank->accounts, cmds[1], new_bal_str);

                        printf("{\"account\": \"%s\", \"withdraw\": %0.2f}\n",
                            cmds[1], amount);
                        fflush(stdout);
                        sprintf(msg_to_send, "%s", "true");
                        bank_send(bank, msg_to_send, strlen(msg_to_send)); 
                        
                    }
                }
            } else {//no accounts exist in bank
                sprintf(msg_to_send, "%s", "false");
                bank_send(bank, msg_to_send, strlen(msg_to_send)); 
            }
            break;
        case 'g':
            if (hash_table_size(bank->accounts) != 0){
                /*account does not exist*/
                if(hash_table_find(bank->accounts, cmds[1]) == NULL){
                    sprintf(msg_to_send, "%s", "false");
                    bank_send(bank, msg_to_send, strlen(msg_to_send));  
                } else {
                    sprintf(msg_to_send, "%s", "true");
                    bank_send(bank, msg_to_send, strlen(msg_to_send)); 
                    sscanf(cmds[2], "%lf", &amount);
                    sscanf(hash_table_find(bank->accounts, cmds[1]), "%lf", 
                        &curr_bal);
                    sprintf(new_bal_str, "%s", (char *) hash_table_find(bank->accounts, cmds[1]));
                    bank_send(bank, new_bal_str, strlen(new_bal_str));
                    printf("{\"account\": \"%s\", \"balance\": %0.2f}\n",
                        cmds[1], curr_bal);
                    fflush(stdout);
                }
            }else {/*no accounts are in the bank*/
               sprintf(msg_to_send, "%s", "false");
               bank_send(bank, msg_to_send, strlen(msg_to_send)); 
            }
            break;
    }
}

void bank_exit()
{
    // TODO: Implement exit function
    // this function is called when SIGTERM is received.
    if(bank != NULL)
    {
        remove(bank->authfile);
        close(bank->sockfd);
        hash_table_free(bank->accounts);
        free(bank);
    }
    exit(0);
}
