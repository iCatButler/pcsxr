/*  Pcsx - Pc Psx Emulator
 *  Copyright (C) 1999-2003  Pcsx Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifdef _WIN32
#include <winsock2.h>
#endif

#include "psxcommon.h"
#include "socket.h"
#include "config.h"

#ifndef _WIN32
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#endif

int StartServer(unsigned short port) {
    struct in_addr localhostaddr;
    struct sockaddr_in localsocketaddr;
    int ret;

#ifdef _WIN32
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0)
        return -1;
#endif

    ret = socket(AF_INET, SOCK_STREAM, 0);

#ifdef _WIN32
    if (ret == INVALID_SOCKET)
        return -1;
#else
    if (ret == -1)
        return ret;
#endif

    SetsNonblock(ret);

    memset((void *)&localhostaddr, 0, sizeof(localhostaddr));
    memset(&localsocketaddr, 0, sizeof(struct sockaddr_in));

#ifdef _WIN32
    localhostaddr.S_un.S_addr = htonl(INADDR_ANY);
#else
    localhostaddr.s_addr = htonl(INADDR_ANY);
#endif
    localsocketaddr.sin_family = AF_INET;
    localsocketaddr.sin_addr = localhostaddr;
    localsocketaddr.sin_port = htons(port);

    if (bind(ret, (struct sockaddr *) &localsocketaddr, sizeof(localsocketaddr)) < 0)
        return -1;

    if (listen(ret, 1) != 0)
        return -1;

    return ret;
}

void StopServer(int s_socket) {
#ifdef _WIN32
    shutdown(s_socket, SD_BOTH);
    closesocket(s_socket);
    WSACleanup();
#else
    shutdown(s_socket, SHUT_RDWR);
    close(s_socket);
#endif
}

int GetClient(int s_socket, int blocking) {
    int new_socket = accept(s_socket, NULL, NULL);

#ifdef _WIN32
    if (new_socket == INVALID_SOCKET)
        return -1;
#else
    if (new_socket == -1)
        return -1;
#endif

#ifndef _WIN32
    if (!blocking)
    {
        int flags;
        flags = fcntl(new_socket, F_GETFL, 0);
        fcntl(new_socket, F_SETFL, flags | O_NONBLOCK);
        return new_socket;
    }
#endif
}

void CloseClient(int client_socket) {
    if (client_socket) {
#ifdef _WIN32
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
#else
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
#endif
    }
}

int HasClient(int client_socket) {
    return client_socket > 0;
}

#ifdef _WIN32

static enum read_socket_err ReadSocketOS(SOCKET client_socket, char *buf, size_t *const len)
{
    const int res = recv(client_socket, buf, len, 0);

    switch (res)
    {
        case SOCKET_ERROR:
            return READ_SOCKET_ERR_RECV;

        case 0:
            return READ_SOCKET_SHUTDOWN;

        default:
            *len = res;

            break;
    }

    return READ_SOCKET_OK;
}

#elif defined(_POSIX_VERSION)

static enum read_socket_err ReadSocketOS(int client_socket, char *buf, size_t *const len)
{
    const ssize_t res = recv(client_socket, buf, *len, 0);

    switch (res)
    {
        case -1:
            return READ_SOCKET_ERR_RECV;

        case 0:
            return READ_SOCKET_SHUTDOWN;

        default:
            *len = res;
            break;
    }

    return READ_SOCKET_OK;
}

#endif /* _WIN32 */

enum read_socket_err ReadSocket(int client_socket, char *buf, size_t *const len) {
    char * endl;

    if (!client_socket || !buf || !len || !*len)
        return READ_SOCKET_ERR_INVALID_ARG;

    return ReadSocketOS(client_socket, buf, len);

#if 0
#ifdef _WIN32
    if (r == SOCKET_ERROR)
#else
    if (r == -1)
#endif
    {
        if (ptr == 0)
            return -1;
        r = 0;
    }
    ptr += r;
    tbuf[ptr] = 0;

    endl = strstr(tbuf, "\r\n");

    if (endl) {
        r = endl - tbuf;
        strncpy(buffer, tbuf, r);

        r += 2;
        memmove(tbuf, tbuf + r, 512 - r);
        ptr -= r;
        memset(tbuf + r, 0, 512 - r);
        r -= 2;

    } else {
        r = 0;
    }

    buffer[r] = 0;
#endif
}

int RawReadSocket(int client_socket, char *buffer, size_t len) {
#if 0
    int r = 0;
    int mlen = len < ptr ? len : ptr;

    if (!client_socket)
        return -1;

    if (ptr) {
        memcpy(buffer, tbuf, mlen);
        ptr -= mlen;
        memmove(tbuf, tbuf + mlen, 512 - mlen);
    }

    if (len - mlen)
        r = recv(client_socket, buffer + mlen, len - mlen, 0);

    if (r == 0) {
        if (!ptr)
            return 0;
    }
#ifdef _WIN32
    if (r == SOCKET_ERROR)
#else
    if (r == -1)
#endif
    {
        if (ptr == 0)
            return -1;
        r = 0;
    }

    r += mlen;

    return r;
#endif
}

void WriteSocket(int client_socket, const void *buffer, size_t len) {
    if (client_socket <= 0)
        return;

    if (send(client_socket, buffer, len, 0) == -1) {
        perror("send():");
    }
}

void SetsBlock(int s_socket) {
#ifdef _WIN32
    u_long b = 0;
    ioctlsocket(s_socket, FIONBIO, &b);
#else
    int flags = fcntl(s_socket, F_GETFL, 0);
    fcntl(s_socket, F_SETFL, flags & ~O_NONBLOCK);
#endif
}

void SetsNonblock(int s_socket) {
#ifdef _WIN32
    u_long b = 1;
    ioctlsocket(s_socket, FIONBIO, &b);
#else
    int flags = fcntl(s_socket, F_GETFL, 0);
    fcntl(s_socket, F_SETFL, flags | O_NONBLOCK);
#endif
}
