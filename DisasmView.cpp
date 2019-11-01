
// DisasmView.cpp

#include "stdafx.h"
#include <commdlg.h>
#include "Main.h"
#include "Views.h"
#include "ToolWindow.h"
#include "Emulator.h"
#include "i8080dis.h"


//////////////////////////////////////////////////////////////////////

// Colors
#define COLOR_RED       RGB(255,0,0)
#define COLOR_BLUE      RGB(0,0,255)
#define COLOR_SUBTITLE  RGB(0,128,0)
#define COLOR_VALUE     RGB(128,128,128)
#define COLOR_VALUEROM  RGB(128,128,192)
#define COLOR_JUMP      RGB(80,192,224)
#define COLOR_CURRENT   RGB(255,255,224)


HWND g_hwndDisasm = (HWND) INVALID_HANDLE_VALUE;  // Disasm View window handle
WNDPROC m_wndprocDisasmToolWindow = NULL;  // Old window proc address of the ToolWindow

HWND m_hwndDisasmViewer = (HWND) INVALID_HANDLE_VALUE;

WORD m_wDisasmBaseAddr = 0;
WORD m_wDisasmNextBaseAddr = 0;

void DisasmView_DoDraw(HDC hdc);
int  DisasmView_DrawDisassemble(HDC hdc, WORD base, WORD previous, int x, int y);
BOOL DisasmView_OnKeyDown(WPARAM vkey, LPARAM lParam);
void DisasmView_SetBaseAddr(WORD base);

BOOL m_okDisasmSubtitles = FALSE;
TCHAR* m_strDisasmSubtitles = NULL;

//////////////////////////////////////////////////////////////////////


void DisasmView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= DisasmViewViewerWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_DISASMVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void DisasmView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndDisasm = CreateWindow(
            CLASSNAME_TOOLWINDOW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
    ::SetWindowText(g_hwndDisasm, _T("Disassemble"));

    // ToolWindow subclassing
    m_wndprocDisasmToolWindow = (WNDPROC) LongToPtr( SetWindowLongPtr(
            g_hwndDisasm, GWLP_WNDPROC, PtrToLong(DisasmViewWndProc)) );

    RECT rcClient;  GetClientRect(g_hwndDisasm, &rcClient);

    m_hwndDisasmViewer = CreateWindowEx(
            WS_EX_STATICEDGE,
            CLASSNAME_DISASMVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            0, 0, rcClient.right, rcClient.bottom,
            g_hwndDisasm, NULL, g_hInst, NULL);
}

// Adjust position of client windows
void DisasmView_AdjustWindowLayout()
{
    RECT rc;  GetClientRect(g_hwndDisasm, &rc);

    if (m_hwndDisasmViewer != (HWND) INVALID_HANDLE_VALUE)
        SetWindowPos(m_hwndDisasmViewer, NULL, 0, 0, rc.right, rc.bottom, SWP_NOZORDER);
}

LRESULT CALLBACK DisasmViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    LRESULT lResult;
    switch (message)
    {
    case WM_DESTROY:
        g_hwndDisasm = (HWND) INVALID_HANDLE_VALUE;  // We are closed! Bye-bye!..
        return CallWindowProc(m_wndprocDisasmToolWindow, hWnd, message, wParam, lParam);
    case WM_SIZE:
        lResult = CallWindowProc(m_wndprocDisasmToolWindow, hWnd, message, wParam, lParam);
        DisasmView_AdjustWindowLayout();
        return lResult;
    default:
        return CallWindowProc(m_wndprocDisasmToolWindow, hWnd, message, wParam, lParam);
    }
    //return (LRESULT)FALSE;
}

LRESULT CALLBACK DisasmViewViewerWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            DisasmView_DoDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
    case WM_KEYDOWN:
        return (LRESULT) DisasmView_OnKeyDown(wParam, lParam);
    case WM_SETFOCUS:
    case WM_KILLFOCUS:
        ::InvalidateRect(hWnd, NULL, TRUE);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

BOOL DisasmView_OnKeyDown(WPARAM vkey, LPARAM /*lParam*/)
{
    switch (vkey)
    {
        //case 0x53:  // S - Load/Unload Subtitles
        //    DisasmView_DoSubtitles();
        //    break;
    case VK_ESCAPE:
        ConsoleView_Activate();
        break;
    default:
        return TRUE;
    }
    return FALSE;
}

void DisasmView_UpdateWindowText()
{
    ::SetWindowText(g_hwndDisasm, _T("Disassemble"));
}


//////////////////////////////////////////////////////////////////////


// Update after Run or Step
void DisasmView_OnUpdate()
{
    m_wDisasmBaseAddr = Emulator_GetPC();
}

void DisasmView_SetBaseAddr(WORD base)
{
    m_wDisasmBaseAddr = base;
    InvalidateRect(m_hwndDisasmViewer, NULL, TRUE);
}


//////////////////////////////////////////////////////////////////////
// Draw functions


void DisasmView_DoDraw(HDC hdc)
{
    // Create and select font
    HFONT hFont = CreateMonospacedFont();
    HGDIOBJ hOldFont = SelectObject(hdc, hFont);
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorOld = SetTextColor(hdc, GetSysColor(COLOR_WINDOWTEXT));
    SetBkMode(hdc, TRANSPARENT);
    //COLORREF colorBkOld = SetBkColor(hdc, GetSysColor(COLOR_WINDOW));

    WORD prevPC = g_wEmulatorPrevCpuPC;
    int yFocus = DisasmView_DrawDisassemble(hdc, m_wDisasmBaseAddr, prevPC, 0, 2 + 0 * cyLine);

    SetTextColor(hdc, colorOld);
    //SetBkColor(hdc, colorBkOld);
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    if (::GetFocus() == m_hwndDisasmViewer)
    {
        RECT rcFocus;
        GetClientRect(m_hwndDisasmViewer, &rcFocus);
        if (yFocus >= 0)
        {
            rcFocus.top = yFocus - 1;
            rcFocus.bottom = yFocus + cyLine;
        }
        DrawFocusRect(hdc, &rcFocus);
    }
}


int DisasmView_DrawDisassemble(HDC hdc, WORD base, WORD previous, int x, int y)
{
    int result = -1;
    int cxChar, cyLine;  GetFontWidthAndHeight(hdc, &cxChar, &cyLine);
    COLORREF colorText = GetSysColor(COLOR_WINDOWTEXT);

    WORD proccurrent = Emulator_GetPC();
    WORD current = base;

    // Draw current line background
    //if (!m_okDisasmSubtitles)  //NOTE: Subtitles can move lines down
    {
        HBRUSH hBrushCurrent = ::CreateSolidBrush(COLOR_CURRENT);
        HGDIOBJ oldBrush = ::SelectObject(hdc, hBrushCurrent);
        int yCurrent = (proccurrent - (current - 5)) * cyLine;
        PatBlt(hdc, 0, yCurrent, 1000, cyLine, PATCOPY);
        ::SelectObject(hdc, oldBrush);
        ::DeleteObject(hBrushCurrent);
    }

    // Читаем из памяти процессора в буфер
    const int nWindowSize = 60;
    BYTE memory[nWindowSize + 2];
    for (int idx = 0; idx < nWindowSize + 2; idx++)
    {
        memory[idx] = Emulator_GetMemoryByte(current + idx - 5);
    }

    WORD address = current - 5;
    WORD disasmfrom = current;

    int length = 0;
    WORD wNextBaseAddr = 0;
    for (int index = 0; index < nWindowSize; )  // Рисуем строки
    {
        DrawHexValue(hdc, x + 5 * cxChar, y, address);  // Address

        // Current position
        if (address == current)
        {
            TextOut(hdc, x + 1 * cxChar, y, _T("  >"), 3);
            result = y;  // Remember line for the focus rect
            TextOut(hdc, x + 1 * cxChar, y, _T("PC"), 2);
            TextOut(hdc, x + 3 * cxChar, y, _T(">>"), 2);
        }

        length = 1;
        if (address - disasmfrom >= 0 && address - disasmfrom < 32768)
        {
            char disasm[32];  disasm[0] = 0;
            TCHAR tdisasm[32];
            length = i8080_disasm(memory + index, disasm, 32);
            wsprintf(tdisasm, _T("%S"), disasm);
            TextOut(hdc, x + 12 * cxChar, y, tdisasm, lstrlen(tdisasm));
        }

        y += cyLine;
        address += length;
        index += length;
    }

    return result;
}


//////////////////////////////////////////////////////////////////////
