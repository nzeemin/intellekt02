
// Dialogs.h

//////////////////////////////////////////////////////////////////////


void ShowAboutBox();

// Input hex value
//   strTitle - dialog caption
//   strPrompt - label text
BOOL InputBoxHex(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strPrompt, WORD* pValue);

BOOL ShowSaveDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, LPCTSTR strDefExt, TCHAR* bufFileName);
BOOL ShowOpenDialog(HWND hwndOwner, LPCTSTR strTitle, LPCTSTR strFilter, TCHAR* bufFileName);

void ShowSettingsDialog();


//////////////////////////////////////////////////////////////////////
