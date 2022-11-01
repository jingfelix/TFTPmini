#include "tftp.h"

#define SERVER_PORT 69
#define BUFF_LEN 1024
/*
    server:
            socket-->bind-->recvfrom-->sendto-->close
*/

int main(int argc, char *argv[])
{
    int server_fd;

    server_fd = socket(AF_INET, SOCK_DGRAM, 0); // AF_INET:IPV4;SOCK_DGRAM:UDP
    if (server_fd < 0)
    {
        printf("create socket fail!\n");
        return -1;
    }

    struct sockaddr_in ser_addr;
    memset(&ser_addr, 0, sizeof(ser_addr));
    ser_addr.sin_family = AF_INET;
    ser_addr.sin_addr.s_addr = inet_addr("0.0.0.0"); // IP地址，需要进行网络序转换，INADDR_ANY：本地地址
    ser_addr.sin_port = htons(SERVER_PORT);          //端口号，需要网络序转换

    int ret = bind(server_fd, (struct sockaddr *)&ser_addr, sizeof(ser_addr));
    if (ret < 0)
    {
        printf("socket bind fail!\n");
        return -1;
    }

    printf("ready to receive data!\n");

    char buf[sizeof(unsigned short) + TFTP_MAX_SIZE + TFTP_MAX_SIZE];

    socklen_t len;
    struct sockaddr_in client_addr;

    int recv_count;
    while (1)
    {
        d_printf("********ENTERING LOOP********\n");
        memset(buf, 0, BUFF_LEN);
        len = sizeof(client_addr);
        recv_count = recvfrom(server_fd, buf, BUFF_LEN, 0, (struct sockaddr *)&client_addr, &len);

        if (recv_count < 0)
        {
            printf("recvfrom error!\n");
            continue;
        }

        printf("Received %d bytes from %s:%d\n", recv_count, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        print_tftp_packet(buf, recv_count);

        int flag = 0;
        unsigned short type = get_tftp_packet_type(buf);
        switch (type)
        {
        case RRQ:
            flag = tftp_rrq_handler(server_fd, buf, recv_count, &client_addr);
            if (flag < 0)
            {
                printf("tftp_rrq_handler error!\n");
            }
            break;
        case WRQ:
            flag = tftp_wrq_handler(server_fd, buf, recv_count, &client_addr);
            if (flag < 0)
            {
                printf("tftp_wrq_handler error!\n");
            }
            break;

        default:
            break;
        }

        memset(buf, 0, BUFF_LEN);
    }

    // has to deal with different request

    close(server_fd);
    return 0;
}