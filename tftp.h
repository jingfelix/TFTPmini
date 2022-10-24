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
        *ptr = (void *)malloc(ptr_size);
        if (!ptr)
        {
            d_printf("malloc error!\n");
            return -1;
        }
        memcpy(*ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short), &packet->u.data.block, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short) + sizeof(unsigned short), packet->u.data.data, packet->u.data.data_len);
    }
    else if (type == ACK)
    {
        ptr_size = sizeof(unsigned short) + sizeof(unsigned short);
        *ptr = (void *)malloc(ptr_size);
        memcpy(*ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short), &packet->u.ack.block, sizeof(unsigned short));
    }
    else if (type == ERROR)
    {
        ptr_size = sizeof(unsigned short) + sizeof(unsigned short) + packet->u.error.error_msg_len;
        *ptr = (void *)malloc(ptr_size);
        memcpy(*ptr, &packet->opcode, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short), &packet->u.error.error_code, sizeof(unsigned short));
        memcpy(*ptr + sizeof(unsigned short) + sizeof(unsigned short), packet->u.error.error_msg, packet->u.error.error_msg_len);
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
    {
        d_printf("print_tftp_packet: invalid parameter!\n");
        return -1;
    }

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

unsigned short get_tftp_packet_block(char *buf)
{
    if (buf == NULL)
        return -1;

    unsigned short block;
    memcpy(&block, buf + sizeof(unsigned short), sizeof(unsigned short));
    block = ntohs(block);

    return block;
}

int tftp_rrq_handler(int server_fd, char *buf, int recv_count, struct sockaddr_in *client_addr)
{

    socklen_t client_addr_len = sizeof(struct sockaddr_in);
    // ready to send data packet

    char filename[TFTP_MAX_SIZE];

    // here copy current path into filename
    if (getcwd(filename, TFTP_MAX_SIZE) == NULL)
    {
        d_printf("getcwd error!\n");
        return -1;
    }
    d_printf("current dir: %s\n", filename);

    int filename_len = strlen(filename);
    filename[filename_len] = '/';
    filename[filename_len + 1] = '\0';
    filename_len++;

    strcpy(filename + filename_len, buf + sizeof(unsigned short));
    /*
    strcpy(filename, buf + sizeof(unsigned short));
    */
    d_printf("filename: %s\n", filename);

    int fd = open(filename, O_RDWR, 0666);
    if (fd < 0)
    {
        d_printf("open file error!\n");
        return -1;
    }

    // 注意接收文件时block为0
    unsigned short block = 1;

    int send_socket = socket(AF_INET, SOCK_DGRAM, 0);

    while (1)
    {
        d_printf("Entering loop\n");
        char data[TFTP_MAX_SIZE] = {0};
        int read_count = read(fd, &data[0], TFTP_MAX_SIZE);
        if (read_count < 0)
        {
            // TODO: send error msg?
            d_printf("read file error!\n");
            return -1;
        }
        else if (read_count == 0)
        {
            // 发一个data为0的包
            d_printf("data: %s\n", data);
            d_printf("data send end\n");
            return 0;
        }
        else
        {
            d_printf("sending data\n");
            d_printf("data: %s\n", data);
            d_printf("data len: %d\n", read_count);
            d_printf("block: %d\n", block);

            struct tftp_packet packet = {0};
            packet.opcode = htons(DATA);
            packet.u.data.block = htons(block);
            packet.u.data.data_len = read_count;
            packet.u.data.data = data;

            char *buf = NULL;
            int buf_size = make_tftp_packet(&packet, DATA, &buf);
            d_printf("buf_size: %d\n", buf_size);
            int flag = print_tftp_packet(buf, buf_size);
            d_printf("print packet flag: %d\n", flag);

            // 这里有个很大的问题，发送的时候是否会绑定端口？？？？？？？
            // 同理，tftp客户端在接收的时候是否应该切换目标端口？？？？？？？
            d_printf("sending data\n");
            sendto(send_socket, buf, buf_size, 0, (struct sockaddr *)client_addr, client_addr_len);
            d_printf("send data packet, block: %u, data_len: %d\n", block, read_count);

            free(buf);
        }

        char recv_buf[TFTP_MAX_SIZE * 2] = {0};

        recv_count = recvfrom(send_socket, buf, TFTP_MAX_SIZE, 0, (struct sockaddr *)client_addr, &client_addr_len);

        unsigned short type = get_tftp_packet_type(buf);

        if (get_tftp_packet_type(buf) != ACK || get_tftp_packet_block(buf) != block)
        {
            // send error msg
            print_tftp_packet(buf, recv_count);
            d_printf("type error or block error\n");
            return -1;
        }

        block++;
    }
}