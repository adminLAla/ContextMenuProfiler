#pragma once
#include <windows.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <string>
#include <vector>
#include <gdiplus.h>

// Forward declarations
typedef HRESULT(WINAPI* CoCreateInstance_t)(REFCLSID, LPUNKNOWN, DWORD, REFIID, LPVOID*);
extern CoCreateInstance_t fpCoCreateInstance;
extern HMODULE g_hModule;
extern CRITICAL_SECTION g_Lock;
extern ULONG_PTR g_GdiplusToken;
extern bool g_ShouldExit;
extern HANDLE g_PipeThread;

// IIDs
extern const IID IID_IContextMenu_;
extern const IID IID_IContextMenu2_;
extern const IID IID_IShellExtInit_;

// Common functions
void LogToFile(const wchar_t* fmt, ...);
std::string WideToUtf8(const std::wstring& wstr);
std::wstring GetModuleDirectory();
void GetFriendlyName(const GUID& guid, wchar_t* outName, int maxLen);
std::string EscapeJson(const std::string& s);

// Icon extraction
bool SaveBitmapAsPng(HBITMAP hBitmap, const wchar_t* outputPath);
HBITMAP CaptureOwnerDrawIcon(IContextMenu2* pMenu2, HMENU hMenu, int menuIndex, int iconSize = 16);
std::wstring GetIconFromRegistry(const CLSID& clsid);

// Shell helpers
IDataObject* CreateDataObjectForFile(const wchar_t* filePath);
PIDLIST_ABSOLUTE GetFolderPidl(const wchar_t* filePath);
HKEY GetProgIDKeyForFile(const wchar_t* filePath);

// Handler functions
void QueryComExtension(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint = NULL);
void QueryExplorerCommand(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint = NULL);
