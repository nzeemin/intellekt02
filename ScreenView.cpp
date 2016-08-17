
// ScreenView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "Emulator.h"


//////////////////////////////////////////////////////////////////////

#define COLOR_BK_BACKGROUND RGB(140,140,140)
#define COLOR_BOARD_WHITE RGB(240,240,240)
#define COLOR_BOARD_BLACK RGB(180,180,180)
#define COLOR_BOARD_TEXT RGB(240,240,240)

HWND g_hwndScreen = NULL;  // Screen View window handle

BYTE m_arrScreen_BoardData[64];

int m_cxScreenWidth = 320;
int m_cyScreenHeight = 354;

void ScreenView_OnDraw(HDC hdc);
BOOL ScreenView_OnKeyEvent(WPARAM vkey, BOOL okExtKey, BOOL okPressed);


//////////////////////////////////////////////////////////////////////

void ScreenView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= ScreenViewWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= NULL; //(HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_SCREENVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void ScreenView_Init()
{
    memset(m_arrScreen_BoardData, 0, sizeof(m_arrScreen_BoardData));
}

void ScreenView_Done()
{
}


// Create Screen View as child of Main Window
void CreateScreenView(HWND hwndParent, int x, int y, int cxWidth)
{
    ASSERT(hwndParent != NULL);

    int xLeft = x;
    int yTop = y;
    int cyScreenHeight = 4 + m_cyScreenHeight + 4;
    int cyHeight = cyScreenHeight;
    cxWidth = 4 + m_cxScreenWidth + 4;

    g_hwndScreen = CreateWindow(
            CLASSNAME_SCREENVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            xLeft, yTop, cxWidth, cyHeight,
            hwndParent, NULL, g_hInst, NULL);
}

LRESULT CALLBACK ScreenViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            //ScreenView_PrepareScreen();  //DEBUG
            ScreenView_OnDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(hWnd);
        break;
    case WM_KEYDOWN:
        if ((lParam & (1 << 30)) != 0)
            return (LRESULT)TRUE;
        return (LRESULT)ScreenView_OnKeyEvent(wParam, (lParam & (1 << 24)) != 0, TRUE);
    case WM_KEYUP:
        return (LRESULT)ScreenView_OnKeyEvent(wParam, (lParam & (1 << 24)) != 0, FALSE);
    case WM_SETCURSOR:
        if (::GetFocus() == g_hwndScreen)
        {
            SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_IBEAM)));
            return (LRESULT) TRUE;
        }
        else
            return DefWindowProc(hWnd, message, wParam, lParam);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

BOOL ScreenView_OnKeyEvent(WPARAM vkey, BOOL okExtKey, BOOL okPressed)
{
    BYTE keyscan;
    switch (vkey)
    {
        //case 0x30: // "0"
    case 0x31: case 0x41: keyscan = 0xf1;  break;  // "1", "A"
    case 0x32: case 0x42: keyscan = 0xf2;  break;  // "2", "B"
    case 0x33: case 0x43: keyscan = 0xf3;  break;  // "3", "C"
    case 0x34: case 0x44: keyscan = 0xf4;  break;  // "4", "D"
    case 0x35: case 0x45: keyscan = 0xf5;  break;  // "5", "E"
    case 0x36: case 0x46: keyscan = 0xf6;  break;  // "6", "F"
    case 0x37: case 0x47: keyscan = 0xf7;  break;  // "7", "G"
    case 0x38: case 0x48: keyscan = 0xf8;  break;  // "8", "H"
    case 0x39: keyscan = 0xf9;  break;  // "9"
    case VK_RETURN: keyscan = 0xdf;  break;  // "ÂÂ"
    default:
        return FALSE;
    }

    Emulator_KeyEvent(keyscan, okPressed);

    return TRUE;
}

void ScreenView_OnDraw(HDC hdc)
{
    RECT rc;  ::GetClientRect(g_hwndScreen, &rc);
    int x = (rc.right - m_cxScreenWidth) / 2;
    int boardy = 20;

    // Empty border
    HBRUSH hBrush = ::CreateSolidBrush(COLOR_BK_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBrush);
    PatBlt(hdc, 0, 0, rc.right, boardy, PATCOPY);
    PatBlt(hdc, 0, 0, x, rc.bottom, PATCOPY);
    PatBlt(hdc, x + m_cxScreenWidth, 0, rc.right - x - m_cxScreenWidth, rc.bottom, PATCOPY);
    PatBlt(hdc, 0, boardy + 8 * 40, rc.right, rc.bottom - boardy + 8 * 40, PATCOPY);
    ::SelectObject(hdc, hOldBrush);
    ::DeleteObject(hBrush);

    HBITMAP hBmpPieces = ::LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CHESS_PIECES));
    HDC hdcMem = ::CreateCompatibleDC(hdc);
    HGDIOBJ hOldBitmap = ::SelectObject(hdcMem, hBmpPieces);

    HBRUSH hbrWhite = ::CreateSolidBrush(COLOR_BOARD_WHITE);
    HBRUSH hbrBlack = ::CreateSolidBrush(COLOR_BOARD_BLACK);
    HFONT hfont = CreateDialogFont();
    HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
    ::SetBkMode(hdc, TRANSPARENT);
    TCHAR buffer[5];
    for (int yy = 0; yy < 8; yy++)
    {
        for (int xx = 0; xx < 8; xx++)
        {
            // Board cell background
            HBRUSH hbr = ((xx ^ yy) & 1) ? hbrBlack : hbrWhite;
            HGDIOBJ hbrOld = ::SelectObject(hdc, hbr);
            PatBlt(hdc, x + xx * 40, boardy + (7 - yy) * 40, 40, 40, PATCOPY);
            ::SelectObject(hdc, hbrOld);

            int index = yy * 8 + xx;
            int figure = m_arrScreen_BoardData[index];
            if (figure != 0)
            {
                RECT rcText;
                rcText.left = x + xx * 40;
                rcText.top = boardy + (7 - yy) * 40;
                rcText.right = rcText.left + 40;
                rcText.bottom = rcText.top + 40;

                int figurex = (6 - ((figure & 0xf) >> 1)) * 40;
                int figurey = (figure & 0x80) ? 0 : 40;
                ::TransparentBlt(hdc, x + xx * 40, boardy + (7 - yy) * 40, 40, 40, hdcMem, figurex, figurey, 40, 40, RGB(128, 128, 128));

                //PrintHexValue(buffer, figure);
                //::DrawText(hdc, buffer + 2, 2, &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_BOTTOM);
            }
        }

        buffer[0] = _T('1') + yy;
        buffer[1] = 0;
        ::SetTextColor(hdc, COLOR_BOARD_TEXT);
        ::TextOut(hdc, x + 8 * 40 + 4, boardy + (7 - yy) * 40 + 20, buffer, 1);
        ::TextOut(hdc, x - 12, boardy + (7 - yy) * 40 + 20, buffer, 1);
    }
    for (int xx = 0; xx < 8; xx++)
    {
        buffer[0] = _T('a') + xx;
        buffer[1] = 0;
        ::SetTextColor(hdc, COLOR_BOARD_TEXT);
        ::TextOut(hdc, x + xx * 40 + 20, 2, buffer, 1);
        ::TextOut(hdc, x + xx * 40 + 20, boardy + 8 * 40 + 2, buffer, 1);
    }

    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hfont);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrBlack);

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteDC(hdcMem);
    ::DeleteObject(hBmpPieces);
}

void ScreenView_RedrawScreen()
{
    bool okChanged = false;
    for (int index = 0; index < 64; index++)
    {
        BYTE oldfigure = m_arrScreen_BoardData[index];
        BYTE figure = Emulator_GetMemoryByte(0xf000 + index);
        okChanged |= (oldfigure != figure);
        m_arrScreen_BoardData[index] = figure;
    }

    if (okChanged)
    {
        HDC hdc = GetDC(g_hwndScreen);
        ScreenView_OnDraw(hdc);
        ::ReleaseDC(g_hwndScreen, hdc);
    }
}

//////////////////////////////////////////////////////////////////////
