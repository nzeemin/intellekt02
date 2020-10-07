/*  This file is part of ITELLEKT02.
    ITELLEKT02 is free software: you can redistribute it and/or modify it under the terms
of the GNU Lesser General Public License as published by the Free Software Foundation,
either version 3 of the License, or (at your option) any later version.
    ITELLEKT02 is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.
    You should have received a copy of the GNU Lesser General Public License along with
ITELLEKT02. If not, see <http://www.gnu.org/licenses/>. */

// DebugView.cpp

#include "stdafx.h"
#include <commctrl.h>
#include "Main.h"
#include "Views.h"
#include "ToolWindow.h"
#include "i8080.h"
#include "Emulator.h"

//////////////////////////////////////////////////////////////////////


// Colors
#define COLOR_RED   RGB(255,0,0)
#define COLOR_BLUE  RGB(0,0,255)


HWND g_hwndDebug = (HWND) INVALID_HANDLE_VALUE;  // Debug View window handle
WNDPROC m_wndprocDebugToolWindow = NULL;  // Old window proc address of the ToolWindow

HWND m_hwndDebugViewer = (HWND) INVALID_HANDLE_VALUE;
HWND m_hwndDebugToolbar = (HWND) INVALID_HANDLE_VALUE;


void DebugView_DoDraw(HDC hdc);
BOOL DebugView_OnKeyDown(WPARAM vkey, LPARAM lParam);
void DebugView_UpdateWindowText();


//////////////////////////////////////////////////////////////////////


void DebugView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = DebugViewViewerWndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = g_hInst;
    wcex.hIcon          = NULL;
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName   = NULL;
    wcex.lpszClassName  = CLASSNAME_DEBUGVIEW;
    wcex.hIconSm        = NULL;

    RegisterClassEx(&wcex);
}

void DebugView_Init()
{
}

void DebugView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndDebug = CreateWindow(
            CLASSNAME_TOOLWINDOW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
    DebugView_UpdateWindowText();

    // ToolWindow subclassing
    m_wndprocDebugToolWindow = (WNDPROC) LongToPtr( SetWindowLongPtr(
            g_hwndDebug, GWLP_WNDPROC, PtrToLong(DebugViewWndProc)) );

    RECT rcClient;  GetClientRect(g_hwndDebug, &rcClient);

    m_hwndDebugViewer = CreateWindowEx(
            WS_EX_STATICEDGE,
            CLASSNAME_DEBUGVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            0, 0, rcClient.right, rcClient.bottom,
            g_hwndDebug, NULL, g_hInst, NULL);

    m_hwndDebugToolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
            WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TRANSPARENT | TBSTYLE_TOOLTIPS | CCS_NOPARENTALIGN | CCS_NODIVIDER | CCS_VERT,
            4, 4, 32, rcClient.bottom, m_hwndDebugViewer,
            (HMENU) 102,
            g_hInst, NULL);

    TBADDBITMAP addbitmap;
    addbitmap.hInst = g_hInst;
    addbitmap.nID = IDB_TOOLBAR;
    SendMessage(m_hwndDebugToolbar, TB_ADDBITMAP, 2, (LPARAM) &addbitmap);

    SendMessage(m_hwndDebugToolbar, TB_BUTTONSTRUCTSIZE, (WPARAM) sizeof(TBBUTTON), 0);
    SendMessage(m_hwndDebugToolbar, TB_SETBUTTONSIZE, 0, (LPARAM) MAKELONG (26, 26));

    TBBUTTON buttons[2];
    ZeroMemory(buttons, sizeof(buttons));
    for (int i = 0; i < sizeof(buttons) / sizeof(TBBUTTON); i++)
    {
        buttons[i].fsState = TBSTATE_ENABLED | TBSTATE_WRAP;
        buttons[i].fsStyle = BTNS_BUTTON;
        buttons[i].iString = -1;
    }
    buttons[0].idCommand = ID_DEBUG_STEPINTO;
    buttons[0].iBitmap = 15;
    buttons[1].idCommand = ID_DEBUG_STEPOVER;
    buttons[1].iBitmap = 16;

    SendMessage(m_hwndDebugToolbar, TB_ADDBUTTONS, (WPARAM) sizeof(buttons) / sizeof(TBBUTTON), (LPARAM) &buttons);
}

// Adjust position of client windows
void DebugView_AdjustWindowLayout()
{
    RECT rc;  GetClientRect(g_hwndDebug, &rc);

    if (m_hwndDebugViewer != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndDebugViewer, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

LRESULT CALLBACK DebugViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LRESULT lResult;
    switch (message)
    {
    case WM_DESTROY:
        g_hwndDebug = (HWND) INVALID_HANDLE_VALUE;  // We are closed! Bye-bye!..
        return CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
    case WM_SIZE:
        lResult = CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
        DebugView_AdjustWindowLayout();
        return lResult;
    default:
        return CallWindowProc(m_wndprocDebugToolWindow, hWnd, message, wParam, lParam);
    }
    //return (LRESULT)FALSE;
}

LRESULT CALLBACK DebugViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_COMMAND:
        ::PostMessage(g_hwnd, WM_COMMAND, wParam, lParam);
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            DebugView_DoDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
    case WM_KEYDOWN:
        return (LRESULT) DebugView_OnKeyDown(wParam, lParam);
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        ::InvalidateRect(hWnd, NULL, TRUE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

BOOL DebugView_OnKeyDown(WPARAM vkey, LPARAM /*lParam*/)
{
    switch (vkey)
    {
    case VK_ESCAPE:
        ConsoleView_Activate();
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

void DebugView_UpdateWindowText()
{
    ::SetWindowText(g_hwndDebug, _T("Debug"));
}


//////////////////////////////////////////////////////////////////////

// Update after Run or Step
void DebugView_OnUpdate()
{
}


//////////////////////////////////////////////////////////////////////
// Draw functions

void DebugView_DrawProcessor(HDC hdc, int x, int y);
void DebugView_DrawMemoryForSP(HDC hdc, int x, int y);

void DebugView_DoDraw(HDC hdc)
{
    // Create and select font
    HFONT hFont = CreateMonospacedFont();
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorOld = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    COLORREF colorBkOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    DebugView_DrawProcessor(hdc, 30 + cxChar * 2, 2 + 1 * cyLine);

    DebugView_DrawMemoryForSP(hdc, 30 + cxChar * 30, 2 + 0 * cyLine);

    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void DebugView_DrawRectangle(HDC hdc, int x1, int y1, int x2, int y2)
{
    HGDIOBJ hOldBrush = ::SelectObject(hdc, ::GetSysColorBrush(COLOR_BTNSHADOW));
    PatBlt(hdc, x1, y1, x2 - x1, 1, PATCOPY);
    PatBlt(hdc, x1, y1, 1, y2 - y1, PATCOPY);
    PatBlt(hdc, x1, y2, x2 - x1, 1, PATCOPY);
    PatBlt(hdc, x2, y1, 1, y2 - y1 + 1, PATCOPY);
    ::SelectObject(hdc, hOldBrush);
}

void DebugView_DrawProcessor(HDC hdc, int x, int y)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorText = GetSysColor(COLOR_WINDOWTEXT);

    DebugView_DrawRectangle(hdc, x - cxChar, y - 8, x + cxChar + 24 * cxChar, y + 8 + cyLine * 10);

    WORD wvalue;
    TCHAR buffer[17];

    TextOut(hdc, x, y + 0 * cyLine, _T("A"), 1);
    wvalue = i8080_regs_a();
    PrintHexValue(buffer, wvalue);
    TextOut(hdc, x + cxChar * 3, y + 0 * cyLine, buffer + 2, 2);
    PrintBinaryValue(buffer, wvalue);
    TextOut(hdc, x + cxChar * 8, y + 0 * cyLine, buffer + 8, 8);

    wvalue = i8080_regs_f();
    TextOut(hdc, x + cxChar * 8, y + 7 * cyLine, _T("SZIH-P-C"), 8);
    TextOut(hdc, x, y + 8 * cyLine, _T("Flags"), 5);
    TextOut(hdc, x + cxChar * 8, y + 8 * cyLine, buffer + 8, 8);

    wvalue = i8080_regs_bc();
    TextOut(hdc, x, y + 1 * cyLine, _T("BC"), 2);
    DrawHexValue(hdc, x + cxChar * 3, y + 1 * cyLine, wvalue);
    DrawBinaryValue(hdc, x + cxChar * 8, y + 1 * cyLine, wvalue);

    wvalue = i8080_regs_de();
    TextOut(hdc, x, y + 2 * cyLine, _T("DE"), 2);
    DrawHexValue(hdc, x + cxChar * 3, y + 2 * cyLine, wvalue);
    DrawBinaryValue(hdc, x + cxChar * 8, y + 2 * cyLine, wvalue);

    wvalue = i8080_regs_hl();
    TextOut(hdc, x, y + 3 * cyLine, _T("HL"), 2);
    DrawHexValue(hdc, x + cxChar * 3, y + 3 * cyLine, wvalue);
    DrawBinaryValue(hdc, x + cxChar * 8, y + 3 * cyLine, wvalue);

    wvalue = i8080_regs_sp();
    TextOut(hdc, x, y + 4 * cyLine, _T("SP"), 2);
    DrawHexValue(hdc, x + cxChar * 3, y + 4 * cyLine, wvalue);
    DrawBinaryValue(hdc, x + cxChar * 8, y + 4 * cyLine, wvalue);

    wvalue = i8080_pc();
    TextOut(hdc, x, y + 5 * cyLine, _T("PC"), 2);
    DrawHexValue(hdc, x + cxChar * 3, y + 5 * cyLine, wvalue);
    DrawBinaryValue(hdc, x + cxChar * 8, y + 5 * cyLine, wvalue);
}

void DebugView_DrawMemoryForSP(HDC hdc, int x, int y)
{
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorText = GetSysColor(COLOR_WINDOWTEXT);
    COLORREF colorOld = SetTextColor(hdc, colorText);

    WORD current = i8080_regs_sp();

    // Читаем из памяти процессора в буфер
    WORD memory[16];
    for (int idx = 0; idx < 16; idx++)
    {
        BYTE lo = Emulator_GetMemoryByte(current + idx * 2 - 12);
        BYTE hi = Emulator_GetMemoryByte(current + idx * 2 - 12 + 1);
        memory[idx] = MAKEWORD(lo, hi);
    }

    WORD address = current - 12;
    for (int index = 0; index < 12; index++)    // Рисуем строки
    {
        // Адрес
        SetTextColor(hdc, colorText);
        DrawHexValue(hdc, x + 4 * cxChar, y, address);

        // Значение по адресу
        WORD value = memory[index];
        //WORD wChanged = Emulator_GetChangeRamStatus(address);
        //SetTextColor(hdc, (wChanged != 0) ? RGB(255, 0, 0) : colorText);
        DrawHexValue(hdc, x + 10 * cxChar, y, value);

        // Текущая позиция
        if (address == current)
        {
            SetTextColor(hdc, colorText);
            TextOut(hdc, x + 0 * cxChar, y, _T("SP>>"), 4);
            //if (current != previous) SetTextColor(hdc, COLOR_RED);
        }

        address += 2;
        y += cyLine;
    }

    SetTextColor(hdc, colorOld);
}


//////////////////////////////////////////////////////////////////////
