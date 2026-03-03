#include "../../include/common.h"
#include <shobjidl.h>

class SimpleSite : public IServiceProvider, public IOleWindow {
    LONG m_ref = 1;
    HWND m_hwnd;
public:
    SimpleSite(HWND hwnd) : m_hwnd(hwnd) {}
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppv) override {
        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IServiceProvider)) {
            *ppv = static_cast<IServiceProvider*>(this); AddRef(); return S_OK;
        }
        if (IsEqualIID(riid, IID_IOleWindow)) {
            *ppv = static_cast<IOleWindow*>(this); AddRef(); return S_OK;
        }
        *ppv = NULL; return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return InterlockedIncrement(&m_ref); }
    ULONG STDMETHODCALLTYPE Release() override {
        LONG r = InterlockedDecrement(&m_ref);
        if (r == 0) delete this; return r;
    }
    HRESULT STDMETHODCALLTYPE QueryService(REFGUID, REFIID, void** ppv) override {
        *ppv = NULL; return E_NOINTERFACE;
    }
    HRESULT STDMETHODCALLTYPE GetWindow(HWND* phwnd) override {
        *phwnd = m_hwnd; return S_OK;
    }
    HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(BOOL) override { return E_NOTIMPL; }
};

void QueryComExtensionInternal(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint);

void QueryComExtension(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint) {
    __try {
        QueryComExtensionInternal(clsid, filePath, response, maxLen, dllHint);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        LogToFile(L"    [CRITICAL] Crash in QueryComExtension\n");
        snprintf(response, maxLen, "{\"success\":false,\"error\":\"Crash in COM Extension\"}");
    }
}

void QueryComExtensionInternal(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint) {
    wchar_t fName[128];
    GetFriendlyName(clsid, fName, 128);
    for (int k = 0; fName[k]; k++) {
        if (wcschr(L"\\/:*?\"<>|", fName[k])) fName[k] = L'_';
    }
    LogToFile(L"[COM] Querying [%ls] for: %ls (Hint: %ls)\n", fName, filePath, dllHint ? dllHint : L"NONE");

    LARGE_INTEGER tStart, tEnd, freq;
    QueryPerformanceFrequency(&freq);

    IUnknown* pUnk = NULL;
    QueryPerformanceCounter(&tStart);
    HRESULT hr = fpCoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, IID_IUnknown, (void**)&pUnk);
    
    // --- 智取：手动加载兜底 (针对 Packaged COM) ---
    if (FAILED(hr) && dllHint && PathFileExistsW(dllHint)) {
        LogToFile(L"    CoCreateInstance Failed (0x%08X), attempting manual load from %ls\n", hr, dllHint);
        HMODULE hMod = LoadLibraryExW(dllHint, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        if (hMod) {
            typedef HRESULT(WINAPI* DllGetClassObject_t)(REFCLSID, REFIID, LPVOID*);
            DllGetClassObject_t pGetClass = (DllGetClassObject_t)GetProcAddress(hMod, "DllGetClassObject");
            if (pGetClass) {
                IClassFactory* pFactory = NULL;
                if (SUCCEEDED(pGetClass(clsid, IID_IClassFactory, (void**)&pFactory))) {
                    hr = pFactory->CreateInstance(NULL, IID_IUnknown, (void**)&pUnk);
                    pFactory->Release();
                }
            }
            // Note: We don't FreeLibrary(hMod) here because the object pUnk might need it.
            // In a real shell, these DLLs stay loaded in Explorer.
        }
    }

    QueryPerformanceCounter(&tEnd);
    double msCreate = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;

    if (FAILED(hr) || !pUnk) {
        snprintf(response, maxLen, "{\"success\":false,\"error\":\"0x%08X\"}", hr);
        return;
    }

    IObjectWithSite* pOws = NULL;
    HWND hwnd = GetShellWindow();
    if (!hwnd) hwnd = GetDesktopWindow();
    if (SUCCEEDED(pUnk->QueryInterface(IID_IObjectWithSite, (void**)&pOws))) {
        SimpleSite* site = new SimpleSite(hwnd);
        pOws->SetSite(static_cast<IServiceProvider*>(site));
        site->Release();
        pOws->Release();
    }

    // 3. 构造上下文
    QueryPerformanceCounter(&tStart);
    IDataObject*     pDataObj   = CreateDataObjectForFile(filePath);
    PIDLIST_ABSOLUTE pidlFolder = GetFolderPidl(filePath);
    HKEY             hkeyProg   = GetProgIDKeyForFile(filePath);
    QueryPerformanceCounter(&tEnd);
    double msContext = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;

    LogToFile(L"    Context: DataObj=%p, pidlFolder=%p, hkeyProg=%p (%.3f ms)\n",
              pDataObj, pidlFolder, hkeyProg, msContext);

    // 4. Initialize
    IShellExtInit* pInit = NULL;
    double msInit = 0;
    if (SUCCEEDED(pUnk->QueryInterface(IID_IShellExtInit_, (void**)&pInit))) {
        QueryPerformanceCounter(&tStart);
        hr = pInit->Initialize(pidlFolder, pDataObj, hkeyProg);
        QueryPerformanceCounter(&tEnd);
        msInit = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;
        LogToFile(L"    Initialize: %.3f ms (0x%08X)\n", msInit, hr);
        pInit->Release();
    }
    msInit += msContext; // Include context creation time in Init phase

    // 5. QueryContextMenu (获取标题是必须的)
    std::wstring names, icons;
    double msQuery = 0;
    IContextMenu* pMenu = NULL;
    
    // --- 智取图标：先看注册表有没有正宅 ---
    std::wstring regIcon = GetIconFromRegistry(clsid);
    bool needCapture = regIcon.empty();

    if (SUCCEEDED(pUnk->QueryInterface(IID_IContextMenu_, (void**)&pMenu))) {
        HMENU hMenu = CreatePopupMenu();
        QueryPerformanceCounter(&tStart);
        hr = pMenu->QueryContextMenu(hMenu, 0, 1, 0x7FFF, CMF_NORMAL | CMF_EXPLORE);
        QueryPerformanceCounter(&tEnd);
        msQuery = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;

        if (SUCCEEDED(hr)) {
            IContextMenu2* pMenu2 = NULL;
            pMenu->QueryInterface(IID_IContextMenu2_, (void**)&pMenu2);

            int count = GetMenuItemCount(hMenu);
            for (int i = 0; i < count; i++) {
                wchar_t buf[256] = {};
                MENUITEMINFOW mii = { sizeof(mii) };
                mii.fMask = MIIM_STRING | MIIM_FTYPE | MIIM_BITMAP | MIIM_ID;
                mii.dwTypeData = buf;
                mii.cch = 255;

                if (GetMenuItemInfoW(hMenu, i, TRUE, &mii) && !(mii.fType & MFT_SEPARATOR) && wcslen(buf) > 0) {
                    if (!names.empty()) { names += L"|"; icons += L"|"; }
                    names += buf;

                    // 只有在没找到注册表图标时，才尝试昂贵的 Hook 捕获
                    bool saved = false;
                    if (needCapture) {
                        wchar_t iconPath[MAX_PATH];
                        swprintf_s(iconPath, L"%ls\\icons\\%ls_%d.png", GetModuleDirectory().c_str(), fName, i);
                        
                        if (mii.hbmpItem && mii.hbmpItem != HBMMENU_CALLBACK && (UINT_PTR)mii.hbmpItem > 12) {
                            saved = SaveBitmapAsPng(mii.hbmpItem, iconPath);
                        } else if (pMenu2) {
                            HBITMAP hBmp = CaptureOwnerDrawIcon(pMenu2, hMenu, i, 20);
                            if (hBmp) {
                                saved = SaveBitmapAsPng(hBmp, iconPath);
                                DeleteObject(hBmp);
                            }
                        }
                        if (saved) icons += iconPath;
                        else icons += L"NONE";
                    } else {
                        // 已经有注册表图标了，这里直接标记
                        icons += L"USE_REGISTRY";
                    }
                }
            }
            if (pMenu2) pMenu2->Release();
        }
        DestroyMenu(hMenu);
        pMenu->Release();
    }

    if (hkeyProg) RegCloseKey(hkeyProg);
    if (pidlFolder) CoTaskMemFree(pidlFolder);
    if (pDataObj) pDataObj->Release();
    pUnk->Release();

    std::string utf8Names = WideToUtf8(names);
    std::string utf8Icons = WideToUtf8(icons);
    std::string utf8RegIcon = WideToUtf8(regIcon);

    snprintf(response, maxLen, 
             "{\"success\":true,\"interface\":\"IContextMenu\",\"names\":\"%s\",\"icons\":\"%s\",\"reg_icon\":\"%s\",\"create_ms\":%.3f,\"init_ms\":%.3f,\"query_ms\":%.3f}",
             EscapeJson(utf8Names).c_str(), EscapeJson(utf8Icons).c_str(), EscapeJson(utf8RegIcon).c_str(),
             msCreate, msInit, msQuery);
}
