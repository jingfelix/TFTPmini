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
        printf("Usage: tftp get <filename> <ip> <mode>\n");
        printf("       tftp put <filename> <ip> <mode>\n\n");
        printf("- filename: the file to be transfered or downloaded\n");
        printf("- ip: the ip address of the server\n");
        printf("- mode: netascii, octet\n");
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
    if (client_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    // setup timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    if (setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt failed\n");
        return -1;
    }

    if (ser_addr.sin_addr.s_addr == INADDR_NONE || client_fd < 0)
    {
        printf("invalid ip address!\n");
        free(ip);
        free(filename);
        close(client_fd);
        return -1;
    }

    char _mode;
    if (argc == 4)
        _mode = 0;
    else if (strcmp(argv[4], "n") == 0)
        _mode = 1;
    else if (strcmp(argv[4], "o") == 0)
        _mode = 0;
    else
    {
        printf("Invalid mode!Please use 'n' for netascii or 'o' for octet\n");
        free(ip);
        free(filename);
        close(client_fd);
        return -1;
    }
    if (strcmp(argv[1], "get") == 0)
    {
        printf("receving file %s from %s:%d\n", filename, ip, SERVER_PORT);
        int flag = send_rrq(client_fd, &ser_addr, filename, _mode);
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
        printf("sending file %s to %s:%d\n", filename, ip, SERVER_PORT);
        int flag = send_wrq(client_fd, &ser_addr, filename, _mode);
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