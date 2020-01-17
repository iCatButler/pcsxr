//
// DF Netplay Plugin
//
// Based on netSock 0.2 by linuzappz.
// The Plugin is free source code.
//

#ifndef __DFNET_H__
#define __DFNET_H__

#include "config.h"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
#elif defined(_MACOSX)
#ifdef __cplusplus
extern "C" {
#endif
#ifdef PCSXRCORE
__private_extern char* Pcsxr_locale_text(char* toloc);
#define _(String) Pcsxr_locale_text(String)
#define N_(String) String
#else
#ifndef PCSXRPLUG
#warning please define the plug being built to use Mac OS X localization!
#define _(msgid) msgid
#define N_(msgid) msgid
#else
//Kludge to get the preprocessor to accept PCSXRPLUG as a variable.
#define PLUGLOC_x(x,y) x ## y
#define PLUGLOC_y(x,y) PLUGLOC_x(x,y)
#define PLUGLOC PLUGLOC_y(PCSXRPLUG,_locale_text)
__private_extern char* PLUGLOC(char* toloc);
#define _(String) PLUGLOC(String)
#define N_(String) String
#endif
#ifdef __cplusplus
}
#endif
#endif
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

typedef void* HWND;

extern struct timeval tm;

#define CALLBACK

long timeGetTime();

#include "psemu_plugin_defs.h"

typedef struct {
	int PlayerNum;
	unsigned short PortNum;
	char ipAddress[32];
} Config;

extern Config conf;

void LoadConf();
void SaveConf();

extern int sock;
extern char *PadSendData;
extern char *PadRecvData;
extern char PadSendSize;
extern char PadRecvSize;
extern char PadSize[2];
extern int PadCount;
extern int PadCountMax;
extern int PadInit;
extern int Ping;
extern volatile int WaitCancel;
extern fd_set rset;
extern fd_set wset;

long sockInit();
long sockShutdown();
long sockOpen();
void sockCreateWaitDlg();
void sockDlgUpdate();
void sockDestroyWaitDlg();
int sockPing();

int ShowPauseDlg();
void SysMessage(const char *fmt, ...);

size_t SEND(const void *pData, int Size, int Mode);
size_t RECV(void *pData, int Size, int Mode);

#endif
