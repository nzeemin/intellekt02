/*  This file is part of ITELLEKT02.
    ITELLEKT02 is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    ITELLEKT02 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
ITELLEKT02. If not, see <http://www.gnu.org/licenses/>. */

// Emulator.cpp

#include "stdafx.h"
#include "Main.h"
#include "Emulator.h"
#include "Views.h"
#include "i8080.h"
#include "i8080dis.h"
#include "SoundGen.h"

void Emulator_TraceInstruction(WORD address);


//////////////////////////////////////////////////////////////////////

WORD g_nEmulatorConfiguration;  // Current configuration

bool g_okEmulatorRunning = false;

BYTE* g_pEmulatorRam = NULL;
BYTE g_wEmulatorPortF4 = 255;
BYTE g_wEmulatorPortF5 = 0;
BYTE g_wEmulatorPortF6 = 0;

bool m_okEmulatorTrace = false;
bool m_okEmulatorSound = false;

int m_nEmulatorCpuTicks = 0;    // CPU ticks remaining

WORD g_wEmulatorCpuPC = 0;      // Current PC value
WORD g_wEmulatorPrevCpuPC = 0;  // Previous PC value

bool Emulator_Init()
{
    g_pEmulatorRam = (BYTE*) ::calloc(65536, 1);

    i8080_init();
    i8080_jump(0);

    if (m_okEmulatorSound)
    {
        SoundGen_Initialize(Settings_GetSoundVolume());
    }

    return true;
}

void Emulator_Done()
{
    ::free(g_pEmulatorRam);  g_pEmulatorRam = NULL;
}

bool Emulator_InitConfiguration(WORD configuration)
{
    WORD nRomResourceId;
    switch (configuration)
    {
    default:
    case EMU_CONF_CHESS1:
        nRomResourceId = IDR_CHESS_ROM;
        break;
    case EMU_CONF_CHESS2:
        nRomResourceId = IDR_CHESS2_ROM;
        break;
    }

    HRSRC hRes = NULL;
    DWORD dwDataSize = 0;
    HGLOBAL hResLoaded = NULL;
    void * pResData = NULL;
    if ((hRes = ::FindResource(NULL, MAKEINTRESOURCE(nRomResourceId), _T("BIN"))) == NULL ||
        (dwDataSize = ::SizeofResource(NULL, hRes)) < 8192 ||
        (hResLoaded = ::LoadResource(NULL, hRes)) == NULL ||
        (pResData = ::LockResource(hResLoaded)) == NULL)
    {
        return false;
    }
    ::memcpy(g_pEmulatorRam, pResData, 8192);

    g_nEmulatorConfiguration = configuration;

    // Reset the CPU
    i8080_init();
    i8080_jump(0);

    return true;
}

void Emulator_Start()
{
    g_okEmulatorRunning = true;

    // Set title bar text
    MainWindow_UpdateWindowTitle();
    MainWindow_UpdateMenu();
}
void Emulator_Stop()
{
    g_okEmulatorRunning = false;

    // Reset title bar message
    MainWindow_UpdateWindowTitle();
    MainWindow_UpdateMenu();
    MainWindow_UpdateAllViews();
}

void Emulator_Reset()
{
    i8080_init();
    i8080_jump(0);

    MainWindow_UpdateAllViews();
}

void Emulator_Step()
{
    if (g_okEmulatorRunning)
        Emulator_Stop();

    i8080_instruction();
    g_wEmulatorCpuPC = i8080_pc();

    MainWindow_UpdateAllViews();
}

WORD Emulator_GetPC()
{
    return i8080_pc();
}
BYTE Emulator_GetMemoryByte(WORD address)
{
    return g_pEmulatorRam[address];
}

bool Emulator_IsBreakpoint()
{
    return false;
}

void Emulator_SetSound(bool soundOnOff)
{
    if (m_okEmulatorSound != soundOnOff)
    {
        if (soundOnOff)
        {
            SoundGen_Initialize(Settings_GetSoundVolume());
        }
        else
        {
            SoundGen_Finalize();
        }
    }

    m_okEmulatorSound = soundOnOff;
}

#if !defined(PRODUCT)
bool Emulator_IsTraceEnabled()
{
    return m_okEmulatorTrace;
}
void Emulator_EnableTrace(bool okEnable)
{
    m_okEmulatorTrace = okEnable;
}
#endif

/*
������ ����� ����� 1/25 ������� = 40 ��
CPU at 2 MHz: 2000000 / 25 = 80000 ticks per frame
*/
int Emulator_SystemFrame()
{
    const int audioticks = 20286 / (SOUNDSAMPLERATE / 25);
    const int frameProcTicks = 4;

    for (int frameticks = 0; frameticks < 20000; frameticks++)
    {
        for (int procticks = 0; procticks < frameProcTicks; procticks++)  // CPU ticks
        {
            if (m_nEmulatorCpuTicks > 0)
                m_nEmulatorCpuTicks--;
            else
            {
#if !defined(PRODUCT)
                if (m_okEmulatorTrace)
                    Emulator_TraceInstruction(i8080_pc());
#endif
                m_nEmulatorCpuTicks = i8080_instruction();
                if (m_nEmulatorCpuTicks < 0)
                    m_nEmulatorCpuTicks = 0;
            }
        }

        if (frameticks % audioticks == 0 && m_okEmulatorSound)  // AUDIO tick
        {
            bool bSoundBit = (g_wEmulatorPortF6 & 0x80) != 0;
            if (bSoundBit)
                SoundGen_FeedDAC(0x1fff, 0x1fff);
            else
                SoundGen_FeedDAC(0x0000, 0x0000);
        }
    }

    return 1;
}

// Update cached values after Run or Step
void Emulator_OnUpdate()
{
}

void Emulator_KeyEvent(BYTE keyscan, BOOL okPressed)
{
    if (okPressed)
        g_wEmulatorPortF4 = keyscan;
    else
        g_wEmulatorPortF4 = 255;
}


//////////////////////////////////////////////////////////////////////
// i8080_hal implementation

int i8080_hal_memory_read_byte(int addr)
{
    ASSERT(g_pEmulatorRam != NULL);
    return g_pEmulatorRam[addr & 0xffff];
}

void i8080_hal_memory_write_byte(int addr, int byte)
{
    ASSERT(g_pEmulatorRam != NULL);
    g_pEmulatorRam[addr & 0xffff] = byte;
}

int i8080_hal_memory_read_word(int addr)
{
    return
        (i8080_hal_memory_read_byte(addr + 1) << 8) |
        i8080_hal_memory_read_byte(addr);
}

void i8080_hal_memory_write_word(int addr, int word)
{
    i8080_hal_memory_write_byte(addr, word & 0xff);
    i8080_hal_memory_write_byte(addr + 1, (word >> 8) & 0xff);
}

int i8080_hal_io_input(int port)
{
    BYTE value = 0;
    switch (port)
    {
    case 0xf4:
        value = g_wEmulatorPortF4;
#if !defined(PRODUCT)
        if (value != 255)
            DebugLogFormat(_T("Port IN  %04x  (%02x) <- %02x\r\n"), i8080_pc() - 2, port, value);
#endif
        break;
    default:
#if !defined(PRODUCT)
        DebugLogFormat(_T("Port IN  %04x  (%02x) <- %02x\r\n"), i8080_pc() - 2, port, value);
#endif
        break;
    }

    return value;
}

void i8080_hal_io_output(int port, int value)
{
#if !defined(PRODUCT)
    DebugLogFormat(_T("Port OUT %04x  (%02x) <- %02x\r\n"), i8080_pc() - 2, port, value);
#endif
    switch (port)
    {
    case 0xf6:
        if (g_wEmulatorPortF6 == 0 && value != 0)
            KeyboardView_SetIndicatorData(value, g_wEmulatorPortF5);
        g_wEmulatorPortF6 = value;
        break;
    case 0xf5:
        g_wEmulatorPortF5 = value;
        if (g_wEmulatorPortF6 != 0)
            KeyboardView_SetIndicatorData(g_wEmulatorPortF6, g_wEmulatorPortF5);
        break;
    default:
        break;
    }
}

void i8080_hal_iff(int on)
{
    // Northing.
}


//////////////////////////////////////////////////////////////////////

#if !defined(PRODUCT)

void Emulator_TraceInstruction(WORD address)
{
    BYTE memory[4];
    for (int idx = 0; idx < 4; idx++)
    {
        memory[idx] = Emulator_GetMemoryByte(address + idx);
    }

    TCHAR bufaddr[5];
    PrintHexValue(bufaddr, address);

    char disasm[32];  disasm[0] = 0;
    TCHAR tdisasm[32];
    int length = i8080_disasm(memory, disasm, 32);
    wsprintf(tdisasm, _T("%S"), disasm);

    TCHAR buffer[64];
    wsprintf(buffer, _T("%s\t%s\r\n"), bufaddr, tdisasm);
    DebugLog(buffer);
}

#endif


//////////////////////////////////////////////////////////////////////
