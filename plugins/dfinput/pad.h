/*
 * Copyright (c) 2009, Wei Mingzhi <whistler_wmz@users.sf.net>.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses>.
 */

#ifndef PAD_H_
#define PAD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _MACOSX
#include "config.h"
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <SDL.h>
#include <SDL_joystick.h>
#if SDL_VERSION_ATLEAST(2,0,0)
#include <SDL_haptic.h>
#include <SDL_gamecontroller.h>
#endif

#ifdef _MACOSX
#include <Carbon/Carbon.h>
typedef void *Display;
#define ThreadID ThreadID_MACOSX
#else
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/XKBlib.h>
#endif

#include "psemu_plugin_defs.h"

#ifdef ENABLE_NLS
#include <libintl.h>
#include <locale.h>
#define _(x)  gettext(x)
#define N_(x) (x)
//If running under Mac OS X, use the Localizable.strings file instead.
#elif defined(_MACOSX)
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
#endif
#else
#define _(x)  (x)
#define N_(x) (x)
#endif

#if SDL_VERSION_ATLEAST(2,0,0)
extern int has_haptic;
#endif

int JoyHapticRumble(int pad, uint32_t low, uint32_t high);

enum {
	DKEY_SELECT = 0,
	DKEY_L3,
	DKEY_R3,
	DKEY_START,
	DKEY_UP,
	DKEY_RIGHT,
	DKEY_DOWN,
	DKEY_LEFT,
	DKEY_L2,
	DKEY_R2,
	DKEY_L1,
	DKEY_R1,
	DKEY_TRIANGLE,
	DKEY_CIRCLE,
	DKEY_CROSS,
	DKEY_SQUARE,
	DKEY_ANALOG,

	DKEY_TOTAL
};

enum {
	EMU_INCREMENTSTATE=0,
	EMU_FASTFORWARDS,
	EMU_LOADSTATE,
	EMU_SAVESTATE,
	EMU_SCREENSHOT,
	EMU_ESCAPE,
	EMU_REWIND,
	EMU_ALTSPEED1,
	EMU_ALTSPEED2,

	EMU_TOTAL
};

enum {
	ANALOG_LEFT = 0,
	ANALOG_RIGHT,

	ANALOG_TOTAL
};

enum { NONE = 0, AXIS, HAT, BUTTON };

typedef struct tagKeyDef {
	uint8_t			JoyEvType;
	union {
		int16_t		d;
		int16_t		Axis;   // positive=axis+, negative=axis-, abs(Axis)-1=axis index
		uint16_t	Hat;	// 8-bit for hat number, 8-bit for direction
		uint16_t	Button; // button number
	} J;
	uint16_t		Key;
	uint8_t			ReleaseEventPending;
} KEYDEF;

enum { ANALOG_XP = 0, ANALOG_XM, ANALOG_YP, ANALOG_YM };

#if SDL_VERSION_ATLEAST(2,0,0)
extern SDL_GameControllerButton controllerMap[DKEY_TOTAL];
#endif

typedef struct tagPadDef {
	int8_t			DevNum;
	uint16_t		Type;
	uint8_t			VisualVibration;
    uint8_t         PhysicalVibration;
	KEYDEF			KeyDef[DKEY_TOTAL];
	KEYDEF			AnalogDef[ANALOG_TOTAL][4];
#if SDL_VERSION_ATLEAST(2,0,0)
	int8_t			UseSDL2;
#endif
} PADDEF;

typedef struct tagEmuDef {
	uint16_t	EmuKeyEvent;
	KEYDEF		Mapping;
} EMUDEF;

typedef struct tagEmuDef2{
	EMUDEF		EmuDef[EMU_TOTAL];
	SDL_Joystick	*EmuKeyDev;
	int8_t		DevNum;
} EMUDEF2;

typedef struct tagConfig {
	uint8_t			Threaded;
	uint8_t			HideCursor;
	uint8_t			PreventScrSaver;
	PADDEF			PadDef[2];
	EMUDEF2			E;
} CONFIG;

typedef struct tagPadState {
	SDL_Joystick		*JoyDev;
	uint8_t				PadMode;
	uint8_t				PadID;
	uint8_t				PadModeKey;
	volatile uint8_t	PadModeSwitch;
	volatile uint16_t	KeyStatus;
	volatile uint16_t	JoyKeyStatus;
	volatile uint8_t	AnalogStatus[ANALOG_TOTAL][2]; // 0-255 where 127 is center position
	volatile uint8_t	AnalogKeyStatus[ANALOG_TOTAL][4];
	volatile int8_t		MouseAxis[2][2];
	uint8_t				Vib0, Vib1;
	volatile uint8_t	VibF[2];
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_Haptic			*haptic;
	SDL_GameController	*GCDev;
#else
#ifdef __linux__
	int			VibrateDev;
	int			VibrateEffect;
	uint8_t			VibrLow, VibrHigh;
	uint32_t		VibrSetTime;
#endif
#endif
} PADSTATE;

typedef struct tagGlobalData {
	CONFIG				cfg;

	uint8_t				Opened;
	Display				*Disp;

	PADSTATE			PadState[2];
	volatile long		KeyLeftOver;
} GLOBALDATA;

extern GLOBALDATA		g;

enum {
	CMD_READ_DATA_AND_VIBRATE = 0x42,
	CMD_CONFIG_MODE = 0x43,
	CMD_SET_MODE_AND_LOCK = 0x44,
	CMD_QUERY_MODEL_AND_MODE = 0x45,
	CMD_QUERY_ACT = 0x46, // ??
	CMD_QUERY_COMB = 0x47, // ??
	CMD_QUERY_MODE = 0x4C, // QUERY_MODE ??
	CMD_VIBRATION_TOGGLE = 0x4D
};

// cfg.c functions...
void LoadPADConfig();
void SavePADConfig();

// sdljoy.c functions...
void InitSDLJoy();
void DestroySDLJoy();
void CheckJoy();

// xkb.c functions...
void InitKeyboard();
void DestroyKeyboard();
void CheckKeyboard();

// analog.c functions...
void InitAnalog();
void CheckAnalog();
int AnalogKeyPressed(uint16_t Key);
int AnalogKeyReleased(uint16_t Key);

// pad.c functions...
char *PSEgetLibName(void);
uint32_t PSEgetLibType(void);
uint32_t PSEgetLibVersion(void);
long PADinit(long flags);
long PADshutdown(void);
long PADopen(unsigned long *Disp);
long PADclose(void);
long PADquery(void);
unsigned char PADstartPoll(int pad);
unsigned char PADpoll(unsigned char value);
long PADreadPort1(PadDataS *pad);
long PADreadPort2(PadDataS *pad);
long PADkeypressed(void);
long PADconfigure(void);
void PADabout(void);
long PADtest(void);

#ifdef __cplusplus
}
#endif

#endif
