#include "gdbstub_sys.h"
#include "gdbstub.h"
#include "libpcsxcore/socket.h"
#include "libpcsxcore/r3000a.h"
#include <stdio.h>

static int server_socket, client_socket;

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

void dbg_sys_process(void)
{
    static struct dbg_state dbg_state;
    update_regs(&dbg_state);
    dbg_main(&dbg_state);
}

int dbg_sys_getc(void)
{
    while (1) {
        char packet;
        size_t len = sizeof packet;
        const enum read_socket_err err = ReadSocket(client_socket, &packet, &len);

        switch (err) {
            case READ_SOCKET_OK:
                return packet;

            case READ_SOCKET_SHUTDOWN:
                client_socket = 0;
                return EOF;

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

int dbg_sys_continue(void)
{
    return 0;
}

int dbg_sys_step(void)
{
    return 0;
}

void dbg_start(void)
{
    const unsigned short port = 3333;

    if (server_socket > 0)
        StopServer(server_socket);

    server_socket = StartServer(port);

    if (server_socket > 0)
        printf("GDB server started on port %hu.\n", port);
    else
        fprintf(stderr, "Could not start GDB server\n");
}
