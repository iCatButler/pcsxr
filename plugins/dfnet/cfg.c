//
// DF Netplay Plugin
//
// Based on netSock 0.2 by linuzappz.
// The Plugin is free source code.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dfnet.h"

#define CFG_FILENAME "dfnet.cfg"

// definitions of globals declared in "dfnet.h"
Config conf;
int sock;
char *PadSendData;
char *PadRecvData;
char PadSendSize;
char PadRecvSize;
char PadSize[2];
int PadCount;
int PadCountMax;
int PadInit;
int Ping;
volatile int WaitCancel;
fd_set rset;
fd_set wset;

void SaveConf() {
	FILE *f;

	f = fopen(CFG_FILENAME, "w");
	if (f == NULL) return;
	fwrite(&conf, 1, sizeof(conf), f);
	fclose(f);
}

void LoadConf() {
	FILE *f;

	f = fopen(CFG_FILENAME, "r");
	if (f == NULL) {
		conf.PlayerNum = 1;
		conf.PortNum = 33306;
		strcpy(conf.ipAddress, "127.0.0.1");
		return;
	}

	fread(&conf, 1, sizeof(conf), f);
	fclose(f);
}
