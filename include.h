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

// get file name from tftp packet buf
char *get_tftp_packet_filename(char *buf);