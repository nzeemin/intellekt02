
// ToolWindow.h

#pragma once

//////////////////////////////////////////////////////////////////////


const LPCTSTR CLASSNAME_TOOLWINDOW = _T("BKBTLTOOLWINDOW");
const LPCTSTR CLASSNAME_OVERLAPPEDWINDOW = _T("BKBTLOVERLAPPEDWINDOW");
const LPCTSTR CLASSNAME_SPLITTERWINDOW = _T("BKBTLSPLITTERWINDOW");

void ToolWindow_RegisterClass();
LRESULT CALLBACK ToolWindow_WndProc(HWND, UINT, WPARAM, LPARAM);

void OverlappedWindow_RegisterClass();

void SplitterWindow_RegisterClass();
LRESULT CALLBACK SplitterWindow_WndProc(HWND, UINT, WPARAM, LPARAM);
HWND SplitterWindow_Create(HWND hwndParent, HWND hwndTop, HWND hwndBottom);


//////////////////////////////////////////////////////////////////////

