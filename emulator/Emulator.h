
// Emulator.h

#pragma once

#include "Views.h"


//////////////////////////////////////////////////////////////////////

enum EmulatorConfiguration
{
    EMU_CONF_CHESS1 = 1,
    EMU_CONF_CHESS2 = 2,
};


//////////////////////////////////////////////////////////////////////

extern WORD g_nEmulatorConfiguration;  // Current configuration

extern bool g_okEmulatorRunning;

extern BYTE* g_pEmulatorRam;
extern WORD g_wEmulatorPrevCpuPC;  // Previous PC value

//////////////////////////////////////////////////////////////////////


bool Emulator_Init();
bool Emulator_InitConfiguration(WORD configuration);
void Emulator_Done();
WORD Emulator_GetPC();
BYTE Emulator_GetMemoryByte(WORD address);
bool Emulator_IsBreakpoint();
void Emulator_SetSound(bool soundOnOff);
void Emulator_Start();
void Emulator_Stop();
void Emulator_Reset();
int  Emulator_SystemFrame();
void Emulator_Step();
void Emulator_KeyEvent(BYTE keyscan, BOOL okPressed);

#if !defined(PRODUCT)
bool Emulator_IsTraceEnabled();
void Emulator_EnableTrace(bool okEnable);
#endif

void Emulator_OnUpdate();


//////////////////////////////////////////////////////////////////////
