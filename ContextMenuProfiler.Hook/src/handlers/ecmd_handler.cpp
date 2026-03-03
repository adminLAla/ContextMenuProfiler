#include "../../include/common.h"
#include <shobjidl.h>

void QueryExplorerCommandInternal(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint);

void QueryExplorerCommand(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint) {
    __try {
        QueryExplorerCommandInternal(clsid, filePath, response, maxLen, dllHint);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        LogToFile(L"    [CRITICAL] Crash in QueryExplorerCommand\n");
        snprintf(response, maxLen, "{\"success\":false,\"error\":\"Crash in Explorer Command\"}");
    }
}

void QueryExplorerCommandInternal(const CLSID& clsid, const wchar_t* filePath, char* response, int maxLen, const wchar_t* dllHint) {
    wchar_t fName[128];
    GetFriendlyName(clsid, fName, 128);
    LogToFile(L"[ECMD] Querying [%ls] for: %ls (Hint: %ls)\n", fName, filePath, dllHint ? dllHint : L"NONE");

    LARGE_INTEGER tStart, tEnd, freq;
    QueryPerformanceFrequency(&freq);

    QueryPerformanceCounter(&tStart);
    IExplorerCommand* pCmd = NULL;
    HRESULT hr = fpCoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER | CLSCTX_LOCAL_SERVER, __uuidof(IExplorerCommand), (void**)&pCmd);
    
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
                    hr = pFactory->CreateInstance(NULL, __uuidof(IExplorerCommand), (void**)&pCmd);
                    pFactory->Release();
                }
            }
        }
    }

    QueryPerformanceCounter(&tEnd);
    double msCreate = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;

    if (FAILED(hr) || !pCmd) {
        snprintf(response, maxLen, "{\"success\":false,\"error\":\"0x%08X\",\"create_ms\":%.3f}", hr, msCreate);
        return;
    }

    // 构造 ShellItemArray
    QueryPerformanceCounter(&tStart);
    IShellItem* pItem = NULL;
    IShellItemArray* pArray = NULL;
    SHCreateItemFromParsingName(filePath, NULL, IID_IShellItem, (void**)&pItem);
    if (pItem) SHCreateShellItemArrayFromShellItem(pItem, IID_IShellItemArray, (void**)&pArray);
    QueryPerformanceCounter(&tEnd);
    double msInit = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;    // --- 智取图标：先看注册表 ---
    std::wstring regIcon = GetIconFromRegistry(clsid);
    bool needCapture = regIcon.empty();

    std::wstring names, icons;
    auto GetCmdIcon = [&](IExplorerCommand* cmd, IShellItemArray* items) {
        if (!needCapture) return std::wstring(L"USE_REGISTRY");
        LPWSTR iconRef = NULL;
        if (SUCCEEDED(cmd->GetIcon(items, &iconRef)) && iconRef) {
            std::wstring res = iconRef;
            CoTaskMemFree(iconRef);
            return res;
        }
        return std::wstring(L"NONE");
    };

    // GetTitle & Icons (Query phase)
    QueryPerformanceCounter(&tStart);
    LPWSTR title = NULL;
    hr = pCmd->GetTitle(pArray, &title);
    if (SUCCEEDED(hr) && title && wcslen(title) > 0) {
        names = title;
        icons = GetCmdIcon(pCmd, pArray);
        CoTaskMemFree(title);
        title = NULL; // Prevent double free
    }

    // SubCommands
    IEnumExplorerCommand* pEnum = NULL;
    if (SUCCEEDED(pCmd->EnumSubCommands(&pEnum)) && pEnum) {
        IExplorerCommand* pSub = NULL;
        ULONG fetched = 0;
        while (pEnum->Next(1, &pSub, &fetched) == S_OK && fetched > 0) {
            LPWSTR subTitle = NULL;
            if (SUCCEEDED(pSub->GetTitle(pArray, &subTitle)) && subTitle) {
                if (!names.empty()) { names += L"|"; icons += L"|"; }
                names += subTitle;
                icons += GetCmdIcon(pSub, pArray);
                CoTaskMemFree(subTitle);
            }
            pSub->Release();
        }
        pEnum->Release();
    }
    QueryPerformanceCounter(&tEnd);
    double msQuery = (double)(tEnd.QuadPart - tStart.QuadPart) / freq.QuadPart * 1000.0;

    EXPCMDSTATE state = ECS_ENABLED;
    pCmd->GetState(pArray, FALSE, &state);

    if (pArray) pArray->Release();
    if (pItem) pItem->Release();
    pCmd->Release();

    std::string utf8Names = WideToUtf8(names);
    std::string utf8Icons = WideToUtf8(icons);
    std::string utf8RegIcon = WideToUtf8(regIcon);

    snprintf(response, maxLen, 
             "{\"success\":true,\"interface\":\"IExplorerCommand\",\"names\":\"%s\",\"icons\":\"%s\",\"reg_icon\":\"%s\",\"create_ms\":%.3f,\"init_ms\":%.3f,\"query_ms\":%.3f,\"state\":%d}",
             EscapeJson(utf8Names).c_str(), EscapeJson(utf8Icons).c_str(), EscapeJson(utf8RegIcon).c_str(),
             msCreate, msInit, msQuery, (int)state);
}
