#include "PikaVM.h"
#include "PikaPlatform_socket.h"
#include "_socket_socket.h"

#if !PIKASCRIPT_VERSION_REQUIRE_MINIMUN(1, 12, 0)
#error "This library requires PikaScript version 1.12.0 or higher"
#endif

void _socket_socket__init(PikaObj* self) {
    int family = obj_getInt(self, "family");
    int type = obj_getInt(self, "type");
    int protocol = obj_getInt(self, "protocol");
    int sockfd = 0;
#ifdef _WIN32
    pika_platform_init_winsock();
#endif
    sockfd = __platform_socket(family, type, protocol);
    if (sockfd < 0) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "socket error");
        return;
    }
    obj_setInt(self, "sockfd", sockfd);
    obj_setInt(self, "blocking", 1);
}

void _socket_socket__close(PikaObj* self) {
    int sockfd = obj_getInt(self, "sockfd");
    __platform_close(sockfd);
#ifdef _WIN32
    pika_platform_cleanup_winsock();
#endif
}

void _socket_socket__send(PikaObj* self, Arg* data) {
    uint8_t* data_send = NULL;
    int len = 0;
    if (arg_getType(data) == ARG_TYPE_STRING) {
        data_send = (uint8_t*)arg_getStr(data);
        len = strGetSize((char*)data_send);
    }

    if (arg_getType(data) == ARG_TYPE_BYTES) {
        data_send = (uint8_t*)arg_getBytes(data);
        len = arg_getBytesSize(data);
    }

    int sockfd = obj_getInt(self, "sockfd");
    int ret = 0;
    ret = __platform_send(sockfd, data_send, len, 0);
    if (ret < 0) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "send error");
        return;
    }
}

void _socket_socket__accept(PikaObj* self) {
    int sockfd = obj_getInt(self, "sockfd");
    int client_sockfd = 0;
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    pika_GIL_EXIT();
    client_sockfd = __platform_accept(sockfd, (struct sockaddr*)&client_addr,
                                      &client_addr_len);
    pika_GIL_ENTER();
    if (client_sockfd < 0) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "accept error");
        return;
    }
#ifdef _WIN32
    pika_platform_init_winsock();
#endif
    obj_setInt(self, "client_sockfd", client_sockfd);
    obj_setStr(self, "client_addr", inet_ntoa(client_addr.sin_addr));
}

Arg* _socket_socket__recv(PikaObj* self, int num) {
    int sockfd = obj_getInt(self, "sockfd");
    int ret = 0;
    uint8_t* data_recv = NULL;
    Arg* res = arg_newBytes(NULL, num);
    data_recv = arg_getBytes(res);
    pika_GIL_EXIT();
    ret = __platform_recv(sockfd, data_recv, num, 0);
    pika_GIL_ENTER();
    if (ret <= 0) {
        if (obj_getInt(self, "blocking")) {
            obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
            if (ret == 0) {
                obj_setSysOut(self, "connect closed");
            } else {
                obj_setSysOut(self, "recv error: %d", ret);
            }
            arg_deinit(res);
            return NULL;
        } else {
            Arg* res_r = arg_newBytes(NULL, 0);
            arg_deinit(res);
            return res_r;
        }
    } else {
        if (ret < num) {
            uint8_t* res_buff = NULL;
            Arg* res_r = arg_newBytes(NULL, ret);
            res_buff = arg_getBytes(res_r);
            pika_platform_memcpy(res_buff, data_recv, ret);
            arg_deinit(res);
            return res_r;
        }
    }
    return res;
}

void _socket_socket__listen(PikaObj* self, int num) {
    int sockfd = obj_getInt(self, "sockfd");
    __platform_listen(sockfd, num);
}

void _socket_socket__connect(PikaObj* self, char* host, int port) {
    int sockfd = obj_getInt(self, "sockfd");
    struct addrinfo hints, *res;
    __platform_memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;

    if (__platform_getaddrinfo(host, NULL, &hints, &res) != 0) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "Failed to resolve domain name");
        return;
    }

    struct sockaddr_in* server_addr = (struct sockaddr_in*)res->ai_addr;
    server_addr->sin_port = pika_platform_htons(port);

    pika_GIL_EXIT();
    int err = __platform_connect(sockfd, (struct sockaddr*)server_addr,
                                 sizeof(*server_addr));
    pika_GIL_ENTER();
    if (0 != err) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "connect error, err = %d", err);
        return;
    }
    if (obj_getInt(self, "blocking") == 0) {
        int flags = pika_platform_fcntl(sockfd, F_GETFL, 0);
        if (__platform_fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) == -1) {
            obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
            obj_setSysOut(self, "Unable to set socket non blocking");
            return;
        }
    }
    __platform_freeaddrinfo(res);
}

void _socket_socket__bind(PikaObj* self, char* host, int port) {
    int sockfd = obj_getInt(self, "sockfd");
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(host);
    int res = __platform_bind(sockfd, (struct sockaddr*)&server_addr,
                              sizeof(server_addr));
    if (res < 0) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "bind error");
        return;
    }
}

char* _socket__gethostname(PikaObj* self) {
    char hostname_buff[128] = {0};
    char* hostname = (char*)hostname_buff;
#ifdef _WIN32
    pika_platform_init_winsock();
#endif
    pika_platform_gethostname(hostname_buff, 128);
#ifdef _WIN32
    pika_platform_cleanup_winsock();
#endif
    return obj_cacheStr(self, hostname);
}

char* _socket__gethostbyname(PikaObj* self, char* host) {
    struct hostent* host_entry;
    char* ip = NULL;
#ifdef _WIN32
    pika_platform_init_winsock();
#endif
    host_entry = pika_platform_gethostbyname(host);
    if (host_entry == NULL) {
        obj_setErrorCode(self, PIKA_RES_ERR_RUNTIME_ERROR);
        obj_setSysOut(self, "gethostbyname error");
        return NULL;
    }
    ip =
        pika_platform_inet_ntoa(*((struct in_addr*)host_entry->h_addr_list[0]));
#ifdef _WIN32
    pika_platform_cleanup_winsock();
#endif
    return obj_cacheStr(self, ip);
}

void _socket_socket__setblocking(PikaObj* self, int sta) {
    obj_setInt(self, "blocking", sta);
}
