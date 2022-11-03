#pragma once
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>

#define TFTP_MAX_SIZE 512
#define RRQ 1
#define WRQ 2
#define DATA 3
#define ACK 4
#define ERROR 5

#define d_printf printf

struct tftp_packet
{
    unsigned short opcode;
    union
    {
        struct
        {
            unsigned short block;
            char *data;
            int data_len;
        } data;
        struct
        {
            unsigned short block;
        } ack;
        struct
        {
            unsigned short error_code;
            char *error_msg;
            int error_msg_len;
            // NOTICE: needs 1 more byte for '\0'
        } error;
        struct
        {
            char *filename;
            int filename_len;
            // NOTICE: here needs 1 byte of 0 to separate filename and mode
            char *mode;
            int mode_len;
            // NOTICE: here needs 1 byte of 0 to separate mode and end
        } request;
    } u;
};

// pack tftp packet into buffer; return the length of the buffer
int make_tftp_packet(struct tftp_packet *packet, int type, char **ptr);

// print tftp packet with format
int print_tftp_packet(char *buf, int len);

// get type(opcode) from tftp packet buf
unsigned short get_tftp_packet_type(char *buf);

// get block number from tftp packet buf
unsigned short get_tftp_packet_block(char *buf);

// RRQ handler for server
int tftp_rrq_handler(int server_fd, char *buf, int recv_count, struct sockaddr_in *client_addr);

// WRQ handler for server
int tftp_wrq_handler(int server_fd, char *buf, int recv_count, struct sockaddr_in *client_addr);

// send RRQ for client
int send_rrq(int client_fd, struct sockaddr_in *ser_addr, char *filename);

// send WRQ for client
int send_wrq(int client_fd, struct sockaddr_in *ser_addr, char *filename);