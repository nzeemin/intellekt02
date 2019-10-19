
// Dialogs.cpp

#include "stdafx.h"
#include <commdlg.h>
#include <commctrl.h>
#include "Dialogs.h"
#include "Emulator.h"
#include "Main.h"

//////////////////////////////////////////////////////////////////////


INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


//////////////////////////////////////////////////////////////////////
// About Box

void ShowAboutBox()
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), g_hwnd, AboutBoxProc);
}

INT_PTR CALLBACK AboutBoxProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
        {
            TCHAR buf[64];
            wsprintf(buf, _T("%S %S"), __DATE__, __TIME__);
            ::SetWindowText(::GetDlgItem(hDlg, IDC_BUILDDATE), buf);
            return (INT_PTR)TRUE;
        }
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
        break;
    }
    return (INT_PTR)FALSE;
}


//////////////////////////////////////////////////////////////////////


BOOL ShowSaveDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, LPCTSTR strDefExt, TCHAR* bufFileName)
{
    *bufFileName = 0;
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrTitle = strTitle;
    ofn.lpstrFilter = strFilter;
    ofn.lpstrDefExt = strDefExt;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    ofn.lpstrFile = bufFileName;
    ofn.nMaxFile = MAX_PATH;

    BOOL okResult = GetSaveFileName(&ofn);
    return okResult;
}

BOOL ShowOpenDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, TCHAR* bufFileName)
{
    *bufFileName = 0;
    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwndOwner;
    ofn.hInstance = g_hInst;
    ofn.lpstrTitle = strTitle;
    ofn.lpstrFilter = strFilter;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrFile = bufFileName;
    ofn.nMaxFile = MAX_PATH;

    BOOL okResult = GetOpenFileName(&ofn);
    return okResult;
}


//////////////////////////////////////////////////////////////////////
// Settings Dialog

void ShowSettingsDialog()
{
    DialogBox(g_hInst, MAKEINTRESOURCE(IDD_SETTINGS), g_hwnd, SettingsProc);
}

int CALLBACK SettingsDialog_EnumFontProc(const LOGFONT* lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)
{
    if ((lpelfe->lfPitchAndFamily & FIXED_PITCH) == 0)
        return TRUE;
    if (lpelfe->lfFaceName[0] == _T('@'))  // Skip vertical fonts
        return TRUE;

    HWND hCombo = (HWND)lParam;

    int item = ::SendMessage(hCombo, CB_FINDSTRING, 0, (LPARAM)lpelfe->lfFaceName);
    if (item < 0)
        ::SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)lpelfe->lfFaceName);

    return TRUE;
}

void FillDebugFontCombo(HWND hCombo)
{
    LOGFONT logfont;  ZeroMemory(&logfont, sizeof logfont);
    logfont.lfCharSet = DEFAULT_CHARSET;
    logfont.lfWeight = FW_NORMAL;
    logfont.lfPitchAndFamily = FIXED_PITCH | FF_DONTCARE;

    HDC hdc = GetDC(NULL);
    EnumFontFamiliesEx(hdc, &logfont, (FONTENUMPROC)SettingsDialog_EnumFontProc, (LPARAM)hCombo, 0);
    ReleaseDC(NULL, hdc);

    Settings_GetDebugFontName(logfont.lfFaceName);
    ::SendMessage(hCombo, CB_SELECTSTRING, 0, (LPARAM)logfont.lfFaceName);
}

INT_PTR CALLBACK SettingsProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM /*lParam*/)
{
    switch (message)
    {
    case WM_INITDIALOG:
        {
            HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
            SendMessage(hVolume, TBM_SETRANGEMIN, 0, (LPARAM)0);
            SendMessage(hVolume, TBM_SETRANGEMAX, 0, (LPARAM)0xffff);
            SendMessage(hVolume, TBM_SETTICFREQ, 0x1000, 0);
            SendMessage(hVolume, TBM_SETPOS, TRUE, (LPARAM)Settings_GetSoundVolume());

            HWND hDebugFont = GetDlgItem(hDlg, IDC_DEBUGFONT);
            FillDebugFontCombo(hDebugFont);
            return (INT_PTR)FALSE;
        }
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK:
            {
                HWND hVolume = GetDlgItem(hDlg, IDC_VOLUME);
                DWORD volume = SendMessage(hVolume, TBM_GETPOS, 0, 0);
                Settings_SetSoundVolume((WORD)volume);

                TCHAR buffer[32];

                GetDlgItemText(hDlg, IDC_DEBUGFONT, buffer, 32);
                Settings_SetDebugFontName(buffer);
            }

            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        case IDCANCEL:
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        default:
            return (INT_PTR)FALSE;
        }
        break;
    }
    return (INT_PTR) FALSE;
}


//////////////////////////////////////////////////////////////////////
