#include "tftp.h"

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
        d_printf("%02x ", buf[i]);
    }
    d_printf("\n");

    unsigned short type;
    memcpy(&type, buf, sizeof(unsigned short));
    type = ntohs(type);

    unsigned short block = 0;
    switch (type)
    {
    case DATA:
        d_printf("TYPE: DATA");
        memcpy(&block, buf, sizeof(unsigned short));
        block = ntohs(block);
        d_printf("BLOCK: %u\n", block);
        for (int i = 2; i < len; i++)
        {
            d_printf("%02x", buf[i]);
        }
        d_printf("\n");
        break;
    case ACK:
        d_printf("TYPE: ACK");
        memcpy(&block, buf, sizeof(unsigned short));
        block = ntohs(block);
        d_printf("BLOCK: %u\n", block);
        break;
    case ERROR:
        d_printf("TYPE: ERROR");
        unsigned short error_code;
        memcpy(&error_code, buf, sizeof(unsigned short));
        error_code = ntohs(error_code);
        d_printf("ERROR CODE: %u", error_code);

        // NOTICE: here needs 1 byte of 0 to separate mode and end

        for (int i = 2; i < len - 1; i++)
        {
            d_printf("%c", buf[i]);
        }
        break;
    case RRQ:
        d_printf("TYPE: RRQ ");
        for (int i = 2; i < len; i++)
        {
            if (buf[i] != '\0')
            {
                d_printf("%c", buf[i]);
            }
            else
            {
                d_printf(" ");
            }
        }
        d_printf("\n");
        break;
    case WRQ:
        d_printf("TYPE: WRQ ");
        for (int i = 2; i < len; i++)
        {
            if (buf[i] != '\0')
            {
                d_printf("%c", buf[i]);
            }
            else
            {
                d_printf(" ");
            }
        }
        d_printf("\n");
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

    int fd = open(filename, O_RDONLY, 0666);
    if (fd < 0)
    {
        d_printf("open file error!\n");
        return -1;
    }

    // 注意接收文件时block为0
    unsigned short block = 1;

    int send_socket = socket(AF_INET, SOCK_DGRAM, 0);

    char data[TFTP_MAX_SIZE] = {0};
    while (1)
    {
        d_printf("Entering loop\n");
        memset(data, 0, TFTP_MAX_SIZE);
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
            // 答：是
            d_printf("sending data\n");
            sendto(send_socket, buf, buf_size, 0, (struct sockaddr *)client_addr, client_addr_len);
            d_printf("send data packet, block: %u, data_len: %d\n", block, read_count);
            free(buf);
        }

        char recv_buf[TFTP_MAX_SIZE * 2] = {0};

        recv_count = recvfrom(send_socket, &recv_buf[0], TFTP_MAX_SIZE, 0, (struct sockaddr *)client_addr, &client_addr_len);

        unsigned short type = get_tftp_packet_type(buf);

        if (get_tftp_packet_type(&recv_buf[0]) != ACK || get_tftp_packet_block(&recv_buf[0]) != block)
        {
            // send error msg
            print_tftp_packet(&recv_buf[0], recv_count);
            d_printf("type error or block error\n");
            return -1;
        }

        block++;
    }
}

int tftp_wrq_handler(int server_fd, char *buf, int recv_count, struct sockaddr_in *client_addr)
{
    // get filename
    char filename[TFTP_MAX_SIZE];
    strcpy(filename, buf + sizeof(unsigned short));
    int filename_len = strlen(filename) + 1;
    d_printf("filename: %s\n", filename);

    // get mode
    // notice: with ascii mode, /r/n should be properly handled
    char mode[TFTP_MAX_SIZE];
    strcpy(mode, buf + sizeof(unsigned short) + filename_len);
    d_printf("mode: %s\n", mode);

    // send ack 0 first
    socklen_t client_addr_len = sizeof(struct sockaddr_in);

    int send_socket = socket(AF_INET, SOCK_DGRAM, 0);

    struct tftp_packet ack_pkt = {0};
    ack_pkt.opcode = htons(ACK);
    ack_pkt.u.ack.block = htons(0);

    // make tftp packet
    char *send_buf = NULL;
    int buf_size = make_tftp_packet(&ack_pkt, ACK, &send_buf);
    if (buf_size < 0)
    {
        d_printf("make ack packet error!\n");
        return -1;
    }

    // send ack=0 packet
    int ack_msg = sendto(send_socket, send_buf, buf_size, 0, (struct sockaddr *)client_addr, client_addr_len);
    if (ack_msg < 0)
    {
        d_printf("send ack error!\n");
        return -1;
    }

    // open file
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        d_printf("open file error!\n");
        return -1;
    }

    // enter main loop, receive data and send ack
    unsigned short block = 1;
    char recv_buf[TFTP_MAX_SIZE * 2] = {0};
    while (1)
    {
        // receive data from client
        memset(recv_buf, 0, TFTP_MAX_SIZE * 2);
        recv_count = recvfrom(send_socket, &recv_buf[0], TFTP_MAX_SIZE * 2, 0, (struct sockaddr *)client_addr, &client_addr_len);
        if (recv_count < 0)
        {
            d_printf("recv data error!\n");
            close(fd);
            return -1;
        }
        d_printf("recv data, block: %u, data_len: %d\n", block, recv_count - 4);

        // check data ack
        if (get_tftp_packet_type(&recv_buf[0]) != DATA || get_tftp_packet_block(&recv_buf[0]) != block)
        {
            // send error msg
            print_tftp_packet(&recv_buf[0], recv_count);
            d_printf("type error or block error\n");
            close(fd);
            return -1;
        }

        // write data to file
        int write_msg = write(fd, &recv_buf[0] + sizeof(unsigned short) * 2, recv_count - 4);
        if (write_msg < 0)
        {
            d_printf("write file error!\n");
            close(fd);
            return -1;
        }

        // make ack
        struct tftp_packet ack_pkt = {0};
        ack_pkt.opcode = htons(ACK);
        ack_pkt.u.ack.block = htons(block);

        // make tftp packet
        send_buf = NULL;
        buf_size = make_tftp_packet(&ack_pkt, ACK, &send_buf);
        if (buf_size < 0)
        {
            d_printf("make ack packet error!\n");
            return -1;
        }

        // send ack packet
        ack_msg = sendto(send_socket, send_buf, buf_size, 0, (struct sockaddr *)client_addr, client_addr_len);
        if (ack_msg < 0)
        {
            d_printf("send ack error!\n");
            close(fd);
            return -1;
        }

        block++;

        // check if the data is the last one
        if (recv_count < TFTP_MAX_SIZE)
        {
            d_printf("last data packet, block: %u, data_len: %d\n", block, recv_count);
            break;
        }
    }
    close(fd);

    return 0;
}

int send_rrq(int client_fd, struct sockaddr_in *ser_addr, char *filename, char _mode)
{
    int send_count = 0;

    // build RRQ packet
    struct tftp_packet packet = {0};
start_rrq:
    memset(&packet, 0, sizeof(struct tftp_packet));
    packet.opcode = htons(RRQ);
    packet.u.request.filename = filename;
    packet.u.request.filename_len = strlen(filename) + 1;

    char netascii_mode[] = "netascii";
    char octet_mode[] = "octet";
    if (_mode == 1)
        packet.u.request.mode = &netascii_mode[0];
    else
        packet.u.request.mode = &octet_mode[0];
    // char mode[] = (_mode==1)?"netascii":"octet";
    packet.u.request.mode_len = (_mode == 1) ? strlen(netascii_mode) + 1 : strlen(octet_mode) + 1;

    char *buf = NULL;

    int buf_size = make_tftp_packet(&packet, RRQ, &buf);
    d_printf("buf_size: %d\n", buf_size);
    print_tftp_packet(buf, buf_size);

    socklen_t ser_len;
    ser_len = sizeof(*ser_addr);

    int msg = sendto(client_fd, buf, buf_size, 0, (struct sockaddr *)ser_addr, ser_len);
    d_printf("sent RRQ packet; MSG: %d\n", msg);
    free(buf);

    // enter recv loop
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        printf("open file error!\n");
        return -1;
    }

    char *recv_buf = malloc(TFTP_MAX_SIZE * 2);
    int recv_count = 0;
    unsigned short block = 1;
    // NOTICE: send ACK to this address
    struct sockaddr_in ser_addr_new = {0};
    socklen_t ser_addr_len = sizeof(ser_addr_new);

    int size = 0;
    time_t start_time = time(NULL);
    struct timeval start_tv;
    gettimeofday(&start_tv, NULL);

    struct tftp_packet ack_pkt = {0};
    while (1)
    {
        // One BIG problem: how to restore the data received properly?
        // CURRENT BUG: recv_count == -1, WHY?
        recv_count = recvfrom(client_fd, recv_buf, TFTP_MAX_SIZE * 2, 0, (struct sockaddr *)&ser_addr_new, &ser_addr_len);

        if (recv_count < 0 || get_tftp_packet_type(recv_buf) != DATA || get_tftp_packet_block(recv_buf) != block)
        {

            if (block == 0 && send_count < 2)
            {
                free(recv_buf);
                close(fd);
                send_count++;
                goto start_rrq;
            }
            // send error msg
            if (send_count < 2)
            {
                send_count++;
                block--;
                goto send_ack;
            }

            unsigned short type = get_tftp_packet_type(recv_buf);
            unsigned short recv_block = get_tftp_packet_block(recv_buf);

            d_printf("recv_count: %d\n", recv_count);
            d_printf("type: %d\n", type);
            d_printf("recv_block: %d\n", recv_block);
            d_printf("current_block: %d\n", block);
            printf("recv error!\n");

            if (get_tftp_packet_block(recv_buf) < block)
            {
                d_printf("recv old data, ignore!\n");
                free(recv_buf);
                continue;
            }

            free(recv_buf);
            close(fd);
            return -1;
        }
        else if (recv_count > 0)
        {
            size += recv_count - 4;
            // write data to file
            // well, actually, we should check after write
            int write_msg = write(fd, recv_buf + sizeof(unsigned short) * 2, recv_count - sizeof(unsigned short) * 2);
            if (write_msg < 0)
            {
                printf("write file error!\n");
                free(recv_buf);
                close(fd);
                return -1;
            }

            d_printf("write data to file, block: %u , data_len: %ld\n", block, recv_count - sizeof(unsigned short) * 2);
        }
        send_count = 0;
        // NOTICE: remember to deal with recv_count == 0
        // which indicates end of the whole program
        // send ACK
    send_ack:
        memset(&ack_pkt, 0, sizeof(struct tftp_packet));
        ack_pkt.opcode = htons(ACK);
        ack_pkt.u.ack.block = htons(block);

        char *ack_buf = NULL;
        int ack_buf_size = make_tftp_packet(&ack_pkt, ACK, &ack_buf);
        // d_printf("ack_buf_size: %d\n", ack_buf_size);

        int ack_msg = sendto(client_fd, ack_buf, ack_buf_size, 0, (struct sockaddr *)&ser_addr_new, ser_addr_len);

        if (recv_count == 0 || recv_count < TFTP_MAX_SIZE)
        {
            printf("recv data end\n");
            break;
        }
        free(ack_buf);
        block++;
    }

    free(recv_buf);
    close(fd);

    struct timeval end_tv;
    gettimeofday(&end_tv, NULL);

    printf("recv %d bytes, time: %ldsec\n", size, time(NULL) - start_time);

    printf("speed: %f KB/s\n", (float)size / 1024 / (end_tv.tv_sec - start_tv.tv_sec + (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0));

    return 0;
}

int send_wrq(int client_fd, struct sockaddr_in *ser_addr, char *filename, char _mode)
{

    // build WRQ packet
    struct tftp_packet packet = {0};
start_wrq:
    memset(&packet, 0, sizeof(packet));
    packet.opcode = htons(WRQ);
    packet.u.request.filename = filename;
    packet.u.request.filename_len = strlen(filename) + 1;

    char netascii_mode[] = "netascii";
    char octet_mode[] = "octet";
    if (_mode == 1)
        packet.u.request.mode = &netascii_mode[0];
    else
        packet.u.request.mode = &octet_mode[0];
    // char mode[] = (_mode==1)?"netascii":"octet";
    packet.u.request.mode_len = (_mode == 1) ? strlen(netascii_mode) + 1 : strlen(octet_mode) + 1;

    char *buf = NULL;

    int buf_size = make_tftp_packet(&packet, WRQ, &buf);
    d_printf("buf_size: %d\n", buf_size);
    print_tftp_packet(buf, buf_size);

    socklen_t ser_len;
    ser_len = sizeof(*ser_addr);

    int msg = sendto(client_fd, buf, buf_size, 0, (struct sockaddr *)ser_addr, ser_len);
    d_printf("sent WRQ packet; MSG: %d\n", msg);
    free(buf);

    // enter send loop
    int fd = open(filename, O_RDWR | O_CREAT, 0666);
    if (fd < 0)
    {
        printf("open file error!\n");
        return -1;
    }

    char *recv_buf = malloc(TFTP_MAX_SIZE * 2);
    int recv_count = 0;
    // NOTICE: in WRQ, the first block received is 0
    unsigned short block = 0;
    struct sockaddr_in ser_addr_new = {0};
    socklen_t ser_addr_len = sizeof(ser_addr_new);
    /*
    // signal for exceeding time
    struct sigaction sa_alarm;
    sa_alarm.sa_handler = sig_alarm_handler;
    sa_alarm.sa_flags = SA_RESETHAND;
    */
    int end_flag = 0;
    int size = 0;
    time_t start_time = time(NULL);

    struct timeval start_tv;
    gettimeofday(&start_tv, NULL);

    while (1)
    {
        memset(recv_buf, 0, TFTP_MAX_SIZE * 2);
        // sigaction(SIGALRM, &sa_alarm, NULL);
        // alarm(5);
        recv_count = recvfrom(client_fd, recv_buf, TFTP_MAX_SIZE * 2, 0, (struct sockaddr *)&ser_addr_new, &ser_addr_len);

        // handling error messages
        if (recv_count < 0 || get_tftp_packet_type(recv_buf) != ACK || get_tftp_packet_block(recv_buf) != block)
        {
            // send error msg

            unsigned short type = get_tftp_packet_type(recv_buf);
            unsigned short recv_block = get_tftp_packet_block(recv_buf);

            d_printf("recv_count: %d\n", recv_count);
            d_printf("type: %d\n", type);
            d_printf("recv_block: %d\n", recv_block);
            d_printf("current_block: %d\n", block);

            printf("recv error!\n");
            free(recv_buf);
            close(fd);
            return -1;
        }
        else
        {
            // read data from file
            char *data_buf = malloc(TFTP_MAX_SIZE);
            int data_len = read(fd, data_buf, TFTP_MAX_SIZE);
            if (data_len < 0)
            {
                if (block == 1)
                    printf("Send end\n");
                else
                    printf("read file error! block: %u\n", block);
                free(recv_buf);
                free(data_buf);
                close(fd);
                return -1;
            }
            else if (data_len < TFTP_MAX_SIZE && data_len != 0)
            {
                printf("read file end! block: %u\n", block);
                end_flag = 1;
            }
            else if (data_len == 0)
            {
                // send a data packet with 0 length
                d_printf("read file end\n");
                if (end_flag == 1)
                {
                    free(recv_buf);
                    free(data_buf);
                    close(fd);
                    printf("recv %d bytes, time: %ldsec\n", size, time(NULL) - start_time);

                    struct timeval end_tv;
                    gettimeofday(&end_tv, NULL);
                    printf("speed: %f KB/s\n", (float)size / 1024 / (end_tv.tv_sec - start_tv.tv_sec + (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0));

                    return 0;
                }
                else
                {
                    end_flag = 1;
                }
            }

            size += data_len;

            block++;

            struct tftp_packet data_pkt = {0};
            data_pkt.opcode = htons(DATA);
            data_pkt.u.data.block = htons(block);
            data_pkt.u.data.data = data_buf;
            data_pkt.u.data.data_len = data_len;

            char *data_buf_new = NULL;
            int data_buf_size = make_tftp_packet(&data_pkt, DATA, &data_buf_new);
            d_printf("data_buf_size: %d\n", data_buf_size);
            if (data_buf_size < 0)
            {
                d_printf("make data packet error!\n");
                free(recv_buf);
                close(fd);
                return -1;
            }

            int data_msg = sendto(client_fd, data_buf_new, data_buf_size, 0, (struct sockaddr *)&ser_addr_new, ser_addr_len);
            if (data_msg < 0)
            {
                printf("send data error!\n");
                free(recv_buf);
                free(data_buf_new);
                close(fd);
                return -1;
            }
        }
    }

    printf("recv %d bytes, time: %ldsec\n", size, time(NULL) - start_time);
    
    struct timeval end_tv;
    gettimeofday(&end_tv, NULL);
    printf("speed: %f KB/s\n", (float)size / 1024 / (end_tv.tv_sec - start_tv.tv_sec + (end_tv.tv_usec - start_tv.tv_usec) / 1000000.0));

    return 0;
}

void _dprintf(const char *fmt, ...)
{
    return;
}