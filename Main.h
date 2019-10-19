#pragma once

#include "res\\resource.h"

//////////////////////////////////////////////////////////////////////


#define MAX_LOADSTRING 100

extern TCHAR g_szTitle[MAX_LOADSTRING];            // The title bar text
extern TCHAR g_szWindowClass[MAX_LOADSTRING];      // Main window class name


extern HINSTANCE g_hInst; // current instance


//////////////////////////////////////////////////////////////////////
// Main Window

extern HWND g_hwnd;  // Main window handle

void MainWindow_RegisterClass();
BOOL CreateMainWindow();
void MainWindow_RestoreSettings();
void MainWindow_UpdateMenu();
void MainWindow_UpdateAllViews();
void MainWindow_ShowHideDebug();
void MainWindow_ShowHideToolbar();
void MainWindow_ShowHideKeyboard();
void MainWindow_AdjustWindowSize();

void MainWindow_SetToolbarImage(int commandId, int imageIndex);
void MainWindow_SetStatusbarText(int part, LPCTSTR message);


//////////////////////////////////////////////////////////////////////
// Assertions checking - MFC-like ASSERT macro

#ifdef _DEBUG

BOOL AssertFailedLine(LPCSTR lpszFileName, int nLine);
#define ASSERT(f)          (void) ((f) || !AssertFailedLine(__FILE__, __LINE__) || (DebugBreak(), 0))
#define VERIFY(f)          ASSERT(f)

#else   // _DEBUG

#define ASSERT(f)          ((void)0)
#define VERIFY(f)          ((void)f)

#endif // !_DEBUG


//////////////////////////////////////////////////////////////////////
// Alerts

void AlertWarning(LPCTSTR sMessage);
void AlertWarningFormat(LPCTSTR sFormat, ...);
BOOL AlertOkCancel(LPCTSTR sMessage);


//////////////////////////////////////////////////////////////////////
// DebugPrint

#if !defined(PRODUCT)

void DebugPrint(LPCTSTR message);
void DebugPrintFormat(LPCTSTR pszFormat, ...);
void DebugLogClear();
void DebugLogCloseFile();
void DebugLog(LPCTSTR message);
void DebugLogFormat(LPCTSTR pszFormat, ...);

#endif // !defined(PRODUCT)


//////////////////////////////////////////////////////////////////////
// Common

HFONT CreateMonospacedFont();
HFONT CreateDialogFont();
void GetFontWidthAndHeight(HDC hdc, int* pWidth, int* pHeight);
void PrintHexByteValue(TCHAR* buffer, BYTE value);
void PrintHexValue(TCHAR* buffer, WORD value);
void PrintBinaryValue(TCHAR* buffer, WORD value);
void DrawHexValue(HDC hdc, int x, int y, WORD value);
void DrawBinaryValue(HDC hdc, int x, int y, WORD value);
bool ParseHexValue(LPCTSTR text, WORD* pValue);


//////////////////////////////////////////////////////////////////////
// Settings

void Settings_Init();
void Settings_Done();
BOOL Settings_GetWindowRect(RECT * pRect);
void Settings_SetWindowRect(const RECT * pRect);
void Settings_SetWindowMaximized(BOOL flag);
BOOL Settings_GetWindowMaximized();
void Settings_SetConfiguration(int configuration);
int  Settings_GetConfiguration();
void Settings_SetDebug(BOOL flag);
BOOL Settings_GetDebug();
void Settings_GetDebugFontName(LPTSTR buffer);
void Settings_SetDebugFontName(LPCTSTR sFontName);
void Settings_SetSound(BOOL flag);
BOOL Settings_GetSound();
void Settings_SetSoundVolume(WORD value);
WORD Settings_GetSoundVolume();
void Settings_SetToolbar(BOOL flag);
BOOL Settings_GetToolbar();
void Settings_SetKeyboard(BOOL flag);
BOOL Settings_GetKeyboard();


//////////////////////////////////////////////////////////////////////
