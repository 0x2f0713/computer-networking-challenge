#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define HOST "112.137.129.129"
#define PORT 27001

#define PKT_HELLO 0
#define PKT_CALC  1
#define PKT_RESULT 2
#define PKT_BYE 3
#define PKT_FLAG 4

#define HEADER_LEN sizeof(header_t)

#define SOCKET_OK 1
#define SOCKET_NOK -1

typedef struct header_s {
    uint32_t type;
    uint32_t length;
} header_t;

char MSV[8] = "21020360";
int rst = 0;
char req_buff[1024], res_buff[1024];
size_t buff_len;
int fd;

void encodeHello(char* buff, size_t* buff_len) {
    header_t header = {
        type : PKT_HELLO,
        length : 8
    };
    memcpy(buff, &header, HEADER_LEN);
    memcpy(buff + HEADER_LEN, &MSV, 8);
    *buff_len = 16;
}

void encodeResult(char* buff, size_t* buff_len, uint32_t data) {
    header_t header = {
        type : PKT_RESULT,
        length : 4
    };
    memcpy(buff, &header, HEADER_LEN);
    memcpy(buff + HEADER_LEN, &data, sizeof(data));
}



int decodeCalc(char* buff) {
    int32_t a, b, res;
    memcpy(&a, buff + HEADER_LEN, 4);
    memcpy(&b, buff + HEADER_LEN + 4, 4);
    res = a + b;
    printf("a: %02x, b: %02x, res = %02x \n", a, b, res);
    encodeResult(req_buff, &buff_len, res);
    return send(fd, req_buff, 12, 0);
}

int decodeFlag(char *buff) {
    char flag[1024];
    memcpy(flag, buff + 8, (int32_t)*(buff + 4));
    printf("Flag: %s\n", flag);
}

int process_pdu(char* buff) {
    int ret = SOCKET_NOK;
    int32_t type;
    memcpy(&type, buff, 4);
    switch (type)
    {
    case PKT_CALC:
        ret = decodeCalc(buff);
        break;
    
    case PKT_BYE:
        rst = 1;
        break;
    
    case PKT_FLAG:
        decodeFlag(buff);
        rst = 1;
        break;

    default:
        break;
    }
    return ret;
}
int main() {
    struct sockaddr_in dest;
    int ret = SOCKET_OK;
    if (inet_pton(AF_INET, HOST, &dest.sin_addr)
        <= 0) {
        printf(
            "\nInvalid address/ Address not supported \n");
        return -1;
    }
    dest.sin_family = AF_INET;
    dest.sin_port = htons(PORT);
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if ((connect(fd, (struct sockaddr*)&dest,
                   sizeof(dest)))
        < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
    encodeHello(req_buff, &buff_len);
    send(fd, req_buff, 16, 0);
    while (!rst && ret != SOCKET_NOK)
    {
        if (recv(fd, res_buff, 1024, 0) > -1)
        {
            ret = process_pdu(res_buff);
            memset(res_buff, 0, sizeof(res_buff));
        }
    }
    close(fd);

}