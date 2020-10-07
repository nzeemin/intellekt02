/*  This file is part of ITELLEKT02.
    ITELLEKT02 is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    ITELLEKT02 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
ITELLEKT02. If not, see <http://www.gnu.org/licenses/>. */

// Intellekt02.cpp : Defines the entry point for the application.

#include "stdafx.h"
#include <commdlg.h>
#include <crtdbg.h>
#include <commctrl.h>
#include "Main.h"
#include "Emulator.h"
#include "Views.h"

//////////////////////////////////////////////////////////////////////
// Global Variables

HINSTANCE g_hInst = NULL; // current instance
HWND g_hwnd = NULL;


//////////////////////////////////////////////////////////////////////
// Forward declarations

BOOL InitInstance(HINSTANCE, int);
void DoneInstance();


//////////////////////////////////////////////////////////////////////


int APIENTRY _tWinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPTSTR    lpCmdLine,
    _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

#ifdef _DEBUG
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);
    int n = 0;
    _CrtSetBreakAlloc(n);
#endif

    g_hInst = hInstance; // Store instance handle in our global variable

    LARGE_INTEGER nFrameStartTime;
    nFrameStartTime.QuadPart = 0;

    // Initialize global strings
    LoadString(hInstance, IDS_APP_TITLE, g_szTitle, MAX_LOADSTRING);
    LoadString(hInstance, IDC_INTELLEKT02, g_szWindowClass, MAX_LOADSTRING);
    MainWindow_RegisterClass();

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
        return FALSE;

    HACCEL hAccelTable = ::LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_INTELLEKT02));

    LARGE_INTEGER nPerformanceFrequency;
    ::QueryPerformanceFrequency(&nPerformanceFrequency);

    // Main message loop
    MSG msg;
    while (true)
    {
        ::QueryPerformanceCounter(&nFrameStartTime);

        if (!g_okEmulatorRunning)
            ::Sleep(20);
        else
        {
            if (Emulator_IsBreakpoint())
                Emulator_Stop();
            else
            {
                if (Emulator_SystemFrame())
                {
                    ScreenView_UpdateScreen();
                }
                ::Sleep(1);
            }
        }

        // Process all queue
        while (::PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ))
        {
            if (msg.message == WM_QUIT)
                goto endprog;

            if (::TranslateAccelerator(g_hwnd, hAccelTable, &msg))
                continue;

            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }

        if (g_okEmulatorRunning && !Settings_GetSound())
        {
            // Slow down to 25 frames per second
            LARGE_INTEGER nFrameFinishTime;  // Frame start time
            ::QueryPerformanceCounter(&nFrameFinishTime);
            LONGLONG nTimeElapsed = (nFrameFinishTime.QuadPart - nFrameStartTime.QuadPart)
                    * 1000 / nPerformanceFrequency.QuadPart;
            if (nTimeElapsed > 0 && nTimeElapsed < 30)  // 1000 millisec / 25 = 40 millisec
            {
                LONG nTimeToSleep = (LONG)(30 - nTimeElapsed);
                ::Sleep((DWORD) nTimeToSleep);
            }
        }
    }
endprog:

    DoneInstance();

#ifdef _DEBUG
    if (_CrtDumpMemoryLeaks())
        ::MessageBeep(MB_ICONEXCLAMATION);
#endif

    return (int) msg.wParam;
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
    INITCOMMONCONTROLSEX ics;  ics.dwSize = sizeof(ics);
    ics.dwICC = ICC_WIN95_CLASSES;
    InitCommonControlsEx(&ics);

#if !defined(PRODUCT)
    DebugLogClear();
#endif
    Settings_Init();
    if (!Emulator_Init())
        return FALSE;
    WORD conf = (WORD) Settings_GetConfiguration();
    if (conf == 0) conf = EMU_CONF_CHESS1;
    if (!Emulator_InitConfiguration(conf))
        return FALSE;

    Emulator_SetSound(Settings_GetSound() != FALSE);

    if (!CreateMainWindow())
        return FALSE;

    return TRUE;
}

// Instance finalization
void DoneInstance()
{
    ScreenView_Done();

    Emulator_Done();

    Settings_Done();
}


//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////


BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine)
{
    TCHAR buffer[360];
    wsprintf(buffer,
            _T("ASSERTION FAILED\n\nFile: %S\nLine: %d\n\n")
            _T("Press Abort to stop the program, Retry to break to the debugger, or Ignore to continue execution."),
            lpszFileName, nLine);
    int result = MessageBox(NULL, buffer, _T("ASSERT"), MB_ICONSTOP | MB_ABORTRETRYIGNORE);

    switch (result)
    {
    case IDRETRY:
        return TRUE;
    case IDIGNORE:
        return FALSE;
    case IDABORT:
        PostQuitMessage(255);
    }
    return FALSE;
}

void AlertWarning(LPCTSTR sMessage)
{
    ::MessageBox(NULL, sMessage, _T("INTELLEKT-02"), MB_OK | MB_ICONEXCLAMATION | MB_TOPMOST);
}
void AlertWarningFormat(LPCTSTR sFormat, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, sFormat);
    _vsntprintf_s(buffer, 512, 512 - 1, sFormat, ptr);
    va_end(ptr);

    ::MessageBox(NULL, buffer, _T("INTELLEKT-02"), MB_OK | MB_ICONEXCLAMATION);
}
BOOL AlertOkCancel(LPCTSTR sMessage)
{
    int result = ::MessageBox(NULL, sMessage, _T("INTELLEKT-02"), MB_OKCANCEL | MB_ICONQUESTION | MB_TOPMOST);
    return (result == IDOK);
}


//////////////////////////////////////////////////////////////////////
// DebugPrint and DebugLog

#if !defined(PRODUCT)

void DebugPrint(LPCTSTR message)
{
    if (g_hwndConsole == NULL)
        return;

    ConsoleView_Print(message);
}

void DebugPrintFormat(LPCTSTR pszFormat, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, pszFormat);
    _vsntprintf_s(buffer, 512, 512 - 1, pszFormat, ptr);
    va_end(ptr);

    DebugPrint(buffer);
}

const LPCTSTR TRACELOG_FILE_NAME = _T("trace.log");
const LPCTSTR TRACELOG_NEWLINE = _T("\r\n");

HANDLE Common_LogFile = NULL;

void DebugLogCreateFile()
{
    if (Common_LogFile == NULL)
    {
        Common_LogFile = ::CreateFile(TRACELOG_FILE_NAME,
                GENERIC_WRITE, FILE_SHARE_READ, NULL,
                CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    }
}

void DebugLogCloseFile()
{
    if (Common_LogFile == NULL)
        return;

    ::CloseHandle(Common_LogFile);
    Common_LogFile = NULL;
}

void DebugLogClear()
{
    DebugLogCreateFile();

    if (Common_LogFile != NULL)
    {
        // Trunkate to zero length
        ::SetFilePointer(Common_LogFile, 0, 0, 0);
        ::SetEndOfFile(Common_LogFile);
    }
}

void DebugLog(LPCTSTR message)
{
    DebugLogCreateFile();

    SetFilePointer(Common_LogFile, 0, NULL, FILE_END);

    DWORD dwLength = lstrlen(message) * sizeof(TCHAR);

    char ascii[256];  *ascii = 0;
    WideCharToMultiByte(CP_ACP, 0, message, dwLength, ascii, 256, NULL, NULL);

    DWORD dwBytesWritten = 0;
    //WriteFile(Common_LogFile, message, dwLength, &dwBytesWritten, NULL);
    WriteFile(Common_LogFile, ascii, strlen(ascii), &dwBytesWritten, NULL);

    //dwLength = lstrlen(TRACELOG_NEWLINE) * sizeof(TCHAR);
    //WriteFile(Common_LogFile, TRACELOG_NEWLINE, dwLength, &dwBytesWritten, NULL);

    //TODO
}

void DebugLogFormat(LPCTSTR pszFormat, ...)
{
    TCHAR buffer[512];

    va_list ptr;
    va_start(ptr, pszFormat);
    _vsntprintf_s(buffer, 512, 512 - 1, pszFormat, ptr);
    va_end(ptr);

    DebugLog(buffer);
}

#endif // !defined(PRODUCT)


//////////////////////////////////////////////////////////////////////


HFONT CreateMonospacedFont()
{
    HFONT font = NULL;
    font = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            RUSSIAN_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            FIXED_PITCH,
            _T("Lucida Console"));
    if (font == NULL)
    {
        font = CreateFont(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                RUSSIAN_CHARSET,
                OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                FIXED_PITCH,
                _T("Courier"));
    }
    if (font == NULL)
        return NULL;

    return font;
}

HFONT CreateDialogFont()
{
    HFONT font = NULL;
    font = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
            VARIABLE_PITCH,
            _T("MS Shell Dlg 2"));

    return font;
}

void GetFontWidthAndHeight(HDC hdc, int* pWidth, int* pHeight)
{
    TEXTMETRIC tm;
    GetTextMetrics(hdc, &tm);
    if (pWidth != NULL)
        *pWidth = tm.tmAveCharWidth;
    if (pHeight != NULL)
        *pHeight = tm.tmHeight;
}

// Print hex 8-bit value to buffer
// buffer size at least 3 characters
void PrintHexByteValue(TCHAR* buffer, BYTE value)
{
    for (int p = 0; p < 2; p++)
    {
        int digit = value & 15;
        buffer[1 - p] = (digit < 10) ? _T('0') + (TCHAR)digit : _T('a') + (TCHAR)(digit - 10);
        value = (value >> 4);
    }
    buffer[2] = 0;
}

// Print hex 16-bit value to buffer
// buffer size at least 5 characters
void PrintHexValue(TCHAR* buffer, WORD value)
{
    for (int p = 0; p < 4; p++)
    {
        int digit = value & 15;
        buffer[3 - p] = (digit < 10) ? _T('0') + (TCHAR)digit : _T('a') + (TCHAR)(digit - 10);
        value = (value >> 4);
    }
    buffer[4] = 0;
}
// Print binary 16-bit value to buffer
// buffer size at least 17 characters
void PrintBinaryValue(TCHAR* buffer, WORD value)
{
    for (int b = 0; b < 16; b++)
    {
        int bit = (value >> b) & 1;
        buffer[15 - b] = bit ? _T('1') : _T('0');
    }
    buffer[16] = 0;
}

void DrawHexValue(HDC hdc, int x, int y, WORD value)
{
    TCHAR buffer[7];
    PrintHexValue(buffer, value);
    TextOut(hdc, x, y, buffer, (int) _tcslen(buffer));
}
void DrawBinaryValue(HDC hdc, int x, int y, WORD value)
{
    TCHAR buffer[17];
    PrintBinaryValue(buffer, value);
    TextOut(hdc, x, y, buffer, 16);
}

// Parse hex value from text
bool ParseHexValue(LPCTSTR text, WORD* pValue)
{
    WORD value = 0;
    TCHAR* pChar = (TCHAR*)text;
    for (int p = 0;; p++)
    {
        if (p > 4) return false;
        TCHAR ch = *pChar;  pChar++;
        if (ch == 0) break;
        if (ch >= _T('0') && ch <= _T('9'))
        {
            value = (value << 4);
            TCHAR digit = ch - _T('0');
            value = value + digit;
        }
        else if (ch >= _T('a') && ch <= _T('f'))
        {
            value = (value << 4);
            TCHAR digit = ch - _T('a') + 10;
            value = value + digit;
        }
        else if (ch >= _T('A') && ch <= _T('F'))
        {
            value = (value << 4);
            TCHAR digit = ch - _T('A') + 10;
            value = value + digit;
        }
        else
            return false;
    }
    *pValue = value;
    return true;
}


//////////////////////////////////////////////////////////////////////
