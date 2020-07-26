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
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include "plugin.h"
#include "plugins.h"
#include "resource.h"
#include <time.h>
#include <stdio.h>

#include "r3000a.h"
#include "Win32.h"
#include "NoPic.h"
#include "misc.h"
#include "sio.h"
#include "pgxp_mem.h"

extern void LidInterrupt();

HANDLE hThread = 0;
BOOL start = FALSE;
BOOL restart = FALSE;
int speed = 100;

void gpuHidePic()
{
	GPU_showScreenPic(NULL);
}

DWORD WINAPI HidePicAfter(LPVOID lpThreadParameter)
{
	while (1)
	{
		while (!start) Sleep(10);

		while (1) {
			Sleep(3000);
			if (!restart)
				break;
			restart = FALSE;
		};

		gpuHidePic();
		start = FALSE;
	}

	return TRUE;
}

void gpuShowPic() {
	char Text[255];
	gzFile f;

	unsigned char *pMem;

	pMem = (unsigned char *)malloc(128 * 96 * 3);
	if (pMem == NULL) return;
	GetStateFilename(Text, StatesC);

	GPU_freeze(2, (GPUFreeze_t *)&StatesC);

	f = gzopen(Text, "rb");
	if (f != NULL) {
		gzseek(f, 32, SEEK_SET); // skip header
		gzseek(f, sizeof(u32), SEEK_CUR);
		gzseek(f, sizeof(boolean), SEEK_CUR);
		gzread(f, pMem, 128 * 96 * 3);
		gzclose(f);
	}else {
		memcpy(pMem, NoPic_Image.pixel_data, 128 * 96 * 3);
		DrawNumBorPic(pMem, StatesC + 1);
	}
	GPU_showScreenPic(pMem);
	free(pMem);

	DWORD ThreadId = 0;
	if (!hThread) hThread = CreateThread(NULL, 0, HidePicAfter, NULL, 0, NULL);

	if (start) restart = TRUE;
	start = TRUE;
}

void GetStateFilename(char *out, int i) {
	char trimlabel[33];
	int j;

	strncpy(trimlabel, CdromLabel, 32);
	trimlabel[32] = 0;
	for (j = 31; j >= 0; j--)
		if (trimlabel[j] == ' ')
			trimlabel[j] = '\0';

	sprintf(out, "sstates\\%.32s-%.9s.%3.3d", trimlabel, CdromId, i);
}

void PADhandleKey(int key) {
	char Text[255];
	int ret;

	if (Running == 0) return;
	switch (key) {
	case 0: break;
	case VK_F1:
		GetStateFilename(Text, StatesC);
		GPU_freeze(2, (GPUFreeze_t *)&StatesC);
		ret = SaveState(Text);
		if (ret == 0)
			sprintf(Text, _("*PCSXR*: Saved State %d"), StatesC + 1);
		else sprintf(Text, _("*PCSXR*: Error Saving State %d"), StatesC + 1);
		GPU_displayText(Text);
		gpuShowPic();
		break;

	case VK_F2:
		if (StatesC < 8) StatesC++;
		else StatesC = 0;
		GPU_freeze(2, (GPUFreeze_t *)&StatesC);
		gpuShowPic();
		break;

	case VK_F3:
		GetStateFilename(Text, StatesC);
		ret = LoadState(Text);
		if (ret == 0)
			sprintf(Text, _("*PCSXR*: Loaded State %d"), StatesC + 1);
		else sprintf(Text, _("*PCSXR*: Error Loading State %d"), StatesC + 1);
		GPU_displayText(Text);
		gpuShowPic();
		break;

	case VK_F4:
		if (speed == 200) {
			speed = 100;
		}
		else {
			speed = 200;
		}
		sprintf(Text, "Speed: %u", speed);
		GPU_displayText(Text);
//		GPU_setSpeed(speed / 100.0);
		GPU_setframelimit(speed == 100);
		break;

	case VK_F5:
		Config.SioIrq ^= 0x1;
		if (Config.SioIrq)
			sprintf(Text, _("*PCSXR*: Sio Irq Always Enabled"));
		else sprintf(Text, _("*PCSXR*: Sio Irq Not Always Enabled"));
		GPU_displayText(Text);
		break;

	case VK_F6:
		Config.Mdec ^= 0x1;
		if (Config.Mdec)
			sprintf(Text, _("*PCSXR*: Black&White Mdecs Only Enabled"));
		else sprintf(Text, _("*PCSXR*: Black&White Mdecs Only Disabled"));
		GPU_displayText(Text);
		break;

	case VK_F7:
		Config.Xa ^= 0x1;
		if (Config.Xa == 0)
			sprintf(Text, _("*PCSXR*: Xa Enabled"));
		else sprintf(Text, _("*PCSXR*: Xa Disabled"));
		GPU_displayText(Text);
		break;

	case VK_F8:
		GPU_makeSnapshot();
		return;

	case VK_F9:
		GPU_displayText(_("*PCSXR*: CdRom Case Opened"));
		SetCdOpenCaseTime(-1);
		LidInterrupt();
		break;

	case VK_F10:
		GPU_displayText(_("*PCSXR*: CdRom Case Closed"));
		SetCdOpenCaseTime(0);
		LidInterrupt();
		break;

	case VK_F11:
		GPU_toggleDebug();
		break;

	case VK_F12:
		SysPrintf("*PCSXR*: CpuReset\n");
		psxReset();
		break;

	case VK_ESCAPE:
		ShowCursor(TRUE); // we want GUI to have cursor always
		Running = 0;
		ClosePlugins();
		SysRunGui();
		break;
	}
}

char charsTable[4] = { "|/-\\" };

BOOL CALLBACK ConnectDlgProc(HWND hW, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[256];
	static int waitState;

	switch (uMsg) {
	case WM_INITDIALOG:
		SetWindowText(hW, _("Connecting..."));

		sprintf(str, _("Please wait while connecting... %c\n"), charsTable[waitState]);
		Static_SetText(GetDlgItem(hW, IDC_CONNECTSTR), str);
		SetTimer(hW, 0, 100, NULL);
		return TRUE;

	case WM_TIMER:
		if (++waitState == 4) waitState = 0;
		sprintf(str, _("Please wait while connecting... %c\n"), charsTable[waitState]);
		Static_SetText(GetDlgItem(hW, IDC_CONNECTSTR), str);
		return TRUE;

		/*		case WM_COMMAND:
					switch (LOWORD(wParam)) {
					case IDCANCEL:
					WaitCancel = 1;
					return TRUE;
					}*/
	}

	return FALSE;
}

#define PARSEPATH(dst, src) \
	ptr = src + strlen(src); \
while (*ptr != '\\' && ptr != src) ptr--; \
	if (ptr != src) { \
	strcpy(dst, ptr+1); \
	}

int _OpenPlugins(HWND hWnd) {
	int ret;

	GPU_clearDynarec(clearDynarec);

	ret = CDR_open();
	//if (ret < 0) { SysMessage (_("Error Opening CDR Plugin")); return -1; }

	SetCurrentDirectory(PcsxrDir);
	if (Config.UseNet && !NetOpened) {
		netInfo info;
		char path[256];

		strcpy(info.EmuName, "PCSXR " PACKAGE_VERSION);
		strncpy(info.CdromID, CdromId, 9);
		strncpy(info.CdromLabel, CdromLabel, 9);
		info.psxMem = psxM;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.GPU_displayText = GPU_displayText;
		info.GPU_showScreenPic = GPU_showScreenPic;
		info.PAD_setSensitive = PAD1_setSensitive;
		sprintf(path, "%s%s", Config.BiosDir, Config.Bios);
		strcpy(info.BIOSpath, path);
		strcpy(info.MCD1path, Config.Mcd1);
		strcpy(info.MCD2path, Config.Mcd2);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Gpu);
		strcpy(info.GPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Spu);
		strcpy(info.SPUpath, path);
		sprintf(path, "%s%s", Config.PluginsDir, Config.Cdr);
		strcpy(info.CDRpath, path);
		NET_setInfo(&info);

		ret = NET_open(hWnd);
		if (ret < 0) {
			if (ret == -2) {
				// -2 is returned when something in the info
				// changed and needs to be synced
				char *ptr;

				PARSEPATH(Config.Bios, info.BIOSpath);
				PARSEPATH(Config.Gpu, info.GPUpath);
				PARSEPATH(Config.Spu, info.SPUpath);
				PARSEPATH(Config.Cdr, info.CDRpath);

				strcpy(Config.Mcd1, info.MCD1path);
				strcpy(Config.Mcd2, info.MCD2path);
				return -2;
			}else {
				Config.UseNet = FALSE;
			}
		}else {
			HWND hW = CreateDialog(gApp.hInstance, MAKEINTRESOURCE(IDD_CONNECT), gApp.hWnd, ConnectDlgProc);
			ShowWindow(hW, SW_SHOW);

			if (NET_queryPlayer() == 1) {
				if (SendPcsxInfo() == -1) Config.UseNet = FALSE;
			}else {
				if (RecvPcsxInfo() == -1) Config.UseNet = FALSE;
			}

			DestroyWindow(hW);
		}
		NetOpened = TRUE;
	}else if (Config.UseNet) {
		NET_resume();
	}

	ret = GPU_open(hWnd);
	if (ret < 0) { SysMessage(_("Error Opening GPU Plugin (%d)"), ret); return -1; }
	GPU_pgxpMemory(0, PGXP_GetMem());
	ret = SPU_open(hWnd);
	if (ret < 0) { SysMessage(_("Error Opening SPU Plugin (%d)"), ret); return -1; }
	SPU_registerCallback(SPUirq);
	ret = PAD1_open(hWnd);
	if (ret < 0) { SysMessage(_("Error Opening PAD1 Plugin (%d)"), ret); return -1; }
	PAD1_registerVibration(GPU_visualVibration);
	PAD1_registerCursor(GPU_cursor);
	ret = PAD2_open(hWnd);
	if (ret < 0) { SysMessage(_("Error Opening PAD2 Plugin (%d)"), ret); return -1; }
	PAD2_registerVibration(GPU_visualVibration);
	PAD2_registerCursor(GPU_cursor);
#ifdef ENABLE_SIO1API
	ret = SIO1_open(hWnd);
	if (ret < 0) { SysMessage(_("Error Opening SIO1 plugin (%d)"), ret); return -1; }
	SIO1_registerCallback(SIO1irq);
#endif

	SetCurrentDirectory(PcsxrDir);

	if (Config.HideCursor)
		ShowCursor(FALSE);
	return 0;
}

int OpenPlugins(HWND hWnd, int internaliso) {
	int ret;

	while ((ret = _OpenPlugins(hWnd)) == -2) {
		ReleasePlugins();
		LoadMcds(Config.Mcd1, Config.Mcd2);
		if (LoadPlugins() == -1) return -1;
	}
	return ret;
}

void ClosePlugins() {
	int ret;

	// PAD plugins have to be closed first, otherwise some plugins like
	// LilyPad will mess up the window handle and cause crash.
	// Also don't check return value here, as LilyPad uses void.
	PAD1_close();
	PAD2_close();

	ret = CDR_close();
	if (ret < 0) { SysMessage(_("Error Closing CDR Plugin")); return; }
	ret = GPU_close();
	if (ret < 0) { SysMessage(_("Error Closing GPU Plugin")); return; }
	ret = SPU_close();
	if (ret < 0) { SysMessage(_("Error Closing SPU Plugin")); return; }
#ifdef ENABLE_SIO1API
	ret = SIO1_close();
	if (ret < 0) { SysMessage(_("Error Closing SIO1 plugin")); return; }
#endif

	if (Config.UseNet) {
		NET_pause();
	}
}

void ResetPlugins() {
	int ret;

	CDR_shutdown();
	GPU_shutdown();
	SPU_shutdown();
	PAD1_shutdown();
	PAD2_shutdown();
#ifdef ENABLE_SIO1API
	SIO1_shutdown();
#endif
	if (Config.UseNet) NET_shutdown();

	ret = CDR_init();
	if (ret != 0) { SysMessage(_("CDRinit error: %d"), ret); return; }
	ret = GPU_init();
	if (ret != 0) { SysMessage(_("GPUinit error: %d"), ret); return; }
	ret = SPU_init();
	if (ret != 0) { SysMessage(_("SPUinit error: %d"), ret); return; }
	ret = PAD1_init(1);
	if (ret != 0) { SysMessage(_("PAD1init error: %d"), ret); return; }
	ret = PAD2_init(2);
	if (ret != 0) { SysMessage(_("PAD2init error: %d"), ret); return; }
#ifdef ENABLE_SIO1API
	ret = SIO1_init();
	if (ret != 0) { SysMessage(_("SIO1init error: %d"), ret); return; }
#endif

	if (Config.UseNet) {
		ret = NET_init();
		if (ret < 0) { SysMessage(_("NETinit error: %d"), ret); return; }
	}

	NetOpened = FALSE;
}
