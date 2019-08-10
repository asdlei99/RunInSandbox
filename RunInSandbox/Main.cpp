#include <conio.h>
#include <iostream>
#include <Shlobj.h>
#include <atlbase.h>
#include <atlcom.h>
#include "ComCreate.hpp"
#include "ProcCreate.hpp"
#include "../TestControl/TestControl_h.h"


int wmain (int argc, wchar_t *argv[]) {
    if (!IsUserAnAdmin())
        std::wcout << L"WARNING: Admin priveledges not detected. Some operations might fail.\n";

    if (argc < 2) {
        std::wcerr << L"Too few arguments\n.";
        std::wcerr << L"Usage: RunInSandbox.exe [ac|li] ProgID [username] [password]\n";
        std::wcerr << L"Usage: RunInSandbox.exe [ac] ExePath [username] [password]\n";
        return -1;
    }

    int arg_idx = 1;
    MODE mode = MODE_PLAIN;
    if (std::wstring(argv[arg_idx]) == L"li") {
        mode = MODE_LOW_INTEGRITY;
        arg_idx++;
    }
    else if (std::wstring(argv[arg_idx]) == L"ac") {
        mode = MODE_APP_CONTAINER;
        arg_idx++;
    }

    // check if 1st argument is a COM class ProgID
    CLSID clsid = {};
    bool progid_provided = SUCCEEDED(CLSIDFromProgID(argv[arg_idx], &clsid));

    if (progid_provided) {
        // initialize multi-threaded COM apartment
        if (FAILED(CoInitializeEx(NULL, COINIT_MULTITHREADED)))
            abort();

    #if 0
        // attempt to disable COM security and enable cloaking
        HRESULT hr = CoInitializeSecurity(nullptr, -1/*auto*/, nullptr, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, nullptr, EOAC_NONE /*EOAC_STATIC_CLOAKING*/, NULL);
        if (FAILED(hr))
            abort();
    #endif

        std::wcout << L"Creating COM object " << argv[arg_idx];
        if (mode == MODE_LOW_INTEGRITY)
            std::wcout << L" in low-integrity...\n";
        else if (mode == MODE_APP_CONTAINER)
            std::wcout << L" in AppContainer...\n";
        else
            std::wcout << L"...\n";

        arg_idx++;
        wchar_t* user = (argc > arg_idx) ? argv[arg_idx++] : nullptr;
        wchar_t* pw   = (argc > arg_idx) ? argv[arg_idx++] : nullptr;
        CComPtr<IUnknown> obj = CoCreateAsUser_impersonate(clsid, mode, user, pw);
        //CComPtr<IUnknown> obj = CoCreateAsUser_dcom(clsid, user, pw);

        // try to add two numbers
        CComPtr< ISimpleCalculator> calc;
        obj.QueryInterface(&calc);
        if (calc) {
            int sum = 0;
            CHECK(calc->Add(2, 3, &sum));

            std::wcout << L"Add(2, 3) returned " << sum << L".\n";
            assert(sum == 2 + 3);
        }

        // try to make window visible
        SetComAttribute(obj, L"Visible", true);
    } else {
        std::wcout << L"Starting executable " << argv[arg_idx];
        if (mode == MODE_LOW_INTEGRITY)
            std::wcout << L" in low-integrity...\n";
        else if (mode == MODE_APP_CONTAINER)
            std::wcout << L" in AppContainer...\n";
        else
            std::wcout << L"...\n";
        ProcCreate_AppContainer(argv[arg_idx], mode);
    }

    std::wcout << L"[done]" << std::endl;
}
