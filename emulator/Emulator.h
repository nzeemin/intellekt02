/*  This file is part of ITELLEKT02.
    ITELLEKT02 is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    ITELLEKT02 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
ITELLEKT02. If not, see <http://www.gnu.org/licenses/>. */

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
