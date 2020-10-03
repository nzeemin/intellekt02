
// ConsoleView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "ToolWindow.h"
#include "Emulator.h"
#include "i8080dis.h"

//////////////////////////////////////////////////////////////////////


COLORREF COLOR_COMMANDFOCUS = RGB(255, 242, 157);

HWND g_hwndConsole = (HWND) INVALID_HANDLE_VALUE;  // Console View window handle
WNDPROC m_wndprocConsoleToolWindow = NULL;  // Old window proc address of the ToolWindow

HWND m_hwndConsoleLog = (HWND) INVALID_HANDLE_VALUE;  // Console log window - read-only edit control
HWND m_hwndConsoleEdit = (HWND) INVALID_HANDLE_VALUE;  // Console line - edit control
HWND m_hwndConsolePrompt = (HWND) INVALID_HANDLE_VALUE;  // Console prompt - static control
HFONT m_hfontConsole = NULL;
WNDPROC m_wndprocConsoleEdit = NULL;  // Old window proc address of the console prompt
HBRUSH m_hbrConsoleFocused = NULL;

void ClearConsole();
void PrintConsolePrompt();
void SaveMemoryDump();
void PrintMemoryDump(WORD address, int lines);
void DoConsoleCommand();
void ConsoleView_AdjustWindowLayout();
LRESULT CALLBACK ConsoleEditWndProc(HWND, UINT, WPARAM, LPARAM);
void ConsoleView_ShowHelp();


const LPCTSTR MESSAGE_UNKNOWN_COMMAND = _T("  Unknown command.\r\n");
const LPCTSTR MESSAGE_WRONG_VALUE = _T("  Wrong value.\r\n");


//////////////////////////////////////////////////////////////////////


void ConsoleView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= ConsoleViewWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_CONSOLEVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

// Create Console View as child of Main Window
void ConsoleView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndConsole = CreateWindow(
            CLASSNAME_TOOLWINDOW, NULL,
            WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
    SetWindowText(g_hwndConsole, _T("Debug Console"));

    // ToolWindow subclassing
    m_wndprocConsoleToolWindow = (WNDPROC) LongToPtr( SetWindowLongPtr(
            g_hwndConsole, GWLP_WNDPROC, PtrToLong(ConsoleViewWndProc)) );

    RECT rcConsole;  GetClientRect(g_hwndConsole, &rcConsole);

    m_hwndConsoleEdit = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            _T("EDIT"), NULL,
            WS_CHILD | WS_VISIBLE,
            90, rcConsole.bottom - 20,
            rcConsole.right - 90, 20,
            g_hwndConsole, NULL, g_hInst, NULL);
    m_hwndConsoleLog = CreateWindowEx(
            WS_EX_CLIENTEDGE,
            _T("EDIT"), NULL,
            WS_CHILD | WS_VSCROLL | WS_VISIBLE | ES_READONLY | ES_MULTILINE,
            0, 0,
            rcConsole.right, rcConsole.bottom - 20,
            g_hwndConsole, NULL, g_hInst, NULL);
    m_hwndConsolePrompt = CreateWindowEx(
            0,
            _T("STATIC"), NULL,
            WS_CHILD | WS_VISIBLE | SS_CENTERIMAGE | SS_CENTER | SS_NOPREFIX,
            0, rcConsole.bottom - 20,
            90, 20,
            g_hwndConsole, NULL, g_hInst, NULL);

    m_hfontConsole = CreateMonospacedFont();
    SendMessage(m_hwndConsolePrompt, WM_SETFONT, (WPARAM) m_hfontConsole, 0);
    SendMessage(m_hwndConsoleEdit, WM_SETFONT, (WPARAM) m_hfontConsole, 0);
    SendMessage(m_hwndConsoleLog, WM_SETFONT, (WPARAM) m_hfontConsole, 0);

    // Edit box subclassing
    m_wndprocConsoleEdit = (WNDPROC) LongToPtr( SetWindowLongPtr(
            m_hwndConsoleEdit, GWLP_WNDPROC, PtrToLong(ConsoleEditWndProc)) );

    ShowWindow(g_hwndConsole, SW_SHOW);
    UpdateWindow(g_hwndConsole);

    ConsoleView_Print(_T("Use 'h' command to show help.\r\n\r\n"));
    PrintConsolePrompt();
    SetFocus(m_hwndConsoleEdit);
}

// Adjust position of client windows
void ConsoleView_AdjustWindowLayout()
{
    RECT rc;  GetClientRect(g_hwndConsole, &rc);
    int promptWidth = 65;

    if (m_hwndConsolePrompt != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndConsolePrompt, NULL, 0, rc.bottom - 20, promptWidth, 20, SWP_NOZORDER);
    if (m_hwndConsoleEdit != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndConsoleEdit, NULL, promptWidth, rc.bottom - 20, rc.right - promptWidth, 20, SWP_NOZORDER);
    if (m_hwndConsoleLog != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndConsoleLog, NULL, 0, 0, rc.right, rc.bottom - 24, SWP_NOZORDER);
}

LRESULT CALLBACK ConsoleViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LRESULT lResult;
    switch (message)
    {
    case WM_DESTROY:
        g_hwndConsole = (HWND) INVALID_HANDLE_VALUE;  // We are closed! Bye-bye!..
        return CallWindowProc(m_wndprocConsoleToolWindow, hWnd, message, wParam, lParam);
    case WM_CTLCOLORSTATIC:
        if (((HWND)lParam) != m_hwndConsoleLog)
            return CallWindowProc(m_wndprocConsoleToolWindow, hWnd, message, wParam, lParam);
        SetBkColor((HDC)wParam, ::GetSysColor(COLOR_WINDOW));
        return (LRESULT) ::GetSysColorBrush(COLOR_WINDOW);
    case WM_CTLCOLOREDIT:
        if (((HWND)lParam) == m_hwndConsoleEdit && ::GetFocus() == m_hwndConsoleEdit)
        {
            if (m_hbrConsoleFocused == NULL)
                m_hbrConsoleFocused = ::CreateSolidBrush(COLOR_COMMANDFOCUS);
            SetBkColor((HDC)wParam, COLOR_COMMANDFOCUS);
            return (LRESULT)m_hbrConsoleFocused;
        }
        return CallWindowProc(m_wndprocConsoleToolWindow, hWnd, message, wParam, lParam);
    case WM_SIZE:
        lResult = CallWindowProc(m_wndprocConsoleToolWindow, hWnd, message, wParam, lParam);
        ConsoleView_AdjustWindowLayout();
        return lResult;
    default:
        return CallWindowProc(m_wndprocConsoleToolWindow, hWnd, message, wParam, lParam);
    }
    //return (LRESULT)FALSE;
}

LRESULT CALLBACK ConsoleEditWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CHAR:
        if (wParam == 13)
        {
            DoConsoleCommand();
            return 0;
        }
        if (wParam == VK_ESCAPE)
        {
            TCHAR command[32];
            GetWindowText(m_hwndConsoleEdit, command, 32);
            if (*command == 0)  // If command is empty
                SetFocus(g_hwndScreen);
            else
                SendMessage(m_hwndConsoleEdit, WM_SETTEXT, 0, (LPARAM)_T(""));  // Clear command
            return 0;
        }
        break;
    }

    return CallWindowProc(m_wndprocConsoleEdit, hWnd, message, wParam, lParam);
}

void ConsoleView_Activate()
{
    if (g_hwndConsole == INVALID_HANDLE_VALUE) return;

    SetFocus(m_hwndConsoleEdit);
}

void ConsoleView_Print(LPCTSTR message)
{
    if (m_hwndConsoleLog == INVALID_HANDLE_VALUE) return;

    // Put selection to the end of text
    SendMessage(m_hwndConsoleLog, EM_SETSEL, 0x100000, 0x100000);
    // Insert the message
    SendMessage(m_hwndConsoleLog, EM_REPLACESEL, (WPARAM) FALSE, (LPARAM) message);
    // Scroll to caret
    SendMessage(m_hwndConsoleLog, EM_SCROLLCARET, 0, 0);
}
void ClearConsole()
{
    if (m_hwndConsoleLog == INVALID_HANDLE_VALUE) return;

    SendMessage(m_hwndConsoleLog, WM_SETTEXT, 0, (LPARAM) _T(""));
}

void PrintConsolePrompt()
{
    TCHAR bufferAddr[5];
    PrintHexValue(bufferAddr, Emulator_GetPC());
    TCHAR buffer[10];
    wsprintf(buffer, _T("%s> "), bufferAddr);
    ::SetWindowText(m_hwndConsolePrompt, buffer);
}


void SaveMemoryDump()
{
    BYTE* pMemory = g_pEmulatorRam;

    // Create file
    HANDLE file;
    file = CreateFile(_T("memdump.bin"),
            GENERIC_WRITE, FILE_SHARE_READ, NULL,
            OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    //SetFilePointer(Common_LogFile, 0, NULL, FILE_END);

    DWORD dwLength = 65536;
    DWORD dwBytesWritten = 0;
    WriteFile(file, pMemory, dwLength, &dwBytesWritten, NULL);
    CloseHandle(file);
}

// Print memory dump
void PrintMemoryDump(WORD address, int lines)
{
    TCHAR buffer[64];
    for (int line = 0; line < lines; line++)
    {
        PrintHexValue(buffer, address);
        buffer[4] = _T(' ');

        for (int j = 0; j < 16; j++)  // Draw bytes as hex values
        {
            buffer[5 + 3 * j] = _T(' ');
            BYTE byte = Emulator_GetMemoryByte(address);
            PrintHexByteValue(buffer + 6 + 3 * j, byte);

            address++;
        }

        buffer[5 + 3 * 16] = _T('\n');
        buffer[5 + 3 * 16 + 1] = 0;
        ConsoleView_Print(buffer);
    }
}
// Print disassembled instructions
// Return value: number of words in the last instruction
int PrintDisassemble(WORD address, BOOL okOneInstr)
{
    const int nWindowSize = 30;
    BYTE memory[nWindowSize + 2];
    for (int idx = 0; idx < nWindowSize + 2; idx++)
    {
        memory[idx] = g_pEmulatorRam[address + idx];
    }

    TCHAR bufaddr[5];
    TCHAR buffer[64];

    int lastLength = 0;
    int length = 0;
    for (int index = 0; index < nWindowSize; )
    {
        PrintHexValue(bufaddr, address);

        char disasm[32];  disasm[0] = 0;
        TCHAR tdisasm[32];
        length = i8080_disasm(memory + index, disasm, 32);
        wsprintf(tdisasm, _T("%S"), disasm);

        wsprintf(buffer, _T("  %s   %s\r\n"), bufaddr, tdisasm);
        ConsoleView_Print(buffer);

        if (okOneInstr)
            break;

        address += length;
        index += length;
    }

    return lastLength;
}

void ConsoleView_StepInto()
{
    // Put command to console prompt
    SendMessage(m_hwndConsoleEdit, WM_SETTEXT, 0, (LPARAM) _T("s"));
    // Execute command
    DoConsoleCommand();
}
void ConsoleView_StepOver()
{
    // Put command to console prompt
    SendMessage(m_hwndConsoleEdit, WM_SETTEXT, 0, (LPARAM) _T("so"));
    // Execute command
    DoConsoleCommand();
}

void ConsoleView_ShowHelp()
{
    ConsoleView_Print(_T("Console command list:\r\n")
            _T("  c          Clear console log\r\n")
            _T("  d          Disassemble\r\n")
            _T("  g          Go; free run\r\n")
            _T("  m          Memory dump at current address\r\n")
            _T("  s          Step Into; executes one instruction\r\n")
            _T("  u          Save memory dump to file memdump.bin\r\n"));
}

void DoConsoleCommand()
{
    // Get command text
    TCHAR command[32];
    GetWindowText(m_hwndConsoleEdit, command, 32);
    SendMessage(m_hwndConsoleEdit, WM_SETTEXT, 0, (LPARAM) _T(""));  // Clear command

    if (command[0] == 0) return;  // Nothing to do

    // Echo command to the log
    TCHAR buffer[36];
    ::GetWindowText(m_hwndConsolePrompt, buffer, 14);
    ConsoleView_Print(buffer);
    wsprintf(buffer, _T(" %s\r\n"), command);
    ConsoleView_Print(buffer);

    BOOL okUpdateAllViews = FALSE;  // Flag - need to update all debug views

    // Execute the command
    switch (command[0])
    {
    case _T('h'):
        ConsoleView_ShowHelp();
        break;
    case _T('c'):  // Clear log
        ClearConsole();
        break;
    case _T('s'):  // Step
        if (command[1] == 0)  // "s" - Step Into, execute one instruction
        {
            PrintDisassemble(Emulator_GetPC(), TRUE);

            Emulator_Step();

            okUpdateAllViews = TRUE;
        }
        break;
    case _T('d'):  // Disassemble
        {
            if (command[1] == 0)  // "d" - disassemble at current address
                PrintDisassemble(Emulator_GetPC(), FALSE);
            else if (command[1] >= _T('0') && command[1] <= _T('7') || command[1] >= _T('a') && command[1] <= _T('f'))  // "dXXXX" - disassemble at address XXXX
            {
                WORD value;
                if (! ParseHexValue(command + 1, &value))
                    ConsoleView_Print(MESSAGE_WRONG_VALUE);
                else
                {
                    PrintDisassemble(value, FALSE);
                }
            }
            else
                ConsoleView_Print(MESSAGE_UNKNOWN_COMMAND);
        }
        break;
    case _T('u'):
        SaveMemoryDump();
        break;
    case _T('m'):
        if (command[1] == 0)  // "m" - dump memory at current address
        {
            PrintMemoryDump(Emulator_GetPC(), 8);
        }
        else
            ConsoleView_Print(MESSAGE_UNKNOWN_COMMAND);
        break;
    case _T('g'):
        if (command[1] == 0)
        {
            Emulator_Start();
        }
        break;
#if !defined(PRODUCT)
    case _T('t'):
        {
            bool okTrace = !Emulator_IsTraceEnabled();
            Emulator_EnableTrace(okTrace);
            if (okTrace)
                ConsoleView_Print(_T("  Trace is ON.\r\n"));
            else
            {
                ConsoleView_Print(_T("  Trace is OFF.\r\n"));
                DebugLogCloseFile();
            }
        }
        break;
#endif
    default:
        ConsoleView_Print(MESSAGE_UNKNOWN_COMMAND);
        break;
    }

    PrintConsolePrompt();

    if (okUpdateAllViews)
    {
        MainWindow_UpdateAllViews();
    }
}


//////////////////////////////////////////////////////////////////////
