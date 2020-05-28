#ifndef GDBSTUB_SYS_H
#define GDBSTUB_SYS_H

typedef unsigned int address;

enum DBG_REGISTER {
	DBG_CPU_MIPS_I_REG_ZERO,
	DBG_CPU_MIPS_I_REG_AT,
	DBG_CPU_MIPS_I_REG_V0,
	DBG_CPU_MIPS_I_REG_V1,
	DBG_CPU_MIPS_I_REG_A0,
	DBG_CPU_MIPS_I_REG_A1,
	DBG_CPU_MIPS_I_REG_A2,
	DBG_CPU_MIPS_I_REG_A3,
	DBG_CPU_MIPS_I_REG_T0,
	DBG_CPU_MIPS_I_REG_T1,
	DBG_CPU_MIPS_I_REG_T2,
	DBG_CPU_MIPS_I_REG_T3,
	DBG_CPU_MIPS_I_REG_T4,
	DBG_CPU_MIPS_I_REG_T5,
	DBG_CPU_MIPS_I_REG_T6,
	DBG_CPU_MIPS_I_REG_T7,
	DBG_CPU_MIPS_I_REG_S0,
	DBG_CPU_MIPS_I_REG_S1,
	DBG_CPU_MIPS_I_REG_S2,
	DBG_CPU_MIPS_I_REG_S3,
	DBG_CPU_MIPS_I_REG_S4,
	DBG_CPU_MIPS_I_REG_S5,
	DBG_CPU_MIPS_I_REG_S6,
	DBG_CPU_MIPS_I_REG_S7,
	DBG_CPU_MIPS_I_REG_T8,
	DBG_CPU_MIPS_I_REG_T9,
	DBG_CPU_MIPS_I_REG_K0,
	DBG_CPU_MIPS_I_REG_K1,
	DBG_CPU_MIPS_I_REG_GP,
	DBG_CPU_MIPS_I_REG_SP,
	DBG_CPU_MIPS_I_REG_S8,
	DBG_CPU_MIPS_I_REG_RA,
	DBG_CPU_MIPS_I_REG_SR,
	DBG_CPU_MIPS_I_REG_LO,
	DBG_CPU_MIPS_I_REG_HI,
	DBG_CPU_MIPS_I_REG_BAD,
	DBG_CPU_MIPS_I_REG_CAUSE,
	DBG_CPU_MIPS_I_REG_PC,

	/* GDB requests 73, where 38 are the ones above and the rest
	 * are the floating-point registers. This way, unused registers
	 * are left to zero. */
	DBG_CPU_NUM_REGISTERS = 73
};

typedef unsigned int reg;

struct dbg_state {
	int signum;
	reg registers[DBG_CPU_NUM_REGISTERS];
};

struct msg {
	enum {
		MSG_TYPE_CONTINUE,
		MSG_TYPE_BREAKPOINT,
		MSG_TYPE_STEP,
		MSG_TYPE_ACK,
		MSG_TYPE_REMOVE_BREAKPOINT,
		MSG_TYPE_SHUTDOWN,

		/* Response frames. */
		MSG_TYPE_HIT
	} type;

	union {
		struct {
			address addr;
		} breakpoint;
	} data;
};

void dbg_start(void);
void dbg_stop(void);
void gdbstub_sys_send(const struct msg *msg);
void gdbstub_sys_recv(struct msg *msg);

#endif /* GDBSTUB_SYS_H */
