
// ScreenView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "Emulator.h"


//////////////////////////////////////////////////////////////////////

#define COLOR_BK_BACKGROUND RGB(140,140,140)
#define COLOR_BOARD_WHITE RGB(240,240,240)
#define COLOR_BOARD_BLACK RGB(180,180,180)

HWND g_hwndScreen = NULL;  // Screen View window handle

int m_cxScreenWidth = 320;
int m_cyScreenHeight = 320;

void ScreenView_OnDraw(HDC hdc);


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
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

void ScreenView_OnDraw(HDC hdc)
{
    RECT rc;  ::GetClientRect(g_hwndScreen, &rc);
    int x = (rc.right - m_cxScreenWidth) / 2;

    HBITMAP hBmpPieces = ::LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_CHESS_PIECES));
    HDC hdcMem = ::CreateCompatibleDC(hdc);
    HGDIOBJ hOldBitmap = ::SelectObject(hdcMem, hBmpPieces);

    HBRUSH hbrWhite = ::CreateSolidBrush(COLOR_BOARD_WHITE);
    HBRUSH hbrBlack = ::CreateSolidBrush(COLOR_BOARD_BLACK);
    HFONT hfont = CreateDialogFont();
    HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
    ::SetBkMode(hdc, TRANSPARENT);
    for (int yy = 0; yy < 8; yy++)
    {
        for (int xx = 0; xx < 8; xx++)
        {
            // Background
            HBRUSH hbr = ((xx ^ yy) & 1) ? hbrBlack : hbrWhite;
            HGDIOBJ hbrOld = ::SelectObject(hdc, hbr);
            PatBlt(hdc, x + xx * 40, (7 - yy) * 40, 40, 40, PATCOPY);
            ::SelectObject(hdc, hbrOld);

            int index = yy * 8 + xx;
            int figure = Emulator_GetMemoryByte(0xf000 + index);
            if (figure != 0)
            {
                RECT rcText;
                rcText.left = x + xx * 40;
                rcText.top = (7 - yy) * 40;
                rcText.right = rcText.left + 40;
                rcText.bottom = rcText.top + 40;

                int figurex = (6 - ((figure & 0xf) >> 1)) * 40;
                int figurey = (figure & 0x80) ? 0 : 40;
                ::TransparentBlt(hdc, x + xx * 40, (7 - yy) * 40, 40, 40, hdcMem, figurex, figurey, 40, 40, RGB(128, 128, 128));

                //TCHAR buffer[5];
                //PrintHexValue(buffer, figure);
                //::DrawText(hdc, buffer + 2, 2, &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_BOTTOM);
            }
        }
    }
    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hfont);
    ::DeleteObject(hbrWhite);
    ::DeleteObject(hbrBlack);

    ::SelectObject(hdcMem, hOldBitmap);
    ::DeleteDC(hdcMem);
    ::DeleteObject(hBmpPieces);

    // Empty border
    HBRUSH hBrush = ::CreateSolidBrush(COLOR_BK_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBrush);
    PatBlt(hdc, 0, 0, rc.right, 4, PATCOPY);
    PatBlt(hdc, 0, 0, x, rc.bottom, PATCOPY);
    PatBlt(hdc, x + m_cxScreenWidth, 0, rc.right - x - m_cxScreenWidth, rc.bottom, PATCOPY);
    PatBlt(hdc, 0, rc.bottom, rc.right, -4, PATCOPY);
    ::SelectObject(hdc, hOldBrush);
    ::DeleteObject(hBrush);
}

void ScreenView_RedrawScreen()
{
    HDC hdc = GetDC(g_hwndScreen);
    ScreenView_OnDraw(hdc);
    ::ReleaseDC(g_hwndScreen, hdc);
}

//////////////////////////////////////////////////////////////////////
