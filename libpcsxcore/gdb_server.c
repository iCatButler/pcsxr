#include "socket.h"
#include "psxcommon.h"
#include "misc.h"
#include "system.h"
#include "dynstr.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>

static int server_socket, client_socket;
static int debugger_active, resetting, reset, paused, ack_expected;

enum { PACKET_SIZE = 256 };

void GdbStartServer(void)
{
    enum { PORT = 3333 };

    if (server_socket > 0) {
        GdbStopServer();
    }

    server_socket = StartServer(PORT);

    if (server_socket > 0) {
        printf("GDB server started on port %hu\n", PORT);
        debugger_active = 1;
    }
    else
    {
        fprintf(stderr, "Could not start GDB server\n");
    }
}

int GdbServerRunning(void)
{
    return server_socket > 0;
}

void GdbStopServer(void)
{
}

static void ack(struct dynstr *const reply)
{
    dynstr_append(reply, "OK");
}

static void nack(struct dynstr *const reply, const int err)
{
    dynstr_append(reply, "E %02X", err);
}

static void HandlePacket(char *const packet, const size_t len)
{
    struct dynstr reply;
    const char *c = packet;

    dynstr_init(&reply);

    if (strstr(packet, "qSupported")) {
        dynstr_append(&reply, "PacketSize=%x;swbreak+;hwbreak+", PACKET_SIZE - 1);
    }
    else {
        fprintf(stderr, "Unexpected packet \"%s\"\n", packet);
        return;
    }

    printf("gdb <- \"%s\"\n", reply.str);
    WriteSocket(client_socket, reply.str, reply.len);
    dynstr_free(&reply);
}

static void ProcessCommands(void)
{
    if (HasClient(client_socket)) {
        char packet[PACKET_SIZE];
        size_t len = sizeof packet;
        const enum read_socket_err err = ReadSocket(client_socket, packet, &len);

        switch (err)
        {
            case READ_SOCKET_OK:
                if (len && len < sizeof packet) {
                    /* gdb apparently does not send null-terminated strings. */
                    packet[len] = '\0';
                    printf("gdb -> \"%s\"\n", packet);
                    HandlePacket(packet, len);
                }
                break;

            case READ_SOCKET_ERR_INVALID_ARG:
                /* Fall through. */
            case READ_SOCKET_ERR_RECV:
                /* Fall through. */
            case READ_SOCKET_SHUTDOWN:
                /* Fall through. */
            default:
                return;
        }
    }
}

void GdbServerProcessDebug(void)
{
    ProcessCommands();
}

void GdbServerVSync(void)
{
    if (!debugger_active || resetting)
        return;

    if (reset) {
        resetting = 1;
        SysReset();
        if (reset == 2)
            LoadCdrom();
        reset = resetting = 0;
        return;
    }

    if (client_socket < 1) {
        client_socket = GetClient(server_socket);
    }

    ProcessCommands();
}
