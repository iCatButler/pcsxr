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

#ifndef __SOCKET_H__
#define __SOCKET_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

enum read_socket_err
{
    READ_SOCKET_OK,
    READ_SOCKET_ERR_INVALID_ARG,
    READ_SOCKET_ERR_RECV,
    READ_SOCKET_SHUTDOWN
};

int StartServer(unsigned short port);
void StopServer(int s_socket);

int GetClient(int s_socket);
void CloseClient(int client_socket);

int HasClient(int client_socket);

enum read_socket_err ReadSocket(int client_socket, char *buffer, size_t *len);
int RawReadSocket(int client_socket, char *buffer, size_t len);
void WriteSocket(int client_socket, const char *buffer, size_t len);

void SetsBlock(int s_socket);
void SetsNonblock(int s_socket);

#ifdef __cplusplus
}
#endif
#endif
