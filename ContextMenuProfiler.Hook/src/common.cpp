#include "../include/common.h"
#include <stdio.h>
#include <shobjidl.h>

CoCreateInstance_t fpCoCreateInstance = NULL;
HMODULE g_hModule = NULL;
CRITICAL_SECTION g_Lock;
ULONG_PTR g_GdiplusToken;
bool g_ShouldExit = false;
HANDLE g_PipeThread = NULL;

const IID IID_IContextMenu_  = {0x000214E4,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const IID IID_IContextMenu2_ = {0x000214F4,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};
const IID IID_IShellExtInit_ = {0x000214E8,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

void LogToFile(const wchar_t* fmt, ...) {
    wchar_t logPath[MAX_PATH];
    swprintf_s(logPath, L"%ls\\hook_internal.log", GetModuleDirectory().c_str());
    va_list args; va_start(args, fmt);
    wchar_t buf[4096]; vswprintf_s(buf, fmt, args); va_end(args);
    FILE* f = NULL;
    if (_wfopen_s(&f, logPath, L"a, ccs=UTF-8") == 0) { fputws(buf, f); fclose(f); }
    OutputDebugStringW(buf);
}

std::string WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, NULL, 0, NULL, NULL);
    std::string res(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &res[0], size, NULL, NULL);
    while (!res.empty() && res.back() == '\0') res.pop_back();
    return res;
}

std::wstring GetModuleDirectory() {
    wchar_t path[MAX_PATH];
    GetModuleFileNameW(g_hModule, path, MAX_PATH);
    PathRemoveFileSpecW(path);
    return path;
}

void GetFriendlyName(const GUID& guid, wchar_t* outName, int maxLen) {
    wchar_t keyPath[128], guidStr[64];
    StringFromGUID2(guid, guidStr, 64);
    swprintf_s(keyPath, L"CLSID\\%s", guidStr);
    HKEY hKey; outName[0] = 0;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD size = maxLen * sizeof(wchar_t);
        if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)outName, &size) != ERROR_SUCCESS) {
            size = maxLen * sizeof(wchar_t);
            RegQueryValueExW(hKey, L"FriendlyName", NULL, NULL, (LPBYTE)outName, &size);
        }
        RegCloseKey(hKey);
    }
    if (wcslen(outName) == 0) wcscpy_s(outName, maxLen, guidStr);
}

std::string EscapeJson(const std::string& s) {
    std::string res;
    for (char c : s) {
        if (c == '\"') res += "\\\"";
        else if (c == '\\') res += "\\\\";
        else res += c;
    }
    return res;
}

bool SaveBitmapAsPng(HBITMAP hBitmap, const wchar_t* outputPath) {
    if (!hBitmap) return false;

    BITMAP bm;
    if (GetObject(hBitmap, sizeof(bm), &bm) == 0) return false;

    wchar_t dir[MAX_PATH];
    wcscpy_s(dir, outputPath);
    PathRemoveFileSpecW(dir);
    SHCreateDirectoryExW(NULL, dir, NULL);

    CLSID pngClsid;
    CLSIDFromString(L"{557CF406-1A04-11D3-9A73-0000F81EF32E}", &pngClsid);

    if (bm.bmBitsPixel == 32) {
        BITMAPINFO bmi = { 0 };
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = bm.bmWidth;
        bmi.bmiHeader.biHeight = -bm.bmHeight;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        int stride = bm.bmWidth * 4;
        BYTE* pixels = (BYTE*)malloc(stride * bm.bmHeight);
        if (pixels) {
            HDC hdc = GetDC(NULL);
            GetDIBits(hdc, hBitmap, 0, bm.bmHeight, pixels, &bmi, DIB_RGB_COLORS);
            ReleaseDC(NULL, hdc);

            // 检查 Alpha 通道
            bool hasAlpha = false;
            for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
                if (pixels[i * 4 + 3] != 0) { hasAlpha = true; break; }
            }

            if (!hasAlpha) {
                for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) pixels[i * 4 + 3] = 255;
            } else {
                // 反预乘
                for (int i = 0; i < bm.bmWidth * bm.bmHeight; i++) {
                    BYTE a = pixels[i * 4 + 3];
                    if (a > 0 && a < 255) {
                        pixels[i * 4 + 0] = (BYTE)min(255, pixels[i * 4 + 0] * 255 / a);
                        pixels[i * 4 + 1] = (BYTE)min(255, pixels[i * 4 + 1] * 255 / a);
                        pixels[i * 4 + 2] = (BYTE)min(255, pixels[i * 4 + 2] * 255 / a);
                    }
                }
            }

            // 重要：必须在 pixels 释放前完成 Save
            Gdiplus::Bitmap gdiBmp(bm.bmWidth, bm.bmHeight, stride, PixelFormat32bppARGB, pixels);
            Gdiplus::Status status = gdiBmp.Save(outputPath, &pngClsid, NULL);
            free(pixels);
            return (status == Gdiplus::Ok);
        }
    }

    // 兜底方案
    Gdiplus::Bitmap* pFallback = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
    if (pFallback) {
        Gdiplus::Status status = pFallback->Save(outputPath, &pngClsid, NULL);
        delete pFallback;
        return (status == Gdiplus::Ok);
    }

    return false;
}

HBITMAP CaptureOwnerDrawIcon(IContextMenu2* pMenu2, HMENU hMenu, int menuIndex, int iconSize) {
    MENUITEMINFOW mii = { sizeof(mii) };
    mii.fMask = MIIM_DATA | MIIM_ID | MIIM_FTYPE;
    if (!GetMenuItemInfoW(hMenu, menuIndex, TRUE, &mii)) return NULL;
    if (mii.fType & MFT_SEPARATOR) return NULL;

    HDC hdcScreen = GetDC(NULL);
    if (!hdcScreen) return NULL;

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = iconSize;
    bmi.bmiHeader.biHeight = -iconSize;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = NULL;
    HBITMAP hBmp = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    if (!hBmp) { ReleaseDC(NULL, hdcScreen); return NULL; }

    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hOld = (HBITMAP)SelectObject(hdcMem, hBmp);
    memset(pBits, 0, iconSize * iconSize * 4);

    // 智取：创建一个真实的隐藏窗口作为绘制容器
    HWND hwndDummy = CreateWindowExW(0, L"STATIC", L"ContextMenuProfilerCapture", 0, 0, 0, 0, 0, NULL, NULL, NULL, NULL);

    __try {
        MEASUREITEMSTRUCT mis = { 0 };
        mis.CtlType = ODT_MENU;
        mis.itemID = mii.wID;
        mis.itemData = mii.dwItemData;
        pMenu2->HandleMenuMsg(WM_MEASUREITEM, 0, (LPARAM)&mis);

        DRAWITEMSTRUCT dis = { 0 };
        dis.CtlType = ODT_MENU;
        dis.itemID = mii.wID;
        dis.itemData = mii.dwItemData;
        dis.itemAction = ODA_DRAWENTIRE;
        dis.itemState = ODS_DEFAULT; // 使用标准状态
        dis.hDC = hdcMem;
        dis.rcItem = { 0, 0, iconSize, iconSize };
        dis.hwndItem = hwndDummy; // 投其所好，给它真窗口
        pMenu2->HandleMenuMsg(WM_DRAWITEM, 0, (LPARAM)&dis);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        LogToFile(L"    [WARN] OwnerDraw Capture Failed\n");
    }

    if (hwndDummy) DestroyWindow(hwndDummy);
    SelectObject(hdcMem, hOld);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    // 像素内容校验
    BYTE* px = (BYTE*)pBits;
    for (int i = 0; i < iconSize * iconSize * 4; i++) {
        if (px[i] != 0) return hBmp; 
    }

    DeleteObject(hBmp);
    return NULL;
}

std::wstring GetIconFromRegistry(const CLSID& clsid) {
    wchar_t guidStr[64], keyPath[256];
    StringFromGUID2(clsid, guidStr, 64);
    HKEY hKey;
    wchar_t iconVal[512] = {};
    DWORD size;

    // 1. 尝试 CLSID 下的标准定义
    const wchar_t* subKeys[] = { L"DefaultIcon", L"Icon" };
    for (const auto& subKey : subKeys) {
        swprintf_s(keyPath, L"CLSID\\%s\\%s", guidStr, subKey);
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            size = sizeof(iconVal);
            if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)iconVal, &size) == ERROR_SUCCESS && wcslen(iconVal) > 0) {
                RegCloseKey(hKey);
                return iconVal;
            }
            RegCloseKey(hKey);
        }
    }

    // 2. 智取：通过 ProgID 关联搜索 (这是 Bandizip 这种插件最常用的方式)
    wchar_t progId[256] = {};
    swprintf_s(keyPath, L"CLSID\\%s\\ProgID", guidStr);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(progId);
        RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)progId, &size);
        RegCloseKey(hKey);
    }

    if (wcslen(progId) > 0) {
        swprintf_s(keyPath, L"%s\\DefaultIcon", progId);
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            size = sizeof(iconVal);
            if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)iconVal, &size) == ERROR_SUCCESS && wcslen(iconVal) > 0) {
                RegCloseKey(hKey);
                return iconVal;
            }
            RegCloseKey(hKey);
        }
    }

    // 3. 终极搜索：从 InProcServer32 路径推断 EXE 图标
    swprintf_s(keyPath, L"CLSID\\%s\\InProcServer32", guidStr);
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, keyPath, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        size = sizeof(iconVal);
        if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)iconVal, &size) == ERROR_SUCCESS && wcslen(iconVal) > 0) {
            RegCloseKey(hKey);
            
            // 如果是 DLL，尝试找同目录下的同名 EXE 或 Bandizip.exe (智取)
            wchar_t fullPath[MAX_PATH];
            wcscpy_s(fullPath, iconVal);
            PathRemoveExtensionW(fullPath);
            
            // 尝试 1: 同名 EXE (e.g. bdzshl.exe)
            wchar_t tryExe[MAX_PATH];
            swprintf_s(tryExe, L"%s.exe", fullPath);
            if (PathFileExistsW(tryExe)) return std::wstring(tryExe) + L",0";

            // 尝试 2: 目录下的主程序 (Bandizip.exe)
            PathRemoveFileSpecW(fullPath);
            swprintf_s(tryExe, L"%s\\Bandizip.exe", fullPath);
            if (PathFileExistsW(tryExe)) return std::wstring(tryExe) + L",0";

            // 尝试 3: DLL 自身资源，补上索引
            std::wstring res = iconVal;
            if (res.find(L",") == std::wstring::npos) res += L",0";
            return res;
        }
        RegCloseKey(hKey);
    }

    return L"";
}

IDataObject* CreateDataObjectForFile(const wchar_t* filePath) {
    PIDLIST_ABSOLUTE pidlFull = NULL;
    if (FAILED(SHParseDisplayName(filePath, NULL, &pidlFull, 0, NULL))) return NULL;
    IShellFolder* pParent = NULL;
    PCUITEMID_CHILD pidlChild = NULL;
    IDataObject* pDataObj = NULL;
    if (SUCCEEDED(SHBindToParent(pidlFull, IID_IShellFolder, (void**)&pParent, &pidlChild))) {
        pParent->GetUIObjectOf(NULL, 1, &pidlChild, IID_IDataObject, NULL, (void**)&pDataObj);
        pParent->Release();
    }
    CoTaskMemFree(pidlFull);
    return pDataObj;
}

PIDLIST_ABSOLUTE GetFolderPidl(const wchar_t* filePath) {
    wchar_t folder[MAX_PATH];
    wcscpy_s(folder, filePath);
    PathRemoveFileSpecW(folder);
    PIDLIST_ABSOLUTE pidl = NULL;
    SHParseDisplayName(folder, NULL, &pidl, 0, NULL);
    return pidl;
}

HKEY GetProgIDKeyForFile(const wchar_t* filePath) {
    const wchar_t* ext = PathFindExtensionW(filePath);
    if (!ext || !*ext) return NULL;

    HKEY hKeyExt = NULL;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, ext, 0, KEY_READ, &hKeyExt) != ERROR_SUCCESS)
        return NULL;

    wchar_t progId[256] = {};
    DWORD size = sizeof(progId);
    RegQueryValueExW(hKeyExt, NULL, NULL, NULL, (LPBYTE)progId, &size);
    RegCloseKey(hKeyExt);

    HKEY hKeyProgID = NULL;
    if (wcslen(progId) > 0)
        RegOpenKeyExW(HKEY_CLASSES_ROOT, progId, 0, KEY_READ, &hKeyProgID);
    if (!hKeyProgID)
        RegOpenKeyExW(HKEY_CLASSES_ROOT, ext, 0, KEY_READ, &hKeyProgID);
    return hKeyProgID;
}
