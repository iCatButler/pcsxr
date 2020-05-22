#ifndef GDB_SERVER_H
#define GDB_SERVER_H

void GdbStartServer(void);
void GdbStopServer(void);
void GdbServerProcessDebug(void);
void GdbServerVSync(void);
int GdbServerRunning(void);

#endif /* GDB_SERVER_H */
