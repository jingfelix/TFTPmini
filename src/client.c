#include "tftp.h"

#define SERVER_PORT 69
// #define SERVER_IP "0.0.0.0"

/*
    client:
            socket-->sendto-->revcfrom-->close
*/

int main(int argc, char *argv[])
{

    if (argc < 4)
    {
        printf("Usage: tftp get <filename> <ip>\n");
        return -1;
    }

    // get destination ip
    char *ip = (char *)malloc(TFTP_MAX_SIZE);
    memset(ip, 0, TFTP_MAX_SIZE);
    strcpy(ip, argv[3]);

    d_printf("ip: %s", ip);

    // get filename
    char *filename = (char *)malloc(TFTP_MAX_SIZE);
    memset(filename, 0, TFTP_MAX_SIZE);
    strcpy(filename, argv[2]);

    d_printf("filename: %s", filename);

    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr(ip);
    ser_addr.sin_port = htons(SERVER_PORT);

    int client_fd;
    client_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (ser_addr.sin_addr.s_addr == INADDR_NONE || client_fd < 0)
    {
        printf("invalid ip address!\n");
        free(ip);
        free(filename);
        close(client_fd);
        return -1;
    }

    

    /*
    while (1)
    {
        socklen_t ser_len;
        ser_len = sizeof(ser_addr);

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
    */
    if (strcmp(argv[1], "get") == 0)
    {
        int flag = send_rrq(client_fd, &ser_addr, filename);
        if (flag < 0)
        {
            d_printf("send_rrq failed!\n");
            free(ip);
            free(filename);
            close(client_fd);
            return -1;
        }
        else
        {
            d_printf("send_rrq success!\n");
        }
    }
    else if (strcmp(argv[1], "put") == 0)
    {
        int flag = send_wrq(client_fd, &ser_addr, filename);
        if (flag < 0)
        {
            d_printf("send_wrq failed!\n");
            free(ip);
            free(filename);
            close(client_fd);
            return -1;
        }
        else
        {
            d_printf("send_wrq success!\n");
        }
    }
    else
    {
        printf("Usage: tftp get <filename> <ip>\n");
    }

    free(ip);
    free(filename);
    close(client_fd);

    return 0;
}