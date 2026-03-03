#include "../include/common.h"
#include "../include/MinHook.h"

// Extern from other files
DWORD WINAPI PipeThread(LPVOID);

HRESULT WINAPI DetourCoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnk, DWORD ctx, REFIID riid, LPVOID* ppv) {
    return fpCoCreateInstance(rclsid, pUnk, ctx, riid, ppv);
}

extern "C" BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_hModule = hMod;
        InitializeCriticalSection(&g_Lock);
        MH_Initialize();
        MH_CreateHookApi(L"ole32", "CoCreateInstance", &DetourCoCreateInstance, (LPVOID*)&fpCoCreateInstance);
        MH_EnableHook(MH_ALL_HOOKS);
        g_PipeThread = CreateThread(NULL, 0, PipeThread, NULL, 0, NULL);
        LogToFile(L"--- Hook DLL v12.3 Ready ---\n");
    } else if (reason == DLL_PROCESS_DETACH) {
        g_ShouldExit = true;
        DeleteCriticalSection(&g_Lock);
        LogToFile(L"--- Hook DLL Detached ---\n");
    }
    return TRUE;
}
