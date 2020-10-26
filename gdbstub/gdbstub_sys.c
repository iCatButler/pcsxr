#include "gdbstub_sys.h"
#include "gdbstub.h"
#include "libpcsxcore/socket.h"
#include "libpcsxcore/r3000a.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>

#ifdef _POSIX_VERSION
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>

static pthread_t thread;
static mqd_t in_queue = -1, out_queue = -1;
#endif

static int server_socket = -1, client_socket = -1;
static enum {
    PAUSED,
} state;

static void update_regs(struct dbg_state *const dbg_state)
{
    dbg_state->registers[DBG_CPU_MIPS_I_REG_ZERO] = 0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_AT] = psxRegs.GPR.n.at;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_V0] = psxRegs.GPR.n.v0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_V1] = psxRegs.GPR.n.v1;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_A0] = psxRegs.GPR.n.a0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_A1] = psxRegs.GPR.n.a1;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_A2] = psxRegs.GPR.n.a2;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_A3] = psxRegs.GPR.n.a3;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_T0] = psxRegs.GPR.n.t0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T1] = psxRegs.GPR.n.t1;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T2] = psxRegs.GPR.n.t2;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T3] = psxRegs.GPR.n.t3;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T4] = psxRegs.GPR.n.t4;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T5] = psxRegs.GPR.n.t5;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T6] = psxRegs.GPR.n.t6;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T7] = psxRegs.GPR.n.t7;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_S0] = psxRegs.GPR.n.s0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S1] = psxRegs.GPR.n.s1;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S2] = psxRegs.GPR.n.s2;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S3] = psxRegs.GPR.n.s3;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S4] = psxRegs.GPR.n.s4;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S5] = psxRegs.GPR.n.s5;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S6] = psxRegs.GPR.n.s6;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S7] = psxRegs.GPR.n.s7;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_T8] = psxRegs.GPR.n.t8;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_T9] = psxRegs.GPR.n.t9;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_K0] = psxRegs.GPR.n.k0;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_K1] = psxRegs.GPR.n.k1;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_GP] = psxRegs.GPR.n.gp;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_SP] = psxRegs.GPR.n.sp;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_S8] = psxRegs.GPR.n.s8;
    dbg_state->registers[DBG_CPU_MIPS_I_REG_RA] = psxRegs.GPR.n.ra;

    dbg_state->registers[DBG_CPU_MIPS_I_REG_SR] = psxRegs.CP0.n.Status;
	dbg_state->registers[DBG_CPU_MIPS_I_REG_LO] = psxRegs.GPR.r[32];
	dbg_state->registers[DBG_CPU_MIPS_I_REG_HI] = psxRegs.GPR.r[33];
	dbg_state->registers[DBG_CPU_MIPS_I_REG_BAD] = psxRegs.CP0.n.BadVAddr;
	dbg_state->registers[DBG_CPU_MIPS_I_REG_CAUSE] = psxRegs.CP0.n.Cause;
	dbg_state->registers[DBG_CPU_MIPS_I_REG_PC] = psxRegs.pc;
}

static int exit_loop;

int dbg_sys_getc(void)
{
    while (1) {
        char packet;
        size_t len = sizeof packet;
        const enum read_socket_err err = ReadSocket(client_socket, &packet, &len);

#ifdef _POSIX_VERSION
        if (exit_loop)
            pthread_exit(NULL);
#endif

        switch (err) {
            case READ_SOCKET_OK:
                return packet;

            case READ_SOCKET_SHUTDOWN: {
                struct msg msg;

                printf("gdb shutdown\n");
                client_socket = 0;
                msg.type = MSG_TYPE_SHUTDOWN;

                    if (mq_send(out_queue, (const char *)&msg, sizeof msg, 0))
                        perror("dbg_sys_getc() mq_send()");
                return EOF;
            }

            case READ_SOCKET_ERR_INVALID_ARG:
                /* Fall through. */
            case READ_SOCKET_ERR_RECV:
                /* Fall through. */
            default:
                break;
        }
    }
}

int dbg_sys_putchar(int ch)
{
    WriteSocket(client_socket, (const char *)&ch, sizeof (char));
}

int dbg_sys_mem_readb(address addr, char *val)
{
    *val = psxMemRead8(addr);
    return 0;
}

int dbg_sys_mem_writeb(address addr, char val)
{
    psxMemWrite8(addr, val);
    return 0;
}

#ifdef _POSIX_VERSION
static int wait_hit_or_break(struct msg *msg)
{
    do {
        int ret = mq_receive(in_queue, (char *)msg, sizeof *msg, 0);

        if (exit_loop)
            return 1;

        if (ret < 0 && errno == EAGAIN) {
            /* Breakpoint has not been hit yet, look for incoming messages from gdb. */
            char packet;
            size_t len = sizeof packet;
            const enum read_socket_err err = ReadSocket(client_socket, &packet, &len);

            switch (err) {
                case READ_SOCKET_OK:
                    if (len && packet == 0x03) {
                        DEBUG_PRINT("received break\n");
                        psxCpu->Halt();
                        return wait_hit_or_break(msg);
                    }

                    break;

                case READ_SOCKET_SHUTDOWN:
                    printf("gdb shutdown\n");
                    client_socket = 0;
                    msg->type = MSG_TYPE_SHUTDOWN;

                    if (mq_send(out_queue, (const char *)msg, sizeof *msg, 0))
                        perror("wait_hit_or_break() mq_send()");

                    return EOF;

                case READ_SOCKET_ERR_INVALID_ARG:
                    /* Fall through. */
                case READ_SOCKET_ERR_RECV:
                    /* Fall through. */
                default:
                    break;
            }
        }
        else if (msg->type != MSG_TYPE_HIT) {
            fprintf(stderr, "unexpected msg.type %d\n", msg->type);
            return 1;
        }
        else
            return 0;
    } while (1);

    return 1;
}
#endif

#ifdef _POSIX_VERSION
int dbg_sys_continue(void)
{
    struct msg msg;

    msg.type = MSG_TYPE_CONTINUE;

    if (mq_send(out_queue, (const char *)&msg, sizeof msg, 0)) {
        perror("dbg_sys_continue(): mq_send()");
        return 1;
    }

    return wait_hit_or_break(&msg);
}
#endif

#ifdef _POSIX_VERSION
int dbg_sys_step(void)
{
    struct msg msg;

    msg.type = MSG_TYPE_STEP;

    if (mq_send(out_queue, (const char *)&msg, sizeof msg, 0)) {
        perror("dbg_sys_step(): mq_send()");
    }

    return wait_hit_or_break(&msg);
}
#endif

#ifdef _POSIX_VERSION
static int wait_ack(struct msg *msg)
{
    int ret;

    do {
        if (exit_loop)
            return 1;

        ret = mq_receive(in_queue, (char *)msg, sizeof *msg, 0);
    } while (ret < 0 && errno == EAGAIN);

    if (msg->type != MSG_TYPE_ACK) {
        fprintf(stderr, "unexpected msg.type %d\n", msg->type);
        return 1;
    }

    return 0;
}
#endif

#ifdef _POSIX_VERSION
int dbg_sys_breakpoint(address addr)
{
    struct msg msg;

    msg.type = MSG_TYPE_BREAKPOINT;
    msg.data.breakpoint.addr = addr;

    if (mq_send(out_queue, (const char *)&msg, sizeof msg, 0)) {
        perror("dbg_sys_breakpoint(): mq_send()");
    }

    return wait_ack(&msg);
}
#endif

#ifdef _POSIX_VERSION
int dbg_sys_del_breakpoint(address addr)
{
    struct msg msg;

    msg.type = MSG_TYPE_REMOVE_BREAKPOINT;

    if (mq_send(out_queue, (const char *)&msg, sizeof msg, 0)) {
        perror("dbg_sys_breakpoint(): mq_send()");
    }

    return wait_ack(&msg);
}
#endif

#ifdef _POSIX_VERSION
static int queue_create(void)
{
    struct mq_attr attr;

    attr.mq_msgsize = sizeof (struct msg);
    attr.mq_flags = 0;
    attr.mq_maxmsg = 4;

    mq_unlink("/pcsxrin");
    in_queue = mq_open("/pcsxrin", O_CREAT | O_RDWR | O_EXCL | O_NONBLOCK, 0600, &attr);

    mq_unlink("/pcsxrout");
    out_queue = mq_open("/pcsxrout", O_CREAT | O_RDWR | O_EXCL, 0600, &attr);

    if ((out_queue < 0) || (in_queue < 0)) {
        perror("mq_open()");
        return 1;
    }

    return 0;
}
#endif

static void *loop(void *const args)
{
    struct dbg_state dbg_state = {0};

    SetsBlock(server_socket);

    if ((client_socket = GetClient(server_socket, 1)) < 0) {
        fprintf(stderr, "GetClient() failed\n");
        return NULL;
    }

    SetsNonblock(client_socket);

    printf("Accepted gdb connection\n");

    while (!exit_loop) {
        update_regs(&dbg_state);
        dbg_main(&dbg_state);
    }

    return NULL;
}

#ifdef _POSIX_VERSION
static void start_thread(void)
{
    if (pthread_create(&thread, NULL, loop, NULL))
        perror("could not start gdb server thread");
}
#endif

#ifdef _POSIX_VERSION
static void stop_thread(void)
{
    if (pthread_join(thread, NULL))
        perror("pthread_join()");

    mq_unlink("/pcsxrin");
    mq_unlink("/pcsxrout");
}
#endif

#ifdef _POSIX_VERSION
void gdbstub_sys_recv(struct msg *msg)
{
    while (out_queue < 0)
        ;

    {
        const ssize_t sz = mq_receive(out_queue, (char *)msg, sizeof *msg, 0);

        if (sz < 0)
            perror("mq_receive");
    }
}
#endif

#ifdef _POSIX_VERSION
void gdbstub_sys_send(const struct msg *msg)
{
    if (mq_send(in_queue, (const char *)msg, sizeof *msg, 0)) {
        perror("dbg_sys_send(): mq_send()");
    }
}
#endif

void dbg_stop(void)
{
    exit_loop = 1;
    stop_thread();

    if (client_socket > 0) {
        StopServer(client_socket);
        printf("Terminated active gdb connection\n");
    }

    if (server_socket > 0) {
        StopServer(server_socket);
        printf("Closed gdb server\n");
    }
}

void dbg_start(void)
{
    if (server_socket > 0) {
        fprintf(stderr, "gdb server already started\n");
        return;
    }
    else {
        const unsigned short port = 3333;

        server_socket = StartServer(port);

        if (server_socket > 0) {
            printf("GDB server started on port %hu.\n", port);
            if (queue_create())
                fprintf(stderr, "could not create gdb stub internal queues\n");
            else
                start_thread();
        }
        else
            fprintf(stderr, "could not start GDB server\n");
    }
}
