#include "include.h"

/*
    pack tftp packet into buffer
    return the length of the buffer
    return -1 if error
*/
int make_tftp_packet(struct tftp_packet *packet, int type, char **ptr)
{
    int ptr_size = 0;

    if (packet == NULL)
        return -2;

    if (type == RRQ || type == WRQ)
    {
        ptr_size = sizeof(unsigned short) + packet->u.request.filename_len + packet->u.request.mode_len;
        *ptr = (void *)malloc(ptr_size);

        if (!ptr)
        {
            d_printf("malloc error!\n");
            return -1;
        }
        memcpy(*ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short), packet->u.request.filename, packet->u.request.filename_len);
        memcpy(*ptr + sizeof(unsigned short) + packet->u.request.filename_len, packet->u.request.mode, packet->u.request.mode_len);
        // serious question: size of the data packet is uncertain
    }
    else if (type == DATA)
    {
        ptr_size = sizeof(unsigned short) + sizeof(unsigned short) + packet->u.data.data_len;
        ptr = (void *)malloc(ptr_size);
        memcpy(ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(ptr + sizeof(unsigned short), &packet->u.data.block, sizeof(unsigned short));
        memcpy(ptr + sizeof(unsigned short) + sizeof(unsigned short), packet->u.data.data, packet->u.data.data_len);
    }
    else if (type == ACK)
    {
        ptr_size = sizeof(unsigned short) + sizeof(unsigned short);
        ptr = (void *)malloc(ptr_size);
        memcpy(ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(ptr + sizeof(unsigned short), &packet->u.ack.block, sizeof(unsigned short));
    }
    else if (type == ERROR)
    {
        ptr_size = sizeof(unsigned short) + sizeof(unsigned short) + packet->u.error.error_msg_len;
        ptr = (void *)malloc(ptr_size);
        memcpy(ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(ptr + sizeof(unsigned short), &packet->u.error.error_code, sizeof(unsigned short));
        memcpy(ptr + sizeof(unsigned short) + sizeof(unsigned short), packet->u.error.error_msg, packet->u.error.error_msg_len);
    }
    else
    {
        return -1;
    }
    return ptr_size;
}

int print_tftp_packet(char *buf, int len)
{
    if (buf == NULL || len < 0)
        return -1;

    for (int i = 0; i < len; i++)
    {
        printf("%02x ", buf[i]);
    }
    printf("\n");

    unsigned short type;
    memcpy(&type, buf, sizeof(unsigned short));
    type = ntohs(type);

    unsigned short block = 0;
    switch (type)
    {
    case DATA:
        printf("TYPE: DATA");
        memcpy(&block, buf, sizeof(unsigned short));
        block = ntohs(block);
        printf("BLOCK: %u\n", block);
        for (int i = 2; i < len; i++)
        {
            printf("%02x", buf[i]);
        }
        printf("\n");
        break;
    case ACK:
        printf("TYPE: ACK");
        memcpy(&block, buf, sizeof(unsigned short));
        block = ntohs(block);
        printf("BLOCK: %u\n", block);
        break;
    case ERROR:
        printf("TYPE: ERROR");
        unsigned short error_code;
        memcpy(&error_code, buf, sizeof(unsigned short));
        error_code = ntohs(error_code);
        printf("ERROR CODE: %u", error_code);

        // NOTICE: here needs 1 byte of 0 to separate mode and end

        for (int i = 2; i < len - 1; i++)
        {
            printf("%c", buf[i]);
        }
        break;
    case RRQ:
        printf("TYPE: RRQ ");
        for (int i = 2; i < len; i++)
        {
            if (buf[i] != '\0')
            {
                printf("%c", buf[i]);
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
        break;
    case WRQ:
        printf("TYPE: WRQ ");
        for (int i = 2; i < len; i++)
        {
            if (buf[i] != '\0')
            {
                printf("%c", buf[i]);
            }
            else
            {
                printf(" ");
            }
        }
        printf("\n");
        break;
    default:
        return -2;
    }
    return 0;
}


unsigned short get_tftp_packet_type(char *buf)
{
    if (buf == NULL)
        return -1;

    unsigned short type;
    memcpy(&type, buf, sizeof(unsigned short));
    type = ntohs(type);

    return type;
}

int tftp_rrq_handler(int server_fd, char *buf, int recv_count, struct sockaddr_in *client_addr)
{

    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    // ready to send data packet

    char filename[TFTP_MAX_SIZE];
    strcpy(filename, buf + sizeof(unsigned short));
    int fd = open(filename, O_RDONLY);
    if (fd < 0)
    {
        d_printf("open file error!\n");
        return -1;
    }

    unsigned short block = 1;
    while(1)
    {
        char data[TFTP_MAX_SIZE] = {0};
        int read_count = read(fd, &data[0], TFTP_MAX_SIZE);
        if (read_count < 0)
        {
            // TODO: send error msg?
            return -1;
        }
        else if (read_count == 0)
        {

        }
        else
        {
            struct tftp_packet packet = {0};
            packet.opcode = htons(DATA);
            packet.u.data.block = htons(block);
            packet.u.data.data_len = read_count;
            memcpy(packet.u.data.data, data, read_count);

            char *buf = NULL;
            int buf_size = tftp_packet_to_buf(&packet, DATA, &buf);


            // 这里有个很大的问题，发送的时候是否会绑定端口？？？？？？？
            // 同理，tftp客户端在接收的时候是否应该切换目标端口？？？？？？？
            sendto(server_fd, buf, buf_size, 0, (struct sockaddr *)client_addr, client_addr_len);
        }
        




        char recv_buf[TFTP_MAX_SIZE * 2] = {0};

        recvfrom(server_fd, buf, TFTP_MAX_SIZE, 0, (struct sockaddr *)client_addr, &client_addr_len);

        block++;
    }




}