
// KeyboardView.cpp

#include "stdafx.h"
#include "Main.h"
#include "Views.h"
#include "Emulator.h"


//////////////////////////////////////////////////////////////////////

#define COLOR_BK_BACKGROUND   RGB(115,115,115)
#define COLOR_KEYBOARD_LITE   RGB(228,228,228)
#define COLOR_KEYBOARD_DARK   RGB(80,80,80)
#define COLOR_KEYBOARD_RED    RGB(255,80,80)
#define COLOR_KEYBOARD_GREEN  RGB(80,255,80)

#define KEYSCAN_NONE 255
#define KEYSCAN_RESET  1

HWND g_hwndKeyboard = (HWND) INVALID_HANDLE_VALUE;  // Keyboard View window handle

int m_nKeyboardBitmapLeft = 0;
int m_nKeyboardBitmapTop = 0;
BYTE m_nKeyboardKeyPressed = KEYSCAN_NONE;  // Scan-code for the key pressed, or KEYSCAN_NONE
BYTE m_arrKeyboardSegmentsData[4] = { 0, 0, 0, 0 };

void KeyboardView_OnDraw(HDC hdc);
int KeyboardView_GetKeyByPoint(int x, int y);
void Keyboard_DrawKey(HDC hdc, BYTE keyscan);


//////////////////////////////////////////////////////////////////////

struct KeyboardKeys
{
    int x, y, w, h;
    LPCTSTR text;
    BYTE scan;
}
m_arrKeyboardKeys[] =
{
    // 1st row
    { 340, 20, 25, 20, _T("СБ"), 0xef },
    { 375, 20, 25, 20, _T("ВИ"), 0xbf }, // good
    { 410, 20, 25, 20, _T("ПП"), 0 },
    { 445, 20, 25, 20, _T("A1"), 0xf1 }, // good
    { 480, 20, 25, 20, _T("B2"), 0xf2 }, // good
    { 515, 20, 25, 20, _T("C3"), 0xf3 }, // good
    { 550, 20, 25, 20, _T("D4"), 0xf4 }, // good
    { 585, 20, 25, 20, _T("E5"), 0xf5 }, // good
    // 2nd row
    { 340, 60, 25, 20, _T("ВВ"), 0xdf }, // good
    { 375, 60, 25, 20, _T("СТ"), 0x7f }, // good
    { 410, 60, 25, 20, _T("УИ"), 0xfa }, // good
    { 445, 60, 25, 20, _T("F6"), 0xf6 }, // good
    { 480, 60, 25, 20, _T("G7"), 0xf7 }, // good
    { 515, 60, 25, 20, _T("H8"), 0xf8 }, // good
    { 550, 60, 25, 20, _T("9" ), 0xf9 },
    { 585, 60, 25, 20, _T("0" ), 0xf0 },
};

const int m_nKeyboardKeysCount = sizeof(m_arrKeyboardKeys) / sizeof(KeyboardKeys);

struct KeyboardIndicator
{
    int x, y, w, h;
    LPCTSTR text;
    bool state;
}
m_arrKeyboardIndicators[] =
{
    {   0, 24, 90, 14, _T("вы выиграли"),  false },
    {   0, 56, 90, 14, _T("вы проиграли"), false },
};
const int m_nKeyboardIndicatorsCount = sizeof(m_arrKeyboardIndicators) / sizeof(KeyboardIndicator);

struct IndicatorSegment
{
    int x0, y0, x1, y1, x2, y2, x3, y3;
}
m_arrIndicatorSegments[] =
{
    { 13, 17, 31, 17, 29, 23, 11, 23 }, // g
    { 12,  0, 18,  0, 12, 20,  6, 20 }, // f
    {  6, 20, 12, 20,  6, 40,  0, 40 }, // e
    {  6, 40, 24, 40, 26, 34,  8, 34 }, // d
    { 30, 20, 36, 20, 30, 40, 24, 40 }, // c
    { 36,  0, 42,  0, 36, 20, 30, 20 }, // b
    { 18,  0, 36,  0, 34,  6, 16,  6 }, // a
    { 34, 34, 40, 36, 33, 44, 30, 44 }, // ,
};
const int m_nKeyboardSegmentsCount = sizeof(m_arrIndicatorSegments) / sizeof(IndicatorSegment);


//////////////////////////////////////////////////////////////////////


void KeyboardView_RegisterClass()
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= KeyboardViewWndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= g_hInst;
    wcex.hIcon			= NULL;
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_BTNFACE + 1);
    wcex.lpszMenuName	= NULL;
    wcex.lpszClassName	= CLASSNAME_KEYBOARDVIEW;
    wcex.hIconSm		= NULL;

    RegisterClassEx(&wcex);
}

void KeyboardView_Init()
{
}

void KeyboardView_Done()
{
}

void KeyboardView_Create(HWND hwndParent, int x, int y, int width, int height)
{
    ASSERT(hwndParent != NULL);

    g_hwndKeyboard = CreateWindow(
            CLASSNAME_KEYBOARDVIEW, NULL,
            WS_CHILD | WS_VISIBLE,
            x, y, width, height,
            hwndParent, NULL, g_hInst, NULL);
}

LRESULT CALLBACK KeyboardViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);

            KeyboardView_OnDraw(hdc);

            EndPaint(hWnd, &ps);
        }
        break;
    case WM_LBUTTONDOWN:
        {
            int x = LOWORD(lParam);
            int y = HIWORD(lParam);
            WORD fwkeys = wParam;

            int keyindex = KeyboardView_GetKeyByPoint(x, y);
            if (keyindex == -1) break;
            BYTE keyscan = (BYTE) m_arrKeyboardKeys[keyindex].scan;
            if (keyscan == KEYSCAN_NONE) break;

            Emulator_KeyEvent(keyscan, TRUE);

            ::SetCapture(g_hwndKeyboard);

            // Draw focus frame for the key pressed
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, keyscan);
            ::ReleaseDC(g_hwndKeyboard, hdc);

            // Remember key pressed
            m_nKeyboardKeyPressed = keyscan;
        }
        break;
    case WM_LBUTTONUP:
        if (m_nKeyboardKeyPressed != KEYSCAN_NONE)
        {
            Emulator_KeyEvent(m_nKeyboardKeyPressed, FALSE);

            ::ReleaseCapture();

            // Draw focus frame for the released key
            HDC hdc = ::GetDC(g_hwndKeyboard);
            Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);
            ::ReleaseDC(g_hwndKeyboard, hdc);

            m_nKeyboardKeyPressed = KEYSCAN_NONE;
        }
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return (LRESULT)FALSE;
}

void KeyboardView_SetIndicatorData(BYTE indicator, BYTE data)
{
    bool redraw = false;
    if (indicator & 1)
    {
        redraw |= (m_arrKeyboardSegmentsData[0] != data);
        m_arrKeyboardSegmentsData[0] = data;
    }
    if (indicator & 2)
    {
        redraw |= (m_arrKeyboardSegmentsData[1] != data);
        m_arrKeyboardSegmentsData[1] = data;
    }
    if (indicator & 4)
    {
        redraw |= (m_arrKeyboardSegmentsData[2] != data);
        m_arrKeyboardSegmentsData[2] = data;
    }
    if (indicator & 8)
    {
        redraw |= (m_arrKeyboardSegmentsData[3] != data);
        m_arrKeyboardSegmentsData[3] = data;
    }

    bool okYouWin = (indicator & 0x10) != 0;
    redraw |= (m_arrKeyboardIndicators[1].state != okYouWin);
    m_arrKeyboardIndicators[1].state = okYouWin;

    bool okYouLost = (indicator & 0x20) != 0;
    redraw |= (m_arrKeyboardIndicators[0].state != okYouLost);
    m_arrKeyboardIndicators[0].state = okYouLost;

    if (redraw)
        InvalidateRect(g_hwndKeyboard, NULL, FALSE);
}

void Keyboard_DrawKey(HDC hdc, BYTE keyscan)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        if (keyscan == m_arrKeyboardKeys[i].scan)
        {
            RECT rcKey;
            rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
            rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
            rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
            rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;
            ::DrawFocusRect(hdc, &rcKey);
        }
    }
}

void Keyboard_DrawSegment(HDC hdc, IndicatorSegment* pSegment, int x, int y)
{
    POINT points[4];
    points[0].x = x + pSegment->x0;
    points[0].y = y + pSegment->y0;
    points[1].x = x + pSegment->x1;
    points[1].y = y + pSegment->y1;
    points[2].x = x + pSegment->x2;
    points[2].y = y + pSegment->y2;
    points[3].x = x + pSegment->x3;
    points[3].y = y + pSegment->y3;
    Polygon(hdc, points, 4);
}
void KeyboardView_OnDraw(HDC hdc)
{
    RECT rc;  ::GetClientRect(g_hwndKeyboard, &rc);

    // Keyboard background
    HBRUSH hBkBrush = ::CreateSolidBrush(COLOR_BK_BACKGROUND);
    HGDIOBJ hOldBrush = ::SelectObject(hdc, hBkBrush);
    ::PatBlt(hdc, 0, 0, rc.right, rc.bottom, PATCOPY);
    ::SelectObject(hdc, hOldBrush);

    if (m_nKeyboardKeyPressed != KEYSCAN_NONE)
        Keyboard_DrawKey(hdc, m_nKeyboardKeyPressed);

    HBRUSH hbrDark = ::CreateSolidBrush(COLOR_KEYBOARD_DARK);
    HBRUSH hbrRed = ::CreateSolidBrush(COLOR_KEYBOARD_RED);
    HBRUSH hbrGreen = ::CreateSolidBrush(COLOR_KEYBOARD_GREEN);
    HPEN hpenDark = ::CreatePen(PS_SOLID, 1, COLOR_KEYBOARD_DARK);

    m_nKeyboardBitmapLeft = (rc.right - 620) / 2;
    m_nKeyboardBitmapTop = (rc.bottom - 100) / 2;
    if (m_nKeyboardBitmapTop < 0) m_nKeyboardBitmapTop = 0;
    if (m_nKeyboardBitmapTop > 16) m_nKeyboardBitmapTop = 16;

    HFONT hfont = CreateDialogFont();
    HGDIOBJ hOldFont = ::SelectObject(hdc, hfont);
    ::SetBkMode(hdc, TRANSPARENT);

    // Draw keys
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;

        ::DrawEdge(hdc, &rcKey, BDR_RAISEDOUTER, BF_RECT);

        LPCTSTR text = m_arrKeyboardKeys[i].text;
        if (text != NULL)
        {
            rcKey.bottom = rcKey.top - 2;
            rcKey.top -= 20;
            ::SetTextColor(hdc, COLOR_KEYBOARD_LITE);
            ::DrawText(hdc, text, wcslen(text), &rcKey, DT_NOPREFIX | DT_SINGLELINE | DT_CENTER | DT_BOTTOM);
        }
    }

    // Draw indicators
    HGDIOBJ hpenOld = ::SelectObject(hdc, hpenDark);
    for (int i = 0; i < m_nKeyboardIndicatorsCount; i++)
    {
        RECT rcText;
        rcText.left = m_nKeyboardBitmapLeft + m_arrKeyboardIndicators[i].x;
        rcText.top = m_nKeyboardBitmapTop + m_arrKeyboardIndicators[i].y;
        rcText.right = rcText.left + m_arrKeyboardIndicators[i].w;
        rcText.bottom = rcText.top + m_arrKeyboardIndicators[i].h;
        RECT rcRadio;
        rcRadio.left = m_nKeyboardBitmapLeft + m_arrKeyboardIndicators[i].x + m_arrKeyboardIndicators[i].w;
        rcRadio.top = m_nKeyboardBitmapTop + m_arrKeyboardIndicators[i].y;
        rcRadio.right = rcRadio.left + 12;
        rcRadio.bottom = rcRadio.top + 12;

        HBRUSH hbr = m_arrKeyboardIndicators[i].state ? hbrRed : hbrDark;
        HGDIOBJ hOldBrush = ::SelectObject(hdc, hbr);
        ::Ellipse(hdc, rcRadio.left, rcRadio.top, rcRadio.right, rcRadio.bottom);
        ::SelectObject(hdc, hOldBrush);

        ::SetTextColor(hdc, COLOR_KEYBOARD_LITE);
        ::DrawText(hdc, m_arrKeyboardIndicators[i].text, wcslen(m_arrKeyboardIndicators[i].text), &rcText, DT_NOPREFIX | DT_SINGLELINE | DT_LEFT | DT_VCENTER);
    }

    // Draw segment indicators
    for (int ind = 0; ind < 4; ind++)
    {
        BYTE data = m_arrKeyboardSegmentsData[ind];
        int segmentsx = m_nKeyboardBitmapLeft + 120 + ind * 46;
        if (ind >= 2) segmentsx += 15;
        int segmentsy = m_nKeyboardBitmapTop + 25;
        for (int i = 0; i < m_nKeyboardSegmentsCount; i++)
        {
            HBRUSH hbr = (data & (1 << i)) ? hbrGreen : hBkBrush;
            HGDIOBJ hOldBrush = ::SelectObject(hdc, hbr);
            Keyboard_DrawSegment(hdc, m_arrIndicatorSegments + i, segmentsx, segmentsy);
            ::SelectObject(hdc, hOldBrush);
        }
    }
    ::SelectObject(hdc, hpenOld);
    ::DeleteObject(hpenDark);

    ::SelectObject(hdc, hOldFont);
    ::DeleteObject(hfont);
    ::DeleteObject(hbrDark);
    ::DeleteObject(hbrRed);
    ::DeleteObject(hbrGreen);
    ::DeleteObject(hBkBrush);
}

// Returns: index of key under the cursor position, or -1 if not found
int KeyboardView_GetKeyByPoint(int x, int y)
{
    for (int i = 0; i < m_nKeyboardKeysCount; i++)
    {
        RECT rcKey;
        rcKey.left = m_nKeyboardBitmapLeft + m_arrKeyboardKeys[i].x;
        rcKey.top = m_nKeyboardBitmapTop + m_arrKeyboardKeys[i].y;
        rcKey.right = rcKey.left + m_arrKeyboardKeys[i].w;
        rcKey.bottom = rcKey.top + m_arrKeyboardKeys[i].h;

        if (x >= rcKey.left && x < rcKey.right && y >= rcKey.top && y < rcKey.bottom)
        {
            return i;
        }
    }
    return -1;
}

//////////////////////////////////////////////////////////////////////
