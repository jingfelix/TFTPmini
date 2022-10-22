#include "tftp.h"

#define SERVER_PORT 69
#define SERVER_IP "0.0.0.0"

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char *argv[])
{
    /*
    if (argc == 1)
    {
        printf("Usage: %s <filename>\n", argv[0]);
        return -1;
    }
    */

    int client_fd;

    client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    ser_addr.sin_port = htons(SERVER_PORT);

    socklen_t ser_len;
    ser_len = sizeof(ser_addr);

    while (1)
    {

        struct tftp_packet pckt = {0};
        pckt.opcode = htons(RRQ);
        char filename[] = "main.c";
        pckt.u.request.filename = &filename[0];
        pckt.u.request.filename_len = strlen(pckt.u.request.filename) + 1;
        char mode[] = "netascii";
        pckt.u.request.mode = &mode[0];
        pckt.u.request.mode_len = strlen(pckt.u.request.mode) + 1;

        char *buf = NULL;

        int buf_len = make_tftp_packet(&pckt, RRQ, &buf);
        d_printf("buf_len: %d\n", buf_len);

        sendto(client_fd, buf, buf_len, 0, (struct sockaddr *)&ser_addr, ser_len);

        int flag = print_tftp_packet(buf, buf_len);
        d_printf("flag: %d\n", flag);
        sleep(1); // send msg every 1 second
        free(buf);
    }

    close(client_fd);

    return 0;
}